// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include "cvm/plusargs.hpp"
#include "src/transactors/axi_sw/axi.h"

DECLARE_int32(eam_decline_excl_pct);
DECLARE_string(eam_pass_invalidate_ratio);
DECLARE_uint64(eam_reservation_ttl);

// AXI Exclusive Access Monitor (EAM).
//
// Direct-mapped reservation table of 8 entries, indexed by AXId[2:0]. An
// exclusive read (ARLOCK) installs/overwrites the entry for its ID and is
// answered with EXOKAY. Any write invalidates reservations owned by *other*
// IDs whose 64B reservation set it overlaps. An exclusive write (AWLOCK)
// succeeds (EXOKAY) only if its own entry is still valid and its control
// fields match the ones captured at reservation time; otherwise the write is
// squashed and answered with OKAY.
//
// The table is system-wide (a process-wide singleton), so a reservation
// registered on one axi port can be cleared or matched from another. All
// axi::operator()() bodies run on the same cvm scheduler, so no locking is
// required; a mutex would be needed if axi instances were ever driven from
// separate threads.
struct eam_entry {
  bool valid = false;
  axi::id_t id = 0;
  axi::addr_t rsv_addr = 0;
  axi::len_t len = axi::len_t(0);
  axi::burst_t burst = axi::BURST_INCR;
  axi::sz_t size = axi::sz_t(0);
  axi::prot_t prot = axi::prot_t(0);
  axi::cache_mem_attr_t cache = axi::cache_mem_attr_t(0);
  // TB clock count at which the reservation was taken, used by
  // +eam_reservation_ttl. Zero when the knob is disabled.
  uint64_t rsv_cycle = 0;
};

struct eam_verdict {
  bool allow_write = true;
  bool override_resp = false;
  axi::resp_t resp = axi::RESP_OKAY;
};

class eam {
public:
  static constexpr std::size_t NUM_ENTRIES = 8;
  static constexpr axi::addr_t RSV_BYTES = 64;

  static eam& instance();

  // Single decision point, called once per transaction after the address is
  // dequeued and before the beat loop.
  eam_verdict on_addr(const axi::a_t& a);

  // Trickbox (eam_helper) programmable knobs. Currently only stored/reported;
  // no behavioural effect on the reservation table yet.
  // The expected programming sequence from a test is: set the LR/SC loop
  // address, set the (randomised) fail count, then arm the mechanism with
  // set_tb_fail_en(true) so that counting only starts once both operands are
  // stable.
  void set_tb_fail_addr(uint64_t addr);
  void set_tb_fail_cnt(uint64_t cnt);
  void set_tb_fail_en(bool en);
  uint64_t tb_fail_addr() const { return tb_fail_addr_; }
  uint64_t tb_fail_cnt() const { return tb_fail_cnt_; }
  bool tb_fail_en() const { return tb_fail_en_; }

private:
  // Returns true if this exclusive write must be failed by the trickbox
  // injection mechanism: tb_fail_en_ is armed, the address matches
  // tb_fail_addr_ exactly and fewer than tb_fail_cnt_ failures have been
  // injected since the last arming. Consumes one count.
  bool tb_fail_excl_write(const axi::a_t& a);

  eam();
  ~eam();

  // Returns true if this exclusive read should be declined per
  // +eam_decline_excl_pct.
  bool decline_excl_read();

  // Returns true if this exclusive write must be failed per the "n:e" pattern
  // of +eam_pass_invalidate_ratio. Advances the mod-(n+e) counter.
  bool pattern_fail_excl_write();

  // Current TB clock count, fetched from SV. Returns 0 (no DPI call) when
  // +eam_reservation_ttl is disabled or the rv_tester scope is not registered.
  static uint64_t current_cycles();

  // Returns true if the reservation has been held for at least
  // +eam_reservation_ttl cycles and must therefore fail the exclusive write.
  bool ttl_expired(const eam_entry& e, uint64_t now);

  static std::size_t index(axi::id_t id) { return id % NUM_ENTRIES; }
  static axi::addr_t rsv_base(axi::addr_t addr) { return addr & ~(RSV_BYTES - 1); }
  static bool fields_match(const eam_entry& e, const axi::a_t& a);

  // Byte span [first, last] touched by the transaction, conservatively
  // computed for FIXED/INCR/WRAP bursts.
  static void span(const axi::a_t& a, axi::addr_t& first, axi::addr_t& last);

  void invalidate_overlaps(const axi::a_t& a);

  std::array<eam_entry, NUM_ENTRIES> t_{};

  // +eam_pass_invalidate_ratio = "n:e": n successes then e failures, repeating.
  unsigned pattern_pass_ = 1;
  unsigned pattern_fail_ = 0;
  unsigned excl_wr_count_ = 0;

  // Programmed by the trickbox eam_helper subdevice.
  uint64_t tb_fail_addr_ = 0;
  uint64_t tb_fail_cnt_ = 0;
  bool tb_fail_en_ = false;
  // Number of failures injected since tb_fail_en_ was last written to 1.
  uint64_t tb_fail_injected_ = 0;

  // Metrics.
  uint64_t declined_excl_read_count_ = 0;
  uint64_t forced_fail_excl_write_count_ = 0;
  uint64_t tb_fail_excl_write_count_ = 0;
  uint64_t ttl_fail_excl_write_count_ = 0;
};
