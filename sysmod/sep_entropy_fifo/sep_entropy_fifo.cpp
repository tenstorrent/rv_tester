#include <iostream>
#include "cvm/plusargs.hpp"
#include "sep_entropy_fifo.h"

DEFINE_uint32(entropy_fifo_size, 8, "Size of the entropy FIFO");
sep_entropy_fifo::sep_entropy_fifo(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc)
    : device(tag, addr, 0xc000, loc, &sep_entropy_fifo::write, &sep_entropy_fifo::read, this){
  cvm::log(cvm::HIGH, "sep_entropy_fifo created with size {}\n", FLAGS_entropy_fifo_size);
}

void
sep_entropy_fifo::read(const transactor::read_t& r, data_t& data) {

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

  uint64_t dword = 0;
  for (size_t i=0; i<length; i++) {
    if (pa+i == fifo_base) {
      dword |= FLAGS_entropy_fifo_size << (i*8);
    } else if (pa+i < fifo_base+8) {
        // do nothing (return zero)
    } else {
      dword |= uint64_t(fifo_.front()) << (i*8);
      fifo_.pop();
    }
  }
  serializeInt(dword, length, data);
}
