#include "external_interrupt_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"

REGISTRY_register(external_interrupt_sequence, INTERRUPTS, cvm::registry::all);

DEFINE_bool(interrupt_trigger_en, true, "Enable event based external_interrupt_sequence in the sim");
DEFINE_int32(interrupt_trigger_count, 10, "Number of nmi sequences in the sim if random mode enabled");
DEFINE_int32(interrupt_trigger_interval,100, "TB cycle interval between nmi sequences in the sim if random mode enabled");

extern "C" {
  void trigger_interrupts_init();
  void trigger_interrupts(uint8_t val);

}

external_interrupt_sequence::external_interrupt_sequence(cvm::topology::loc_t loc, unsigned id) : loc_(loc), id_(id), scope_(nullptr) {

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });

  // trigger sequence threads
  if (FLAGS_interrupt_trigger_en) {
    trigger_mode_thread();
  }
}

external_interrupt_sequence::~external_interrupt_sequence() {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_interrupts_nmi_count\": \"{}\"}}\n", id_, nmi_count_);
}



void external_interrupt_sequence::trigger_mode_thread() {
  auto *task = +[] (external_interrupt_sequence* m) -> cvm::messenger::task<void> {
    co_await m->trigger_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};


cvm::messenger::task<void> external_interrupt_sequence::trigger_mode() {
  // Wait for next selected trigger
  while(1){
    std::cout<<"\nwhile 1\n";
  co_await trigger();
    std::cout<<"\ngot trigger 1\n";

  // Wait for tick after trigger

  drive_interrupt(id_, ASSERT);

  // Wait for next tick generated after a random width "nmi_width"
  co_await trigger();
    std::cout<<"\ngot trigger 2\n";

  drive_interrupt(id_, DEASSERT);
  }
}

void external_interrupt_sequence::init() {
  cvm::registry::callbacks.push(
    scope_,
    []() {
      trigger_interrupts_init();
    });
}

void external_interrupt_sequence::drive_interrupt(unsigned hart, uint8_t assert) {
  cvm::registry::callbacks.push(
    scope_,
    [assert, hart]() {
      cvm::log(cvm::HIGH, "[interrupts][h{}] {} nmi\n", hart, assert ? "assert" : "deassert");
      trigger_interrupts(assert);
    });
}

cvm::messenger::task<void> external_interrupt_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::interrupts::m_nmi_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> external_interrupt_sequence::trigger() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::interrupts::m_event_trigger_tick<>>(loc_);
  co_return;
}

