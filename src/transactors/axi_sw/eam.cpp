// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "eam.hpp"
#include "cvm/logger.hpp"

eam& eam::instance() {
  static eam inst;
  return inst;
}

bool eam::fields_match(const eam_entry& e, const axi::a_t& a) {
  return e.rsv_addr == rsv_base(a.addr) &&
         e.len == a.len &&
         e.burst == a.burst &&
         e.size == a.size &&
         e.prot == a.prot &&
         e.cache == a.cache;
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
  const bool pass = was_valid && fields_match(e, a);
  e.valid = false;

  v.override_resp = true;
  if (pass) {
    v.resp = axi::RESP_EXOKAY;
    cvm::log(cvm::HIGH, "[eam] exclusive write pass: entry={}, id={}, addr={:#x}\n", index(a.id), a.id, a.addr);
  } else {
    v.allow_write = false;
    v.resp = axi::RESP_OKAY;
    cvm::log(cvm::HIGH, "[eam] exclusive write fail (squashed): entry={}, id={}, addr={:#x}, entry_valid={}\n",
             index(a.id), a.id, a.addr, was_valid);
  }
  return v;
}
