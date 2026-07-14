// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// -*- c++ -*-

#pragma once

#include <unistd.h>
#include "src/sysmod/trickbox/subdevice.h"
#include <iostream>
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
#include "whisper_client.h"

class evt_trigger : public subdevice {
public:
  evt_trigger(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc);
  // Destructor.
  virtual ~evt_trigger();

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

  cvm::messenger::task<void> read(uint64_t addr, size_t length, data_t& data);
  void read_dev(uint64_t addr, size_t length, data_t& data) override;
  virtual void write(uint64_t addr, size_t length, const data_t& data,
                     const strb_t& strb) override;
  void reset() override {
  }

protected:
private:
  std::array<uint64_t, 1024> tboxmem_{};
  uint64_t tboxmem_base_ = 0x9078000;

  //unsigned hartCount;
};
