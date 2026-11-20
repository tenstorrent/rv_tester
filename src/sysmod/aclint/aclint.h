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
#include "src/transactors/axi_sw/axi.h"
#include "cvm/registry.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "transactor.h"
#include "rv_tester_transactions.hpp"

DECLARE_uint64(aclint_mtime_offset);
DECLARE_uint64(aclint_timesync_offset);
DECLARE_uint64(aclint_mtimecmp0_offset);
DECLARE_string(aclint_scratch_offsets);
DECLARE_uint64(aclint_ctime_hart_addr);
DECLARE_bool(aclint_ctime_per_hart);
DECLARE_uint64(aclint_ctime_hart_stride);
DECLARE_uint64(aclint_local_sync_latency);
DECLARE_uint64(aclint_timer_update_latency);
DECLARE_uint64(aclint_c_mscfg1_addr);

// Define a core local interruptor (aClint) at the given address
// and for the given hart count.
class aclint : public device {
public:
  // mtimecmp0-7 plus mtimecmp8 (cluster wakeup), independent of the populated harts.
  static constexpr unsigned NUM_MTIMECMP = 9;

  // RTL CSR_MTIMECMP*_RESET_VALUE; mirrored in aclint.sv.
  static constexpr uint64_t MTIMECMP_RESET = 0xffffffffffffffffULL;

  // timesync, timesync1..timesync7: one hart-select bit per hart, 64 harts per
  // register, contiguous from aclint_timesync_offset. A write syncs the harts
  // whose bits are set; bits of unpopulated harts are ignored.
  static constexpr unsigned TIMESYNC_HARTS_PER_REG = 64;
  static constexpr unsigned NUM_TIMESYNC_REGS = 8;
  static constexpr unsigned MAX_TIMESYNC_HARTS = TIMESYNC_HARTS_PER_REG * NUM_TIMESYNC_REGS;
  // Hart-select mask, one word per timesync register.
  using hart_mask_t = std::vector<uint64_t>;

  // c_mscfg1: DisAclintTimeSync [9] (reset 1 = legacy load on CTIME write),
  // AclintProgRefTimeIncr [17:10] (reset 10, 0 reads as 10).
  static constexpr uint8_t DEFAULT_REF_TIME_INCR = 10;
  static constexpr unsigned C_MSCFG1_DIS_TIME_SYNC_BIT = 9;
  static constexpr unsigned C_MSCFG1_REF_TIME_INCR_LSB = 10;
  static constexpr uint64_t C_MSCFG1_RESET_FIELDS =
      (uint64_t(1) << C_MSCFG1_DIS_TIME_SYNC_BIT) | (uint64_t(DEFAULT_REF_TIME_INCR) << C_MSCFG1_REF_TIME_INCR_LSB);

  /// Define a aCLINT device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + size - 1]
  /// axiMstLoc / ctimeAddr are used to broadcast mtime to the core CTIME MMR
  aclint(const std::string& tag, uint64_t addr, uint64_t size, unsigned hartCount,
         cvm::topology::loc_t loc,
         cvm::topology::loc_t axiMstLoc = {}, uint64_t ctimeAddr = 0);

  virtual ~aclint();

  // csrw arrives on m_rvfi, hardware CSR updates on m_csri.
  void configure() override;
  void process(const rv_tester_transactions::cosim::m_rvfi<>& rvfi);
  void process(const rv_tester_transactions::cosim::m_csri<>& csri);

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

  // MMR read; an unmodelled register or access size answers DECERR.
  cvm::messenger::task<std::uint8_t> read(const read_t& r, data_t& data);

  // MMR write: mtimecmp updates the compare, mtime and timesync start a time
  // synchronization. An unmodelled register or access size answers DECERR.
  cvm::messenger::task<std::uint8_t> write(const transactor::write_t& w);

  // Commit strobe started in SV; logged with the harts it reaches.
  void syncPulse(uint64_t mtime, uint64_t target);

protected:
  // Align the selected harts to mtime x: two-phase local sync, or the legacy broadcast.
  void startSync(uint64_t x, const hart_mask_t& harts);

  // One broadcast write when every populated hart is selected, else one write
  // per selected hart window.
  void writeCtime(uint64_t value, const hart_mask_t& harts);

  // Issue one CTIME write and log its completion.
  void issueCtimeWrite(uint64_t addr, uint64_t value, int hart);

  void trackCsrWrite(unsigned hart, uint64_t addr, uint64_t mask, uint64_t data);

  hart_mask_t allHarts() const;
  hart_mask_t timesyncHarts(unsigned reg, uint64_t select) const;
  bool hartSelected(const hart_mask_t& harts, unsigned hart) const;
  bool allSelected(const hart_mask_t& harts) const;
  unsigned selectedCount(const hart_mask_t& harts) const;
  std::string hartList(const hart_mask_t& harts) const;

private:
  unsigned hartCount_ = 1;

  std::vector<uint32_t> soft_;        // Software interrupt: one per hart.
  std::vector<uint64_t> timeCompare_; // mtimecmp mirror (MMR reads); NUM_MTIMECMP entries.
  std::vector<uint64_t> timesync_;    // timesync register mirror (MMR reads); NUM_TIMESYNC_REGS entries.
  std::vector<uint64_t> scratchOffsets_; // plain read/write MMRs
  std::vector<uint64_t> scratch_;

  cvm::topology::loc_t axiMstLoc_; // AXI master used for time broadcast.
  uint64_t ctimeAddr_ = 0;         // Core CTIME broadcast target (0 = disabled).
  cvm::messenger::pool<transactor::write_response_t>::channel_info writeRespChannel_{};
  unsigned ctimeWrOutstanding_ = 0;
  hart_mask_t pendingSyncHarts_;   // harts staged by the sync in flight

  bool localSync_ = false;                      // c_mscfg1.DisAclintTimeSync == 0
  uint8_t refTimeIncr_ = DEFAULT_REF_TIME_INCR; // c_mscfg1.AclintProgRefTimeIncr
  std::vector<uint64_t> cMscfg1_;               // per-hart c_mscfg1 shadow, tracked fields only

  std::mutex mutex_;
};
