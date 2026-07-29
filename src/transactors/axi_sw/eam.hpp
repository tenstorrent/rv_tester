// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <array>
#include <cstddef>
#include "axi.h"

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

private:
  eam() = default;

  static std::size_t index(axi::id_t id) { return id % NUM_ENTRIES; }
  static axi::addr_t rsv_base(axi::addr_t addr) { return addr & ~(RSV_BYTES - 1); }
  static bool fields_match(const eam_entry& e, const axi::a_t& a);

  // Byte span [first, last] touched by the transaction, conservatively
  // computed for FIXED/INCR/WRAP bursts.
  static void span(const axi::a_t& a, axi::addr_t& first, axi::addr_t& last);

  void invalidate_overlaps(const axi::a_t& a);

  std::array<eam_entry, NUM_ENTRIES> t_{};
};
