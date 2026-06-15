#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "src/sysmod/io_dev/io_dev.h"
#include "sysmod_plusargs.h"


io_dev::io_dev(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc)
  : device(tag, addr, size, loc, &io_dev::write, &io_dev::read, this)
{
  if (FLAGS_load != "") {
    init_elf(FLAGS_load);
  }
}

void io_dev::write(const transactor::write_t& w) {
  auto& addr = w.addr;
  auto& length = w.length;
  auto& data = w.data;
  auto& strb = w.strb;

  for (size_t i = 0; i < length; i++) {
    if (strb[i]) {
      m_.write(addr + i, 1, &data[i]);
    }
  }
  return;
}

void io_dev::read(const transactor::read_t& r, data_t& data) {
  auto& addr = r.addr;
  auto& length = r.length;

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
