#include "external_interrupt_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"

REGISTRY_register(external_interrupt_sequence, INTERRUPTS, cvm::registry::all);

DEFINE_bool(interrupt_trigger_en, true, "Enable event based external_interrupt_sequence in the sim");
DEFINE_string(trigger_interrupt_count, "7:10", "Number of MSI in the sim if random mode enabled");
DEFINE_string(trigger_interrupt_weight_ratio, "6:2:2", "Ratio of Number of interrupts randomly driven  in phases after trigger event");
DEFINE_int32(interrupt_trigger_interval,10, "Max TB cycle interval between MSI random mode enabled");


external_interrupt_sequence::external_interrupt_sequence(cvm::topology::loc_t loc, unsigned id) : loc_(loc), id_(id), scope_(nullptr) {

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });
  cvm::registry::messenger.connect<rv_tester_transactions::triggers::m_event_trigger_tick<>>(
      loc_,
      [this](const rv_tester_transactions::triggers::m_event_trigger_tick<>& t) { return this->capture_trigger_info(t.event_trigger); }); 
  
 
  axi_mst_loc_l = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST", 0);
  
  triggers_loc = cvm::topology::get_from_hierarchy("TOP.PLATFORM.TRIGGERS", 0);

  trigger_interrupt_count_ =  cvm::rand::get<uint32_t>(FLAGS_trigger_interrupt_count);
  // trigger sequence threads`
  interrupts_driven = 0;
  if (FLAGS_interrupt_trigger_en) {
    trigger_mode_thread();
  }
}

external_interrupt_sequence::~external_interrupt_sequence() {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_external_interrupts_count\": \"{}\"}}\n", id_, ext_interrupt_count_);
}

void external_interrupt_sequence::gen_interrupt_timings(){
  //TODO: uncomment once dists are working fine
  //  std::stringstream ss(FLAGS_trigger_interrupt_ratio);
  //  std::vector<double> weights(FLAGS_trigger_interrupt_count);
  //  cvm::rand::discrete_dist<uint8_t> dist(weights);
  //  std::string temp;
  //  std::vector<int> values;

  //  while (getline(ss, temp, ':')) {
  //     values.push_back(std::stoi(temp));
  //  }
  //  for (int i=1; i<=10; i++)
  //     weights[i] = values[0];
  //  for (int i=11; i<=30; i++)
  //     weights[i] = values[1];
  //  for (int i=30; i<=100; i++)
  //     weights[i] = values[2];

}

void external_interrupt_sequence::capture_trigger_info(int32_t trigger_info){
  last_trigger = current_trigger;  
  current_trigger = trigger_info;  
}

void external_interrupt_sequence::trigger_mode_thread() {
  auto *task = +[] (external_interrupt_sequence* m) -> cvm::messenger::task<void> {
    co_await m->trigger_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};


cvm::messenger::task<void> external_interrupt_sequence::trigger_mode() {
  while(1){
    bool abrupt_exit = false;
    // Wait for next selected trigger
    co_await trigger();
    if(last_trigger != current_trigger){ //trigger transition detected
      gen_interrupt_timings();//empty as of today
      interrupts_driven = 0;
    }

    if(interrupts_driven < trigger_interrupt_count_){
       uint8_t num = rng1() % FLAGS_interrupt_trigger_interval ;
       //wait for num cycles before driving next MSI
       for(int i =0; i< num;i++){
         co_await trigger();
         if(last_trigger != current_trigger){
           abrupt_exit = true;
           break;
         }  
       }
      
       if(!abrupt_exit){
         drive_interrupt();
         interrupts_driven++;
       }
    }
  }
}


cvm::messenger::task<void> external_interrupt_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::triggers::m_event_trigger_tick<>>(triggers_loc);
  co_return;
}

cvm::messenger::task<void> external_interrupt_sequence::trigger() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::triggers::m_event_trigger_tick<>>(triggers_loc);
  co_return;
}

  // Used to assert/deassert a interrupter interrupt (PIPI) for given hart.
void external_interrupt_sequence::drive_interrupt(){
    
   unsigned interrupt_num  =  (rng1() % (FLAGS_imsic_intr_threshold )) ; 
   unsigned interrupt_file =  (rng1() % (3 )) ; //gen either machine supervisor or hypervisor file
   unsigned interrupt_hart =  id_; // select hart_id where event was triggered
   unsigned vs_id          =  (rng1() % (FLAGS_imsic_vs_id_threshold )) ; //sel vs id
   
   if(interrupt_file == 0x02) interrupt_num %= FLAGS_imsic_vs_intr_threshold;

   cvm::log(cvm::LOW,"[ExtInterruptSeq] IMSIC interrupt num: {} interrupt file: {} Interrupt hart:{} hypervisor/supervisor id : {}\n", static_cast<uint32_t>(interrupt_num), interrupt_file, interrupt_hart, vs_id);
   
   uint32_t addr;
   if(interrupt_file == 0x0){
      addr = msi_m_file_addr + (interrupt_hart << 18);
   }else if(interrupt_file == 0x01){
      addr = msi_s_file_addr + (interrupt_hart << 18);
   }else if(interrupt_file == 0x02){
      addr = msi_vs_file_addr+ (vs_id << 12) + (interrupt_hart << 18);
   }else{
      cvm::log(cvm::ERROR, "[ExtInterruptSeq] Wrong IMSIC interrupt file specified\n");
   }
   uint32_t length = 0x40;

   std::vector<uint8_t> data;
   std::vector<bool> strb;
   for (uint8_t i = 0; i < 64; ++i) {
     data.push_back(0x0);
     strb.push_back(0x0);
   }  
   for (uint8_t i = 0; i < 4; ++i) {
     uint8_t currentByte = static_cast<uint8_t>((interrupt_num >> (8 * i)) & 0xFF);
     data[i] = currentByte;
     strb[i] = 0x1;
   }
   cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
 
  }
