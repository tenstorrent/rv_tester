#include "nmi_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "rv_tester/rv_tester_plusargs.h"

REGISTRY_register(nmi_sequence, INTERRUPTS, cvm::registry::all);

DEFINE_string(nmi, "off", "Enable nmi_sequence in the sim - off/random/uarch_trigger");
DEFINE_bool(nmi_rand_en, false, "Enable nmi_sequence tick");
DEFINE_string(nmi_count, "0:4", "Number of nmi sequences in the sim if random mode enabled");
DEFINE_string(nmi_start_interval, "1000:4000", "TB cycle interval between reset and first nmi sequence in the sim if random mode enabled");
DEFINE_string(nmi_interval, "1000:4000", "TB cycle interval between nmi sequences in the sim if random mode enabled");
DEFINE_string(nmi_width, "1:1", "TB cycle width of nmi pulses in the sim if random mode enabled");

extern "C" {
  void drive_nmi(uint8_t val);
}

nmi_sequence::nmi_sequence(cvm::topology::loc_t loc, unsigned id) : loc_(loc), id_(id), scope_(nullptr) {

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });
  triggers_loc = cvm::topology::get_from_hierarchy("TOP.PLATFORM.TRIGGERS", 0);

  // Deassert signal comes from trickbox
  cvm::registry::messenger.connect<uint8_t>(loc_, [this](uint8_t assert) { return this->nmi(assert); });

  // nmi sequence threads
  if (FLAGS_nmi_rand_en || (FLAGS_nmi == "random")) {
    random_mode_thread();
  } else if (FLAGS_nmi == "uarch_trigger") {
    trigger_mode_thread();
  }
}

nmi_sequence::~nmi_sequence() {
  if (FLAGS_metrics)
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_nmi_toggled_count\": \"{}\"}}\n", id_, nmi_count_);
}

void nmi_sequence::random_mode_thread() {
  auto *task = +[] (nmi_sequence* m) -> cvm::messenger::task<void> {
    co_await m->random_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void nmi_sequence::trigger_mode_thread() {
  auto *task = +[] (nmi_sequence* m) -> cvm::messenger::task<void> {
    co_await m->trigger_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> nmi_sequence::random_mode() {

  while (true) {
    // Wait for next tick generated after a random interval "nmi_interval"
    co_await assert_tick();

    nmi_count_++;
    cvm::log(cvm::HIGH, "[interrupts][h{}] Starting nmi sequence - count = {}\n", id_, nmi_count_);

    nmi(ASSERT);
  }
  co_return;
}

cvm::messenger::task<void> nmi_sequence::trigger_mode() {
  while(1){
     // Wait for next selected trigger
     co_await trigger();

     nmi(ASSERT);
  }
}

void nmi_sequence::nmi(uint8_t assert) {
  cvm::registry::callbacks.push(
    scope_,
    [assert, this]() {
      cvm::log(cvm::HIGH, "[interrupts][h{}] {} nmi\n", id_, assert ? "assert" : "deassert");
      drive_nmi(assert);
    });
}

cvm::messenger::task<void> nmi_sequence::assert_tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::interrupts::m_nmi_assert_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> nmi_sequence::trigger() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::triggers::m_event_trigger_tick<>>(triggers_loc);
  co_return;
}


