#pragma once

#include "bridge_if.h"

class bridge_base {

public:

  virtual ~bridge_base() {}
  
  virtual void process_dut_instr_retire(hart_id_t hart, rv_instr_t &d) = 0;
  virtual void process_dut_instr_group_retire(hart_id_t hart, rv_instr_group_t &d) = 0;
  virtual void process_steps(hart_id_t hart, uint32_t n_retire, uint64_t cycle, uint64_t steps, uint64_t skips, uint64_t final) = 0;
  virtual void process_dut_csr_hw_update(hart_id_t hart, csr_t &c) = 0;
  virtual void process_dut_mcm_read(hart_id_t hart, mem_t &m, bool cache) = 0;
  virtual void process_dut_mcm_insert(hart_id_t hart, mem_t &m) = 0;
  virtual void process_dut_mcm_bypass(hart_id_t hart, mem_t &m, bool cache) = 0;
  virtual void process_dut_mcm_write(hart_id_t hart, mem_cl_t &m) = 0;
  virtual void process_dut_mcm_ifetch(hart_id_t hart, mem_t &m) = 0;
  virtual void process_dut_mcm_ievict(hart_id_t hart, mem_t &m) = 0;
  virtual void enter_debug_mode(rv_debug_t &d) = 0;
  virtual void exit_debug_mode(rv_debug_t &d) = 0;
  virtual void process_dut_nmi(hart_id_t hart, rv_nmi_t &n) = 0;
  virtual void process_dut_interrupt(hart_id_t hart, rv_intr_t &i) = 0;
  virtual void process_dut_timer(hart_id_t hart, rv_intr_t &i) = 0;
  virtual void process_dut_mtip(hart_id_t hart, uint64_t cycle, bool mtip, bool trap_intr) = 0;
  virtual void process_dut_imsic_msi(hart_id_t hart, mem_t &m) = 0;
  virtual void process_debug_haltreq(bool haltreq) = 0;
  /// Align bridge/Whisper debug state with DUT (RVFI m_debug.enter); controlled by +sync_debug_mode_from_dut (default on).
  virtual void sync_debug_mode_from_dut(hart_id_t hart, uint64_t cycle, bool dut_in_debug) = 0;
};
