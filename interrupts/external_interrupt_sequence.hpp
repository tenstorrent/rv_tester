#pragma once
#include <iostream>
#include <unistd.h>
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"
#include "interrupts.hpp"
//#include "triggers.hpp"
#include "transactor.h"
#include "svdpi.h"

class external_interrupt_sequence {

  public:

    external_interrupt_sequence(cvm::topology::loc_t loc, unsigned id);
    ~external_interrupt_sequence();

    void set_scope(svScope s) { scope_ = s; }

  private:

    void trigger_mode_thread();

    cvm::messenger::task<void> trigger_mode();

    cvm::messenger::task<void> tick();
    cvm::messenger::task<void> trigger();

    void init();
    void drive_interrupt();

  private:

    cvm::topology::loc_t loc_;
    cvm::topology::loc_t axi_mst_loc_l;
    unsigned id_;
    svScope scope_;

   
    cvm::rand::rng<int64_t> rng1;

    uint32_t ext_interrupt_count_ = 0;
    uint32_t ext_trig_interrupt_count_ = 0;
    uint32_t msi_m_file_addr  = 0x40000000;
    uint32_t msi_v_file_addr  = 0x44000000;
    uint32_t msi_vs_file_addr = 0x44000000;
};
