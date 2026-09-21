// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

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

class mti_sequence {

public:
  mti_sequence(cvm::topology::loc_t loc, unsigned id);
  ~mti_sequence();

  void configure();

private:
  void random_mode_thread();
  void uarch_trigger_mode_thread();

  cvm::messenger::task<void> random_mode();
  cvm::messenger::task<void> uarch_trigger_mode();

  cvm::messenger::task<void> assert_tick();
  cvm::messenger::task<void> trigger();

  void mti(uint8_t assert);

  cvm::rand::uniform_dist<int64_t> rng1;

private:
  cvm::topology::loc_t loc_;
  cvm::topology::loc_t triggers_loc;
  unsigned id_;

  uint32_t mti_count_ = 0;
};
