// Licensed under the Apache License, Version 2.0, see LICENSE.TT for details

#pragma once

#include <string>
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

class bridge : public bridge_base {

using src_t = cac::src_t;
using resource_t = cac::resource_t;
using resource_id_t = cac::resource_id_t;
using size_8_bytes_t = cac::size_8_bytes_t;
using CacCore = cac::CacCore;

public:
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
  virtual void process_dut_instr_group_retire(hart_id_t hart, rv_instr_group_t& d) override;
  virtual void process_dut_csr_hw_update(hart_id_t hart, csr_t& c) override;

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

private:

  typedef enum {
    read,
    write,
    fetch
  } memclass_t;

private:
  
  void update_dut_state(hart_id_t hart, rv_instr_t& d);
  void update_whisper_state(hart_id_t hart, whisper_state_t& w);
  void step(hart_id_t hart, whisper_state_t& w);
  void print_instr(hart_id_t hart, const whisper_state_t& w);
  void print_instr_stdout(hart_id_t hart, const rv_instr_t& d);
  void print_instr_stdout(hart_id_t hart, const whisper_state_t& w);
  void print_resource(hart_id_t hart, const whisper_state_t& w);
  void update_pc(hart_id_t hart, src_t src, uint64_t data);
  void update_priv(hart_id_t hart, src_t src, uint32_t data);
  void update_insn(hart_id_t hart, src_t src, uint32_t data);
  void update_regs(hart_id_t hart, const rv_instr_t& d);
  void update_regs(hart_id_t hart, const whisper_state_t& w, uint32_t vec_slice_index = 0);
  void update_regs(hart_id_t hart, src_t src, resource_t resource, uint64_t addr, const std::vector<size_8_bytes_t>&& dword_vec);
  void update_mem(hart_id_t hart, rv_instr_t& d);
  void update_csr(hart_id_t hart, src_t src, uint64_t addr, uint64_t data, cac::optional_const_ref<size_8_bytes_t> mask_ref = std::nullopt, bool shadow_csr = false);
  uint64_t modify_csr_data(hart_id_t hart, uint64_t addr, uint64_t data);
  size_8_bytes_t modify_csr_mask(hart_id_t hart, uint64_t addr, size_8_bytes_t mask);
  uint64_t get_csr(hart_id_t hart, src_t src, uint64_t addr);
  uint64_t get_csr_mask(hart_id_t hart, uint64_t addr);
  uint64_t get_csr_poke_mask(hart_id_t hart, uint64_t addr);
  std::string get_csr_name(const std::string& addr);
  bool is_custom_csr(uint64_t addr);
  bool is_supported_csr(uint64_t addr);

  void translation_check(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);
  uint64_t translate(hart_id_t hart, uint64_t va, uint8_t priv, memclass_t memclass);

  void process_lrsc_pre_step(hart_id_t hart, const rv_instr_t& d);
  void process_debug_pre_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);
  void process_interrupt_pre_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);
  void process_interrupt_post_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);
  void process_exception_post_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);
  void process_satp_write_post_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);

  void process_timer_sw_interrupt(hart_id_t hart, rv_intr_t& i);
  void process_external_interrupt(hart_id_t hart, rv_intr_t& i);
  void check_and_defer_interrupt(hart_id_t hart, uint64_t time, uint64_t mip);
  void check_interrupt(hart_id_t hart, uint64_t mip, bool& taken, uint64_t& cause);
  void defer_interrupt(hart_id_t hart, uint64_t time, uint64_t mip);
  void poke_mip(hart_id_t hart, uint64_t time, uint64_t mip);
  void peek_mip(hart_id_t hart, uint64_t time, uint64_t& mip);
  void peek_seip(hart_id_t hart, uint64_t time, uint64_t& val);

  bool is_vector(const std::string& instr);
  bool is_compressed(const std::string& instr);
  bool is_ucode(const std::string& instr);
  bool does_instr_match_resynch_list(const rv_instr_t& d, const std::string& instr);
  bool does_instr_match_resynch_condition(const rv_instr_t& d, const std::string& instr);
  bool clint_read(const rv_instr_t& d);
  bool boot_read(const rv_instr_t& d);
  bool debug_mem_access(const rv_instr_t& d);
  bool htif_read(const rv_instr_t& d);
  bool hpm_counter_read(const std::string& instr);
  bool mip_mismatch(const std::string& instr);
  bool imsic_mismatch(const std::string& instr);
  void resynch(hart_id_t hart, const rv_instr_group_t& d);
  void resynch(hart_id_t hart, const rv_instr_t& d);
  std::string get_nth_word(const std::string& s, int n);

private:

  cvm::file_logger log;
  cvm::topology::loc_t loc_;
  unsigned id_;

  int num_harts_ = 0;
  int xlen_ = 0;
  int vlen_ = 0;
  CacCore cac_;
  CacCore csr_cac_;

  // Previous instruction's whisper state
  whisper_state_t pw_{};
  whisper_state_t ppw_{};

  // Create a copy of whisper instr in similar format as dut
  rv_instr_t w_;

  uint32_t step_ = 1;

  // State variables
  bool ecall_ = false;
  bool debug_mode_ = false;
  bool excp_in_debug_mode = false;
  uint64_t satp_ = 0;
  uint64_t new_satp_ = 0;

  bool resynch_intr_cause_mismatch_ = false;
  bool resynch_csr_ = false;

  bool deferred_intr_ = false;
  uint64_t mip_ = 0;
  uint64_t prev_mip_ = 0;
  uint64_t e_mip_ = 0;
  uint64_t prev_e_mip_ = 0;
  uint64_t deferred_mip_ = 0;
  bool prev_sync_intr_ = 0;
  bool all_interrupts_defer_= 0;
  bool prev_resync_excp_defer_intr_ = 0;
  uint64_t pre_csr_defermip_ = 0;
  bool pre_undeferred_intr_;
  bool post_undeferred_intr_;
  std::array<uint32_t, max_intr> intr_age_{};
  uint32_t max_pend_intr_age_ = 0;

  // Memmap
  memmap::memmap_t memmap_;

  std::array<int, 16> num_taken_interrupts_{};
  int num_exceptions_ = 0;

  size_8_bytes_t dword_vec_array [vlen/64] = {0};
  int unmask_bits_instr, unmask_bits_uop = 0;
  std::vector<std::string> cosim_resynch_csr_defaults;

  bool terminated_ = false;

};
