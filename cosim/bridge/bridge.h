// Licensed under the Apache License, Version 2.0, see LICENSE.TT for details

#pragma once

#include <string>
#include <algorithm>
#include <iomanip>

#include "cvm/topology.hpp"
#include "bridge_base.h"
#include "memmap.h"
#include "cvm/logger.hpp"
#include "src/cacCore.h"
#include "arch_sample.h"        // ArchSample

#include "whisper_client_socket.h"
#include "whisper_client_shm.h"

class bridge : public bridge_base {

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
  bool whisper_connect(std::string cmd, int timeout);

  void final_phase();
  void report_metrics();

private:

  typedef enum {
    int_reg = 0,
    fp_reg = 1,
    vec_reg = 4
  } resource_t;

  typedef enum {
    dut,
    whisper
  } src_t;

  typedef enum {
    read,
    write,
    fetch
  } memclass_t;

private:

  std::string get_whisper_cmd();

  void update_dut_state(hart_id_t hart, rv_instr_t& d);
  void update_whisper_state(hart_id_t hart, whisper_state_t& w);
  void step(hart_id_t hart, whisper_state_t& w);
  void print_instr(hart_id_t hart, const whisper_state_t& w);
  void print_resource(hart_id_t hart, const whisper_state_t& w);
  void update_pc(hart_id_t hart, src_t src, uint64_t data);
  void update_regs(hart_id_t hart, const rv_instr_t& d);
  void update_regs(hart_id_t hart, const whisper_state_t& w);
  void update_regs(hart_id_t hart, src_t src, resource_t resource, uint64_t addr, size8BytesT dword_array[]);
  void update_mem(hart_id_t hart, rv_instr_t& d);
  void translation_check(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);

  uint64_t translate(hart_id_t hart, uint64_t va, uint8_t priv, memclass_t memclass);

  void handle_debug(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);
  void handle_interrupt(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);
  void handle_exception(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);
  void handle_satp(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);

  void check_interrupt(hart_id_t hart);
  void poke_pend_interrupt(hart_id_t hart);
  void poke_interrupt(hart_id_t hart, uint64_t mip);
  void poke_seip(hart_id_t hart, bool val);

  bool is_ecall(const whisper_state_t& w);
  bool does_instr_match_resynch_list(const whisper_state_t& w);
  bool does_prev_instr_match_resynch_list(const whisper_state_t& w);
  bool does_instr_match_resynch_condition(const rv_instr_t& d, const whisper_state_t& w);
  bool clint_read(const rv_instr_t& d);
  bool htif_read(const rv_instr_t& d);
  bool mhpm_counter_read(const whisper_state_t& w);
  bool lrsc_fail(const whisper_state_t& w);
  bool xtval_read(const whisper_state_t& w);
  void resynch(hart_id_t hart, const rv_instr_t& d);

private:

  std::unique_ptr<whisperClient> client_;

  cvm::file_logger log;

  int num_harts_ = 0;
  int xlen_ = 0;
  int vlen_ = 0;
  cvm::topology::loc_t loc_;
  CacCore cac_;
  ArchSample archcov;

  // Previous instruction's whisper state
  whisper_state_t pw_ {};

  // Create a copy of whisper instr in similar format as dut
  rv_instr_t w_;

  // State variables
  bool ecall_ = false;
  bool debug_mode_ = false;

  uint64_t satp_ = 0;
  uint64_t new_satp_ = 0;

  std::array<bool, max_harts> is_intr_pend_{};
  std::array<uint64_t, max_harts> pend_intr_{};
  std::array<uint32_t, max_harts> pend_intr_count_{};
  std::array<bool, max_harts> is_seip_pend_{};
  std::array<bool, max_harts> pend_seip_count_{};

  // Memmap
  memmap::memmap_t memmap_;

  // Metrics map
  std::array<std::map<std::string, std::string>, max_harts> metrics_;

  int num_stores_ = 0;
};
