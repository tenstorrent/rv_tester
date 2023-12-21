#ifndef _RISCV_DEBUG_MODULE_H
#define _RISCV_DEBUG_MODULE_H

#include <memory>
#include <stdint.h>
#include <set>
#include <vector>
#include <cassert>
#include <unordered_set>

#include "cvm/logger.hpp"
#include "cvm/topology.hpp"
#include "rv_tester_transactions.hpp"

#include "processor.h"
#include "debug_defines.h"
#include "opcodes.h"
#include "encoding.h"
#include "decode.h"
#include "debug_rom.h"
#include "debug_rom_defines.h"

#define max_hartid 1 // Define the maximum number of harts in the system
#define halt_on_reset false

typedef uint64_t reg_t;



class aplic_monitor_t
{
public:
  aplic_monitor_t(cvm::topology::loc_t, unsigned);



  // Called for every cycle the JTAG TAP spends in Run-Test/Idle.
  // void run_test_idle();

  // Called when one of the attached harts was reset.
  //void proc_reset(unsigned id);

  void process(const rv_tester_transactions::aplic_monitor::msi_req<> &msi_req);
  void process(const rv_tester_transactions::aplic_monitor::aplic_mmr_load_cmd<> &aplic_mmr_load_cmd);
  void process(const rv_tester_transactions::aplic_monitor::aplic_mmr_load_data<> &aplic_mmr_load_data);
  void process(const rv_tester_transactions::aplic_monitor::aplic_mmr_store<> &aplic_mmr_store);

private:
  // cvm::file_logger log;
  void reset();
};

#endif
