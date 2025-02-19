#pragma once

#include "aplic/Aplic.hpp"
#include "sysmod/device.h"

class aplic_device : public device {
public:
  aplic_device(std::string tag, uint64_t addr, size_t size,
      cvm::topology::loc_t loc, std::shared_ptr<TT_APLIC::Aplic> aplic)
    : device(tag, addr, size, loc, &aplic_device::write, &aplic_device::read,
        this), aplic_(aplic) {}

  void read(const transactor::read_t& r, data_t& data);
  void write(const transactor::write_t& w);

private:
  std::shared_ptr<TT_APLIC::Aplic> aplic_;

  // Copy n bytes from the given integer, x, to the data iterator
  // following little endian convention. If n is larger than the size
  // of x, then copy zero bytes after copying the bytes of x.
  template <typename INT>
  void serializeInt(INT x, size_t n, data_t& data)
  {
    for (unsigned i = 0; i < n; ++i, x >>= 8)
      data[i] = x & 0xff;
  }

  // Copy bytes from data iterator into the given integer following
  // lilttle endian convention.
  template <typename INT>
    void deserializeInt(const data_t& data, INT& x)
    {
      x = 0;
      for (unsigned i = 0; i < sizeof(x); ++i)
        x |= INT(data[i]) << i*8;
    }
};
