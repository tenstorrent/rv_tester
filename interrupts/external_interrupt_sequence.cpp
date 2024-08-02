#include "external_interrupt_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"

REGISTRY_register(external_interrupt_sequence, INTERRUPTS, cvm::registry::all);

DEFINE_bool(interrupt_trigger_en, true, "Enable event based external_interrupt_sequence in the sim");
DEFINE_int32(interrupt_trigger_count, 10, "Number of nmi sequences in the sim if random mode enabled");
DEFINE_int32(interrupt_trigger_interval,100, "TB cycle interval between nmi sequences in the sim if random mode enabled");


external_interrupt_sequence::external_interrupt_sequence(cvm::topology::loc_t loc, unsigned id) : loc_(loc), id_(id), scope_(nullptr) {

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });
  
  auto masters = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST");
  
  axi_mst_loc_l = masters[0];
  cvm::rand::seed(1);
  // trigger sequence threads
  if (FLAGS_interrupt_trigger_en) {
    trigger_mode_thread();
  }
}

external_interrupt_sequence::~external_interrupt_sequence() {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_external_interrupts_count\": \"{}\"}}\n", id_, ext_interrupt_count_);
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

  drive_interrupt();

  // Wait for next tick generated after a random width "nmi_width"
  co_await trigger();
    std::cout<<"\ngot trigger 2\n";

  drive_interrupt();
}
}


cvm::messenger::task<void> external_interrupt_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::triggers::m_event_trigger_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> external_interrupt_sequence::trigger() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::triggers::m_event_trigger_tick<>>(loc_);
  co_return;
}

  // Used to assert/deassert a interrupter interrupt (PIPI) for given hart.
  void external_interrupt_sequence::drive_interrupt()
  {
    unsigned interrupt_num  = (rng1() % (FLAGS_imsic_intr_threshold )) ; //gen iter between 1 to max simul instr
    unsigned interrupt_file = (rng1() % (3 )) ; //gen iter between 1 to max simul instr
    unsigned interrupt_hart = (rng1() % (FLAGS_imsic_hart_threshold )) ; //gen iter between 1 to max simul instr
    unsigned vs_id          = (rng1() % (FLAGS_imsic_vs_intr_threshold )) ; //gen iter between 1 to max simul instr
    

    cvm::log(cvm::HIGH,"[Ext Internal Seq] IMSIC interrupt num: {} interrupt file: {} Interrupt hart:{} hypervisor/supervisor id : {}\n", static_cast<uint32_t>(interrupt_num), interrupt_file, interrupt_hart, vs_id);
    
    uint32_t addr1 = 0x900;
    if(interrupt_file == 0x0){
       addr1 = msi_m_file_addr + (interrupt_hart << 18);
    }else if(interrupt_file == 0x01){
       addr1 = msi_v_file_addr + (interrupt_hart << 18);;
    }else if(interrupt_file == 0x02){
       addr1 = msi_vs_file_addr+ (vs_id << 12) + (interrupt_hart << 18);
    }else{
       cvm::log(cvm::ERROR, "[Trickbox] Wrong IMSIC interrupt file specified\n");
    }
    uint32_t length1 = 0x40;

    std::vector<uint8_t> data1;
    std::vector<bool> strb1;
    for (uint8_t i = 0; i < 64; ++i) {
      data1.push_back(0x0);
      strb1.push_back(0x0);
    }  
    for (uint8_t i = 0; i < 4; ++i) {
      uint8_t currentByte = static_cast<uint8_t>((interrupt_num >> (8 * i)) & 0xFF);
      data1[i] = currentByte;
      strb1[i] = 0x1;
    }
    cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr1, length1, data1, strb1});
 
  }