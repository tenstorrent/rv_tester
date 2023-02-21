#include "clint.h"

clint::clint(const std::string& tag, const std::string& type, uint64_t addr, unsigned hartCount,
             cvm::topology::loc_t loc)
  : device(tag, type, addr, 0xc000 /* size */, loc), hartCount_(hartCount), soft_(hartCount),
    timeCompare_(hartCount), timerIntPrev_(hartCount, 0), timer_(0)
{
}


clint::~clint()
{
  terminate_ = true;
  if (timerThread_.joinable())
    timerThread_.join();
}


void
clint::selfTick(useconds_t delta)
{
  auto func = [this, delta]() {
    while (true)
      {
	usleep(delta);
	if (terminate_)
	  return;
	else
	  {
	    std::lock_guard<std::mutex> lock(mutex_);
	   //  tick();
	  }
      }
  };

  timerThread_ = std::thread(func);
}


void
clint::read(uint64_t addr, size_t length, data_t& data)
{
  if (not has_addr(addr))
    return;

  uint64_t offset = addr - device::addr();
  if (offset < 0x4000)
    {
      // Sofware interrupt: 1 word per hart.
      if ((offset % 4) != 0)
	return;  // Address must be a multiple of 4.
      unsigned hartIx = offset / 4;
      uint32_t word = (hartIx < hartCount_) ? soft_.at(hartIx) : 0;
      serializeInt(word, length, data);
      return;
    }

  if (offset == 0xbff8)
    {
      serializeInt(timer_, length, data);
      return;
    }

  // Time compare. 1 double word per hart.
  if ((offset % 8) != 0)
    return;
  offset -= 0x4000;
  unsigned hartIx = offset / 8;
  uint64_t dword = hartIx < hartCount_ ? timeCompare_.at(hartIx) : 0;
  serializeInt(dword, length, data);
}


void
clint::write(uint64_t addr, size_t length, const data_t& data,
		 const strb_t& strb)
{
  if (not has_addr(addr))
    return;

  uint64_t offset = addr - device::addr();
  if (offset < 0x4000)
    {
      // Sofware interrupt: 1 word per hart.
      unsigned hartIx = offset / 4;
      if ((offset % 4) != 0 or length < 4 or hartIx >= hartCount_)
	return;
      unsigned word = 0;
      deserializeInt(data, word);
      soft_.at(hartIx) = word & 1;
      softwareInterrupt(hartIx, word & 1);
    }

  if (length == 8)
    {
      std::lock_guard<std::mutex> lock(mutex_);

      offset -= 0x4000;

      if (offset == 0x7ff8)
	deserializeInt(data, timer_);
      else
	{
	  unsigned hartIx = offset / 8;
	  if (hartIx >= hartCount_)
	    return;

	  // Time compare. 1 double word per hart.
	  uint64_t dword = 0;
	  deserializeInt(data, dword);
	  timeCompare_.at(hartIx) = dword;
	}

      processTimerInterrupts();
    }
}
