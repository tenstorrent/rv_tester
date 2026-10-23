// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// -*- c++ -*-
#pragma once

#include "src/sysmod/device.h"
#include "cvm/topology.hpp"
#include "cvm/logger.hpp"
#include <set>

/// Model an htif (host target interface) device
class htif : public device {

public:
  htif(const std::string& tag, uint64_t addr, cvm::topology::loc_t loc);

  virtual ~htif();

  // Reads outside of device range are ignored. Reads with length
  // different than 8 are ignored.
  void read(const transactor::read_t& r, data_t& data);

  // Writes outside of device range are ignored. Writes with length
  // different than 8 are ignored.
  void write(const transactor::write_t& w);

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

  struct terminate_t {
    bool low_priority_based = false;
    bool passed = false;
  };

  struct exit_t {
    uint64_t terminate : 1;
    uint64_t code : 47;
    uint64_t cmd : 8;
    uint64_t dev : 8;
  };

  struct hart_exit_t {
    uint64_t terminate : 1;
    uint64_t code : 15;
    uint64_t hart_id : 16;
    uint64_t reserved : 16;
    uint64_t cmd : 8;
    uint64_t dev : 8;
  };

private:
  void hart_exit(const hart_exit_t& exit, uint32_t axi_id);
  void tohost_exit(const exit_t& exit, uint32_t axi_id);
  void end_test(bool passed);
  std::string exit_progress() const;

  uint64_t to_ = 0;
  uint64_t from_ = 0;
  cvm::file_logger htif_log_;

  class pty {
  private:
    int master = -1;
    int slave = -1;

  public:
    pty();
    ~pty();
    int read();
    int write(char c);
  };
  pty pty_;

  int passed_ = 0;
  int failed_ = 0;
  std::set<uint16_t> enabled_harts_;
  std::set<uint16_t> exited_harts_;
  std::set<uint16_t> failed_harts_;
  bool ended_ = false;
};
