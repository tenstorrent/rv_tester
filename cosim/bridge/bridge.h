// Licensed under the Apache License, Version 2.0, see LICENSE.TT for details

#pragma once

#include <string>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <vector>

#include "cvm/topology.hpp"
#include "bridge_base.h"
#include "memmap.h"
#include "cvm/logger.hpp"
#include "src/cac_core.h"
#include "src/cac_lib.h"

#include "whisper_client.h"
#include "rv_tester/rv_tester_structs.h"
#include "cvm/registry.hpp"

class bridge : public bridge_base {

private:
  using src_t = cac::src_t;
  using resource_t = cac::resource_t;
  using resource_id_t = cac::resource_id_t;
  using CacCore = cac::CacCore;
  uint64_t previous_cycle_;


public:
  // Usec by some functions in bridge.cpp
  using size_8_bytes_t = uint64_t;

  struct error {};

  bridge(int num_harts, int xlen, int vlen, cvm::topology::loc_t loc, unsigned id);
  ~bridge();

  // DUT Interface API
  // Process instruction called on retire
  //   - Metadata
  //   - PC
  //   - Register Read/Write
  //   - CSRs
  //   - Memory Access
  //   - AMO
  //   - Table Walks
  //   - Exceptions/interrupt
  virtual void process_dut_instr_retire(hart_id_t hart, rv_instr_t& d) override;
  virtual void process_steps(hart_id_t hart, uint32_t n_retire, uint64_t cycle, uint64_t steps, uint64_t skips, uint64_t final_steps) override;
  virtual void process_dut_instr_group_retire(hart_id_t hart, rv_instr_group_t& d) override;
  virtual void process_dut_csr_hw_update(hart_id_t hart, csr_t& c) override;
  virtual void process_compare_gp_regs(hart_id_t hart, const std::array<std::uint64_t, 32>& array);
  virtual void process_compare_fp_regs(hart_id_t hart, const std::array<std::uint64_t, 32>& array);
  virtual void process_compare_vc_regs(hart_id_t hart, const std::array<std::bitset<256>, 32>& array);
  virtual void process_compare_vc_regs(hart_id_t hart, const std::array<std::uint64_t, 32>& array);

  // Process memory access
  //   - Read (Ld completion)
  //   - Insert (St merge buffer insertion)
  //   - Write (St cache write)
  virtual void process_dut_mcm_read(hart_id_t hart, mem_t& m) override;
  virtual void process_dut_mcm_insert(hart_id_t hart, mem_t& m) override;
  virtual void process_dut_mcm_bypass(hart_id_t hart, mem_t& m) override;
  virtual void process_dut_mcm_write(hart_id_t hart, mem_cl_t& m) override;
  virtual void process_dut_mcm_ifetch(hart_id_t hart, mem_t& m) override;
  virtual void process_dut_mcm_ievict(hart_id_t hart, mem_t& m) override;

  // Interrupts
  virtual void process_dut_interrupt(hart_id_t hart, rv_intr_t& i) override;
  virtual void process_dut_imsic_msi(hart_id_t hart, mem_t& m) override;

  // Debug mode
  virtual void enter_debug_mode(rv_debug_t& d) override;
  virtual void exit_debug_mode(rv_debug_t& d) override;

  void reset();
  void csr_init();

  void final_phase();
  void report_metrics();
  void process(const rv_tester::terminate_called &);
  void set_patch_mode(bool patch_mode) { patch_mode_ = int(patch_mode); }

private:

  typedef enum {
    read,
    write,
    fetch
  } memclass_t;

private:
  
  void update_dut_state(hart_id_t hart, rv_instr_t& d);
  void arch_state(whisper_state_t& w);
  void update_whisper_state(hart_id_t hart, whisper_state_t& w);
  void step(hart_id_t hart, whisper_state_t& w);
  void compare_dut_whisper_state(hart_id_t hart, const whisper_state_t& w, const rv_instr_t& d);
  void print_instr(hart_id_t hart, const whisper_state_t& w);
  void print_instr_stdout(hart_id_t hart, const rv_instr_t& d);
  void print_instr_stdout(hart_id_t hart, const whisper_state_t& w);
  void print_resource(hart_id_t hart, const whisper_state_t& w);
  void update_pc(hart_id_t hart, src_t src, uint64_t data);
  void update_priv(hart_id_t hart, src_t src, uint32_t data);
  void update_insn(hart_id_t hart, src_t src, uint32_t data);
  void update_flags(hart_id_t hart, src_t src, uint32_t data);
  void update_regs(hart_id_t hart, const rv_instr_t& d);
  void update_regs(hart_id_t hart, const whisper_state_t& w, uint32_t vec_slice_index = 0);
  void update_regs(hart_id_t hart, src_t src, resource_t resource, uint64_t addr, const std::vector<size_8_bytes_t>&& dword_vec);
  void update_mem_attr(hart_id_t hart, src_t src, uint32_t data);
  void update_csr(hart_id_t hart, src_t src, uint64_t addr, uint64_t data, cac::optional_const_ref<size_8_bytes_t> mask_ref = std::nullopt, bool shadow_csr = false, bool check_en = true);
  uint64_t modify_csr_data(hart_id_t hart, uint64_t addr, uint64_t data);
  size_8_bytes_t modify_csr_mask(hart_id_t hart, uint64_t addr, uint64_t data, size_8_bytes_t mask);
  uint64_t get_csr(hart_id_t hart, src_t src, uint64_t addr);
  uint64_t get_csr_mask(hart_id_t hart, uint64_t addr);
  uint64_t get_csr_poke_mask(hart_id_t hart, uint64_t addr);
  std::string get_csr_name(const std::string& addr);
  bool is_custom_csr(uint64_t addr);
  bool is_pmacfg_csr(uint64_t addr);
  bool is_chicken_bit_csr(uint64_t addr);

  void translation_check(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);
  uint64_t translate(hart_id_t hart, uint64_t va, uint8_t priv, memclass_t memclass);

  // Process pre/post-step
  void pre_step_lrsc_poke(       hart_id_t hart, const rv_instr_t& d);
  void pre_step_debug_poke(      hart_id_t hart, const rv_instr_t& d);
  void pre_step_interrupt_poke(  hart_id_t hart, const rv_instr_t& d,       whisper_state_t& w);
  void post_step_interrupt_poke( hart_id_t hart, const rv_instr_t& d, const whisper_state_t& w);
  void post_step_exception_poke( hart_id_t hart, const rv_instr_t& d,       whisper_state_t& w);
  void post_step_satp_write_poke(hart_id_t hart, const rv_instr_t& d, const whisper_state_t& w);

  void process_imsic_msi(hart_id_t hart, const mem_t& m);
  void process_local_interrupt(hart_id_t hart, rv_intr_t& i);
  void process_external_interrupt(hart_id_t hart, rv_intr_t& i);
  void check_and_defer_interrupt(hart_id_t hart, uint64_t time, uint64_t mip);
  void check_interrupt(hart_id_t hart, uint64_t mip, bool& taken, uint64_t& cause);
  void defer_interrupt(hart_id_t hart, uint64_t time, uint64_t mip);
  void resetsstc_poke(hart_id_t hart, uint64_t cycle, uint64_t csr);
  void setsstc_poke(hart_id_t hart, uint64_t cycle, uint64_t csr);
  void poke_mip(hart_id_t hart, uint64_t time, uint64_t mip);
  void peek_mip(hart_id_t hart, uint64_t time, uint64_t& mip);
  void peek_seip(hart_id_t hart, uint64_t time, uint64_t& val);
  void get_gp_reg(uint32_t reg, uint64_t& data);
  void get_fp_reg(uint32_t reg, uint64_t& data);
  void get_vec_reg(uint32_t reg, std::array<std::uint8_t, 32>& data);


  bool is_custom_excp(uint64_t cause);
  bool is_vector(const std::string& instr);
  bool disable_pa_check_vec(hart_id_t hart);
  bool is_compressed(const std::string& instr);
  bool is_ucode(const std::string& instr);
  bool is_renamed_csr(const std::string& instr);
  bool does_instr_match_resynch_list(const rv_instr_t& d, const std::string& instr);
  bool does_instr_match_resynch_condition(const rv_instr_t& d, const std::string& instr);
  bool clint_read(const rv_instr_t& d);
  bool tbox_read(const rv_instr_t& d);
  bool boot_read(const rv_instr_t& d);
  bool debug_mem_access(const rv_instr_t& d);
  bool unsupported_mmr_access(const rv_instr_t& d);
  bool unsupported_csr_access(const std::string& instr);
  bool cpl_smc_access(const rv_instr_t& d);
  bool htif_read(const rv_instr_t& d);
  bool hpm_counter_read(const std::string& instr);
  bool mip_mismatch(const std::string& instr);
  bool topi_mismatch(const std::string& instr);
  bool topei_mismatch(const std::string& instr);
  void resynch(hart_id_t hart, const rv_instr_group_t& d);
  void resynch(hart_id_t hart, const rv_instr_t& d);
  std::string get_nth_word(const std::string& s, int n);

private:

  cvm::file_logger bridge_log_;
  cvm::topology::loc_t loc_;
  unsigned id_;

  int num_harts_ = 0;
  int xlen_ = 0;
  int vlen_ = 0;
  CacCore cac_;
  CacCore csr_cac_;

  uint64_t order_ = 0;

  // Previous instruction's whisper state
  whisper_state_t pw_{};
  whisper_state_t ppw_{};

  // Create a copy of whisper instr in similar format as dut
  rv_instr_t w_;
  rv_instr_t pd_;

  uint32_t step_ = 1;
  uint64_t whisper_time_=0;
  uint64_t rvfi_calls_=0;
  uint64_t num_sp_accesses_ = 0;

  // State variables
  bool ecall_ = false;
  bool debug_mode_ = false;
  bool excp_in_debug_mode = false;
  bool lrsc_fail_ = false;
  bool twoStage_ = false;
  bool zicbom_ = false;

  uint64_t satp_ = 0;
  uint64_t new_satp_ = 0;

  uint16_t mprv_ = 0;
  uint16_t mpp_ = 0;
  uint16_t mpv_ = 0;
  bool csr_rename_en_ = false;

  uint64_t dummy_data_ = 0;
  hart_id_t dummy_hart_ = 0;

  bool resynch_intr_cause_mismatch_ = false;
  bool resynch_csr_ = false;

  bool deferred_intr_ = false;
  bool vstimecmppoked_ = false;
  bool stimecmppoked_ = false;
  uint64_t intrtopriv_ = 3;
  std::vector<mem_t> mem_poke_{};
  uint64_t mip_ = 0;
  uint64_t prev_mip_ = 0;
  uint64_t e_mip_ = 0;
  uint64_t prev_e_mip_ = 0;
  uint64_t deferred_mip_ = 0;
  bool prev_sync_intr_ = 0;
  bool all_interrupts_defer_= 0;
  bool prev_resync_excp_defer_intr_ = 0;
  uint64_t pre_csr_defermip_ = 0;
  uint64_t resynch_icause_ = 0;
  bool pre_undeferred_intr_;
  bool post_undeferred_intr_;
  std::array<uint32_t, max_intr> intr_age_{};
  uint32_t max_pend_intr_age_ = 0;
  std::chrono::high_resolution_clock::time_point end_time_;
  std::chrono::high_resolution_clock::time_point start_of_test_;
  bool first_call_ = true;
  

  // Memmap
  memmap::memmap_t memmap_;

  std::array<std::array<int, 16>, 12> num_taken_interrupts_{};

  int num_exceptions_ = 0;

  size_8_bytes_t dword_vec_array [vlen/64] = {0};
  int unmask_bits_instr, unmask_bits_uop = 0;
  std::vector<std::string> cosim_resynch_csr_defaults;

  bool terminated_ = false;
  int patch_mode_  = 0; // 0:not in patch mode, 1: entered patch mode, step whisper, >2: inside patch mode, dont step whisper

  template <typename... Args>
      void print(cvm::verbosity_level v, Args&&... args) {
          cvm::log(v, std::forward<Args>(args)...);
          if (v <= cvm::verbosity_level::ERROR) {
              cvm::registry::messenger.signal<error>(loc_, {});
          }
      }

};
