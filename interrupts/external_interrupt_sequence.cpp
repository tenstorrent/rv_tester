#include "external_interrupt_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "rv_tester/rv_tester_plusargs.h"

REGISTRY_register(external_interrupt_sequence, INTERRUPTS, cvm::registry::all);

DEFINE_bool(patch_interrupt_trigger_en, false, "Enable patch event based external_interrupt_sequence in the sim");
DEFINE_bool(uarch_interrupt_trigger_en, false, "Enable event based external_interrupt_sequence in the sim");
DEFINE_string(trigger_interrupt_count, "7:10", "Number of MSI in the sim if random mode enabled");
DEFINE_string(trigger_interrupt_weight_ratio, "6:2:2", "Ratio of Number of interrupts randomly driven  in phases after trigger event");
DEFINE_int32(interrupt_trigger_interval,10, "Max TB cycle interval between MSI random mode enabled");


external_interrupt_sequence::external_interrupt_sequence(cvm::topology::loc_t loc, unsigned id) : loc_(loc), id_(id), scope_(nullptr) {

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });
 
  axi_mst_loc_l = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST", 0);
  triggers_loc = cvm::topology::get_from_hierarchy("TOP.PLATFORM.TRIGGERS", 0);

  cvm::registry::messenger.connect<rv_tester_transactions::triggers::m_event_trigger_tick<>>(
      triggers_loc,
      [this](const rv_tester_transactions::triggers::m_event_trigger_tick<>& t) { return this->capture_trigger_info(t.event_trigger, t.per_core_evt_vector); }); 
  
  trigger_interrupt_count_ =  cvm::rand::get<uint32_t>(FLAGS_trigger_interrupt_count);
  // trigger sequence threads`
  interrupts_driven = 0;
  if (FLAGS_patch_interrupt_trigger_en) {
    patch_trigger_mode_thread();
  }
  if (FLAGS_uarch_interrupt_trigger_en) {
    uarch_trigger_mode_thread();
  }
}

external_interrupt_sequence::~external_interrupt_sequence() {
  if (FLAGS_metrics)
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

void external_interrupt_sequence::capture_trigger_info(int32_t trigger_info, int32_t per_core_trigger_vlds){
  last_trigger = current_trigger;  
  current_trigger = trigger_info; 
  drive_msi_in_curr_hart = (per_core_trigger_vlds == (1 << id_));
}

void external_interrupt_sequence::patch_trigger_mode_thread() {
  auto *task = +[] (external_interrupt_sequence* m) -> cvm::messenger::task<void> {
    co_await m->patch_trigger_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void external_interrupt_sequence::uarch_trigger_mode_thread() {
  auto *task = +[] (external_interrupt_sequence* m) -> cvm::messenger::task<void> {
    co_await m->uarch_trigger_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};


cvm::messenger::task<void> external_interrupt_sequence::patch_trigger_mode() {
  while(1){
    bool abrupt_exit = false;
    // Wait for next selected trigger
    co_await delayed_trigger();
    if(last_trigger != current_trigger){ //trigger transition detected
      gen_interrupt_timings();//empty as of today
      interrupts_driven = 0;
    }
    if(interrupts_driven < trigger_interrupt_count_){
       uint8_t num = rng1() % FLAGS_interrupt_trigger_interval ;
       //wait for num cycles before driving next MSI
       for(int i =0; i< num;i++){
         co_await delayed_trigger();
         if(last_trigger != current_trigger){
           abrupt_exit = true;
           break;
         }  
       }
      
       if(!abrupt_exit & drive_msi_in_curr_hart){
         cvm::log(cvm::LOW,"[ExtInterruptSeq] driving external interrupt due to patch_trigger");
         drive_interrupt();
         interrupts_driven++;
       }
    }
  }
}

cvm::messenger::task<void> external_interrupt_sequence::uarch_trigger_mode() {
  while(1){
    co_await delayed_trigger(); // As trigger and capture_info on same event, using a delayed trigger to drive interrupt
    if(drive_msi_in_curr_hart){
      cvm::log(cvm::LOW,"[ExtInterruptSeq] driving external interrupt due to uarch_trigger");
      drive_interrupt();
      interrupts_driven++;
    }
  }
}

cvm::messenger::task<void> external_interrupt_sequence::delayed_trigger() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::triggers::m_event_trigger_delayed_tick<>>(triggers_loc);
  co_return;
}

  // Used to assert/deassert a interrupter interrupt (PIPI) for given hart.
void external_interrupt_sequence::drive_interrupt(){
  unsigned intr_num = 1;
  unsigned intr_file = 0;
  unsigned intr_hart = id_;
  unsigned intr_vs_id = 0;
	unsigned disable_flags = FLAGS_disable_m_imsic_intr |( FLAGS_disable_s_imsic_intr <<1) |( FLAGS_disable_vs_imsic_intr <<2);
  if(disable_flags == 0x7)
	  cvm::log(cvm::ERROR, "[ExtInterruptSeq] Cant generate IMSIC interrupts when all interrupts are disabled \n");

	do{
    intr_file = (rng1() % (3 )) ; //gen iter between 1 to max simul instr
	}while(((1<< intr_file)& disable_flags) != 0);

  intr_num =  (rng1() % (FLAGS_imsic_intr_threshold ));

	if(!FLAGS_disable_vs_imsic_intr)
    intr_vs_id = (rng1() % (FLAGS_imsic_vs_id_threshold )) ; //gen iter between 1 to max simul instr
  if(intr_file == 0x02) intr_num %= FLAGS_imsic_vs_intr_threshold;  
  cvm::log(cvm::LOW,"[ExtInterruptSeq] IMSIC interrupt num: {} interrupt file: {} Interrupt hart:{} hypervisor/supervisor id : {}\n", static_cast<uint32_t>(intr_num), intr_file, intr_hart, intr_vs_id);
   
   uint32_t addr;
   if(intr_file == 0x0){
      addr = msi_m_file_addr + (intr_hart << 18);
   }else if(intr_file == 0x01){
      addr = msi_s_file_addr + (intr_hart << 18);
   }else if(intr_file == 0x02){
      addr = msi_vs_file_addr+ (intr_vs_id << 12) + (intr_hart << 18);
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
     uint8_t currentByte = static_cast<uint8_t>((intr_num >> (8 * i)) & 0xFF);
     data[i] = currentByte;
     strb[i] = 0x1;
   }
   cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
 
  }
