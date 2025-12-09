#include "external_interrupt_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "rv_tester/rv_tester_plusargs.h"
#include "whisper_client.h"
#include "cosim/bridge/bridge_plusargs.h"

REGISTRY_register(external_interrupt_sequence, INTERRUPTS, cvm::registry::all);

static bool validate_interrupt_trigger_rand_delay_min(const char* flagname, const int value) {
  if (value <= 0) {
      cvm::log(cvm::NONE, "Invalid value for +{}={}, must be >= 1, as we currently don't support injecting multiple interrupts in a single cycle\n", flagname, value);
      return false;
  }
  return true;
}
DEFINE_bool(interrupt_trigger_enable, false, "Enable event based external_interrupt_sequence in the sim");
DEFINE_int32(interrupt_trigger_count, 1, "Number of MSI in the sim if random mode enabled");
DEFINE_int32(interrupt_trigger_rand_delay_min, 1, "min TB cycle interval between MSI random mode enabled");
DEFINE_validator(interrupt_trigger_rand_delay_min, &validate_interrupt_trigger_rand_delay_min);
DEFINE_int32(interrupt_trigger_rand_delay_max, 16, "max TB cycle interval between MSI random mode enabled");
DEFINE_int32(interrupt_trigger_initial_delay, 0, "Initial delay after which interrupt trigger starts");
DEFINE_int32(interrupt_trigger_uarch_event_mask, 0, "Bitmask to enable specific uarch event triggers");
DEFINE_string(interrupt_trigger_uarch_event_names, "", "Comma-separated list of uarch event names to trigger interrupts");
DEFINE_string(interrupt_trigger_label, "", "Label to trigger interrupt");
DEFINE_string(interrupt_trigger_pc, "", "Comma-separated list of PC addresses to trigger interrupts");

external_interrupt_sequence::external_interrupt_sequence(cvm::topology::loc_t loc, unsigned id) : loc_(loc), id_(id), scope_(nullptr) {

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });

  axi_mst_loc_l = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST", 0);
  triggers_loc = cvm::topology::get_from_hierarchy("TOP.PLATFORM.TRIGGERS", id_);

  cvm::registry::messenger.connect<rv_tester_transactions::triggers::m_event_trigger_tick<>>(
      triggers_loc,
      [this](const rv_tester_transactions::triggers::m_event_trigger_tick<>& t) { return this->capture_trigger_info(t.event_trigger, t.per_core_evt_vector); });

  cvm::log(cvm::MEDIUM, "external_interrupt_sequence constructor\n");
  // trigger sequence threads`
  if (FLAGS_interrupt_trigger_enable) {
    interrupt_trigger_thread();
  }
}

external_interrupt_sequence::~external_interrupt_sequence() {
  if (FLAGS_metrics){
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"external_interrupts_count\": \"{}\"}}\n", ext_interrupt_count_);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"external_interrupts_count_vgein_triggered\": \"{}\"}}\n", intr_vs_id_vgein_);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"external_interrupts_count_random_triggered\": \"{}\"}}\n", intr_vs_id_random_);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"external_interrupts_count_two_triggered\": \"{}\"}}\n", intr_vs_id_two_);
  }
}

void external_interrupt_sequence::capture_trigger_info(int32_t trigger_info, int32_t per_core_trigger_vlds){
  last_trigger = current_trigger;
  current_trigger = trigger_info;
  drive_msi_in_curr_hart = (per_core_trigger_vlds == (1 << id_));
  cvm::log(cvm::LOW,"[ExtInterruptSeq] capture_trigger_info: hart_id={}, per_core_trigger_vlds=0x{:x}, expected=0x{:x}, drive_msi_in_curr_hart={}\n", 
           id_, per_core_trigger_vlds, (1 << id_), drive_msi_in_curr_hart);
}

void external_interrupt_sequence::interrupt_trigger_thread() {
  auto *task = +[] (external_interrupt_sequence* m) -> cvm::messenger::task<void> {
    co_await m->interrupt_trigger();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> external_interrupt_sequence::interrupt_trigger() {
  while(1){
    // TODO/FIXME use normal trigger instead of delayed trigger
    co_await delayed_trigger(); // As trigger and capture_info on same event, using a delayed trigger to drive interrupt
    cvm::log(cvm::LOW,"[ExtInterruptSeq] drive_msi_in_curr_hart = {}\n", drive_msi_in_curr_hart);
    if(drive_msi_in_curr_hart){
      cvm::log(cvm::LOW,"[ExtInterruptSeq] driving external interrupt due to trigger \n");
      drive_interrupt();
      ext_interrupt_count_++;
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
  unsigned intr_hart = get_logical_core_id(id_);
  unsigned intr_vs_id = 0;
  bool is_vgein_intr = false;
	unsigned disable_flags = FLAGS_disable_m_imsic_intr |( FLAGS_disable_s_imsic_intr <<1) |( FLAGS_disable_vs_imsic_intr <<2);
  if(disable_flags == 0x7)
	  cvm::log(cvm::ERROR, "Error: [ExtInterruptSeq] Cant generate IMSIC interrupts when all interrupts are disabled \n");

	do{
    intr_file = (rng1() % (3 )) ; //gen iter between 1 to max simul instr
	}while(((1<< intr_file)& disable_flags) != 0);

  intr_num =  (rng1() & (FLAGS_imsic_intr_mask ));
  cvm::log(cvm::MEDIUM,"[ExtInterruptSeq] Driving interrupt for Physical hart: {}, intr_num: {}\n", id_, intr_num);

	if(!FLAGS_disable_vs_imsic_intr)
    intr_vs_id = (rng1() % (FLAGS_imsic_vs_id_threshold )) + 1; //gen iter between 1 to GEILEN
  if(intr_file == 0x02) intr_num &= FLAGS_imsic_vs_intr_mask;
  cvm::log(cvm::LOW,"[ExtInterruptSeq] IMSIC interrupt num: {} interrupt file: {} Interrupt logical hart:{} hypervisor/supervisor id : {}\n", static_cast<uint32_t>(intr_num), intr_file, intr_hart, intr_vs_id);

   uint32_t addr;
   if(intr_file == 0x0){
      addr = msi_m_file_addr + (intr_hart << 18);
   }else if(intr_file == 0x01){
      addr = msi_s_file_addr + (intr_hart << 18);
   }else if(intr_file == 0x02){
      cvm::log(cvm::MEDIUM,"[ExtInterruptSeq] Driving VS interrupt for hart: {}, intr_num: {}\n", id_, intr_num);
      uint64_t data_misa;
      uint64_t mask;
      uint64_t poke_mask;
      uint64_t read_mask;
      bool valid;
      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), id_, 0x301, data_misa, mask, poke_mask, read_mask, valid)|| !valid) && FLAGS_whisper_client_check)
        cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek csr : MISA in drive_interrupt()\n", id_);

      if ((data_misa >> 7) & 0x1) { // guest external interrupts based on hstatus.VGEIN
        is_vgein_intr = true;
        uint64_t data;
        uint64_t mask;
        uint64_t poke_mask;
        uint64_t read_mask;
        bool valid;
        if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), id_, 0x600, data, mask, poke_mask, read_mask, valid)|| !valid) && FLAGS_whisper_client_check)
          cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek csr : HSTATUS in drive_interrupt()\n", id_);

        // Extract VGEIN from hstatus (bits 17:12)
        uint32_t vgein = (data >> 12) & 0x3F;

        // Get HGEIE value
        uint64_t hgeie_data;
        if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), id_, 0x607, hgeie_data, mask, poke_mask, read_mask, valid)|| !valid) && FLAGS_whisper_client_check)
          cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek csr : HGEIE in drive_interrupt()\n", id_);

        // 50% chance for single vs dual interrupt
        bool generate_dual_interrupt = (rng1() % 2) == 0;

        if (!generate_dual_interrupt) {
          // Single interrupt case
          if ((rng1() % 100) < 70) {
            // 70% chance to use VGEIN
            intr_vs_id = vgein;
            intr_vs_id_vgein_++;
          } else {
            // 30% chance to use random VS ID != VGEIN
            do {
              intr_vs_id = (rng1() % FLAGS_imsic_vs_id_threshold) + 1; // Range [1,5]
            } while (intr_vs_id == vgein);
            intr_vs_id_random_++;
          }

          // Drive single interrupt
          addr = msi_vs_file_addr + (intr_vs_id << 12) + (intr_hart << 18);
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
        } else {
          // Dual interrupt case
          // First interrupt with VGEIN
          addr = msi_vs_file_addr + (vgein << 12) + (intr_hart << 18);
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

          // Second interrupt with different VS ID
          uint32_t second_vs_id;
          if ((rng1() % 100) < 70) {
            // 70% chance to pick VS ID with HGEIE set
            do {
              second_vs_id = (rng1() % FLAGS_imsic_vs_id_threshold) + 1; // Range [1,5]
            } while ((second_vs_id == vgein) || ((hgeie_data & ~(1ULL<<vgein)) != 0 && !(hgeie_data & (1ULL << second_vs_id))));
            // Chosen VS should not be equal to vgein, and if any HGEIE bits(except the vgein bit) are set, chosen VS should have its HGEIE bit set
          } else {
            // 30% chance to pick VS ID with HGEIE not set
            do {
              second_vs_id = (rng1() % FLAGS_imsic_vs_id_threshold) + 1; // Range [1,5]
            } while ((second_vs_id == vgein) || ((hgeie_data & ~(1ULL<<vgein)) != (0x3E & ~(1ULL<<vgein)) && (hgeie_data & (1ULL << second_vs_id))));
            // Chosen VS should not be equal to vgein, and if any of the HGEIE bits(except the vgein bit) is not set, chosen VS should have its HGEIE bit not set
          }

          // Drive second interrupt
          addr = msi_vs_file_addr + (second_vs_id << 12) + (intr_hart << 18);
          cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
          intr_vs_id_two_++;
        }
      } else {
        is_vgein_intr = false;
        intr_vs_id = (rng1() % FLAGS_imsic_vs_id_threshold) + 1; // Range [1,5]
        addr = msi_vs_file_addr+ (intr_vs_id << 12) + (intr_hart << 18);
      }
   }else{
      cvm::log(cvm::ERROR, "Error:[ExtInterruptSeq] Wrong IMSIC interrupt file specified\n");
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
   if (!is_vgein_intr) {
     cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
   }

  }

uint32_t external_interrupt_sequence::get_logical_core_id(uint32_t physical_hart_id) {
  std::istringstream ss(FLAGS_hart_enable_id);
  std::string token;
  uint32_t logical_id = 0;
  
  // Parse the hart enable ID string and find the logical position
  while (std::getline(ss, token, ',')) {
    uint64_t hart_id = std::stoull(token);
    if (hart_id == physical_hart_id) {
      return logical_id;
    }
    logical_id++;
  }
  
  // Return invalid value if physical hart ID not found
  return 0xFFFFFFFF;
}
