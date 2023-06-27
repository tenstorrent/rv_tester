#include <iostream>
#include "sysmod_mem.h"


void sysmod_mem::write(uint64_t addr, size_t length, const data_t& data, const strb_t& strb) {
  if (not has_addr(addr))
    return;

  for (size_t i = 0; i < length; i++) {
    if (strb[i]) {
      m_.write(addr + i, 1, &data[i]);
    }
  }
  return;
}

cvm::messenger::task<void> sysmod_mem::read(uint64_t addr, size_t length, data_t& data) {
  if (not has_addr(addr))
    co_return;

  m_.read(addr, length, data.data());

  co_return;
}

void sysmod_mem::backdoor_read(uint64_t addr, size_t length, data_t& data) {
  m_.read(addr, length, data.data());

  return;
}

bool sysmod_mem::init_hex(const std::string& path) {
    try {
        m_.load_verilog_hex(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}

bool sysmod_mem::init_elf(const std::string& path) {
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}
