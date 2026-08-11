// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "src/transactors/axi_sw/eam.hpp"
#include "cvm/logger.hpp"
#include "cvm/random.hpp"
#include "rv_tester_plusargs.h"

DEFINE_int32(eam_decline_excl_pct, 0, "Percentage of exclusive reads (ARLOCK) the EAM declines: no reservation is taken and OKAY is returned instead of EXOKAY");
DEFINE_string(eam_pass_invalidate_ratio, "1:0", "Exclusive write (AWLOCK) pass/fail pattern in format 'n:e': n exclusive writes succeed, then e are failed (reservation invalidated), repeating");

eam& eam::instance() {
  static eam inst;
  return inst;
}

eam::eam() {
  const std::string& p = FLAGS_eam_pass_invalidate_ratio;
  const size_t delim = p.find(':');
  if (delim == std::string::npos) {
    cvm::log(cvm::ERROR, "Error: [eam] +eam_pass_invalidate_ratio='{}' is not in 'n:e' format\n", p);
    return;
  }

  try {
    pattern_pass_ = std::stoul(p.substr(0, delim));
    pattern_fail_ = std::stoul(p.substr(delim + 1));
  } catch (const std::exception&) {
    cvm::log(cvm::ERROR, "Error: [eam] +eam_pass_invalidate_ratio='{}' is not in 'n:e' format\n", p);
    pattern_pass_ = 1;
    pattern_fail_ = 0;
    return;
  }

  if (pattern_pass_ + pattern_fail_ == 0) {
    cvm::log(cvm::ERROR, "Error: [eam] +eam_pass_invalidate_ratio='{}' must have n+e greater than zero\n", p);
    pattern_pass_ = 1;
    pattern_fail_ = 0;
  }

  cvm::log(cvm::HIGH, "[eam] config: decline_excl_pct={}, pass_invalidate_ratio={}:{}\n",
           FLAGS_eam_decline_excl_pct, pattern_pass_, pattern_fail_);
}

eam::~eam() {
  if (FLAGS_metrics) {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"eam_declined_exclusive_read_count\": \"{}\"}}\n", declined_excl_read_count_);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"eam_forced_fail_exclusive_write_count\": \"{}\"}}\n", forced_fail_excl_write_count_);
  }
}

bool eam::decline_excl_read() {
  if (FLAGS_eam_decline_excl_pct <= 0)
    return false;

  const uint64_t r = cvm::rand::lcg::generate<uint64_t>(100);
  return int32_t(r) < FLAGS_eam_decline_excl_pct;
}

bool eam::pattern_fail_excl_write() {
  const unsigned period = pattern_pass_ + pattern_fail_;
  const unsigned phase = excl_wr_count_ % period;
  excl_wr_count_ = (excl_wr_count_ + 1) % period;
  return phase >= pattern_pass_;
}

bool eam::fields_match(const eam_entry& e, const axi::a_t& a) {
  // AxCACHE is deliberately NOT compared: cache_mem_attr_t is the decoded
  // AxCACHE attribute, and the allocate-hint bits carry read-channel vs
  // write-channel meaning (e.g. WB_RA on AR vs WB_WA on AW for the same
  // region), so a legal LR/SC pair can legitimately differ here. It is still
  // captured in the entry for debug.
  return e.rsv_addr == rsv_base(a.addr) &&
         e.len == a.len &&
         e.burst == a.burst &&
         e.prot == a.prot;
}

void eam::span(const axi::a_t& a, axi::addr_t& first, axi::addr_t& last) {
  const axi::addr_t num_bytes = axi::addr_t(1) << a.size;
  const axi::addr_t aligned_addr = a.addr / num_bytes * num_bytes;
  const axi::addr_t burst_len = axi::addr_t(a.len) + 1;
  const axi::addr_t dtsize = num_bytes * burst_len;

  if (a.burst == axi::BURST_FIXED) {
    // Every beat targets the same address window.
    first = aligned_addr;
    last = aligned_addr + num_bytes - 1;
  } else if (a.burst == axi::BURST_WRAP) {
    // Wrapping bursts stay inside their dtsize-aligned container.
    first = a.addr / dtsize * dtsize;
    last = first + dtsize - 1;
  } else {
    first = aligned_addr;
    last = aligned_addr + dtsize - 1;
  }
}

void eam::invalidate_overlaps(const axi::a_t& a) {
  axi::addr_t first, last;
  span(a, first, last);

  const std::size_t own = index(a.id);
  for (std::size_t i = 0; i < NUM_ENTRIES; i++) {
    if (i == own || !t_[i].valid)
      continue;

    const axi::addr_t rsv_first = t_[i].rsv_addr;
    const axi::addr_t rsv_last = rsv_first + RSV_BYTES - 1;
    if (first <= rsv_last && rsv_first <= last) {
      t_[i].valid = false;
      cvm::log(cvm::HIGH, "[eam] invalidate: entry={}, entry_id={}, rsv_addr={:#x} by write id={}, addr={:#x}\n",
               i, t_[i].id, rsv_first, a.id, a.addr);
    }
  }
}

eam_verdict eam::on_addr(const axi::a_t& a) {
  eam_verdict v;

  if (a.lock && a.atop.transaction != axi::NON_ATOMIC) {
    cvm::log(cvm::ERROR, "Error: [eam] exclusive access combined with an atomic transaction is not supported: id={}, addr={:#x}\n",
             a.id, a.addr);
  }

  if (!a.w) {
    if (!a.lock)
      return v;

    // DV knob: randomly decline the exclusive read. No reservation is taken
    // (and any stale one for this ID is dropped) so a later SC cannot pass.
    if (decline_excl_read()) {
      declined_excl_read_count_++;
      t_[index(a.id)].valid = false;
      cvm::log(cvm::HIGH, "[eam] exclusive read declined (+eam_decline_excl_pct={}): entry={}, id={}, addr={:#x}\n",
               FLAGS_eam_decline_excl_pct, index(a.id), a.id, a.addr);
      v.override_resp = true;
      v.resp = axi::RESP_OKAY;
      return v;
    }

    // Exclusive read: install/overwrite the reservation for this ID.
    eam_entry& e = t_[index(a.id)];
    e = eam_entry{true, a.id, rsv_base(a.addr), a.len, a.burst, a.size, a.prot, a.cache};
    cvm::log(cvm::HIGH, "[eam] reserve: entry={}, id={}, addr={:#x}, rsv_addr={:#x}\n",
             index(a.id), a.id, a.addr, e.rsv_addr);

    v.override_resp = true;
    v.resp = axi::RESP_EXOKAY;
    return v;
  }

  // Every write, exclusive or not, kills overlapping reservations of other IDs.
  invalidate_overlaps(a);

  if (!a.lock)
    return v;

  // Exclusive write: only its own entry can grant success.
  eam_entry& e = t_[index(a.id)];
  const bool was_valid = e.valid;
  // DV knob: force-fail this exclusive write per the 'n:e' pattern.
  const bool forced_fail = pattern_fail_excl_write();
  const bool pass = was_valid && fields_match(e, a) && !forced_fail;
  // Count writes that would have passed but were failed solely by forced_fail.
  if (forced_fail && was_valid && fields_match(e, a))
    forced_fail_excl_write_count_++;
  e.valid = false;

  v.override_resp = true;
  if (pass) {
    v.resp = axi::RESP_EXOKAY;
    cvm::log(cvm::HIGH, "[eam] exclusive write pass: entry={}, id={}, addr={:#x}\n", index(a.id), a.id, a.addr);
  } else {
    v.allow_write = false;
    v.resp = axi::RESP_OKAY;
    cvm::log(cvm::HIGH, "[eam] exclusive write fail (squashed): entry={}, id={}, addr={:#x}, entry_valid={}, forced_fail={}\n",
             index(a.id), a.id, a.addr, was_valid, forced_fail);
  }
  return v;
}
