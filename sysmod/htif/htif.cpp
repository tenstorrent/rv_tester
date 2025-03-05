#include <iostream>
#include <cassert>
#include <unistd.h>
#include <pty.h>
#include <termios.h>
#include <poll.h>
#include "htif.h"
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cosim/utils/eot/eot_plusargs.h"

DEFINE_bool(htif_flip, false, "Reverse the htif tohost/fromhost address order");
DEFINE_bool(htif_log, true, "tee htif characters to htif.log");
DEFINE_bool(pty, false, "Use a pseudo-terminal for the HTIF console");

// https://stackoverflow.com/a/45675259
htif::pty::pty()
{
  if (!FLAGS_pty)
  {
    return;
  }

  struct termios tty;
  tty.c_iflag = (tcflag_t) 0;
  tty.c_lflag = (tcflag_t) 0;
  tty.c_cflag = CS8;
  tty.c_oflag = (tcflag_t) 0;

  char name[256];
  auto e = openpty(&master, &slave, name, &tty, nullptr);
  if(0 > e) {
    master = -1;
    slave = -1;
    cvm::log(cvm::ERROR, "Error: {}\n", std::strerror(errno));
  }

  cvm::log(cvm::NONE, "HTIF PTY: {}\n", name);
}

htif::pty::~pty()
{
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
  : device(tag, addr, 16 /* size */, loc, &htif::write, &htif::read, this), to_(0), from_(0), htif_log_("htif.log")
{
}


htif::~htif()
{
}

static bool
hasPendingInput(int fd)
{
  static bool firstTime = true;

  if (firstTime)
    {
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
readCharNonBlocking(int fd)
{
  if (not hasPendingInput(fd))
    return 0;

  char c = 0;
  int ret = ::read(fd, &c, sizeof(c));
  if (ret == 1)
    {
      return c;
    }
  else if (ret == -1)
    {
      cvm::log(cvm::ERROR, "Error: readCharNonBlocking: unexpected fail on read, errno={}\n", strerror(errno));
      return -1;
    }

  // We raced with something else that consumed the input between poll and read
  return 0;
}

void
htif::read(const transactor::read_t& r, data_t& data)
{
  auto& addr = r.addr;
  auto& length = r.length;

  if (length != 8 or (addr % 8) != 0)
    return;

  uint64_t offset = addr - this->addr();
  uint64_t di = offset / 8;  // Double word index
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


void
htif::write(const transactor::write_t& w)
{
  auto& addr = w.addr;
  // auto& length = w.length;
  auto& data = w.data;

  if ((addr % 8) != 0)
    return;

  uint64_t dword = 0;
  deserializeInt(data, dword);

  uint64_t offset = addr - this->addr();
  uint64_t di = offset / 8;  // Double word index

  if ((di == 1) ^ FLAGS_htif_flip)
    {
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
      } else {
	    pty_.write(c);
	    putchar(c);
	    fflush(stdout);
      }
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
  } 
  else if (dev == 0 and cmd == 0) {
    if ((payload & 1) && ((payload >> 1) == uint64_t(0))) {
      cvm::log(cvm::NONE, "Pass condition detected - tohost[0] = 1, , tohost[47:1] = 0\n");
      if (FLAGS_eot != "tohost_all")
        cvm::registry::messenger.signal<terminate_t>(loc(), terminate_t{.low_priority_based = true});
	  }
    else
      cvm::log(cvm::ERROR, "Fail condition detected - tohost[0]={:#x}, tohost[47:1]={:#x}",(payload & 1), (payload >> 1));
  } 
  else {
    assert(0);
  }
}
