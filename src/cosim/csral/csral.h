// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// CSRAL: the runtime CSR model for cosim (docs/csral_plan.md §4.2).
//
// One object that, per hart, knows the full CSR list (from the generated
// csral_tables.hpp), what the DUT last wrote (from instruction retire and
// from csri implicit hardware updates), and what the ISS (whisper) currently
// holds (its mirror). It owns every whisper CSR poke/peek — pokes write
// through to the mirror so the two can never silently drift — and it owns
// the CSR write checks, replacing the bridge's csr_cac_ + hand-coded skip
// lists + modify_csr_data/modify_csr_mask special cases.
//
// The bridge stays in charge of WHEN things happen (pre/post-step ordering,
// interrupt deferral, debug mode); CSRAL is the single answer for HOW CSR
// state is stored, legalized, synced, and compared.

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <gflags/gflags.h>

#include "csral_tables.hpp"
#include "cvm/topology.hpp"

DECLARE_string(csral_save_restore);
DECLARE_string(csral_reset_check);

class csral {
public:
  using hart_id_t = std::uint16_t;

  enum class src_t {
    dut,
    iss
  };

  enum class check_class_t {
    sw_write, // retire-path CSR writes (legacy gate: +csr_rd_check)
    hw_update // csri implicit hardware updates (legacy gate: +csr_wr_check)
  };

  // One CSR whose DUT and ISS mirrors disagree under the effective compare
  // mask. `action` is the policy verdict; the bridge decides what a resynch
  // means and does the printing, so error signaling stays on its path.
  struct mismatch_t {
    std::uint16_t csr_index;
    std::string_view csr_name;
    std::uint16_t address;
    std::uint64_t dut;  // masked DUT value
    std::uint64_t iss;  // masked ISS value
    std::uint64_t mask; // effective compare mask used
    CSRAL::on_mismatch_t action;
    check_class_t check_class;
    std::string fields; // human-readable per-field diff, e.g. "MPP: DUT=0x0 ISS=0x3"
  };

  // Spec reset vs whisper reset disagreement found by init_check().
  struct reset_mismatch_t {
    std::string_view csr_name;
    std::uint16_t address;
    std::uint64_t spec;
    std::uint64_t whisper;
  };

  enum class reset_check_t {
    error,
    warn,
    off
  };

  // Constructing parses +csral_save_restore (comma list of
  // "<gate_csr>.<gate_field>" condition names; default "misa.H"; empty
  // disables save/restore everywhere). Unknown names and view-only
  // conditions throw std::invalid_argument — a typo must be a startup
  // error, not a silently ignored knob.
  csral(int num_harts, cvm::topology::loc_t whisper_loc);

  // ---- reset / init (replaces bridge::csr_init) -------------------------
  // Seed both mirrors from the spec reset values, clear queues and stashes,
  // re-evaluate conditions from reset state.
  void reset(hart_id_t hart);
  // Peek whisper for every CSR and compare against the spec reset value.
  // Returns the disagreements (per +csral_reset_check / kResetCheck the
  // bridge escalates or logs them); whisper's values are then adopted into
  // BOTH mirrors — whisper is the authority for the run, the spec check
  // exists to catch spec-vs-whisper drift at time zero.
  std::vector<reset_mismatch_t> init_check(hart_id_t hart);
  reset_check_t reset_check_severity() const { return reset_check_; }

  // ---- DUT-side updates --------------------------------------------------
  // Retire-path write (SW writes and trap CSR updates reported by RVFI).
  // Applies the whisper/spec write mask, the registered legalize hooks
  // (WARL and cross-CSR effects), merges into the DUT mirror, fans out to
  // aliases, queues a check, and evaluates masking-condition transitions.
  void sw_write(hart_id_t hart, std::uint32_t addr, std::uint64_t wdata, std::uint64_t wmask, std::uint8_t priv, std::uint64_t cycle);
  // csri implicit hardware update: masked by whisper's poke mask.
  void hw_update(hart_id_t hart, std::uint32_t addr, std::uint64_t wdata, std::uint64_t wmask, std::uint64_t cycle);

  // ---- ISS-side updates ---------------------------------------------------
  // Whisper step change report ('c'): full-value mirror update + alias
  // fan-out, and queues a check (legacy CacCore parity: an ISS-only change
  // must surface as "ISS wrote, DUT didn't").
  void iss_write(hart_id_t hart, std::uint32_t addr, std::uint64_t value, std::uint64_t cycle);
  // Force-refresh the ISS mirror from a live whisper peek (patch/debug flows).
  void iss_refresh(hart_id_t hart, std::uint32_t addr);

  // ---- whisper access: THE only path to whisper CSR pokes/peeks ----------
  // Poke = "make whisper hold this value"; writes through to the ISS mirror.
  bool poke(hart_id_t hart, std::uint32_t addr, std::uint64_t value, std::uint64_t cycle);
  // Read-modify-write of one named field (peeks volatile CSRs live first).
  bool poke_field(hart_id_t hart, std::uint32_t addr, std::string_view field_name, std::uint64_t field_value, std::uint64_t cycle);
  // Live whisper read; refreshes the ISS mirror. ok (if given) reports RPC
  // success — a CSR with policy may_not_exist fails quietly.
  std::uint64_t peek(hart_id_t hart, std::uint32_t addr, bool* ok = nullptr);

  // Mirror reads (replaces bridge::get_csr). Volatile CSRs (policy
  // volatile_csr) read the ISS side live from whisper.
  std::uint64_t read(hart_id_t hart, src_t src, std::uint32_t addr);
  std::uint64_t read_field(hart_id_t hart, src_t src, std::uint32_t addr, std::string_view field_name);

  // ---- masks (replaces get_csr_mask / get_csr_poke_mask) -----------------
  // Sourced per kCompareMaskSource (whisper live peek or spec tables); the
  // dcsr-in-debug special case follows set_debug_mode().
  std::uint64_t write_mask(hart_id_t hart, std::uint32_t addr);
  std::uint64_t poke_mask(hart_id_t hart, std::uint32_t addr);
  std::uint64_t read_mask(hart_id_t hart, std::uint32_t addr);
  void set_debug_mode(bool debug) { debug_mode_ = debug; }

  // ---- checking (replaces csr_cac_ + skip lists) --------------------------
  // Drains the queued CSRs and compares DUT vs ISS mirrors under the
  // effective mask (write mask, minus fields whose masking condition is
  // currently inactive). Returns every mismatch with its policy action;
  // entries whose class is disabled via set_check_enable are dropped.
  std::vector<mismatch_t> check(hart_id_t hart, std::uint64_t cycle);
  // The bridge resynched this hart: the next check() drains its queue
  // without reporting (parity with today's resynch_csr_ behavior).
  void note_resynch(hart_id_t hart);
  void set_check_enable(check_class_t cls, bool enable);

  // ---- masking conditions & save/restore (plan §4.4) ----------------------
  // Condition names are "<gate_csr>.<gate_field>", e.g. "misa.H". Active =
  // the gate field's effective DUT value (its own gates applied, one level
  // along the DAG) is nonzero.
  bool condition_active(hart_id_t hart, std::string_view condition_name) const;
  bool save_restore_enabled(std::string_view condition_name) const;
  // Fires after a condition flips. On the off->on edge CSRAL has already
  // poked the stashed masked field values back into whisper; restored_addrs
  // lists those CSRs so the bridge can refresh interrupt bookkeeping.
  using condition_cb = std::function<void(hart_id_t, const CSRAL::condition_t&, bool now_active, const std::vector<std::uint32_t>& restored_addrs, std::uint64_t cycle)>;
  void on_condition_change(condition_cb cb) { condition_cb_ = std::move(cb); }

  // ---- behavioral hooks (cross-CSR WARL the spec cannot express) ----------
  // Run during sw_write after the base write-mask is applied; may adjust the
  // effective data and mask. Defaults registered by the constructor port
  // modify_csr_data/modify_csr_mask (PMP lock, stateen hierarchy, hgatp
  // modes, PMM legal values, srmcfg, vl).
  using legalize_hook = std::function<void(csral&, hart_id_t, std::uint32_t addr, std::uint8_t priv, std::uint64_t raw_wmask, std::uint64_t& data, std::uint64_t& mask)>;
  void register_legalize_hook(std::uint32_t addr, legalize_hook hook);

  // ---- reporting -----------------------------------------------------------
  void dump(hart_id_t hart, const std::function<void(std::string_view name, std::uint64_t dut, std::uint64_t iss)>& emit);

  // Table lookups exposed for callers.
  static const CSRAL::csr_t* row(std::uint32_t addr) { return CSRAL::find_by_address(addr); }
  static const CSRAL::field_t* field_of(const CSRAL::csr_t& csr, std::string_view field_name);

  // True when `addr` is a PMP address CSR whose entry is locked in the
  // corresponding pmpcfg (peeked live from whisper). Used by the default PMP
  // legalize hook and by the ISS-update skip, both ports of bridge behavior.
  bool pmp_locked(hart_id_t hart, std::uint32_t addr);
  // False only for a CSR whose exists_if condition is currently inactive
  // (e.g. hypervisor CSRs while misa.H=0). Unknown addresses return true.
  bool exists(hart_id_t hart, std::uint32_t addr) const;
  // A[1] of the entry's pmpcfg byte: NAPOT, driving granularity read-back.
  bool pmp_napot(hart_id_t hart, std::uint32_t addr);

private:
  struct entry_t {
    std::uint64_t dut = 0;
    std::uint64_t iss = 0;
    bool dut_valid = false;
    bool iss_valid = false;
    std::uint64_t last_dut_cycle = 0;
    std::uint64_t last_iss_cycle = 0;
  };

  struct queued_t {
    std::uint16_t csr_index;
    check_class_t check_class;
  };

  struct cond_state_t {
    bool active = false;
    std::vector<std::uint64_t> stash; // per target, masked field bits captured while the condition is off
    std::vector<bool> stash_valid;    // per target: a capture actually happened
  };

  struct hart_state_t {
    std::vector<entry_t> entries;    // index-aligned with CSRAL::kCsrs
    std::vector<queued_t> queue;     // pending checks, insertion order
    std::vector<bool> queued;        // per csr_index: already in queue
    std::vector<cond_state_t> conds; // index-aligned with CSRAL::kConditions
    bool resynch_pending = false;
  };

  // Mirror write + alias fan-out + condition re-evaluation. Never queues.
  void apply(hart_id_t hart, src_t src, std::uint16_t csr_index, std::uint64_t data, std::uint64_t mask, std::uint64_t cycle, bool fan_out = true);
  void fan_out_aliases(hart_id_t hart, src_t src, std::uint16_t csr_index, std::uint64_t cycle);
  void queue_check(hart_id_t hart, std::uint16_t csr_index, check_class_t cls);
  void evaluate_conditions(hart_id_t hart, std::uint16_t gate_csr_index, std::uint64_t cycle);
  std::uint64_t effective_dut_value(hart_id_t hart, std::uint16_t csr_index) const;
  std::uint64_t inactive_condition_mask(hart_id_t hart, std::uint16_t csr_index) const;
  bool field_alias_pair(std::uint16_t a, std::uint16_t b) const;
  bool pmp_cfg_bit(hart_id_t hart, std::uint32_t addr, unsigned bit);
  bool csr_exists(hart_id_t hart, std::uint16_t csr_index) const;
  void refresh_inactive_stashes(hart_id_t hart, std::uint16_t csr_index);
  bool whisper_peek_csr(hart_id_t hart, std::uint32_t addr, std::uint64_t& value, std::uint64_t& wmask, std::uint64_t& pmask, std::uint64_t& rmask, bool quiet);
  std::string field_diff(std::uint16_t csr_index, std::uint64_t dut, std::uint64_t iss, std::uint64_t mask) const;
  void register_default_hooks();
  // Recompute every condition's activeness to a fixpoint (gates can chain
  // along the DAG, so a single ordered pass is not enough at reset/init).
  void recompute_all_conditions(hart_id_t hart);

  int num_harts_;
  cvm::topology::loc_t whisper_loc_;
  std::vector<hart_state_t> harts_;
  std::vector<std::uint64_t> view_bits_cache_;                 // per csr_index
  std::vector<std::vector<std::uint16_t>> conditions_by_gate_; // csr_index -> condition indices it gates
  std::vector<bool> save_restore_enabled_;                     // per condition index
  std::vector<std::pair<std::uint32_t, legalize_hook>> hooks_;
  condition_cb condition_cb_;
  bool check_enable_sw_ = true;
  bool check_enable_hw_ = true;
  bool debug_mode_ = false;
  reset_check_t reset_check_ = reset_check_t::error;
  // Addresses of CSRs the default hooks special-case (0 when the spec lacks them).
  std::uint32_t vl_addr_ = 0;
  std::uint32_t pmpaddr0_ = 0;
  std::uint32_t pmpcfg0_ = 0;
};
