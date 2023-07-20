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

class bridge : public bridge_base {

using src_t = cac::src_t;
using resource_t = cac::resource_t;
using resource_id_t = cac::resource_id_t;
using size_8_bytes_t = cac::size_8_bytes_t;
using CacCore = cac::CacCore;

public:
  bridge(int num_harts, int xlen, int vlen, cvm::topology::loc_t loc);
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

  // Process memory access
  //   - Read (Ld completion)
  //   - Insert (St merge buffer insertion)
  //   - Write (St cache write)
  virtual void process_dut_mem_read(hart_id_t hart, mem_t& m) override;
  virtual void process_dut_mb_insert(hart_id_t hart, mem_t& m) override;
  virtual void process_dut_mb_drain(hart_id_t hart, mem_cl_t& m) override;

  // Interrupts
  virtual void process_dut_interrupt(hart_id_t hart, rv_intr_t &i) override;

  // Debug mode
  virtual void enter_debug_mode(rv_debug_t& d) override;
  virtual void exit_debug_mode(rv_debug_t& d) override;

  void reset();
  bool whisper_connect();

  void final_phase();
  void report_metrics();

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
  void print_instr_stdout(hart_id_t hart, const whisper_state_t& w);
  void print_resource(hart_id_t hart, const whisper_state_t& w);
  void update_pc(hart_id_t hart, src_t src, uint64_t data);
  void update_insn(hart_id_t hart, src_t src, uint32_t data);
  void update_regs(hart_id_t hart, const rv_instr_t& d);
  void update_regs(hart_id_t hart, const whisper_state_t& w);
  void update_regs(hart_id_t hart, src_t src, resource_t resource, uint64_t addr, const std::vector<size_8_bytes_t>&& dword_vec);
  void update_mem(hart_id_t hart, rv_instr_t& d);
  void translation_check(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);

  uint64_t translate(hart_id_t hart, uint64_t va, uint8_t priv, memclass_t memclass);

  void process_debug_pre_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);
  void process_interrupt_pre_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);
  void process_interrupt_post_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);
  void process_exception_post_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);
  void process_satp_write_post_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);

  void get_whisper_mip(hart_id_t hart, uint64_t& mip);
  void get_whisper_intr_status(hart_id_t hart, bool& taken, uint64_t& cause);
  void update_intr_age(hart_id_t hart, const rv_instr_t& d);
  void poke_intr_defer_status(hart_id_t hart, uint64_t time, uint64_t mip);
  void poke_mip(hart_id_t hart, uint64_t time, uint64_t);
  void poke_seip(hart_id_t hart, uint64_t time, bool val);

  bool is_ecall(const whisper_state_t& w);
  bool does_instr_match_resynch_list(const whisper_state_t& w);
  bool does_prev_instr_match_resynch_list(const whisper_state_t& w);
  bool does_instr_match_resynch_condition(hart_id_t hart, const rv_instr_t& d, const whisper_state_t& w);
  bool clint_read(const rv_instr_t& d);
  bool htif_read(const rv_instr_t& d);
  bool hpm_counter_read(const whisper_state_t& w);
  bool lrsc_fail(const whisper_state_t& w);
  bool mip_timing_mismatch(hart_id_t hart, const whisper_state_t& w);
  bool xtval_read(const whisper_state_t& w);
  void resynch(hart_id_t hart, const rv_instr_t& d);
  std::string get_nth_word(const std::string& s, int n);

private:

  std::unique_ptr<whisperClient<uint64_t>> client_;

  cvm::file_logger log;

  int num_harts_ = 0;
  int xlen_ = 0;
  int vlen_ = 0;
  cvm::topology::loc_t loc_;
  CacCore cac_;

  // Previous instruction's whisper state per-hart
  std::array<whisper_state_t, max_harts> pw_ {};

  // Previous previous instruction's whisper state per-hart
  std::array<whisper_state_t, max_harts> ppw_ {};

  // Create a copy of whisper instr in similar format as dut
  rv_instr_t w_;

  std::array<uint32_t, max_harts> step_{1};

  // State variables
  bool ecall_ = false;
  bool debug_mode_ = false;

  uint64_t satp_ = 0;
  uint64_t new_satp_ = 0;

  bool resynch_intr_cause_mismatch_ = false;

  std::array<uint64_t, max_harts> mip_{0};
  std::array<uint64_t, max_harts> intr_pins_{0};
  std::array<uint64_t, max_harts> prev_intr_pins_{0};
  std::array<std::array<uint32_t, max_intr>, max_harts> intr_age_{};

  // Memmap
  memmap::memmap_t memmap_;

  int num_stores_ = 0;
};
