// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// CSRAL: the cosim CSR model — per-hart DUT/ISS mirrors, the only path to
// whisper CSR pokes/peeks, and the CSR write checks. See the CSRAL section
// of docs/source/user_guides/cosim.rst for semantics.

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
    sw_write, // retire-path CSR writes (gated by +csr_rd_check)
    hw_update // csri implicit hardware updates (gated by +csr_wr_check)
  };

  // One CSR whose mirrors disagree; the bridge prints and acts on `action`.
  struct mismatch_t {
    std::uint16_t csr_index;
    std::string_view csr_name;
    std::uint16_t address;
    std::uint64_t dut;  // masked DUT value
    std::uint64_t iss;  // masked ISS value
    std::uint64_t mask; // effective compare mask used
    CSRAL::on_mismatch_t action;
    check_class_t check_class;
    std::string fields; // per-field diff, e.g. "MPP: DUT=0x0 ISS=0x3"
  };

  // Spec-vs-whisper reset disagreement found by init_check().
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

  // Parses +csral_save_restore; unknown or view-only condition names throw.
  csral(int num_harts, cvm::topology::loc_t whisper_loc);

  // Seed both mirrors from the spec resets; clear queues, stashes, conditions.
  void reset(hart_id_t hart);
  // Compare whisper's resets to the spec, then adopt whisper into both mirrors.
  std::vector<reset_mismatch_t> init_check(hart_id_t hart);
  reset_check_t reset_check_severity() const { return reset_check_; }

  // Retire-path write: write mask + legalize hooks + DUT mirror + check queue.
  void sw_write(hart_id_t hart, std::uint32_t addr, std::uint64_t wdata, std::uint64_t wmask, std::uint8_t priv, std::uint64_t cycle);
  // csri implicit hardware update: masked by whisper's poke mask.
  void hw_update(hart_id_t hart, std::uint32_t addr, std::uint64_t wdata, std::uint64_t wmask, std::uint64_t cycle);
  // Hardware-forced side effect (e.g. mideleg VS bits on a misa.H edge): raw mask, no check queued.
  void dut_force(hart_id_t hart, std::uint32_t addr, std::uint64_t wdata, std::uint64_t wmask, std::uint64_t cycle);

  // Whisper step change report: ISS mirror + aliases, queues a check.
  void iss_write(hart_id_t hart, std::uint32_t addr, std::uint64_t value, std::uint64_t cycle);
  // Force-refresh the ISS mirror from a live whisper peek.
  void iss_refresh(hart_id_t hart, std::uint32_t addr);

  // Poke writes through to the ISS mirror.
  bool poke(hart_id_t hart, std::uint32_t addr, std::uint64_t value, std::uint64_t cycle);
  // Read-modify-write of one named field (peeks volatile CSRs live first).
  bool poke_field(hart_id_t hart, std::uint32_t addr, std::string_view field_name, std::uint64_t field_value, std::uint64_t cycle);
  // Live whisper read; refreshes the ISS mirror. ok (if given) reports RPC success.
  std::uint64_t peek(hart_id_t hart, std::uint32_t addr, bool* ok = nullptr);

  // Mirror reads; volatile CSRs read the ISS side live from whisper.
  std::uint64_t read(hart_id_t hart, src_t src, std::uint32_t addr);
  std::uint64_t read_field(hart_id_t hart, src_t src, std::uint32_t addr, std::string_view field_name);

  // Masks per kCompareMaskSource; dcsr-in-debug follows set_debug_mode().
  std::uint64_t write_mask(hart_id_t hart, std::uint32_t addr);
  std::uint64_t poke_mask(hart_id_t hart, std::uint32_t addr);
  std::uint64_t read_mask(hart_id_t hart, std::uint32_t addr);
  void set_debug_mode(bool debug) { debug_mode_ = debug; }

  // Drain the queued CSRs and compare mirrors under the effective mask.
  std::vector<mismatch_t> check(hart_id_t hart, std::uint64_t cycle);
  // Next check() drains this hart's queue without reporting.
  void note_resynch(hart_id_t hart);
  void set_check_enable(check_class_t cls, bool enable);

  // Condition names are "<gate_csr>.<gate_field>", e.g. "misa.H".
  bool condition_active(hart_id_t hart, std::string_view condition_name) const;
  bool save_restore_enabled(std::string_view condition_name) const;
  // Fires after a condition flips; restored_addrs lists CSRs the on-edge restore poked.
  using condition_cb = std::function<void(hart_id_t, const CSRAL::condition_t&, bool now_active, const std::vector<std::uint32_t>& restored_addrs, std::uint64_t cycle)>;
  void on_condition_change(condition_cb cb) { condition_cb_ = std::move(cb); }

  // Cross-CSR WARL hooks run during sw_write; may adjust the effective data and mask.
  using legalize_hook = std::function<void(csral&, hart_id_t, std::uint32_t addr, std::uint8_t priv, std::uint64_t raw_wmask, std::uint64_t& data, std::uint64_t& mask)>;
  void register_legalize_hook(std::uint32_t addr, legalize_hook hook);

  void dump(hart_id_t hart, const std::function<void(std::string_view name, std::uint64_t dut, std::uint64_t iss)>& emit);

  static const CSRAL::csr_t* row(std::uint32_t addr) { return CSRAL::find_by_address(addr); }
  static const CSRAL::field_t* field_of(const CSRAL::csr_t& csr, std::string_view field_name);

  // The pmpaddr entry's pmpcfg L bit (live peek): gates ISS updates and the PMP hook.
  bool pmp_locked(hart_id_t hart, std::uint32_t addr);
  // False only while the CSR's exists_if condition is inactive.
  bool exists(hart_id_t hart, std::uint32_t addr) const;
  // The pmpaddr entry's pmpcfg A[1] bit (NAPOT), driving granularity read-back.
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
    std::vector<std::uint64_t> stash; // per target: masked bits captured at the off-edge, sticky while off
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
  bool whisper_peek_csr(hart_id_t hart, std::uint32_t addr, std::uint64_t& value, std::uint64_t& wmask, std::uint64_t& pmask, std::uint64_t& rmask, bool quiet);
  std::string field_diff(std::uint16_t csr_index, std::uint64_t dut, std::uint64_t iss, std::uint64_t mask) const;
  void register_default_hooks();
  // Fixpoint over the gate DAG (needed at reset/init where gates chain).
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
  // Addresses the default hooks special-case (0 when the spec lacks them).
  std::uint32_t vl_addr_ = 0;
  std::uint32_t pmpaddr0_ = 0;
  std::uint32_t pmpcfg0_ = 0;
};
