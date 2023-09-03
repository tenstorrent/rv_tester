#include "cvm/plusargs.hpp"
#include "trickbox.h"

trickbox::trickbox(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, 0xc0000 /* size */, loc, &trickbox::write, &trickbox::read, this), axi_mst_loc_l(axi_mst_loc)
{
  subdevice* sub = nullptr;
  interrupter_base = addr;
  sub = new interrupter("interrupter", interrupter_base, 1, loc);
  subdevices_.emplace_back(sub);
  sub = new debugger("debugger", addr + 0x50000, 1, loc);
  subdevices_.emplace_back(sub);
  sub = new msi_driver("msi_driver", addr + 0x60000, 1, loc, axi_mst_loc_l);
  subdevices_.emplace_back(sub);
  sub = new uc_helper("uc_helper", addr + 0x80000, 1, loc);
  subdevices_.emplace_back(sub);
}


trickbox::~trickbox()
{
  terminate_ = true;
}


cvm::messenger::task<void>
trickbox::read(const transactor::read_t&, data_t&)
{
  co_return;
}


void
trickbox::write(const transactor::write_t& w)
{
  auto& addr = w.addr;
  auto& length = w.length;
  auto& data = w.data;
  auto& strb = w.strb;

  for (auto& d : subdevices_) {
    d->write(addr,length,data,strb);
  }
}
