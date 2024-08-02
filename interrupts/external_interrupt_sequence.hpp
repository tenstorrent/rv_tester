#pragma once
#include <iostream>
#include <unistd.h>
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

    void trigger_mode_thread();

    cvm::messenger::task<void> trigger_mode();

    cvm::messenger::task<void> tick();
    cvm::messenger::task<void> trigger();

    void init();
    void drive_interrupt(unsigned hart, uint8_t assert);

  private:

    cvm::topology::loc_t loc_;
    unsigned id_;
    svScope scope_;

    uint32_t nmi_count_ = 0;
};
