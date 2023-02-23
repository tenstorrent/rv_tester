#include "cvm/plusargs.hpp"
#include "trickbox.h"

trickbox::trickbox(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc)
  : device(tag, addr, 0xc0000 /* size */, loc), hartCount_(hartCount)
{
  device* subdevice = nullptr;
  interrupter_base = addr;
  subdevice = new interrupter("interrupter", interrupter_base, 1, loc);
  subdevices_.emplace_back(subdevice); 
}


trickbox::~trickbox()
{
  terminate_ = true;
}


void
trickbox::read(uint64_t addr, size_t length, data_t& data)
{
  if (not has_addr(addr))
    return;

}


void
trickbox::write(uint64_t addr, size_t length, const data_t& data,
		 const strb_t& strb)
{
  for (auto& d : subdevices_) {
    d->write(addr,length,data,strb);
  }
    
}
