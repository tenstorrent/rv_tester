// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <sstream>
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

htif::htif(const std::string& tag, uint64_t addr, cvm::topology::loc_t loc)
    : device(tag, addr, 16 /* size */, loc, &htif::write, &htif::read, this), to_(0), from_(0), htif_log_("htif.log") {
  if (!FLAGS_payload_has_hart_id)
    return;
  std::istringstream ss(FLAGS_hart_enable_id);
  std::string token;
  while (std::getline(ss, token, ','))
    enabled_harts_.insert(static_cast<uint16_t>(std::stoul(token)));
}

htif::~htif() {
  if (passed_ == 0 && failed_ == 0)
    return;
  cvm::log(cvm::NONE, "HTIF exit summary: passed {}, failed {}, expected {} harts\n",
           passed_, failed_, FLAGS_num_harts);
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
      uint16_t hart_id = (payload >> 16) & 0xffff;
      if (!enabled_harts_.count(hart_id)) {
        cvm::log(cvm::ERROR, "Error: illegal, hart id {} is not enabled but does an exit axi_id={}\n", hart_id, w.id);
      } else {
        uint64_t status = payload & ~(uint64_t(0xffff) << 16);
        if ((status & 1) && ((status >> 1) == uint64_t(0))) {
          if (passed_harts_.insert(hart_id).second)
            passed_++;
          cvm::log(cvm::NONE, "Pass condition detected - tohost[0] = 1, tohost[47:1] = 0 hart_id={} ({}/{}) axi_id={}\n", hart_id, passed_, enabled_harts_.size(), w.id);
        } else {
          if (failed_harts_.insert(hart_id).second)
            failed_++;
          cvm::log(cvm::ERROR, "Error: Fail condition detected - tohost[0]={:#x}, tohost[47:1]={:#x} hart_id={} ({}/{}) axi_id={}\n", (status & 1), (status >> 1), hart_id, failed_, enabled_harts_.size(), w.id);
        }
        if (passed_harts_.size() + failed_harts_.size() == enabled_harts_.size()) {
          cvm::registry::messenger.signal<terminate_t>(cvm::topology::get_from_hierarchy("TOP.PLATFORM", 0), terminate_t{.low_priority_based = true, .passed = failed_harts_.empty()});
        }
      }
    } else if ((payload & 1) && ((payload >> 1) == uint64_t(0))) {
      passed_++;
      cvm::log(cvm::NONE, "Pass condition detected - tohost[0] = 1, tohost[47:1] = 0 ({}/{}) axi_id={}\n", passed_, FLAGS_num_harts, w.id);
      if (FLAGS_eot != "tohost_all") {
        cvm::registry::messenger.signal<terminate_t>(cvm::topology::get_from_hierarchy("TOP.PLATFORM", 0), terminate_t{.low_priority_based = true, .passed = true});
      }
    } else {
      failed_++;
      cvm::log(cvm::ERROR, "Error: Fail condition detected - tohost[0]={:#x}, tohost[47:1]={:#x} ({}/{}) axi_id={}\n", (payload & 1), (payload >> 1), failed_, FLAGS_num_harts, w.id);
    }
    if (FLAGS_eot == "tohost_all" && passed_ + failed_ == int(FLAGS_num_harts)) {
      cvm::registry::messenger.signal<terminate_t>(cvm::topology::get_from_hierarchy("TOP.PLATFORM", 0), terminate_t{.low_priority_based = true, .passed = failed_ == 0});
    }
  } else {
    assert(0);
  }
}
