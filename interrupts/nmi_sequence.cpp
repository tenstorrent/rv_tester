#include "nmi_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"

REGISTRY_register(nmi_sequence, INTERRUPTS, cvm::registry::all);

DEFINE_string(nmi, "off", "Enable nmi_sequence in the sim - off/random/patch_trigger/uarch_trigger");
DEFINE_string(random_nmi_count, "0:4", "Number of nmi sequences in the sim if random mode enabled");
DEFINE_string(nmi_start_interval, "1000:4000", "TB cycle interval between reset and first nmi sequence in the sim if random mode enabled");
DEFINE_string(nmi_interval, "1000:4000", "TB cycle interval between nmi sequences in the sim if random mode enabled");
DEFINE_string(nmi_width, "200:500", "TB cycle width of nmi pulses in the sim if random mode enabled");
DEFINE_int32(patch_mode_nmi_interval,10,"Number of Maximum cycles between two nmi while entering patch mode");
DEFINE_bool(nmi_uarch_event, false, "Flag used for generation of NMI of uarch events");

extern "C" {
  void interrupts_init();
  void interrupts_nmi(uint8_t val);
  void interrupts_nmi_triggered();

  uint8_t interrupts_get_nmi_en(const char* mode) {
    return (std::string(mode) != "off");
  }
}

nmi_sequence::nmi_sequence(cvm::topology::loc_t loc, unsigned id) : loc_(loc), id_(id), scope_(nullptr) {

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });
  triggers_loc = cvm::topology::get_from_hierarchy("TOP.PLATFORM.TRIGGERS", 0);

  // nmi sequence threads
  if (FLAGS_nmi == "random") {
    random_mode_thread();
  } else if (FLAGS_nmi == "patch_trigger") {
    patch_trigger_mode_thread();
  } else if (FLAGS_nmi == "uarch_trigger") {
    uarch_trigger_mode_thread();
  }
}

nmi_sequence::~nmi_sequence() {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_nmi_toggled_count\": \"{}\"}}\n", id_, nmi_count_);
}

void nmi_sequence::random_mode_thread() {
  auto *task = +[] (nmi_sequence* m) -> cvm::messenger::task<void> {
    co_await m->random_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void nmi_sequence::patch_trigger_mode_thread() {
  auto *task = +[] (nmi_sequence* m) -> cvm::messenger::task<void> {
    co_await m->patch_trigger_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void nmi_sequence::uarch_trigger_mode_thread() {
  auto *task = +[] (nmi_sequence* m) -> cvm::messenger::task<void> {
    co_await m->uarch_trigger_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> nmi_sequence::random_mode() {

  co_await reset();
  cvm::log(cvm::NONE, "[interrupts] reset\n");

  while (true) {
    // Wait for next tick generated after a random interval "nmi_interval"
    co_await tick();

    nmi_count_++;
    cvm::log(cvm::HIGH, "[interrupts][h{}] Starting nmi sequence - count = {}\n", id_, nmi_count_);

    nmi(id_, ASSERT);

    // Wait for next tick generated after a random width "nmi_width"
    co_await tick();

    nmi(id_, DEASSERT);
  }
  co_return;
}

cvm::messenger::task<void> nmi_sequence::patch_trigger_mode() {
  while(1){
     // Wait for next selected trigger
     co_await trigger();

     // Wait for random ticks after trigger
     int num_ticks = rng1() %FLAGS_patch_mode_nmi_interval;  
     for(int i=0;i<num_ticks;i++)
       co_await tick();

     cvm::log(cvm::HIGH, "[interrupts][h{}] Starting event triggered nmi sequence\n", id_);
     nmi(id_, ASSERT);

     // Wait for next tick generated after a random width "nmi_width"
     co_await tick();

     nmi(id_, DEASSERT);
  }
}

cvm::messenger::task<void> nmi_sequence::uarch_trigger_mode() {
  while(1){
     // Wait for next selected trigger
     co_await trigger();

     nmi(id_, ASSERT);
     nmi_triggered(id_);

     // Wait for next tick generated after a random width "nmi_width"
     co_await tick();

     nmi(id_, DEASSERT);
  }
}

cvm::messenger::task<void> nmi_sequence::reset() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::interrupts::m_reset<>>(loc_);
  co_return;
}

void nmi_sequence::init() {
  cvm::registry::callbacks.push(
    scope_,
    []() {
      interrupts_init();
    });
}

void nmi_sequence::nmi(unsigned hart, uint8_t assert) {
  cvm::registry::callbacks.push(
    scope_,
    [assert, hart]() {
      cvm::log(cvm::HIGH, "[interrupts][h{}] {} nmi\n", hart, assert ? "assert" : "deassert");
      interrupts_nmi(assert);
    });
}

void nmi_sequence::nmi_triggered(unsigned hart) {
  cvm::registry::callbacks.push(
    scope_,
    [hart]() {
      cvm::log(cvm::HIGH, "[interrupts][h{}] trigger assert nmi\n", hart);
      interrupts_nmi_triggered();
    });
}

cvm::messenger::task<void> nmi_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::interrupts::m_nmi_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> nmi_sequence::trigger() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::triggers::m_event_trigger_tick<>>(triggers_loc);
  co_return;
}


