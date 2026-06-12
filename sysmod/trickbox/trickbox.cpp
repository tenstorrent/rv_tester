#include "cvm/plusargs.hpp"
#include "trickbox.h"
#include "sysmod/sysmod_plusargs.h"
#include "cosim/dut_if/rvfi/rvfi_plusargs.h"


trickbox::trickbox(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc )
  : device(tag, addr, 0x1c00000 /* size */, loc, &trickbox::write, &trickbox::read, this)
{

  if (FLAGS_load != "") {
    init_elf(FLAGS_load);
  }

  subdevice* sub = nullptr;
  interrupter_base = addr;
  sub = new interrupter("interrupter", interrupter_base, cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second, loc);
  subdevices_.emplace_back(sub);
  sub = new debugger("debugger", addr + 0x50000, 1, loc);
  subdevices_.emplace_back(sub);
  sub = new evt_trigger("evt_trigger", addr + 0x78000, 1, loc);
  subdevices_.emplace_back(sub);
  sub = new uc_helper("uc_helper", addr + 0x80000, 1, loc, m_);
  subdevices_.emplace_back(sub);
  sub = new io_coh_helper("io_coh_helper", addr + 0x89000, 1, loc, m_);
  subdevices_.emplace_back(sub);
  sub = new ras_helper("ras_helper", addr + 0x89800, 1, loc, m_);
  subdevices_.emplace_back(sub);
  sub = new dma("dma", addr + 0x90000, 1, loc, m_);
  subdevices_.emplace_back(sub);
  sub = new post_si_pcietc_helper("post_si_pcietc_helper", addr + 0x1000000, 1, loc);
  subdevices_.emplace_back(sub);
}


trickbox::~trickbox()
{
}

void trickbox::configure()
{
  device::configure();
  for (auto& d : subdevices_) {
    d->configure();
  }
}

bool trickbox::init_elf(const std::string& path) {
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}


void trickbox::read(const transactor::read_t& r, data_t& data) {

  if (!FLAGS_rvfi)
      return;
  auto& addr = r.addr;
  auto& length = r.length;

  for (auto& d : subdevices_) {
    d->read_dev(addr,length,data);
  }
  cvm::log (cvm::HIGH,"TRICKBOX READ::::: ADDR:{:#x} \n",addr);
  cvm::log (cvm::HIGH,"TRICKBOX READ::::: DATA byte 0:{:#x} \n",(uint32_t)data[0]);
  cvm::log (cvm::HIGH,"TRICKBOX READ::::: DATA byte 1:{:#x} \n",(uint32_t)data[1]);
  cvm::log (cvm::HIGH,"TRICKBOX READ::::: DATA byte 2:{:#x} \n",(uint32_t)data[2]);

  return;
}

void
trickbox::write(const transactor::write_t& w)
{
  if (!FLAGS_rvfi)
      return;
  auto& addr = w.addr;
  auto& length = w.length;
  auto& data = w.data;
  auto& strb = w.strb;

  for (auto& d : subdevices_) {
    d->write(addr,length,data,strb);
  }
}
void trickbox::backdoor_write(uint64_t addr, size_t length, data_t& data, strb_t& strb) 
{
uint64_t t_data;  
deserializeInt(data,t_data);

cvm::log (cvm::HIGH,"TRICKBOX BACKDOOR WRITE::::: ADDR:{:#x} DATA:{:#x}\n",addr,t_data);
  for (auto& d : subdevices_) {
    d->write(addr,length,data,strb);
  }
}
