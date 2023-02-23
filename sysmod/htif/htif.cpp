#include <iostream>
#include <cassert>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include "htif.h"
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"

DEFINE_bool(htif_terminate, true, "Call $finish on write to tohost");

htif::htif(const std::string& tag, uint64_t addr, cvm::topology::loc_t loc)
  : device(tag, addr, 16 /* size */, loc)
{
}


htif::~htif()
{
}


void
htif::read(uint64_t addr, size_t length, data_t& data)
{
  if (not has_addr(addr) or length != 8 or (addr % 8) != 0)
    return;

  uint64_t offset = addr - this->addr();
  uint64_t di = offset / 8;  // Double word index
  uint64_t dword = di == 0? to_ : from_;
  serializeInt(dword, length, data);
  return;
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
  if (::read(fd, &c, sizeof(c)) == 1)
    return c;

  std::cerr << "readCharNonBlocking: unexpected fail on read\n";
  return -1;
}


void
htif::write(uint64_t addr, size_t length, const data_t& data,
	    const strb_t& strb)
{
  if (not has_addr(addr) or length != 8 or (addr % 8) != 0)
    return;

  uint64_t dword = 0;
  deserializeInt(data, dword);

  uint64_t offset = addr - this->addr();
  uint64_t di = offset / 8;  // Double word index

  if (di == 1)
    {
      from_ = dword;
      return;
    }

  // Writing to-host.
  uint64_t payload = (dword << 16) >> 16;
  unsigned cmd = (dword >> 48) & 0xff;
  unsigned dev = (dword >> 56) & 0xff;

  if (dev == 1)
    {
      if (cmd == 1)
	{
	  char c = payload;
	  if (c)
	    {
	      putchar(c);
	      fflush(stdout);
	    }
	}
      else if (cmd == 0)
	{
	  int ch = readCharNonBlocking(fileno(stdin));
	  if (ch > 0)
	    from_ = ((payload >> 48) << 48) | uint64_t(ch);
	}
    }
  else if (dev == 0 and cmd == 0)
    {
      if (payload & 1)
	{
	  std::cerr << "Terminating because of write tohost\n";
          cvm::registry::messenger.signal<terminate_t>(loc(), terminate_t{FLAGS_htif_terminate});
	}
    }
  else
    {
      assert(0);
    }
}
