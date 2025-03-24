#pragma once

#include "sysmod/device.h"
#include "cvm/logger.hpp"
#include "cvm/random.hpp"

class sep_entropy_fifo : public device
{
  private:
    std::queue<uint8_t> fifo_;
    cvm::rand::uniform_dist<uint8_t> rng1;
  public:
    void write(const transactor::write_t&) {
      cvm::log(cvm::ERROR, "Error: Writes currently not supported for sep_entropy_fifo\n");
    };
    void read(const transactor::read_t& r, data_t& data);
};
