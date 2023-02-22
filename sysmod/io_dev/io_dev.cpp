#include <iostream>
#include "io_dev.h"


void io_dev::write(uint64_t addr, size_t length, const data_t& data, const strb_t& strb,
                cbs_t& cbs) {
  if (not has_addr(addr))
    return;

  for (size_t i = 0; i < length; i++) {
    if (strb[i]) {
      m_.write(addr + i, 1, &data[i]);
    }
  }
  return;
}

void io_dev::read(uint64_t addr, size_t length, data_t& data, cbs_t& cbs) {
  if (not has_addr(addr))
    return;

  m_.read(addr, length, data.data());

  return;
}


bool io_dev::init_elf(const std::string& path) {
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}
