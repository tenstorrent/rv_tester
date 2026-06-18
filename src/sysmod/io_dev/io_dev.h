#pragma once

#include "src/sysmod/device.h"
#include "cvm/topology.hpp"
#include <string>
#include <mem_manager.h>

class io_dev : public device {

private:
  mem_manager m_;

public:
  void write(const transactor::write_t& w);

  void read(const transactor::read_t& r, data_t& data);

  // add max mem size
  io_dev(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc);

  /// Initialize memory with elf file.
  bool init_elf(const std::string& path);
};
