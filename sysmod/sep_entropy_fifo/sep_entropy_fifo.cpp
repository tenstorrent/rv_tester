#include <iostream>
#include "cvm/plusargs.hpp"
#include "sep_entropy_fifo.h"

DEFINE_uint32(entropy_fifo_size, 8, "Size of the entropy FIFO");
void sep_entropy_fifo::read(const transactor::read_t& r, data_t& data) {
  auto pa = r.addr;
  auto length = r.length;
  uint64_t fifo_base = addr();
  uint64_t fifo_size = size();
  if ((pa+length) > (fifo_base+fifo_size)) {
    cvm::log(cvm::ERROR, "Error: Read outside of device range for sep_entropy_fifo, addr={:x} size={}\n", pa, length);
    return;
  } else if (length > FLAGS_entropy_fifo_size) {
    cvm::log(cvm::ERROR, "Error: Reads with length greater than {} are not supported for sep_entropy_fifo\n", FLAGS_entropy_fifo_size);
    return;
  }

  while (fifo_.size() < FLAGS_entropy_fifo_size)
    fifo_.push(rng1());

  while (length-- > 0) {
    if (pa++ == fifo_base)
      data.push_back(FLAGS_entropy_fifo_size);
    else
      data.push_back(fifo_.front());
    fifo_.pop();
  }
}
