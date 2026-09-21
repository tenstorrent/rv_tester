// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "mti_sequence.hpp"
#include "sysmod_plusargs.h"
#include "rv_tester_plusargs.h"

REGISTRY_register(mti_sequence, INTERRUPTS, cvm::registry::all);

DEFINE_string(mti, "off", "Enable mti_sequence in the sim - off/random/uarch_trigger");
DEFINE_bool(mti_rand_en, false, "Enable mti_sequence tick");
DEFINE_string(mti_count, "0:4", "Number of mti sequences in the sim if random mode enabled");
DEFINE_string(mti_start_interval, "1000:4000", "TB cycle interval between reset and first mti sequence in the sim if random mode enabled");
DEFINE_string(mti_interval, "1000:4000", "TB cycle interval between mti sequences in the sim if random mode enabled");
DEFINE_string(mti_width, "1:1", "TB cycle width of mti pulses in the sim if random mode enabled");

extern "C" {
void drive_mti(uint8_t val);
}

mti_sequence::mti_sequence(cvm::topology::loc_t loc, unsigned id) : loc_(loc), id_(id) {

  triggers_loc = cvm::topology::get_from_hierarchy("TOP.PLATFORM.TRIGGERS", 0);
}

void mti_sequence::configure() {

  // Deassert signal comes from trickbox (uses uint16_t to differentiate from NMI which uses uint8_t)
  cvm::registry::messenger.connect<uint16_t>(loc_, [this](uint16_t assert) { return this->mti(assert); });

  // mti sequence threads
  if (FLAGS_mti_rand_en || (FLAGS_mti == "random")) {
    random_mode_thread();
  } else if (FLAGS_mti == "uarch_trigger") {
    uarch_trigger_mode_thread();
  } else if (FLAGS_mti != "off") {
    cvm::log(cvm::ERROR, "Error: [mti_sequence][h{}] Invalid value for +mti flag: '{}'. Valid values are: off, random, uarch_trigger\n", id_, FLAGS_mti);
  }
}

mti_sequence::~mti_sequence() {
  if (FLAGS_metrics)
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_mti_toggled_count\": \"{}\"}}\n", id_, mti_count_);
}

void mti_sequence::random_mode_thread() {
  auto* task = +[](mti_sequence* m) -> cvm::messenger::task<void> {
    co_await m->random_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void mti_sequence::uarch_trigger_mode_thread() {
  auto* task = +[](mti_sequence* m) -> cvm::messenger::task<void> {
    co_await m->uarch_trigger_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> mti_sequence::random_mode() {

  while (true) {
    // Wait for next tick generated after a random interval "mti_interval"
    co_await assert_tick();

    mti_count_++;
    cvm::log(cvm::HIGH, "[interrupts][h{}] Starting mti sequence - count = {}\n", id_, mti_count_);

    mti(ASSERT);
  }
  co_return;
}

cvm::messenger::task<void> mti_sequence::uarch_trigger_mode() {
  while (1) {
    // Wait for next selected trigger
    co_await trigger();

    mti(ASSERT);
  }
}

void mti_sequence::mti(uint8_t assert) {
  cvm::registry::callbacks.push(
      loc_,
      [assert, this]() {
        cvm::log(cvm::HIGH, "[interrupts][h{}] {} mti\n", id_, assert ? "assert" : "deassert");
        drive_mti(assert);
      });
}

cvm::messenger::task<void> mti_sequence::assert_tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::interrupts::m_mti_assert_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> mti_sequence::trigger() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::triggers::m_event_trigger_tick<>>(triggers_loc);
  co_return;
}
