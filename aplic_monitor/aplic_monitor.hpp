#ifndef _RISCV_DEBUG_MODULE_H
#define _RISCV_DEBUG_MODULE_H
#include <iostream>
#include <memory>
#include <stdint.h>
#include <set>
#include <vector>
#include <cassert>
#include <unordered_set>

#include "cvm/logger.hpp"
#include "cvm/topology.hpp"
#include "rv_tester_transactions.hpp"
#include "Aplic.hpp"


#define max_hartid 1 // Define the maximum number of harts in the system
#define halt_on_reset false

typedef uint64_t reg_t;



class aplic_monitor
{
public:
  aplic_monitor(cvm::topology::loc_t, unsigned);



  // Called for every cycle the JTAG TAP spends in Run-Test/Idle.
  // void run_test_idle();

  // Called when one of the attached harts was reset.
  //void proc_reset(unsigned id);

  void process(const rv_tester_transactions::aplic_monitor::msi_req<> &msi_req);
  void process(const rv_tester_transactions::aplic_monitor::aplic_intr_req<> &aplic_intr_req);
  void process(const rv_tester_transactions::aplic_monitor::aplic_mmr_load_cmd<> &aplic_mmr_load_cmd);
  void process(const rv_tester_transactions::aplic_monitor::aplic_mmr_load_data<> &aplic_mmr_load_data);
  void process(const rv_tester_transactions::aplic_monitor::aplic_mmr_store<> &aplic_mmr_store);
  
  std::shared_ptr<TT_APLIC::Domain> root;
private:
  // cvm::file_logger log;
  void reset();
  unsigned hartCount = 2;
  unsigned interruptCount = 33;
  unsigned domainCount = 4;

  uint64_t addr = 0x1000000;
  uint64_t stride = 32*1024;
  
};

#endif
