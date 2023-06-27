#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "io_dev.h"


DECLARE_string(load);


io_dev::io_dev(const std::string& tag, uint64_t addr, size_t size)
  : device(tag, addr, size, cvm::topology::null)
{
  if (FLAGS_load != "") {
    std::cout << "loading " << FLAGS_load << "\n";
    init_elf(FLAGS_load);
  }
}

void io_dev::write(uint64_t addr, size_t length, const data_t& data, const strb_t& strb) {
  if (not has_addr(addr))
    return;

  for (size_t i = 0; i < length; i++) {
    if (strb[i]) {
      m_.write(addr + i, 1, &data[i]);
    }
  }
  return;
}

cvm::messenger::task<void> io_dev::read(uint64_t addr, size_t length, data_t& data) {
  if (not has_addr(addr))
    co_return;

  m_.read(addr, length, data.data());
  co_return;
}


bool io_dev::init_elf(const std::string& path) {
  std::cout<<"[IO_DEV]: Device init elf\n";
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}
