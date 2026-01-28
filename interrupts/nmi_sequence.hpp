#pragma once
#include <iostream>
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"
#include "interrupts.hpp"
#include "transactor.h"
#include "svdpi.h"

class nmi_sequence {

  public:

    nmi_sequence(cvm::topology::loc_t loc, unsigned id);
    ~nmi_sequence();

    void set_scope(svScope s) { scope_ = s; }

  private:

    void random_mode_thread();
    void trigger_mode_thread();

    cvm::messenger::task<void> random_mode();
    cvm::messenger::task<void> trigger_mode();

    cvm::messenger::task<void> assert_tick();
    cvm::messenger::task<void> trigger();
    cvm::messenger::task<void> reset();

    void init();
    void nmi(uint8_t assert);
    void nmi_triggered(unsigned hart);
    
    cvm::rand::uniform_dist<int64_t> rng1;

  private:

    cvm::topology::loc_t loc_;
    cvm::topology::loc_t triggers_loc;
    unsigned id_;
    svScope scope_;

    uint32_t nmi_count_ = 0;
};
