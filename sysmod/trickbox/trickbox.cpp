#include "cvm/plusargs.hpp"
#include "trickbox.h"
DECLARE_string(load);
trickbox::trickbox(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, 0xc0000 /* size */, loc, &trickbox::write, &trickbox::read, this), axi_mst_loc_l(axi_mst_loc)
{

  if (FLAGS_load != "") {
    std::cout << "loading " << FLAGS_load << "\n";
    init_elf(FLAGS_load);
  }

  subdevice* sub = nullptr;
  interrupter_base = addr;
  sub = new interrupter("interrupter", interrupter_base, 1, loc);
  subdevices_.emplace_back(sub);
  sub = new debugger("debugger", addr + 0x50000, 1, loc);
  subdevices_.emplace_back(sub);
  sub = new msi_driver("msi_driver", addr + 0x60000, 1, loc, axi_mst_loc_l);
  subdevices_.emplace_back(sub);
  sub = new uc_helper("uc_helper", addr + 0x80000, 1, loc, m_);
  subdevices_.emplace_back(sub);

}


trickbox::~trickbox()
{
  terminate_ = true;
}

bool trickbox::init_elf(const std::string& path) {
  std::cout<<"[IO_DEV]: Device init elf\n";
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}



// void
// trickbox::read(const transactor::read_t& r, data_t& data)
// {
//   auto& addr = r.addr;
//   auto& length = r.length;
//   uint64_t offset = addr - device::addr();
//   if (offset == 0x80000) {
//     //std::cout << data <<"\n";
//     uint32_t word = 1;
//     serializeInt(word, length, data);
//     return;
//   }
//   return;
// }
void trickbox::read(const transactor::read_t& r, data_t& data) {
  auto& addr = r.addr;
  auto& length = r.length;
  std::cout << "TRICKBOX READ::::: ADDR: "<<std::hex<<addr<<"\n";
  m_.read(addr, length, data.data());
  return;
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
