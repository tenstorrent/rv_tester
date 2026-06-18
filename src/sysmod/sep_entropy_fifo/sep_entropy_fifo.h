#pragma once

#include "src/sysmod/device.h"
#include "cvm/logger.hpp"
#include "cvm/random.hpp"

class sep_entropy_fifo : public device {
private:
  std::queue<uint32_t> fifo_;
  cvm::rand::uniform_dist<uint32_t> rng1;

public:
  sep_entropy_fifo(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc);
  void write(const transactor::write_t&) {
    cvm::log(cvm::ERROR, "Error: Writes currently not supported for sep_entropy_fifo\n");
  };
  void read(const transactor::read_t& r, data_t& data);
  void fill_fifo();
  template <typename INT>
  void serializeInt(INT x, size_t n, data_t& data) {
    for (unsigned i = 0; i < n; ++i, x >>= 8)
      data[i] = x & 0xff;
  }
};
