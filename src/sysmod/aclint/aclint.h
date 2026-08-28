// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// -*- c++ -*-

#pragma once

#include <mutex>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <functional>
#include "src/sysmod/device.h"
#include "src/sysmod/clint/clint.h"
#include "cvm/registry.hpp"
#include "cvm/plusargs.hpp"
#include "transactor.h"

DECLARE_uint64(aclint_mtime_offset);
DECLARE_uint64(aclint_timesync_offset);
DECLARE_uint64(aclint_mtimecmp0_offset);

// Define a core local interruptor (aClint) at the given address
// and for the given hart count. The size will be 48k bytes.
class aclint : public device {
public:
  /// Define a aCLINT device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  /// axiMstLoc / ctimeAddr are used to broadcast mtime to the core CTIME MMR
  aclint(const std::string& tag, uint64_t addr, unsigned hartCount,
         cvm::topology::loc_t loc,
         cvm::topology::loc_t axiMstLoc = {}, uint64_t ctimeAddr = 0);

  // Destructor.
  virtual ~aclint();

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

  /// Read length bytes from the given address to the data iterator.
  /// No-op if address is outside the range of this aclint or if
  /// address is not properly aligned.
  void read(const transactor::read_t& r, data_t& data);

  // Write to this aclint. Call softwareInterrupt with flag set to 0/1
  // if a hart software interrupt entry is written. Update time
  // compare and call timerInterrupt if a hart time compare entry is
  // written. Call timerInterrupt on every hart if timer is written.
  //
  // This is a no-op if address is not aligned, if length is not 4 for
  // software interrupt entries, if length is not 8 for
  // timer/time-compare entries.
  void write(const transactor::write_t& w);

protected:
  // Broadcast the current mtime to the core CTIME MMR via an AXI master
  // write.
  void broadcastTime();

private:
  unsigned hartCount_ = 1;

  std::vector<uint32_t> soft_;        // Software interrupt: one per hart.
  std::vector<uint64_t> timeCompare_; // mtimecmp mirror (MMR reads); one per hart.

  cvm::topology::loc_t axiMstLoc_; // AXI master used for time broadcast.
  uint64_t ctimeAddr_ = 0;         // Core CTIME MMR target (0 = disabled).

  std::mutex mutex_;
};
