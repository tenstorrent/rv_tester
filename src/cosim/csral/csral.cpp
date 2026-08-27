// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "csral.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include <fmt/format.h>

#include "cvm/logger.hpp"
#include "cvm/registry.hpp"
#include "whisper_client.h"

// The complete set of masking conditions with save/restore enabled, as a
// comma list of "<gate_csr>.<gate_field>" names (the plusarg IS the enabled
// set — there is deliberately no YAML knob; docs/csral_plan.md decision 10).
// Empty disables save/restore everywhere. Unknown names and view-only
// conditions (delegation views have no storage to restore) are startup errors.
DEFINE_string(csral_save_restore, "misa.H", "Masking conditions with save/restore enabled, e.g. misa.H,menvcfg.STCE. Empty = none.");
// Runtime override of the generated kResetCheck severity (error|warn|off).
DEFINE_string(csral_reset_check, "", "Override the reset-check severity: error, warn, or off. Empty = use the generated default.");

namespace {

using WC = whisperClient<uint64_t>;

std::uint64_t positioned_mask(std::uint8_t msb, std::uint8_t lsb) {
  std::uint64_t mask = 0;
  for (int bit = lsb; bit <= msb && bit < 64; ++bit)
    mask |= 1ull << bit;
  return mask;
}

} // namespace

bool csral::field_alias_pair(std::uint16_t a, std::uint16_t b) const {
  for (std::size_t i = 0; i < CSRAL::kNumFieldAliases; ++i) {
    const auto& fa = CSRAL::kFieldAliases[i];
    if ((fa.csr == a && fa.alias_csr == b) || (fa.csr == b && fa.alias_csr == a))
      return true;
  }
  return false;
}

const CSRAL::field_t* csral::field_of(const CSRAL::csr_t& csr, std::string_view field_name) {
  for (std::size_t i = 0; i < csr.field_count; ++i) {
    const auto& f = CSRAL::kFields[csr.field_first + i];
    if (f.name == field_name)
      return &f;
  }
  return nullptr;
}

csral::csral(int num_harts, cvm::topology::loc_t whisper_loc)
    : num_harts_(num_harts),
      whisper_loc_(whisper_loc),
      harts_(static_cast<std::size_t>(num_harts)) {
  for (auto& h : harts_) {
    h.entries.resize(CSRAL::kNumCsrs);
    h.queued.assign(CSRAL::kNumCsrs, false);
    h.conds.resize(CSRAL::kNumConditions);
    for (std::size_t c = 0; c < CSRAL::kNumConditions; ++c) {
      h.conds[c].stash.assign(CSRAL::kConditions[c].target_count, 0);
      h.conds[c].stash_valid.assign(CSRAL::kConditions[c].target_count, false);
    }
  }

  // Per-CSR union of declared field bits: the value a view exposes of its
  // alias target, and the default alias-propagation mask.
  view_bits_cache_.resize(CSRAL::kNumCsrs);
  for (std::size_t i = 0; i < CSRAL::kNumCsrs; ++i) {
    std::uint64_t bits = 0;
    const auto& row = CSRAL::kCsrs[i];
    for (std::size_t f = 0; f < row.field_count; ++f)
      bits |= CSRAL::kFields[row.field_first + f].mask;
    view_bits_cache_[i] = bits;
  }

  conditions_by_gate_.resize(CSRAL::kNumCsrs);
  for (std::size_t c = 0; c < CSRAL::kNumConditions; ++c)
    conditions_by_gate_[CSRAL::kConditions[c].gate_csr].push_back(static_cast<std::uint16_t>(c));

  // Parse +csral_save_restore: the plusarg is the complete enabled set.
  save_restore_enabled_.assign(CSRAL::kNumConditions, false);
  std::stringstream ss(FLAGS_csral_save_restore);
  std::string token;
  while (std::getline(ss, token, ',')) {
    token.erase(0, token.find_first_not_of(" \t"));
    token.erase(token.find_last_not_of(" \t") + 1);
    if (token.empty())
      continue;
    bool found = false;
    for (std::size_t c = 0; c < CSRAL::kNumConditions; ++c) {
      if (CSRAL::kConditions[c].name == token) {
        if (CSRAL::kConditions[c].view_only)
          throw std::invalid_argument(fmt::format("+csral_save_restore: condition '{}' targets only alias views (no storage to restore); it is mask-only", token));
        save_restore_enabled_[c] = true;
        found = true;
        break;
      }
    }
    if (!found)
      throw std::invalid_argument(fmt::format("+csral_save_restore: unknown condition '{}' (names are <gate_csr>.<gate_field>, e.g. misa.H)", token));
  }

  reset_check_ = [] {
    switch (CSRAL::kResetCheck) {
    case CSRAL::reset_check_t::warn:
      return reset_check_t::warn;
    case CSRAL::reset_check_t::off:
      return reset_check_t::off;
    case CSRAL::reset_check_t::error:
    default:
      return reset_check_t::error;
    }
  }();
  if (!FLAGS_csral_reset_check.empty()) {
    if (FLAGS_csral_reset_check == "error")
      reset_check_ = reset_check_t::error;
    else if (FLAGS_csral_reset_check == "warn")
      reset_check_ = reset_check_t::warn;
    else if (FLAGS_csral_reset_check == "off")
      reset_check_ = reset_check_t::off;
    else
      throw std::invalid_argument(fmt::format("+csral_reset_check: '{}' is not error|warn|off", FLAGS_csral_reset_check));
  }

  register_default_hooks();
  for (int h = 0; h < num_harts_; ++h)
    reset(static_cast<hart_id_t>(h));
}

// ---- reset / init ----------------------------------------------------------

void csral::reset(hart_id_t hart) {
  auto& h = harts_.at(hart);
  for (std::size_t i = 0; i < CSRAL::kNumCsrs; ++i) {
    h.entries[i] = entry_t{};
    h.entries[i].dut = CSRAL::kCsrs[i].reset;
    h.entries[i].iss = CSRAL::kCsrs[i].reset;
  }
  h.queue.clear();
  h.queued.assign(CSRAL::kNumCsrs, false);
  h.resynch_pending = false;
  for (std::size_t c = 0; c < CSRAL::kNumConditions; ++c) {
    h.conds[c].stash.assign(CSRAL::kConditions[c].target_count, 0);
    h.conds[c].stash_valid.assign(CSRAL::kConditions[c].target_count, false);
  }
  recompute_all_conditions(hart);
}

void csral::recompute_all_conditions(hart_id_t hart) {
  auto& h = harts_.at(hart);
  bool changed = true;
  while (changed) {
    changed = false;
    for (std::size_t c = 0; c < CSRAL::kNumConditions; ++c) {
      const auto& cond = CSRAL::kConditions[c];
      const bool now = (effective_dut_value(hart, cond.gate_csr) & cond.gate_mask) != 0;
      if (now != h.conds[c].active) {
        h.conds[c].active = now;
        changed = true;
      }
    }
  }
}

std::vector<csral::reset_mismatch_t> csral::init_check(hart_id_t hart) {
  std::vector<reset_mismatch_t> mismatches;
  auto& h = harts_.at(hart);
  for (std::size_t i = 0; i < CSRAL::kNumCsrs; ++i) {
    const auto& row = CSRAL::kCsrs[i];
    if (row.address == CSRAL::kNoDirectAddress)
      continue; // indirect-only: not peekable by CSR address
    std::uint64_t value, wmask, pmask, rmask;
    if (!whisper_peek_csr(hart, row.address, value, wmask, pmask, rmask, row.policy.may_not_exist))
      continue;
    if (reset_check_ != reset_check_t::off && value != row.reset) {
      // Drift severity follows the CSR's own policy: a CSR whose runtime
      // mismatches are skipped (vtype, dcsr, ...) should not fail reset
      // either — its drift is logged, not returned.
      if (row.policy.check && row.policy.check_reset && row.policy.on_mismatch == CSRAL::on_mismatch_t::error)
        mismatches.push_back({row.name, row.address, row.reset, value});
      else
        cvm::log(cvm::MEDIUM, "[csral] reset drift on '{}' (policy-demoted): spec={:#x} whisper={:#x}\n", row.name, row.reset, value);
    }
    // Whisper is the authority for the run; the check above exists to catch
    // spec-vs-whisper drift at time zero, not to overrule whisper.
    h.entries[i].dut = value;
    h.entries[i].iss = value;
    h.entries[i].dut_valid = h.entries[i].iss_valid = true;
  }
  recompute_all_conditions(hart);
  return mismatches;
}

// ---- mirror plumbing ---------------------------------------------------------

bool csral::exists(hart_id_t hart, std::uint32_t addr) const {
  const auto* row = CSRAL::find_by_address(addr);
  return row == nullptr || csr_exists(hart, static_cast<std::uint16_t>(CSRAL::index_of(*row)));
}

bool csral::csr_exists(hart_id_t hart, std::uint16_t csr_index) const {
  const auto exists_if = CSRAL::kCsrs[csr_index].exists_if;
  return exists_if < 0 || harts_.at(hart).conds[static_cast<std::size_t>(exists_if)].active;
}

void csral::apply(hart_id_t hart, src_t src, std::uint16_t csr_index, std::uint64_t data, std::uint64_t mask, std::uint64_t cycle, bool fan_out) {
  auto& e = harts_.at(hart).entries[csr_index];
  if (src == src_t::dut) {
    e.dut = (e.dut & ~mask) | (data & mask);
    e.dut_valid = true;
    e.last_dut_cycle = cycle;
  } else {
    e.iss = (e.iss & ~mask) | (data & mask);
    e.iss_valid = true;
    e.last_iss_cycle = cycle;
  }
  if (fan_out)
    fan_out_aliases(hart, src, csr_index, cycle);
  if (src == src_t::dut && !conditions_by_gate_[csr_index].empty())
    evaluate_conditions(hart, csr_index, cycle);
}

void csral::fan_out_aliases(hart_id_t hart, src_t src, std::uint16_t csr_index, std::uint64_t cycle) {
  const auto& e = harts_.at(hart).entries[csr_index];
  const std::uint64_t value = (src == src_t::dut) ? e.dut : e.iss;

  // CSR-level aliases are same-position views: a view propagates its declared
  // bits to its target and a target refreshes every view of it. Pairs that a
  // field alias connects are SHIFTED views (frm[2:0] lives at fcsr[7:5]);
  // there the field-alias rows are authoritative and the same-position
  // fan-out must stay out of the way.
  const auto& row = CSRAL::kCsrs[csr_index];
  if (row.alias_of >= 0 && !field_alias_pair(csr_index, static_cast<std::uint16_t>(row.alias_of)))
    apply(hart, src, static_cast<std::uint16_t>(row.alias_of), value, view_bits_cache_[csr_index], cycle, false);
  for (std::size_t i = 0; i < CSRAL::kNumCsrs; ++i) {
    if (CSRAL::kCsrs[i].alias_of == static_cast<std::int16_t>(csr_index) && !field_alias_pair(csr_index, static_cast<std::uint16_t>(i)))
      apply(hart, src, static_cast<std::uint16_t>(i), value, view_bits_cache_[i], cycle, false);
  }

  // Field-level aliases are shifted views (fcsr <-> fflags/frm).
  for (std::size_t i = 0; i < CSRAL::kNumFieldAliases; ++i) {
    const auto& fa = CSRAL::kFieldAliases[i];
    if (fa.csr == csr_index) {
      const std::uint64_t bits = (value & positioned_mask(fa.msb, fa.lsb)) >> fa.lsb;
      apply(hart, src, fa.alias_csr, bits << fa.alias_lsb, positioned_mask(fa.alias_msb, fa.alias_lsb), cycle, false);
    }
    if (fa.alias_csr == csr_index) {
      const std::uint64_t bits = (value & positioned_mask(fa.alias_msb, fa.alias_lsb)) >> fa.alias_lsb;
      apply(hart, src, fa.csr, bits << fa.lsb, positioned_mask(fa.msb, fa.lsb), cycle, false);
    }
  }
}

void csral::queue_check(hart_id_t hart, std::uint16_t csr_index, check_class_t cls) {
  if (!CSRAL::kCsrs[csr_index].policy.check)
    return;
  auto& h = harts_.at(hart);
  if (h.queued[csr_index])
    return;
  h.queued[csr_index] = true;
  h.queue.push_back({csr_index, cls});
}

// ---- conditions & save/restore ------------------------------------------------

std::uint64_t csral::inactive_condition_mask(hart_id_t hart, std::uint16_t csr_index) const {
  std::uint64_t mask = 0;
  const auto& h = harts_.at(hart);
  for (std::size_t c = 0; c < CSRAL::kNumConditions; ++c) {
    if (h.conds[c].active)
      continue;
    const auto& cond = CSRAL::kConditions[c];
    for (std::size_t t = 0; t < cond.target_count; ++t) {
      const auto& tgt = CSRAL::kMaskedTargets[cond.target_first + t];
      if (tgt.csr == csr_index)
        mask |= tgt.mask;
    }
  }
  return mask;
}

std::uint64_t csral::effective_dut_value(hart_id_t hart, std::uint16_t csr_index) const {
  // Gate values are read through the model's masked view: a gate behind an
  // off upstream gate reads 0 (the generator validated the graph is a DAG).
  return harts_.at(hart).entries[csr_index].dut & ~inactive_condition_mask(hart, csr_index);
}

void csral::evaluate_conditions(hart_id_t hart, std::uint16_t gate_csr_index, std::uint64_t cycle) {
  auto& h = harts_.at(hart);
  std::vector<std::uint16_t> worklist{gate_csr_index};
  while (!worklist.empty()) {
    const std::uint16_t gate = worklist.back();
    worklist.pop_back();
    for (std::uint16_t c : conditions_by_gate_[gate]) {
      const auto& cond = CSRAL::kConditions[c];
      const bool now = (effective_dut_value(hart, gate) & cond.gate_mask) != 0;
      auto& cs = h.conds[c];
      if (now == cs.active)
        continue;
      cs.active = now;
      std::vector<std::uint32_t> restored;
      if (!now) {
        // Off-edge: capture the masked field bits the DUT will retain. The
        // snapshot is STICKY until the next off-edge — while the condition is
        // off the masked bits are inaccessible to software (that is what
        // masked means), so writes during the off period must not touch the
        // stash (legacy kept the saved masked bits sticky the same way; a
        // stash refreshed from while-off mirror values restores the forced-0
        // reads instead of the retained state, which zeroed whisper's mideleg
        // VS bits after an H on-edge).
        if (save_restore_enabled_[c]) {
          for (std::size_t t = 0; t < cond.target_count; ++t) {
            const auto& tgt = CSRAL::kMaskedTargets[cond.target_first + t];
            cs.stash[t] = h.entries[tgt.csr].dut & tgt.mask;
            cs.stash_valid[t] = true;
          }
        }
      } else if (save_restore_enabled_[c]) {
        // On-edge: put the stashed bits back into whisper (write-through, so
        // the ISS mirror stays coherent by construction). Targets never
        // captured (the condition has been off since reset with no writes)
        // stay untouched — the legacy save/restore restored only saved
        // entries too.
        for (std::size_t t = 0; t < cond.target_count; ++t) {
          const auto& tgt = CSRAL::kMaskedTargets[cond.target_first + t];
          const auto& row = CSRAL::kCsrs[tgt.csr];
          if (!cs.stash_valid[t] || row.address == CSRAL::kNoDirectAddress)
            continue;
          bool ok = false;
          const std::uint64_t cur = peek(hart, row.address, &ok);
          if (!ok)
            continue;
          if (poke(hart, row.address, (cur & ~tgt.mask) | cs.stash[t], cycle))
            restored.push_back(row.address);
        }
      }
      // A flipped condition changes the effective value of its targets, which
      // may themselves gate further conditions (menvcfg -> henvcfg chains).
      for (std::size_t t = 0; t < cond.target_count; ++t) {
        const auto& tgt = CSRAL::kMaskedTargets[cond.target_first + t];
        if (!conditions_by_gate_[tgt.csr].empty())
          worklist.push_back(tgt.csr);
      }
      if (condition_cb_)
        condition_cb_(hart, cond, now, restored, cycle);
    }
  }
}

bool csral::condition_active(hart_id_t hart, std::string_view condition_name) const {
  for (std::size_t c = 0; c < CSRAL::kNumConditions; ++c) {
    if (CSRAL::kConditions[c].name == condition_name)
      return harts_.at(hart).conds[c].active;
  }
  throw std::invalid_argument(fmt::format("csral: unknown condition '{}'", condition_name));
}

bool csral::save_restore_enabled(std::string_view condition_name) const {
  for (std::size_t c = 0; c < CSRAL::kNumConditions; ++c) {
    if (CSRAL::kConditions[c].name == condition_name)
      return save_restore_enabled_[c];
  }
  throw std::invalid_argument(fmt::format("csral: unknown condition '{}'", condition_name));
}

// ---- DUT / ISS updates ---------------------------------------------------------

void csral::sw_write(hart_id_t hart, std::uint32_t addr, std::uint64_t wdata, std::uint64_t wmask, std::uint8_t priv, std::uint64_t cycle) {
  const auto* row = CSRAL::find_by_address(addr);
  if (row == nullptr) {
    // Not a spec CSR: legacy parity is to store nothing and never compare
    // (update_csr left the check disabled when the address lookup missed).
    cvm::log(cvm::HIGH, "[csral] sw write to CSR {:#x} not in the spec; not modeled\n", addr);
    return;
  }
  std::uint64_t data = wdata;
  // vl is exempt from whisper's write mask (port of modify_csr_mask's
  // special case: the DUT-reported mask applies as-is).
  std::uint64_t mask = (vl_addr_ != 0 && addr == vl_addr_) ? wmask : (wmask & write_mask(hart, addr));
  for (const auto& [hook_addr, hook] : hooks_) {
    if (hook_addr == addr)
      hook(*this, hart, addr, priv, wmask, data, mask);
  }
  const auto idx = static_cast<std::uint16_t>(CSRAL::index_of(*row));
  if (!csr_exists(hart, idx))
    return; // parity: the bridge dropped updates to hypervisor CSRs while misa.H=0
  apply(hart, src_t::dut, idx, data, mask, cycle);
  queue_check(hart, idx, check_class_t::sw_write);
}

void csral::hw_update(hart_id_t hart, std::uint32_t addr, std::uint64_t wdata, std::uint64_t wmask, std::uint64_t cycle) {
  const auto* row = CSRAL::find_by_address(addr);
  if (row == nullptr) {
    cvm::log(cvm::HIGH, "[csral] hw update to CSR {:#x} not in the spec; not modeled\n", addr);
    return;
  }
  const auto idx = static_cast<std::uint16_t>(CSRAL::index_of(*row));
  if (!csr_exists(hart, idx))
    return;
  apply(hart, src_t::dut, idx, wdata, wmask & poke_mask(hart, addr), cycle);
  queue_check(hart, idx, check_class_t::hw_update);
}

void csral::dut_force(hart_id_t hart, std::uint32_t addr, std::uint64_t wdata, std::uint64_t wmask, std::uint64_t cycle) {
  const auto* row = CSRAL::find_by_address(addr);
  if (row == nullptr) {
    cvm::log(cvm::HIGH, "[csral] dut force to CSR {:#x} not in the spec; not modeled\n", addr);
    return;
  }
  const auto idx = static_cast<std::uint16_t>(CSRAL::index_of(*row));
  if (!csr_exists(hart, idx))
    return;
  apply(hart, src_t::dut, idx, wdata, wmask, cycle);
}

void csral::iss_write(hart_id_t hart, std::uint32_t addr, std::uint64_t value, std::uint64_t cycle) {
  const auto* row = CSRAL::find_by_address(addr);
  if (row == nullptr)
    return;
  const auto idx = static_cast<std::uint16_t>(CSRAL::index_of(*row));
  if (!csr_exists(hart, idx))
    return;
  // Parity with the bridge: whisper's change reports on locked PMP entries
  // are skipped (bridge.cpp:1877-1889).
  if (pmp_locked(hart, addr))
    return;
  apply(hart, src_t::iss, idx, value, ~0ull, cycle);
  queue_check(hart, idx, check_class_t::sw_write);
}

void csral::iss_refresh(hart_id_t hart, std::uint32_t addr) {
  bool ok = false;
  (void)peek(hart, addr, &ok);
}

// ---- whisper access -------------------------------------------------------------

bool csral::whisper_peek_csr(hart_id_t hart, std::uint32_t addr, std::uint64_t& value, std::uint64_t& wmask, std::uint64_t& pmask, std::uint64_t& rmask, bool quiet) {
  bool valid = false;
  const bool ok = cvm::registry::messenger.call<WC::whisperPeekCsrRPC>(whisper_loc_, static_cast<int>(hart), addr, value, wmask, pmask, rmask, valid) && valid;
  if (!ok && !quiet)
    cvm::log(cvm::MEDIUM, "[csral] whisper peek failed for CSR {:#x}\n", addr);
  return ok;
}

bool csral::poke(hart_id_t hart, std::uint32_t addr, std::uint64_t value, std::uint64_t cycle) {
  bool valid = false;
  const bool ok = cvm::registry::messenger.call<WC::whisperPokeRPC>(whisper_loc_, static_cast<int>(hart), cycle, 'c', addr, value, false, false, valid);
  if (!ok) {
    cvm::log(cvm::MEDIUM, "[csral] whisper poke failed for CSR {:#x}\n", addr);
    return false;
  }
  // Write-through: the mirror is what we believe whisper holds.
  if (const auto* row = CSRAL::find_by_address(addr))
    apply(hart, src_t::iss, static_cast<std::uint16_t>(CSRAL::index_of(*row)), value, ~0ull, cycle);
  return true;
}

bool csral::poke_field(hart_id_t hart, std::uint32_t addr, std::string_view field_name, std::uint64_t field_value, std::uint64_t cycle) {
  const auto* row = CSRAL::find_by_address(addr);
  if (row == nullptr)
    return false;
  const auto* f = field_of(*row, field_name);
  if (f == nullptr) {
    cvm::log(cvm::MEDIUM, "[csral] poke_field: CSR '{}' has no field '{}'\n", row->name, field_name);
    return false;
  }
  const std::uint64_t base = row->policy.volatile_csr ? peek(hart, addr) : read(hart, src_t::iss, addr);
  return poke(hart, addr, CSRAL::field_insert(*f, base, field_value), cycle);
}

std::uint64_t csral::peek(hart_id_t hart, std::uint32_t addr, bool* ok_out) {
  const auto* row = CSRAL::find_by_address(addr);
  bool valid = false;
  std::uint64_t value = 0;
  const bool quiet = row != nullptr && row->policy.may_not_exist;
  const bool ok = cvm::registry::messenger.call<WC::whisperPeekRPC>(whisper_loc_, static_cast<int>(hart), 'c', addr, value, valid);
  if (ok && row != nullptr)
    apply(hart, src_t::iss, static_cast<std::uint16_t>(CSRAL::index_of(*row)), value, ~0ull, harts_.at(hart).entries[CSRAL::index_of(*row)].last_iss_cycle);
  if (!ok && !quiet)
    cvm::log(cvm::MEDIUM, "[csral] whisper peek failed for CSR {:#x}\n", addr);
  if (ok_out != nullptr)
    *ok_out = ok;
  return value;
}

std::uint64_t csral::read(hart_id_t hart, src_t src, std::uint32_t addr) {
  const auto* row = CSRAL::find_by_address(addr);
  if (row == nullptr)
    return 0;
  if (src == src_t::iss && row->policy.volatile_csr)
    return peek(hart, addr); // volatile: the mirror cannot be trusted
  const auto& e = harts_.at(hart).entries[CSRAL::index_of(*row)];
  return src == src_t::dut ? e.dut : e.iss;
}

std::uint64_t csral::read_field(hart_id_t hart, src_t src, std::uint32_t addr, std::string_view field_name) {
  const auto* row = CSRAL::find_by_address(addr);
  if (row == nullptr)
    return 0;
  const auto* f = field_of(*row, field_name);
  return f == nullptr ? 0 : CSRAL::field_extract(*f, read(hart, src, addr));
}

// ---- masks ------------------------------------------------------------------------

std::uint64_t csral::write_mask(hart_id_t hart, std::uint32_t addr) {
  const auto* row = CSRAL::find_by_address(addr);
  if (row != nullptr && !csr_exists(hart, static_cast<std::uint16_t>(CSRAL::index_of(*row))))
    return 0; // parity: the bridge never peeked hypervisor CSRs while misa.H=0
  if (CSRAL::kCompareMaskSource == CSRAL::compare_mask_source_t::spec)
    return row != nullptr ? row->spec_write_mask : ~0ull;
  std::uint64_t value, wmask, pmask, rmask;
  if (!whisper_peek_csr(hart, addr, value, wmask, pmask, rmask, row != nullptr && row->policy.may_not_exist))
    return row != nullptr ? row->spec_write_mask : 0; // fail CLOSED, never all-ones
  // Port of bridge::get_csr_mask: dcsr in debug mode uses the poke mask.
  std::uint64_t result = (debug_mode_ && row != nullptr && row->name == "dcsr") ? pmask : (wmask & rmask);
  if (CSRAL::kCompareMaskSource == CSRAL::compare_mask_source_t::both_warn && row != nullptr && result != row->spec_write_mask)
    cvm::log(cvm::HIGH, "[csral] CSR '{}': whisper write mask {:#x} != spec write mask {:#x}\n", row->name, result, row->spec_write_mask);
  return result;
}

std::uint64_t csral::poke_mask(hart_id_t hart, std::uint32_t addr) {
  std::uint64_t value, wmask, pmask, rmask;
  const auto* row = CSRAL::find_by_address(addr);
  if (row != nullptr && !csr_exists(hart, static_cast<std::uint16_t>(CSRAL::index_of(*row))))
    return 0;
  if (!whisper_peek_csr(hart, addr, value, wmask, pmask, rmask, row != nullptr && row->policy.may_not_exist))
    return row != nullptr ? view_bits_cache_[CSRAL::index_of(*row)] : 0; // fail closed to declared bits
  return pmask;
}

std::uint64_t csral::read_mask(hart_id_t hart, std::uint32_t addr) {
  std::uint64_t value, wmask, pmask, rmask;
  const auto* row = CSRAL::find_by_address(addr);
  if (!whisper_peek_csr(hart, addr, value, wmask, pmask, rmask, row != nullptr && row->policy.may_not_exist))
    return row != nullptr ? view_bits_cache_[CSRAL::index_of(*row)] : 0;
  return rmask;
}

// ---- checking ----------------------------------------------------------------------

std::string csral::field_diff(std::uint16_t csr_index, std::uint64_t dut, std::uint64_t iss, std::uint64_t mask) const {
  const auto& row = CSRAL::kCsrs[csr_index];
  std::string out;
  std::uint64_t covered = 0;
  for (std::size_t i = 0; i < row.field_count; ++i) {
    const auto& f = CSRAL::kFields[row.field_first + i];
    covered |= f.mask;
    if (((dut ^ iss) & mask & f.mask) == 0)
      continue;
    out += fmt::format("{}{}: DUT={:#x} ISS={:#x}", out.empty() ? "" : ", ", f.name, CSRAL::field_extract(f, dut), CSRAL::field_extract(f, iss));
  }
  const std::uint64_t stray = (dut ^ iss) & mask & ~covered;
  if (stray != 0)
    out += fmt::format("{}undeclared bits {:#x}", out.empty() ? "" : ", ", stray);
  return out;
}

std::vector<csral::mismatch_t> csral::check(hart_id_t hart, std::uint64_t cycle) {
  (void)cycle;
  auto& h = harts_.at(hart);
  std::vector<mismatch_t> out;
  if (h.resynch_pending) {
    // The bridge just resynched this hart; today's checker swallows the
    // whole group after a resynch, so drain quietly once.
    h.resynch_pending = false;
    for (const auto& q : h.queue)
      h.queued[q.csr_index] = false;
    h.queue.clear();
    return out;
  }
  for (const auto& q : h.queue) {
    h.queued[q.csr_index] = false;
    if ((q.check_class == check_class_t::sw_write && !check_enable_sw_) || (q.check_class == check_class_t::hw_update && !check_enable_hw_))
      continue;
    const auto& row = CSRAL::kCsrs[q.csr_index];
    if (!csr_exists(hart, q.csr_index))
      continue; // the CSR stopped existing (its gate turned off) before the check ran
    const auto& e = h.entries[q.csr_index];
    // Masked-by fields compare only while their condition is active; the
    // masks were applied at update time, so this is the only compare mask.
    const std::uint64_t mask = ~inactive_condition_mask(hart, q.csr_index);
    std::uint64_t diff = (e.dut ^ e.iss) & mask;
    if (diff != 0 && row.policy.volatile_csr && row.address != CSRAL::kNoDirectAddress) {
      // A volatile CSR's mirror may be stale (whisper moves it between our
      // updates); re-read live before reporting a resynch-worthy mismatch.
      bool ok = false;
      (void)peek(hart, row.address, &ok);
      if (ok)
        diff = (e.dut ^ h.entries[q.csr_index].iss) & mask;
    }
    if (diff == 0)
      continue;
    out.push_back({q.csr_index, row.name, row.address, e.dut & mask, e.iss & mask, mask, row.policy.on_mismatch, q.check_class, field_diff(q.csr_index, e.dut, e.iss, mask)});
  }
  h.queue.clear();
  return out;
}

void csral::note_resynch(hart_id_t hart) {
  harts_.at(hart).resynch_pending = true;
}

void csral::set_check_enable(check_class_t cls, bool enable) {
  (cls == check_class_t::sw_write ? check_enable_sw_ : check_enable_hw_) = enable;
}

// ---- reporting ------------------------------------------------------------------------

void csral::dump(hart_id_t hart, const std::function<void(std::string_view, std::uint64_t, std::uint64_t)>& emit) {
  const auto& h = harts_.at(hart);
  for (std::size_t i = 0; i < CSRAL::kNumCsrs; ++i)
    emit(CSRAL::kCsrs[i].name, h.entries[i].dut, h.entries[i].iss);
}

// ---- default legalize hooks (ports of modify_csr_data / modify_csr_mask) -------------

void csral::register_legalize_hook(std::uint32_t addr, legalize_hook hook) {
  hooks_.emplace_back(addr, std::move(hook));
}

bool csral::pmp_cfg_bit(hart_id_t hart, std::uint32_t addr, unsigned bit) {
  if (pmpaddr0_ == 0 || addr < pmpaddr0_ || addr > pmpaddr0_ + 15)
    return false;
  const std::uint64_t i = addr - pmpaddr0_;
  const std::uint64_t cfg_reg = ((i * 8) / 64) * 2;
  const std::uint64_t cfg_index = (i * 8) % 64;
  // The bridge's mask path peeked a pmpaddr register here by mistake
  // (bridge.cpp:3262); the config lives in pmpcfg, as its data path used.
  std::uint64_t cfg, wmask, pmask, rmask;
  if (!whisper_peek_csr(hart, static_cast<std::uint32_t>(pmpcfg0_ + cfg_reg), cfg, wmask, pmask, rmask, false))
    return false;
  return (cfg >> (cfg_index + bit)) & 0x1;
}

// pmpcfg byte: R=0 W=1 X=2 A=[4:3] L=7. The lock bit gates ISS-side updates;
// A[1] (NAPOT) drives the granularity read-back adjustment in the hook.
bool csral::pmp_locked(hart_id_t hart, std::uint32_t addr) {
  return pmp_cfg_bit(hart, addr, 7);
}

bool csral::pmp_napot(hart_id_t hart, std::uint32_t addr) {
  return pmp_cfg_bit(hart, addr, 4);
}

void csral::register_default_hooks() {
  auto addr_of = [](std::string_view name) -> std::uint32_t {
    const auto* r = CSRAL::find_by_name(name);
    return r == nullptr ? 0 : r->address;
  };

  vl_addr_ = addr_of("vl");
  pmpaddr0_ = addr_of("pmpaddr0");
  pmpcfg0_ = addr_of("pmpcfg0");

  // PMP address entries: a locked entry (pmpcfgX bit 7 of its byte) forces
  // the NAPOT low bits; an unlocked one clears them (bridge.cpp:3205-3221,
  // 3252-3268).
  if (pmpaddr0_ != 0 && pmpcfg0_ != 0) {
    for (std::uint32_t k = 0; k < 16; ++k) {
      register_legalize_hook(pmpaddr0_ + k, [](csral& m, hart_id_t hart, std::uint32_t addr, std::uint8_t, std::uint64_t, std::uint64_t& data, std::uint64_t& mask) {
        // Granularity read-back (G=10): a NAPOT entry reads its low 9 bits
        // as ones, TOR/OFF reads its low 10 bits as zeros. Keyed on
        // pmpcfg A[1] exactly like modify_csr_data (bridge.cpp:3216); the
        // L bit only gates ISS updates.
        if (m.pmp_napot(hart, addr)) {
          data |= 0x1ff;
          mask |= 0x1ff;
        } else {
          data &= 0xfffffffffffffc00ull;
          mask |= 0x3ff;
        }
      });
    }
  }

  // Smstateen hierarchy: hstateenX/sstateenX writes are ANDed with
  // mstateenX (and hstateenX for VS-mode sstateen writes) — bridge.cpp:3222-3240.
  for (std::uint32_t k = 0; k < 4; ++k) {
    const std::uint32_t mst = addr_of(fmt::format("mstateen{}", k));
    const std::uint32_t hst = addr_of(fmt::format("hstateen{}", k));
    const std::uint32_t sst = addr_of(fmt::format("sstateen{}", k));
    if (mst == 0)
      continue;
    auto stateen_hook = [mst, hst](csral& m, hart_id_t hart, std::uint32_t addr, std::uint8_t priv, std::uint64_t, std::uint64_t& data, std::uint64_t& mask) {
      (void)mask;
      bool ok = false;
      const std::uint64_t mst_val = m.peek(hart, mst, &ok);
      if (ok)
        data &= mst_val;
      constexpr std::uint8_t kPrivVS = 9;
      if (hst != 0 && priv == kPrivVS && addr != hst) {
        const std::uint64_t hst_val = m.peek(hart, hst, &ok);
        if (ok)
          data &= hst_val;
      }
    };
    if (hst != 0)
      register_legalize_hook(hst, stateen_hook);
    if (sst != 0)
      register_legalize_hook(sst, stateen_hook);
  }

  // hgatp: only modes {0,8,9,10} are legal; an illegal mode write leaves the
  // mode bits unwritten (bridge.cpp:3269-3284).
  if (const std::uint32_t hgatp = addr_of("hgatp"); hgatp != 0) {
    register_legalize_hook(hgatp, [hgatp](csral& m, hart_id_t hart, std::uint32_t, std::uint8_t, std::uint64_t raw_wmask, std::uint64_t& data, std::uint64_t& mask) {
      // The mode read uses the DUT's raw write mask (bridge.cpp:3272).
      const std::uint64_t mode = (data & raw_wmask) >> 60;
      const bool valid_mode = mode == 0 || mode == 8 || mode == 9 || mode == 10;
      if (!valid_mode) {
        mask &= 0x0fffffffffffffffull;
      } else {
        const std::uint64_t mode_mask = (m.read(hart, src_t::dut, hgatp) >> 60) | mode;
        mask &= (mode_mask << 60) | 0x0fffffffffffffffull;
      }
    });
  }

  // Pointer-masking PMM fields accept only {0,2}; an illegal value leaves the
  // field unwritten (bridge.cpp:3286-3329). hstatus holds PMM at [49:48], the
  // cfg registers at [33:32].
  auto pmm_hook = [](std::uint32_t lo) {
    return [lo](csral&, hart_id_t, std::uint32_t, std::uint8_t, std::uint64_t, std::uint64_t& data, std::uint64_t& mask) {
      const std::uint64_t pmm = ((data & mask) >> lo) & 0x3;
      if (pmm != 0 && pmm != 2)
        mask &= ~(0x3ull << lo);
    };
  };
  for (std::string_view name : {"mseccfg", "menvcfg", "henvcfg", "senvcfg"}) {
    if (const std::uint32_t a = addr_of(name); a != 0)
      register_legalize_hook(a, pmm_hook(32));
  }
  if (const std::uint32_t a = addr_of("hstatus"); a != 0)
    register_legalize_hook(a, pmm_hook(48));

  // srmcfg: RCID/MCID legality (bridge.cpp:3331-3338).
  if (const std::uint32_t srmcfg = addr_of("srmcfg"); srmcfg != 0) {
    register_legalize_hook(srmcfg, [](csral& m, hart_id_t hart, std::uint32_t addr, std::uint8_t, std::uint64_t raw_wmask, std::uint64_t& data, std::uint64_t& mask) {
      // Legality is judged on the DUT's raw write (bridge.cpp:3332-3336).
      const std::uint64_t eff = data & raw_wmask;
      std::uint64_t result = 0xfff0fffull;
      if (!(((eff & result) & 0xFFF) <= 0xF) || (((eff & result) & 0xFFF0000ull) != 0x0))
        result = 0;
      else
        result &= m.write_mask(hart, addr);
      mask = result;
    });
  }
}
