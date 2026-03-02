#pragma once
#include <iostream>
#include <unistd.h>
#include <sstream>
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"
#include "interrupts.hpp"
#include "transactor.h"
#include "svdpi.h"

class external_interrupt_sequence {

  public:

    external_interrupt_sequence(cvm::topology::loc_t loc, unsigned id);
    ~external_interrupt_sequence();

    void set_scope(svScope s) { scope_ = s; }

  private:

    void interrupt_injection_thread();

    cvm::messenger::task<void> interrupt_trigger();

    cvm::messenger::task<void> delayed_trigger();

    void init();
    void drive_interrupt();
    void capture_trigger_info(int32_t a, int32_t b);
    void gen_interrupt_timings();
    uint32_t get_logical_core_id(uint32_t physical_hart_id);

  private:

    cvm::topology::loc_t loc_;
    cvm::topology::loc_t axi_mst_loc_l;
    cvm::topology::loc_t triggers_loc;
    unsigned id_;
    svScope scope_;



    cvm::rand::uniform_dist<uint32_t> rng1;
    uint32_t trigger_interrupt_count_;


    uint32_t ext_interrupt_count_ = 0;
    uint32_t ext_trig_interrupt_count_ = 0;
    uint32_t msi_m_file_addr;
    uint32_t msi_s_file_addr;
    uint32_t msi_vs_file_addr;
    int32_t  last_trigger     = 0;
    int32_t  current_trigger  = 0;
    bool     drive_msi_in_curr_hart;

    uint32_t intr_vs_id_vgein_ = 0;
    uint32_t intr_vs_id_random_ = 0;
    uint32_t intr_vs_id_two_ = 0;
    uint32_t ncores_;
};
