#pragma once

#include "bridge_if.h"

class bridge_base {

public:

  virtual ~bridge_base() {}
  
  virtual void process_dut_instr_retire(hart_id_t hart, rv_instr_t &d) = 0;
  virtual void process_dut_mcm_read(hart_id_t hart, mem_t &m) = 0;
  virtual void process_dut_mcm_insert(hart_id_t hart, mem_t &m) = 0;
  virtual void process_dut_mcm_write(hart_id_t hart, mem_t &m) = 0;
  virtual void enter_debug_mode(rv_debug_t &d) = 0;
  virtual void exit_debug_mode(rv_debug_t &d) = 0;
  virtual void process_dut_interrupt(hart_id_t hart, rv_intr_t &i) = 0;
};
