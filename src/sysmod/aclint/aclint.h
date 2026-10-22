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
#include "src/sysmod/aclint/aclint_cfg.h"
#include "cvm/registry.hpp"
#include "cvm/plusargs.hpp"
#include "transactor.h"

DECLARE_uint64(aclint_mtime_offset);
DECLARE_uint64(aclint_timesync_offset);
DECLARE_uint64(aclint_mtimecmp0_offset);
DECLARE_uint64(aclint_ctime_addr);
DECLARE_bool(aclint_ctime_per_hart);
DECLARE_uint64(aclint_ctime_hart_stride);

// Define a core local interruptor (aClint) at the given address
// and for the given hart count.
class aclint : public device {
public:
  // mtimecmp0-7 (8 physical cores) plus mtimecmp8 (cluster wakeup) occupy a
  // contiguous stride-8 window and are implemented regardless of how many
  // harts are populated, so the MMR array is not sized from hartCount.
  static constexpr unsigned NUM_MTIMECMP = 9;

  // Matches the RTL CSR_MTIMECMP*_RESET_VALUE and the Whisper timecmp_reset;
  // kept in step with MTIMECMP_RESET in aclint.sv.
  static constexpr uint64_t MTIMECMP_RESET = 0xffffffffffffffffULL;

  /// Define a aCLINT device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + size - 1]
  /// axiMstLoc / ctimeAddr are used to broadcast mtime to the core CTIME MMR
  aclint(const std::string& tag, uint64_t addr, uint64_t size, unsigned hartCount,
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

  // Write to this aclint. Update time compare on a mtimecmp entry
  // write; start a time synchronization on a mtime or timesync write.
  //
  // This is a no-op if address is not aligned, if length is not 4 or 8.
  void write(const transactor::write_t& w);

protected:
  // Time synchronization towards the cores. x is the mtime value the cores
  // have to align to (spec 7.5.2 two-phase local sync, or the legacy single
  // step broadcast when local sync is disabled).
  void startSync(uint64_t x);

  // Issue the CTIME MMR write(s) carrying value: one broadcast write, or one
  // write per populated hart window.
  void writeCtime(uint64_t value);

private:
  unsigned hartCount_ = 1;

  std::vector<uint32_t> soft_;        // Software interrupt: one per hart.
  std::vector<uint64_t> timeCompare_; // mtimecmp mirror (MMR reads); NUM_MTIMECMP entries.

  cvm::topology::loc_t axiMstLoc_; // AXI master used for time broadcast.
  uint64_t ctimeAddr_ = 0;         // Core CTIME MMR target (0 = disabled).

  aclint_cfg_t cfg_;

  std::mutex mutex_;
};
