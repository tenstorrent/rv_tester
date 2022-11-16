#include <iostream>
#include "mem.h"


void mem::write(uint64_t addr, size_t length, const data_t& data, const strb_t& strb,
                cbs_t& cbs) {
  if (not has_addr(addr))
    return;

  for (size_t i = 0; i < length; i++) {
      uint64_t value = data[i];
      if (strb[i]) {
          m_.write(addr + i, 1, value);
      }
  }
  return;
}

void mem::read(uint64_t addr, size_t length, data_t& data, cbs_t& cbs) {
  if (not has_addr(addr))
    return;

  for (size_t i = 0; i < length; i++) {
      uint64_t value;
      m_.read(addr + i, 1, value);
      data[i] = value;
  }
  return;
}

bool mem::init_hex(const std::string& path) {
    return m_.loadHexFile(path);
}
