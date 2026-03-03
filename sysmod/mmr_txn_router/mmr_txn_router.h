#pragma once

#include "sysmod/device.h"
#include "cvm/topology.hpp"
#include <string>
#include <queue>
#include "cvm/registry.hpp"
#include "transactor.h"
#include "transactors/axi_sw/axi.h"

class mmr_txn_router : public device {
public:

  void write(const transactor::write_t& w);
  cvm::messenger::task<void> read(const transactor::read_t& r, data_t& data);

  mmr_txn_router(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);
private:

  cvm::topology::loc_t axi_mst_loc_l;
  cvm::messenger::pool<axi::r_t>::channel_info channel;
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
