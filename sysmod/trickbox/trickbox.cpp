#include "cvm/plusargs.hpp"
#include "trickbox.h"

trickbox::trickbox(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc)
  : device(tag, addr, 0xc0000 /* size */, loc)
{
  device* subdevice = nullptr;
  interrupter_base = addr;
  subdevice = new interrupter("interrupter", interrupter_base, 1, loc);
  subdevices_.emplace_back(subdevice);
  subdevice = new debugger("debugger", addr + 0x50000, 1, loc);
  subdevices_.emplace_back(subdevice);
}


trickbox::~trickbox()
{
  terminate_ = true;
}


cvm::messenger::task<void>
trickbox::read(uint64_t addr, size_t, data_t&)
{
  co_return;
}


void
trickbox::write(uint64_t addr, size_t length, const data_t& data,
		 const strb_t& strb)
{
  for (auto& d : subdevices_) {
    d->write(addr,length,data,strb);
  }

}
