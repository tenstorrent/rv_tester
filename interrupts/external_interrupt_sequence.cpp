#include "external_interrupt_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "rv_tester/rv_tester_plusargs.h"
#include "whisper_client.h"
#include "cosim/bridge/bridge_plusargs.h"
#include "common/device_address_map/device_address_map.h"

REGISTRY_register(external_interrupt_sequence, INTERRUPTS, cvm::registry::all);

static bool validate_interrupt_injection_rand_delay_min(const char* flagname, const int value) {
  if (value <= 0) {
      cvm::log(cvm::NONE, "Invalid value for +{}={}, must be >= 1, as we currently don't support injecting multiple interrupts in a single cycle\n", flagname, value);
      return false;
  }
  return true;
}
DEFINE_bool(interrupt_injection_enable, false, "Enable event based external_interrupt_sequence in the sim");
DEFINE_int32(interrupt_injection_count, 1, "Number of MSI in the sim if random mode enabled");
DEFINE_int32(interrupt_injection_rand_delay_min, 1, "min TB cycle interval between MSI random mode enabled");
DEFINE_validator(interrupt_injection_rand_delay_min, &validate_interrupt_injection_rand_delay_min);
DEFINE_int32(interrupt_injection_rand_delay_max, 16, "max TB cycle interval between MSI random mode enabled");
DEFINE_string(interrupt_injection_initial_delay, "0:0", "Initial delay range (min:max) after which interrupt trigger starts");
DEFINE_int32(interrupt_injection_event_mask, 0, "Bitmask to enable specific uarch event triggers");
DEFINE_string(interrupt_injection_event_names, "", "Comma-separated list of uarch event names to trigger interrupts");
DEFINE_string(interrupt_injection_label, "", "Label to trigger interrupt");
DEFINE_string(interrupt_injection_pc, "", "Comma-separated list of PC addresses to trigger interrupts");

// ---- Tick-based random MSI plusargs (moved from interrupter.cpp) ----
DEFINE_bool(random_imsic_intr, false, "Drive random interrupts");
DEFINE_bool(trickbox_write_enables_intr, false, "Require software write to trickbox address 0x9004040 before random interrupts start");
DEFINE_bool(disable_m_imsic_intr, false, "Drive random imsic interrupts to M file");
DEFINE_bool(disable_s_imsic_intr, false, "Drive random imsic interrupts to S file");
DEFINE_bool(disable_vs_imsic_intr, true, "Drive random imsic interrupts to VS file");
DEFINE_bool(disable_random_hart_imsic_intr, false, "Drive random imsic interrupts to random harts");
DEFINE_int32(imsic_intr_delay_min, 3, "Minimum delay between 2 consecutive interrupts");
DEFINE_int32(imsic_intr_delay_max, 5, "Maximum delay between 2 consecutive interrupts");
DEFINE_uint64(imsic_intr_mask, 0xff, "imsic_intr interrupts mask value");
DEFINE_uint64(imsic_vs_intr_mask, 0x3f, "imsic_vs_intr interrupts mask value");
DEFINE_int32(imsic_vs_id_threshold, 5, "imsic guest file id threshold value");
DEFINE_int32(imsic_hart_threshold, 1, "harts threshold value");
DEFINE_int32(imsic_intr_start_delay, 5000, "delay after which random interrupts should start");
DEFINE_string(imsic_intr_disable_mask, "0x00", "Set bit in hex string to disable random generation of interrupt");
DEFINE_int32(max_intr_count, 0, "Maximum interrupts that can be driven per test");

// ---- Backpressure plusarg ----
DEFINE_uint32(msi_backpressure_threshold, 8, "Minimum free AXI IDs required to proceed with MSI write (0 disables backpressure)");
DEFINE_uint32(msi_backpressure_timeout, 10000, "Timeout for MSI backpressure");

// Logging
DEFINE_bool(enable_external_interrupt_sequence_debug, false, "Enable external_interrupt_sequence debug");

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
external_interrupt_sequence::external_interrupt_sequence(cvm::topology::loc_t loc, unsigned id)
  : sequence_log_file_("h" + std::to_string(id) + "_external_interrupt_sequence.log"), loc_(loc), id_(id), msi_wait_timeout_(0) {

  sysmod_loc_ = cvm::topology::get_from_type("SYSMOD", 0);
  axi_mst_loc_l = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST", 0);
  triggers_loc = cvm::topology::get_from_hierarchy("TOP.PLATFORM.TRIGGERS", id_);
  ncores_ = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;

  cvm::registry::messenger.connect<rv_tester_transactions::triggers::m_event_trigger_tick<>>(
      triggers_loc,
      [this](const rv_tester_transactions::triggers::m_event_trigger_tick<>& t) {
        return this->capture_trigger_info(t.event_trigger, t.per_core_evt_vector);
      });

  // Sysmod tick connection: only hart 0 handles tick-based random MSI
  if (id_ == 0) {
    cvm::registry::messenger.connect<rv_tester_transactions::sysmod::tick<>>(
        sysmod_loc_,
        [this](const rv_tester_transactions::sysmod::tick<>& t) {
          this->on_sysmod_tick(t.advance);
        });

    // Directed MSI from trickbox writes (signaled on sysmod location)
    cvm::registry::messenger.connect<directed_msi_request_t>(
        sysmod_loc_,
        [this](const directed_msi_request_t& req) {
          this->on_directed_msi(req);
        });
  }

  log(cvm::MEDIUM, "EXTERNAL_INTERRUPT_SEQUENCE constructed for hart {}\n", id_);

  if (FLAGS_interrupt_injection_enable) {
    interrupt_injection_thread();
  }

  uint32_t cluster_id = 0;
  msi_m_file_addr = generate_imsic_m_addr(cluster_id, 0);
  msi_s_file_addr = generate_imsic_s_addr(cluster_id, 0);
  msi_vs_file_addr = msi_s_file_addr;

  // Tick-based random MSI init (hart 0 only)
  if (id_ == 0 && FLAGS_random_imsic_intr) {
    log(cvm::MEDIUM, "[ExtInterruptSeq] Enable random IMSIC MSIs\n");
    uint32_t rand_num = (rng1() % 2) + 1;
    if (FLAGS_imsic_intr_delay_min) {
      rand_num = (rng1() % (FLAGS_imsic_intr_delay_max - FLAGS_imsic_intr_delay_min + 1)) + FLAGS_imsic_intr_delay_min;
    }
    timer_ = 0;
    timer_rand_intr_ = timer_ + FLAGS_imsic_intr_start_delay + (rand_num * timer_advance_);
    if (FLAGS_max_intr_count > 0) {
      limit_interrupts_ = 1;
    }
  }
}

// ---------------------------------------------------------------------------
// Destructor -- merged metrics from interrupter and event-triggered path
// ---------------------------------------------------------------------------
external_interrupt_sequence::~external_interrupt_sequence() {
  if (FLAGS_metrics) {
    log(cvm::NONE, "INFO_PASS_METRIC:{{\"external_interrupts_count\": \"{}\"}}\n", ext_interrupt_count_);
    log(cvm::NONE, "INFO_PASS_METRIC:{{\"guest_external_interrupt_count_vgein\": \"{}\"}}\n", intr_vs_id_vgein_);
    log(cvm::NONE, "INFO_PASS_METRIC:{{\"guest_external_interrupt_count_random\": \"{}\"}}\n", intr_vs_id_random_);
    log(cvm::NONE, "INFO_PASS_METRIC:{{\"guest_external_interrupt_count_two\": \"{}\"}}\n", intr_vs_id_two_);
  }
}

void external_interrupt_sequence::capture_trigger_info(int32_t trigger_info, int32_t per_core_trigger_vlds){
  last_trigger = current_trigger;
  current_trigger = trigger_info;
  drive_msi_in_curr_hart = (per_core_trigger_vlds == (1 << id_));
  log(cvm::HIGH, "[ExtInterruptSeq] capture_trigger_info: hart_id={}, per_core_trigger_vlds=0x{:x}, expected=0x{:x}, drive_msi_in_curr_hart={}\n",
           id_, per_core_trigger_vlds, (1 << id_), drive_msi_in_curr_hart);
}

void external_interrupt_sequence::interrupt_injection_thread() {
  auto *task = +[] (external_interrupt_sequence* m) -> cvm::messenger::task<void> {
    co_await m->interrupt_trigger();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> external_interrupt_sequence::interrupt_trigger() {
  while (1) {
    co_await delayed_trigger();
    log(cvm::HIGH, "[ExtInterruptSeq] drive_msi_in_curr_hart = {}\n", drive_msi_in_curr_hart);
    if (drive_msi_in_curr_hart) {
      while (check_axi_backpressure()) co_await delayed_trigger();
      log(cvm::HIGH, "[ExtInterruptSeq] driving external interrupt due to trigger\n");

      // Randomize MSI parameters
      unsigned intr_file = 0;
      unsigned disable_flags = FLAGS_disable_m_imsic_intr | (FLAGS_disable_s_imsic_intr << 1) | (FLAGS_disable_vs_imsic_intr << 2);
      if (disable_flags == 0x7)
        log(cvm::ERROR, "Error: [ExtInterruptSeq] Cant generate IMSIC interrupts when all interrupts are disabled\n");
      do {
        intr_file = rng1() % 3;
      } while (((1 << intr_file) & disable_flags) != 0);

      uint64_t intr_num;
      do {
        intr_num = rng1() & FLAGS_imsic_intr_mask;
        if (intr_file == 0x02) intr_num &= FLAGS_imsic_vs_intr_mask;
      } while (intr_num == 0);

      unsigned intr_hart = get_logical_core_id(id_);
      unsigned intr_vs_id = 0;
      if (!FLAGS_disable_vs_imsic_intr)
        intr_vs_id = (rng1() % FLAGS_imsic_vs_id_threshold) + 1;

      send_msi(intr_num, intr_file, intr_hart, intr_vs_id, false, false);
      ext_interrupt_count_++;
    }
  }
}

cvm::messenger::task<void> external_interrupt_sequence::delayed_trigger() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::triggers::m_event_trigger_delayed_tick<>>(triggers_loc);
  co_return;
}

// ---------------------------------------------------------------------------
// Tick-based random MSI path (hart 0 only)
// ---------------------------------------------------------------------------
void external_interrupt_sequence::on_sysmod_tick(uint64_t advance) {
  timer_ += advance;
  timer_advance_ = advance;
  log(cvm::FULL, "[ExtInterruptSeq] tick {} advance {}\n", timer_, timer_advance_);

  // Gate on trickbox_write_enables_intr: query the interrupter's enable flag via RPC
  bool intr_enable_check = true;
  if (FLAGS_trickbox_write_enables_intr) {
    bool enabled = false;
    if (cvm::registry::messenger.call<interrupter::intr_enable_read_RPC>(sysmod_loc_, enabled))
      intr_enable_check = enabled;
    else
      intr_enable_check = false;
  }

  if (!FLAGS_random_imsic_intr || !intr_enable_check)
    return;
  if (limit_interrupts_ && (intr_count_ > (int)FLAGS_max_intr_count))
    return;
  if (timer_ < timer_rand_intr_)
    return;

  if (check_axi_backpressure()) return;

  // Randomize MSI parameters
  unsigned intr_file = 0;
  unsigned disable_flags = FLAGS_disable_m_imsic_intr | (FLAGS_disable_s_imsic_intr << 1) | (FLAGS_disable_vs_imsic_intr << 2);
  if (disable_flags == 0x7) {
    log(cvm::ERROR, "Error: [ExtInterruptSeq] Cant generate IMSIC interrupts when all interrupts are disabled\n");
    return;
  }
  do {
    intr_file = rng1() % 3;
  } while (((1 << intr_file) & disable_flags) != 0);

  cvm::rand::uniform_dist<uint64_t> rand_intr_num_64b(0, UINT64_MAX);
  uint64_t intr_num = rand_intr_num_64b() & FLAGS_imsic_intr_mask;
  if (!FLAGS_disable_vs_imsic_intr && intr_file == 2)
    intr_num = rand_intr_num_64b() & FLAGS_imsic_vs_intr_mask;

  unsigned intr_hart = 0;
  if (!FLAGS_disable_random_hart_imsic_intr)
    intr_hart = rng1() % FLAGS_imsic_hart_threshold;

  unsigned intr_vs_id = 0;
  if (!FLAGS_disable_vs_imsic_intr)
    intr_vs_id = (rng1() % FLAGS_imsic_vs_id_threshold) + 1;

  bool exp_err_rsp = (intr_hart < FLAGS_num_harts) ? false : true;

  log(cvm::HIGH, "[ExtInterruptSeq] Driving random imsic_intr num={} file={} hart={}\n", intr_num, intr_file, intr_hart);
  send_msi(intr_num, intr_file, intr_hart, intr_vs_id, false, exp_err_rsp);

  if (limit_interrupts_)
    intr_count_++;

  // Schedule next random MSI
  uint32_t rand_num = (rng1() % (FLAGS_imsic_intr_delay_max - FLAGS_imsic_intr_delay_min + 1)) + FLAGS_imsic_intr_delay_min;
  timer_rand_intr_ = timer_ + (rand_num * timer_advance_);
  log(cvm::HIGH, "[ExtInterruptSeq] Next random imsic_intr at {}\n", timer_rand_intr_);
  ext_interrupt_count_++;
}

// ---------------------------------------------------------------------------
// Directed MSI from trickbox writes
// ---------------------------------------------------------------------------
void external_interrupt_sequence::on_directed_msi(const directed_msi_request_t& req) {
  unsigned intr_file = (req.packed_data >> 12) & 0xf;
  unsigned intr_hart = (req.packed_data >> 16) & 0xfff;
  unsigned vs_id     = (req.packed_data >> 28) & 0x3f;
  bool disable_vs_rand = (req.packed_data >> 40) & 0x1;
  bool exp_err_rsp = (intr_hart < FLAGS_num_harts) ? false : true;

  log(cvm::HIGH, "[ExtInterruptSeq] Directed MSI: num={} file={} hart={} vs_id={}\n",
           req.intr_num, intr_file, intr_hart, vs_id);
  send_msi(req.intr_num, intr_file, intr_hart, vs_id, disable_vs_rand, exp_err_rsp);
  ext_interrupt_count_++;
}

// ---------------------------------------------------------------------------
// Unified MSI dispatch -- VGEIN randomization + AXI write construction
// ---------------------------------------------------------------------------
void external_interrupt_sequence::send_msi(uint64_t intr_num, unsigned intr_file,
                                           unsigned intr_hart, unsigned vs_id,
                                           bool disable_vs_id_rand, bool exp_err_rsp) {
  bool is_vgein_intr = false;
  uint32_t addr = 0;

  log(cvm::HIGH, "[ExtInterruptSeq] send_msi: num={} file={} hart={} vs_id={}\n",
           intr_num, intr_file, intr_hart, vs_id);

  if (intr_file == 0x0) {
    addr = msi_m_file_addr + (intr_hart << 18);
  } else if (intr_file == 0x01) {
    addr = msi_s_file_addr + (intr_hart << 18);
  } else if (intr_file == 0x02) {
    log(cvm::MEDIUM, "[ExtInterruptSeq] Driving VS interrupt for hart={}, num={}\n", intr_hart, intr_num);

    // Peek MISA to check H-extension
    uint64_t data_misa, mask, poke_mask, read_mask;
    bool valid;
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(
            cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0),
            intr_hart, 0x301, data_misa, mask, poke_mask, read_mask, valid) || !valid) && FLAGS_whisper_client_check)
      log(cvm::ERROR, "Error: Hart {}: Failed to peek MISA in send_msi()\n", intr_hart);

    if (((data_misa >> 7) & 0x1) && !disable_vs_id_rand) {
      is_vgein_intr = true;

      // Peek HSTATUS for VGEIN
      uint64_t hstatus_data;
      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(
              cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0),
              intr_hart, 0x600, hstatus_data, mask, poke_mask, read_mask, valid) || !valid) && FLAGS_whisper_client_check)
        log(cvm::ERROR, "Error: Hart {}: Failed to peek HSTATUS in send_msi()\n", intr_hart);
      uint32_t vgein = (hstatus_data >> 12) & 0x3F;

      // Peek HGEIE
      uint64_t hgeie_data;
      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(
              cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0),
              intr_hart, 0x607, hgeie_data, mask, poke_mask, read_mask, valid) || !valid) && FLAGS_whisper_client_check)
        log(cvm::ERROR, "Error: Hart {}: Failed to peek HGEIE in send_msi()\n", intr_hart);

      // 50% chance for single vs dual interrupt
      bool generate_dual_interrupt = (rng1() % 2) == 0;

        if (!generate_dual_interrupt) {
          // Single interrupt case
          if ((rng1() % 100) < 70) {
            // 70% chance to use VGEIN
            vs_id = vgein;
            intr_vs_id_vgein_++;
          } else {
            // 30% chance to use random VS ID != VGEIN
            do {
              vs_id = (rng1() % FLAGS_imsic_vs_id_threshold) + 1; // Range [1,5]
            } while (vs_id == vgein);
            intr_vs_id_random_++;
          }

        addr = msi_vs_file_addr + (vs_id << 12) + (intr_hart << 18);
        uint32_t length = 0x40;
        std::vector<uint8_t> data(64, 0);
        std::vector<bool> strb(64, false);
        for (uint8_t i = 0; i < 4; ++i) {
          data[i] = static_cast<uint8_t>((intr_num >> (8 * i)) & 0xFF);
          strb[i] = true;
        }
        cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb, exp_err_rsp});
      } else {
        // Dual interrupt -- first with VGEIN
        addr = msi_vs_file_addr + (vgein << 12) + (intr_hart << 18);
        uint32_t length = 0x40;
        std::vector<uint8_t> data(64, 0);
        std::vector<bool> strb(64, false);
        for (uint8_t i = 0; i < 4; ++i) {
          data[i] = static_cast<uint8_t>((intr_num >> (8 * i)) & 0xFF);
          strb[i] = true;
        }
        cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb, exp_err_rsp});

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

        addr = msi_vs_file_addr + (second_vs_id << 12) + (intr_hart << 18);
        cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb, exp_err_rsp});
        intr_vs_id_two_++;
      }
    } else {
      is_vgein_intr = false;
      if (!disable_vs_id_rand) vs_id = (rng1() % FLAGS_imsic_vs_id_threshold) + 1;
      addr = msi_vs_file_addr + (vs_id << 12) + (intr_hart << 18);
    }
  } else {
    log(cvm::ERROR, "Error: [ExtInterruptSeq] Wrong IMSIC interrupt file specified\n");
    return;
  }

  // Non-VGEIN path: construct and send AXI write
  if (!is_vgein_intr) {
    uint32_t length = 0x40;
    std::vector<uint8_t> data(64, 0);
    std::vector<bool> strb(64, false);
    for (uint8_t i = 0; i < 4; ++i) {
      data[i] = static_cast<uint8_t>((intr_num >> (8 * i)) & 0xFF);
      strb[i] = true;
    }
    cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb, exp_err_rsp});
  }
}

// ---------------------------------------------------------------------------
// Backpressure: query the AXI master for free write IDs
// ---------------------------------------------------------------------------
bool external_interrupt_sequence::check_axi_backpressure() {
  if (FLAGS_msi_backpressure_threshold <= 0)
    return false;
  msi_wait_timeout_++;
  unsigned free_ids = cvm::registry::messenger.call<axi_mst_t::free_aw_ids_rpc>(axi_mst_loc_l);
  if (free_ids <= FLAGS_msi_backpressure_threshold) {
    log(cvm::HIGH, "[ExtInterruptSeq] Backpressure: only {} free AXI IDs (threshold={}), deferring MSI\n",
             free_ids, FLAGS_msi_backpressure_threshold);
    msi_wait_timeout_ = 0;
    return true;
  }
  if (msi_wait_timeout_ >= FLAGS_msi_backpressure_timeout) {
    log(cvm::ERROR, "Error: MSI Backpressure timeout: {} cycles\n", msi_wait_timeout_);
    return false;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------
uint32_t external_interrupt_sequence::get_logical_core_id(uint32_t physical_hart_id) {
  if (ncores_ == 1) {
    return 0;
  }
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
  log(cvm::ERROR, "Error: [ExtInterruptSeq] Physical hart ID {} not found in hart_enable_id list\n", physical_hart_id);
  return 0;
}
