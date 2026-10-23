// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <sstream>
#include <charconv>
#include <limits>
#include <string>
#include <cassert>
#include <unistd.h>
#include <pty.h>
#include <termios.h>
#include <poll.h>
#include "src/sysmod/htif/htif.h"
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "src/cosim/utils/eot/eot_plusargs.h"
#include "sysmod_plusargs.h"

DEFINE_bool(htif_flip, false, "Reverse the htif tohost/fromhost address order");
DEFINE_bool(htif_log, true, "tee htif characters to htif.log");
DEFINE_bool(pty, false, "Use a pseudo-terminal for the HTIF console");
DEFINE_bool(payload_has_hart_id, false, "tohost payload[31:16] carries the exiting hart id; terminate when every enabled hart has exited");

// https://stackoverflow.com/a/45675259
htif::pty::pty() {
  if (!FLAGS_pty) {
    return;
  }

  struct termios tty;
  tty.c_iflag = (tcflag_t)0;
  tty.c_lflag = (tcflag_t)0;
  tty.c_cflag = CS8;
  tty.c_oflag = (tcflag_t)0;

  char name[256];
  auto e = openpty(&master, &slave, name, &tty, nullptr);
  if (0 > e) {
    master = -1;
    slave = -1;
    cvm::log(cvm::ERROR, "Error: {}\n", std::strerror(errno));
  }

  cvm::log(cvm::NONE, "HTIF PTY: {}\n", name);
}

htif::pty::~pty() {
  if (slave != -1)
    close(slave);
  if (master != -1)
    close(master);
}

int htif::pty::write(char c) {
  if (master == -1)
    return -1;

  return ::write(master, &c, 1);
}

int htif::pty::read() {
  if (master == -1)
    return -1;

  struct pollfd pollfd;
  pollfd.fd = master;
  pollfd.events = POLLIN;
  int code = poll(&pollfd, 1, 0);
  if (!(code == 1 && (pollfd.revents & POLLIN) != 0))
    return 0;

  char c;
  int r = ::read(master, &c, sizeof(c));
  if (r == 1)
    return c;

  if (r == -1) {
    cvm::log(cvm::ERROR, "Error: pty read: unexpected fail on read, errno={}\n", strerror(errno));
    return -1;
  }

  cvm::log(cvm::ERROR, "Error: pty read: unexpected fail on read\n");
  return -1;
}

static std::set<uint16_t> parse_hart_ids(const char* flag, const std::string& list) {
  std::set<uint16_t> hart_ids;
  std::istringstream ss(list);
  std::string token;
  while (std::getline(ss, token, ',')) {
    uint64_t hart_id = 0;
    const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), hart_id, 10);
    if (token.empty() || error == std::errc::invalid_argument || end != token.data() + token.size()) {
      cvm::log(cvm::ERROR, "Error: [htif] +{}={} has the entry '{}', which is not a decimal hart id\n", flag, list, token);
      continue;
    }
    if (error == std::errc::result_out_of_range || hart_id > std::numeric_limits<uint16_t>::max()) {
      cvm::log(cvm::ERROR, "Error: [htif] +{}={} has the hart id {}, which is above {}\n", flag, list, token, std::numeric_limits<uint16_t>::max());
      continue;
    }
    if (!hart_ids.insert(static_cast<uint16_t>(hart_id)).second)
      cvm::log(cvm::ERROR, "Error: [htif] +{}={} has the hart id {} more than once\n", flag, list, token);
  }
  return hart_ids;
}

htif::htif(const std::string& tag, uint64_t addr, cvm::topology::loc_t loc)
    : device(tag, addr, 16 /* size */, loc, &htif::write, &htif::read, this), to_(0), from_(0), htif_log_("htif.log") {
  if (FLAGS_payload_has_hart_id) {
    if (FLAGS_eot == "max_instr")
      cvm::log(cvm::ERROR, "Error: [htif] +payload_has_hart_id ends the run when every enabled hart has exited, so it cannot be used with +eot=max_instr\n");
    const bool by_mhartid = !FLAGS_hart_mhartid.empty();
    enabled_harts_ = by_mhartid ? parse_hart_ids("hart_mhartid", FLAGS_hart_mhartid) : parse_hart_ids("hart_enable_id", FLAGS_hart_enable_id);
    if (enabled_harts_.empty())
      cvm::log(cvm::ERROR, "Error: [htif] +payload_has_hart_id needs the hart ids of the enabled harts in +hart_mhartid or +hart_enable_id\n");
    return;
  }
  if (FLAGS_eot != "tohost_all")
    return;
  cvm::log(cvm::NONE, "[htif] +eot=tohost_all counts tohost exit writes, so a hart that writes tohost twice is counted twice. +payload_has_hart_id counts each hart once\n");
  if (FLAGS_num_harts == 0)
    cvm::log(cvm::ERROR, "Error: [htif] +eot=tohost_all needs +num_harts above 0\n");
  if (FLAGS_hart_enable_id.empty())
    return;
  const auto listed = parse_hart_ids("hart_enable_id", FLAGS_hart_enable_id);
  if (listed.size() != FLAGS_num_harts)
    cvm::log(cvm::ERROR, "Error: [htif] +eot=tohost_all ends after +num_harts={} exits, but +hart_enable_id={} lists {} harts\n", FLAGS_num_harts, FLAGS_hart_enable_id, listed.size());
}

static std::string hart_list(const std::set<uint16_t>& harts) {
  std::string list;
  for (uint16_t hart : harts)
    list += (list.empty() ? "" : ",") + std::to_string(hart);
  return list.empty() ? "none" : list;
}

htif::~htif() {
  if (FLAGS_payload_has_hart_id) {
    if (enabled_harts_.empty())
      return;
    std::set<uint16_t> passed_harts;
    std::set<uint16_t> missing_harts;
    for (uint16_t hart : enabled_harts_) {
      if (!exited_harts_.count(hart))
        missing_harts.insert(hart);
      else if (!failed_harts_.count(hart))
        passed_harts.insert(hart);
    }
    cvm::log(cvm::NONE, "HTIF exit summary: {} of {} enabled harts exited; passed {}, failed {}, missing {}\n",
             exited_harts_.size(), enabled_harts_.size(), hart_list(passed_harts), hart_list(failed_harts_), hart_list(missing_harts));
    return;
  }
  if (passed_ == 0 && failed_ == 0)
    return;
  const std::string expected = FLAGS_eot == "tohost_all" ? ", expected " + std::to_string(FLAGS_num_harts) : "";
  cvm::log(cvm::NONE, "HTIF exit summary: {} exit writes, passed {}, failed {}{}\n", passed_ + failed_, passed_, failed_, expected);
}

static bool
hasPendingInput(int fd) {
  static bool firstTime = true;

  if (firstTime) {
    firstTime = false;
    struct termios term;
    tcgetattr(fd, &term);
    cfmakeraw(&term);
    term.c_lflag &= ~ECHO;
    tcsetattr(fd, 0, &term);
  }

  struct pollfd inPollfd;
  inPollfd.fd = fd;
  inPollfd.events = POLLIN;
  int code = poll(&inPollfd, 1, 0);
  return code == 1 and (inPollfd.revents & POLLIN) != 0;
}

static int
readCharNonBlocking(int fd) {
  if (not hasPendingInput(fd))
    return 0;

  char c = 0;
  int ret = ::read(fd, &c, sizeof(c));
  if (ret == 1) {
    return c;
  } else if (ret == -1) {
    cvm::log(cvm::ERROR, "Error: readCharNonBlocking: unexpected fail on read, errno={}\n", strerror(errno));
    return -1;
  }

  // We raced with something else that consumed the input between poll and read
  return 0;
}

void htif::read(const transactor::read_t& r, data_t& data) {
  auto& addr = r.addr;
  auto& length = r.length;

  if (length != 8 or (addr % 8) != 0)
    return;

  uint64_t offset = addr - this->addr();
  uint64_t di = offset / 8; // Double word index
                            //
  bool to = (di == 0) ^ FLAGS_htif_flip;

  uint64_t dword;

  if (to) {
    dword = to_;
  } else {
    if (FLAGS_pty) {
      int ch;
      ch = pty_.read();
      if (ch < 0)
        ch = readCharNonBlocking(fileno(stdin));
      if (ch > 0)
        from_ = uint64_t(1) << 56 | uint64_t(ch);
      else
        from_ = 0;
    }
    dword = from_;
  }
  serializeInt(dword, length, data);
  return;
}

void htif::record_exit(std::optional<uint16_t> hart_id, bool passed) {
  if (!hart_id) {
    (passed ? passed_ : failed_)++;
    return;
  }
  if (!exited_harts_.insert(*hart_id).second) {
    cvm::log(cvm::ERROR, "Error: [htif] hart id {} exited more than once\n", *hart_id);
    passed = false;
  }
  if (!passed)
    failed_harts_.insert(*hart_id);
}

void htif::end_test_if_done(bool per_hart, bool passed) {
  bool done = passed;
  bool test_passed = true;
  if (per_hart) {
    done = exited_harts_.size() == enabled_harts_.size();
    test_passed = failed_harts_.empty();
  } else if (FLAGS_eot == "tohost_all") {
    done = passed_ + failed_ == int(FLAGS_num_harts);
    test_passed = failed_ == 0;
  }
  if (done)
    end_test(test_passed);
}

std::string htif::exit_progress() const {
  if (FLAGS_payload_has_hart_id)
    return " (" + std::to_string(exited_harts_.size()) + " of " + std::to_string(enabled_harts_.size()) + " harts exited)";
  if (FLAGS_eot == "tohost_all")
    return " (exit " + std::to_string(passed_ + failed_) + " of " + std::to_string(FLAGS_num_harts) + ")";
  return "";
}

void htif::end_test(bool passed) {
  if (ended_)
    return;
  ended_ = true;
  cvm::registry::messenger.signal<terminate_t>(cvm::topology::get_from_hierarchy("TOP.PLATFORM", 0), terminate_t{.low_priority_based = true, .passed = passed});
}

void htif::write(const transactor::write_t& w) {
  auto& addr = w.addr;
  // auto& length = w.length;
  auto& data = w.data;

  if ((addr % 8) != 0)
    return;

  uint64_t dword = 0;
  deserializeInt(data, dword);

  uint64_t offset = addr - this->addr();
  uint64_t di = offset / 8; // Double word index

  if ((di == 1) ^ FLAGS_htif_flip) {
    from_ = dword;
    return;
  }

  // Writing to-host.
  uint64_t payload = (dword << 16) >> 16;
  unsigned cmd = (dword >> 48) & 0xff;
  unsigned dev = (dword >> 56) & 0xff;

  if (dev == 1 && cmd == 1) {
    char c = payload;
    if (c) {
      if (FLAGS_htif_log) {
        std::string s(1, c);
        htif_log_.log(cvm::NONE, s);
        htif_log_.flush();
      }
      pty_.write(c);
      putchar(c);
      fflush(stdout);
    }
  } else if (dev == 1 && cmd == 0) {
    if (!FLAGS_pty) {
      int ch;
      ch = pty_.read();
      if (ch < 0)
        ch = readCharNonBlocking(fileno(stdin));
      if (ch > 0)
        from_ = ((payload >> 48) << 48) | uint64_t(ch);
    }
  } else if (dev == 0 and cmd == 0) {
    if (FLAGS_payload_has_hart_id) {
      const auto exit = std::bit_cast<hart_exit_t>(dword);
      const uint16_t hart_id = exit.hart_id;
      if (!enabled_harts_.count(hart_id)) {
        cvm::log(cvm::ERROR, "Error: illegal, hart id {} is not enabled but does an exit axi_id={}\n", hart_id, w.id);
        end_test(false);
      } else {
        const bool passed = exit.terminate && exit.code == 0 && exit.reserved == 0;
        record_exit(hart_id, passed);
        if (passed) {
          cvm::log(cvm::NONE, "Pass condition detected - tohost[0] = 1, tohost[47:1] = 0 hart_id={}{} axi_id={}\n", hart_id, exit_progress(), w.id);
        } else {
          cvm::log(cvm::ERROR, "Error: Fail condition detected - tohost[0]={:#x}, code={:#x}, reserved={:#x} hart_id={}{} axi_id={}\n", uint64_t(exit.terminate), uint64_t(exit.code), uint64_t(exit.reserved), hart_id, exit_progress(), w.id);
        }
        end_test_if_done(true, passed);
      }
    } else {
      const auto exit = std::bit_cast<exit_t>(dword);
      const bool passed = exit.terminate && exit.code == 0;
      record_exit(std::nullopt, passed);
      if (passed) {
        cvm::log(cvm::NONE, "Pass condition detected - tohost[0] = 1, tohost[47:1] = 0{} axi_id={}\n", exit_progress(), w.id);
      } else {
        cvm::log(cvm::ERROR, "Error: Fail condition detected - tohost[0]={:#x}, tohost[47:1]={:#x}{} axi_id={}\n", uint64_t(exit.terminate), uint64_t(exit.code), exit_progress(), w.id);
      }
      end_test_if_done(false, passed);
    }
  } else {
    assert(0);
  }
}
