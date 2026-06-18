// -*- c++ -*-

#pragma once

#include <unistd.h>
#include "src/sysmod/trickbox/subdevice.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <cmath>
#include "common/pcg_random.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/messenger.hpp"
#include <mem_manager.h>

// Define post_si_pcietc_helper at the given address
class post_si_pcietc_helper : public subdevice {
public:
  /// Define a post_si_pcietc_helper device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  post_si_pcietc_helper(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc);
  // Destructor.
  virtual ~post_si_pcietc_helper();

  // Copy n bytes from the given integer, x, to the data iterator
  // following little endian convention. If n is larger than the size
  // of x, then copy zero bytes after copying the bytes of x.
  template <typename INT>
  void serializeInt(INT x, size_t n, data_t& data) {
    for (unsigned i = 0; i < n; ++i, x >>= 8)
      data[i] = x & 0xff;
  }

  // Copy bytes from data iterator into the given integer following
  // lilttle endian convention.
  template <typename INT>
  void deserializeInt(const data_t& data, INT& x) {
    x = 0;
    for (unsigned i = 0; i < sizeof(x); ++i)
      x |= INT(data[i]) << i * 8;
  }

  void read_dev(uint64_t addr, size_t length, data_t& data) override;

  virtual void write(uint64_t addr, size_t length, const data_t& data,
                     const strb_t& strb) override;
  void reset() override {
  }

protected:
  //Check plusarg usage
  void checkUsage();

private:
  uint64_t post_si_pcietc_helper_base = 0x9089800;
  cvm::topology::loc_t overlay_driver_loc_;

  pcg_extras::seed_seq_from<std::random_device> seed_source;
  pcg32 rng;

public:
};
