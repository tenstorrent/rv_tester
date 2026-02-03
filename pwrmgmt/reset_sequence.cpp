#include "reset_sequence.hpp"
#include "patch_utils.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "pmu/pmu_plusargs.h"
#include "rv_tester/rv_tester_plusargs.h"
#include "cosim/bridge_if/bridge_params.h"
#include "cosim/utils/general/util.h"
#include <sstream>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <algorithm>
#include "cvm/logger.hpp"

REGISTRY_register(reset_sequence, PWRMGMT, cvm::registry::all);

DEFINE_bool(pwrmgmt, false, "Runtime disable for pwrmgmt");
DEFINE_bool(cpl_core_en, false, "Enable reset, bootup flow via CPL FW");
DEFINE_bool(clc4_nack, false, "Configure CPL FW to give nack for CC4 requests, default FW would always gives ack");
DEFINE_bool(tj_shutdown, false, "Enable CPL SRAM TjShutdown programming...");
DEFINE_uint32(pll_pwrup_timeout, 50, "Number of soc cycles expected for pll power up to complete");
DEFINE_bool(pll_dfs, false, "Enable dfs sequence during cold boot");
DEFINE_uint32(pll_dfs_freq, 1200, "Clock freq for dfs");
DEFINE_uint32(pll_dfs_timeout, 100, "Number of soc cycles expected for pll dfs to complete");
DEFINE_uint32(num_thubs, 4, "Number of temprature hubs");
DEFINE_string(warm_reset, "off", "Enable warm resets in the sim - off/random/trigger");
DEFINE_string(warm_reset_count, "0:4", "Number of warm resets in the sim if random mode enabled");
DEFINE_string(warm_reset_interval, "2000:10000", "TB cycle interval between warm resets in the sim if random mode enabled");
DEFINE_string(warm_reset_trigger_type, "", "Send warm reset on a trigger");
DEFINE_string(warm_reset_trigger_interval, "rand:0:100", "TB cycle interval from trigger to warm reset");
DEFINE_string(warm_reset_sram_hold, "0:0", "Sram hold");
DEFINE_string(warm_reset_debug_hold, "0:1", "Debug hold");
DEFINE_string(warm_reset_critical_hold, "0:1", "Critical hold");
DEFINE_bool(patch_en, false, "Enable instruction patching");
DEFINE_bool(boot_from_smc, true, "Boot cluster from SMC");
DEFINE_bool(tj_max, false, "Program lower TJ Max Threshold");
DEFINE_bool(patch_cpl_filter_dis, false, "Disable programming of inbound and outbound filters in core");
DEFINE_bool(patch_mmr_check, false, "Enable read write checking of patch related registers");
DEFINE_bool(patch_ram_check, false, "Enable read write checking of patch ram region");
DEFINE_bool(patch_cfg_lock, false, "Lock the patch mmrs while boot programming ");
DEFINE_bool(fuse_mmr_check, false, "Check RW and lockability of fuses ");
DEFINE_bool(init_smc_infilters, false, "Enable filter programming for JTAG and Overlay to access SRAM ");
DEFINE_bool(init_smc_cpl_ras_ibf, false, "Enable filter programming for CPL to access RAS MMRs ");
DEFINE_string(patch_ucode_path, "", "Path to hex file containing patch ucode (assembly file with .s extension should be in same directory)");
DEFINE_string(patches, "WFI,SUB,BLT,AMOSWAP", "+patches=<instr1>,<instr2>,<instr3>,<instr4>; default will be picked if not specified ");
DEFINE_string(disable_patches, "AMOSWAP", "+disable_patches=<instr1>,<instr2>,<instr3>,<instr4>; default will be picked if not specified ");
DEFINE_bool(rand_patch, false, "Randomly pick 4 instructions available in the hex file to be patched");
DEFINE_bool(sw_fuse_program_enable, true, "Program the AXI switch fuse during boot");
DEFINE_string(init_csr_resetseq, "", "+init_csr_resetseq=<unit(mc=8,ms=4,fe=2,ls=1)>:<csr_num>:<val>,... ");
DEFINE_string(init_mmr_resetseq, "", "+init_mmr_resetseq=<mmr_addr>:<size(8|4)>:<val>,... ");
DEFINE_string(rmw_csr_resetseq, "", "+rmw_csr_resetseq=<unit(mc=8,ms=4,fe=2,ls=1)>:<csr_num>:<val>:<mask>,... ");
DEFINE_string(rmw_mmr_resetseq, "", "+rmw_mmr_resetseq=<mmr_addr>:<size(8|4)>:<val>:<mask>,... ");
DEFINE_bool(trace_fuse_4B_access, true, "Enable filter programming for JTAG and Overlay to access SRAM ");
DEFINE_bool(fuse_based_clock_gating, true, "Enable clock gating based on fuse programming");
DEFINE_uint32(jtag_drain_cycles, 100, "Number of cycles to drain pending jtag transaction");

extern "C" {
  void pwrmgmt_init();
  void pwrmgmt_cold_reset(uint8_t val);
  void pwrmgmt_warm_reset(uint8_t val);
  void pwrmgmt_reset_hold(uint8_t sram, uint8_t debug, uint8_t critical);
  void pwrmgmt_force_ref_clk(uint8_t val);

  uint8_t pwrmgmt_get_warm_reset_en(const char* mode) {
    return (std::string(mode) != "off");
  }

  uint8_t pwrmgmt_get_pwrmgmt_en_from_plusargs(const char* m) {
    const char* mode = cvm_plusargs_get_string(m);
    if (!mode) {
      cvm::log(cvm::ERROR, "Error: pwrmgmt mode is not set\n");
      assert(false);
      return false;
    }
    return (std::string(mode) != "off");
  }
}

reset_sequence::reset_sequence(cvm::topology::loc_t loc, unsigned) : loc_(loc), scope_(nullptr) {

  // Topology
  axi_loc_[SMC] = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_SMC_MST", 0);
  axi_loc_[OVERLAY] = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST", 0);
  num_cores_ = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });

  // Channels
  r_channel_[SMC] = cvm::registry::messenger.channel<axi::r_t>(axi_loc_[SMC]);
  b_channel_[SMC] = cvm::registry::messenger.channel<axi::b_t>(axi_loc_[SMC]);
  r_channel_[OVERLAY] = cvm::registry::messenger.channel<axi::r_t>(axi_loc_[OVERLAY]);
  b_channel_[OVERLAY] = cvm::registry::messenger.channel<axi::b_t>(axi_loc_[OVERLAY]);

  // Reset count
  cvm::registry::messenger.connect<int>(loc_, [this](int c) { return this->start(c); });

  // ID Widths
  id_width_[SMC] = cvm::topology::attr(axi_loc_[SMC], "ID_WIDTH").second;
  id_width_[OVERLAY] = cvm::topology::attr(axi_loc_[OVERLAY], "ID_WIDTH").second;

  boot_interface = FLAGS_boot_from_smc ? SMC: OVERLAY;
}

void reset_sequence::start(int reset_count) {

  reset_count_ = reset_count;

  cvm::log(cvm::NONE, "[reset_sequence] count = {}\n", reset_count_);
  
  // Sequence threads
  if (reset_count_ < 0)
    cold_reset_sequence_thread();

  if (reset_count_ >= 0)
    warm_reset_sequence_thread();
}

reset_sequence::~reset_sequence() {
   if (FLAGS_metrics)
     cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"pwrmgmt_warm_reset_count\": \"{}\"}}\n", reset_count_);
}

void reset_sequence::cold_reset_sequence_thread() {
  auto *task = +[] (reset_sequence* m) -> cvm::messenger::task<void> {
    co_await m->cold_reset_sequence();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void reset_sequence::warm_reset_sequence_thread() {
  auto *task = +[] (reset_sequence* m) -> cvm::messenger::task<void> {
    co_await m->warm_reset_sequence();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> reset_sequence::cold_reset_sequence() {
  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
    co_await tick();

  // Init values for all pins
  // Assert cold reset
  init();
  //FIXME co_await cold_reset_ack();

  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
    co_await tick();

  // Deassert cold reset
  cold_reset(0);
  co_await cold_reset_ack();

  if (!FLAGS_pwrmgmt) {
    force_ref_clk(0);
    co_return;
  }

  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
    co_await tick();

  // Deassert force_ref_clk
  force_ref_clk(0);
  co_await force_ref_clk_ack();

  // Wait for 32 clock ticks
  for (int i=0; i<32; ++i)
    co_await tick();

  // PLL cold powerup sequence
  if(!FLAGS_cpl_core_en) co_await pll_startup_sequence();

  // PLL dfs sequence
  if (FLAGS_pll_dfs| (FLAGS_clk_profile!=0))
    co_await pll_dfs_sequence();

  co_await cpl_sram_fuse_configuration();
  // Reset controller sequence
  if(FLAGS_cpl_core_en) {
    co_await program_tjshutdown_in_cpl_sram();
    co_await cpl_fw_reset_sequence(COLD);
  } else {
    co_await cpl_reset_sequence(COLD);
  }

  // Wait for next tick
  co_await tick();

  co_return;
}

cvm::messenger::task<void> reset_sequence::warm_reset_sequence() {
  // Assert force_ref_clk
  force_ref_clk(1);

  co_await tick();

  // Assert holds
  reset_hold(
    cvm::rand::get<uint32_t>(FLAGS_warm_reset_sram_hold),
    cvm::rand::get<uint32_t>(FLAGS_warm_reset_debug_hold),
    cvm::rand::get<uint32_t>(FLAGS_warm_reset_critical_hold)
  );

  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
    co_await tick();

  // Assert warm reset
  warm_reset(1);

  int32_t warm_reset_cycles = 16;
  if (FLAGS_jtag_en) {
    warm_reset_cycles += FLAGS_jtag_drain_cycles; // cycles to drain pending jtag transaction
  }
  // Wait for warm_reset_cycles clock ticks
  for (int i=0; i<warm_reset_cycles; ++i)
    co_await tick();

  // Deassert warm reset
  warm_reset(0);

  if (!FLAGS_pwrmgmt) {
    force_ref_clk(0);
    co_return;
  }

  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
    co_await tick();

  // Deassert force_ref_clk
  force_ref_clk(0);

  // Wait for 32 clock ticks
  for (int i=0; i<32; ++i)
    co_await tick();

  // PLL cold powerup sequence
  if(!FLAGS_cpl_core_en) co_await pll_startup_sequence();

  // PLL dfs sequence
  if (FLAGS_pll_dfs| (FLAGS_clk_profile!=0))
    co_await pll_dfs_sequence();

  co_await cpl_sram_fuse_configuration();

  if(FLAGS_cpl_core_en) {
    co_await cpl_fw_reset_sequence(WARM);
  } else {
    co_await cpl_reset_sequence(WARM);
  }

  // Wait for next tick
  co_await tick();

  co_return;
}

cvm::messenger::task<void> reset_sequence::cpl_reset_sequence(rst_t rst_type) {
  co_await release_cpl_reset();

  if (rst_type == COLD){  
    co_await program_fuses();
    if (FLAGS_fuse_mmr_check)
      co_await fuse_mmr_check(WARM);
  }
  if (FLAGS_fuse_mmr_check)
    co_await disabled_mmr_csr_check();
  
  co_await program_thub_max_threshold();

  if(FLAGS_init_smc_infilters) {
    co_await init_smc_filters();
  }
  if(FLAGS_init_smc_cpl_ras_ibf) {
    co_await init_smc_ras_ibf_filters();
  }

  if (FLAGS_patch_en && rst_type == COLD) { 
    co_await program_patch();
  } else if (FLAGS_patch_ram_check) {
    co_await patch_ram_check();
  };
  co_await init_csr();
  co_await rmw_csr();
  co_await init_mmr();
  co_await rmw_mmr();
  co_await program_fe_resetvector();
  co_await program_mtime(rst_type);
  if (FLAGS_enable_ntrace_in_boot) {
    co_await enable_ntrace();
  }
  co_await release_cpl_nofetch();
  co_await tick();
  co_return;
}

cvm::messenger::task<void> reset_sequence::cpl_sram_fuse_configuration() {
  uint64_t fuse =  fuse_val();
  co_await write(cpl_sram_fuse_cfg, SZ_8B, fuse, boot_interface);
  co_await write(cpl_sram_core_reset_vector_cfg, SZ_8B, FLAGS_resetpc, boot_interface);
  co_return;
}

cvm::messenger::task<void> reset_sequence::cpl_fw_reset_sequence(rst_t rst_type) {
  if(FLAGS_clc4_nack) co_await disable_clc4_entry();
  co_await write(cpl_core_reset_csr, SZ_4B, 0xFFFFFFFF, boot_interface);
  co_await wait_reset_release();
  co_await program_thub_max_threshold();

  if(FLAGS_init_smc_infilters) {
    co_await init_smc_filters();
  }
  if(FLAGS_init_smc_cpl_ras_ibf) {
    co_await init_smc_ras_ibf_filters();
  }
  co_await init_csr();
  co_await rmw_csr();
  co_await init_mmr();
  co_await rmw_mmr();
  co_await check_system_config_done();
  co_await program_mtime(rst_type);
  co_await send_start_of_execution_to_cpl();
  co_await wait_nofetch_release();
  co_await tick();
  co_return;
}
/*
 ------------------- FW reference struct -----------------
 struct s_smc_cpl_comm_req {
    uint64_t smc_cpl_req_valid : 1;
    uint64_t smc_cpl_req_read_req : 1;
    uint64_t smc_cpl_req_rsvd : 18;
    uint64_t smc_cpl_req_addr: 8;
    uint64_t smc_cpl_req_thub_id: 4;
    uint64_t smc_cpl_req_data : 32;
}; 
 ------------------- FW reference struct -----------------
*/
cvm::messenger::task<void> reset_sequence::program_tjshutdown_in_cpl_sram() {
  if(FLAGS_tj_shutdown){
    // THUB-0
    cvm::rand::uniform_dist<uint64_t> tj_shutdown_threshold(200, 400);
    uint64_t mmr_data = 0x80000FFF | (tj_shutdown_threshold() << 16);
    uint64_t data = 0x1400001 | (mmr_data << 32);
    co_await write(cpl_sram_thub_config_base, SZ_8B, data, boot_interface);

    if(FLAGS_num_harts > 2){
      // THUB-1
      mmr_data = 0x80000FFF | (tj_shutdown_threshold() << 16);
      data = 0x11400001 | (mmr_data << 32);
      co_await write((cpl_sram_thub_config_base + 8), SZ_8B, data, boot_interface);
    }
    if(FLAGS_num_harts==8) {
      // THUB-2
      mmr_data = 0x80000FFF | (tj_shutdown_threshold() << 16);
      data = 0x21400001 | (mmr_data << 32);
      co_await write((cpl_sram_thub_config_base + 16), SZ_8B, data, boot_interface);
      
      // THUB-3
      mmr_data = 0x80000FFF | (tj_shutdown_threshold() << 16);
      data = 0x31400001 | (mmr_data << 32);
      co_await write((cpl_sram_thub_config_base + 24), SZ_8B, data, boot_interface);
    }
  }
  co_return;
};

cvm::messenger::task<void> reset_sequence::send_start_of_execution_to_cpl() {
  auto data = co_await read(rst_ctl_nofetch, SZ_4B, boot_interface);
  data = data | (1 << rst_ctl_nofetch_clustercorego_idx);
  data = data & (~(1 << rst_ctl_nofetch_cfg_done_idx));
  co_await write(rst_ctl_nofetch, SZ_4B, data, boot_interface);
  co_return;
}

cvm::messenger::task<void> reset_sequence::check_system_config_done() {
  uint32_t count = 0;
  while (true) {
    
    for (int i=0; i<10; ++i)
      co_await tick();

    auto data = co_await read(rst_ctl_nofetch, SZ_4B, boot_interface);
    if (data & (1 << rst_ctl_nofetch_cfg_done_idx))
      break;

    count++;
    if (count > (3000 * FLAGS_num_harts))
      cvm::log(cvm::ERROR, "Error: System check config not done... \n");
  }
  co_return;
}

cvm::messenger::task<void> reset_sequence::pll_startup_sequence() {
  co_await write(pll_control, SZ_4B, (1 << wakeup_req_idx), boot_interface);
  co_await check_pll_status();
  co_await clear_pll_status();

  co_return;
}

cvm::messenger::task<void> reset_sequence::check_pll_status() {
  uint32_t count = 0;
  while (true) {
    co_await tick();
    auto data = co_await read(pll_interrupts, SZ_4B, boot_interface);
    if (data & (1 << cold_powerup_idx))
      break;

    count++;
    if (count > FLAGS_pll_pwrup_timeout)
      cvm::log(cvm::ERROR, "Error: PLL cold power up not done after {} soc clocks\n", FLAGS_pll_pwrup_timeout);
  }
  co_return;
}

cvm::messenger::task<void> reset_sequence::program_mtime(rst_t rst_type) {
  // Writing random non-zero time value to ACLINT Mtime before No-fetch release
  cvm::rand::uniform_dist<uint32_t> aclint_mtime(5000, 10000);
  uint64_t rand_mtime  = aclint_mtime();

  if(rst_type == WARM) {
    cvm::rand::uniform_dist<uint32_t> aclint_mtime(50000, 100000);
    rand_mtime = aclint_mtime();
    co_await write(aclint_mtime_mmr, SZ_8B, rand_mtime);
  }
  else { // COLD Reset
    co_await write(aclint_mtime_mmr, SZ_8B, rand_mtime);
  }
  co_return;
}

cvm::messenger::task<void> reset_sequence::program_fe_resetvector() {
  cvm::log(cvm::NONE, "[pwrmgmt] Programming FE reset vector to 0x{:x}\n", FLAGS_resetpc);
  co_await tick();
  uint32_t i = 0;
  while (i++ < FLAGS_num_harts)
    co_await write(core_resetvector_mmr + ((i-1) * core_fuse_offset), SZ_8B, FLAGS_resetpc);
  co_await tick();
  co_return;
}

cvm::messenger::task<void> reset_sequence::clear_pll_status() {
  co_await tick();
  co_await write(pll_interrupts, SZ_4B, (1 << cold_powerup_idx), boot_interface);
  co_await write(pll_interrupts, SZ_4B, (1 << wakeup_done_idx), boot_interface);
  co_await write(pll_control, SZ_4B, (0 << wakeup_req_idx), boot_interface);

  co_return;
}

cvm::messenger::task<void> reset_sequence::pll_dfs_sequence() {
  if (FLAGS_clk_profile !=0) {
    auto loc = cvm::topology::get_from_type("CLKI", 0);
    FLAGS_pll_dfs_freq = cvm::topology::list_attr(loc, fmt::format("PROFILE{}_CLOCK_FREQ_MHZ",FLAGS_clk_profile)).second[1];
  }
  cvm::log(cvm::MEDIUM, "[pwrmgmt] Core frequency is being changed to {} based on clk_profile {}\n", FLAGS_pll_dfs_freq, FLAGS_clk_profile);
  
  // Calculate PLL parameters using common utility function
  auto params = pll_utils::calculate_pll_params(FLAGS_pll_dfs_freq);
  
#ifdef MOVELLUS_PLL_MODEL
  cvm::log(cvm::MEDIUM, "[pwrmgmt] Using Movellus PLL: fcw_int={}, postdiv={} for {} MHz\n", 
           params.fcw_int, params.postdiv, FLAGS_pll_dfs_freq);
  
  // Configure PLL Parameters1 register (fcw_int)
  auto rdata1 = co_await read(pll_parameters1, SZ_4B, boot_interface);
  auto new_params1 = pll_utils::configure_pll_parameters1(static_cast<uint32_t>(rdata1), params);
  co_await write(pll_parameters1, SZ_4B, new_params1, boot_interface);
  
  // Configure PLL Parameters0 register (postdiv)
  auto rdata0 = co_await read(pll_parameters0, SZ_4B, boot_interface);
  auto new_params0 = pll_utils::configure_pll_parameters0(static_cast<uint32_t>(rdata0), params);
  co_await write(pll_parameters0, SZ_4B, new_params0, boot_interface);
  
#else
  // Generic PLL model
  cvm::log(cvm::MEDIUM, "[pwrmgmt] Using Generic PLL: freq_ratio={} for {} MHz\n", 
           params.freq_ratio, FLAGS_pll_dfs_freq);
  
  auto rdata0 = co_await read(pll_parameters0, SZ_4B, boot_interface);
  auto new_params0 = pll_utils::configure_pll_parameters0(static_cast<uint32_t>(rdata0), params);
  co_await write(pll_parameters0, SZ_4B, new_params0, boot_interface);
#endif
  co_await write(pll_control, SZ_4B, (1 << dfs_req_idx), boot_interface);

  uint32_t count = 0;
  while (true) {
    co_await tick();
    auto data = co_await read(pll_interrupts, SZ_4B, boot_interface);
    if (data & (1 << dfs_done_idx))
      break;

    count++;
    if (count > FLAGS_pll_dfs_timeout)
      cvm::log(cvm::ERROR, "Error: PLL dfs not done after {} soc clocks\n", FLAGS_pll_dfs_timeout);
  }

  co_return;
}

cvm::messenger::task<void> reset_sequence::release_cpl_reset() {
  co_await tick();
  co_await write(rst_ctl_warm, SZ_4B, (1 << cpl_cl_warm_reset_n), boot_interface);

  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
  co_await tick();

  co_await write(rst_ctl_warm, SZ_4B, (1 << cpl_force_ss_to_ref_clock_n | 1 << cpl_cl_warm_reset_n), boot_interface);

  co_return;
}

cvm::messenger::task<void> reset_sequence::wait_reset_release() {
  
  cvm::log(cvm::NONE, "Wait for warm reset release by CPL...\n");
  while (true) {
    // Wait for 16 clock ticks
    for (int i=0; i<50; ++i)
      co_await tick();

    auto data = co_await read(rst_ctl_warm, SZ_4B, boot_interface);
    if ((data & (1 << cpl_force_ss_to_ref_clock_n)) && (data & (1 << cpl_cl_warm_reset_n))) {
      cvm::log(cvm::NONE, "Wait for warm reset release by CPL... {}, {} , {} \n",data,cpl_force_ss_to_ref_clock_n,cpl_cl_warm_reset_n);
      break;
    }
  }

  co_return;
}

cvm::messenger::task<void> reset_sequence::wait_nofetch_release() {
  
  cvm::log(cvm::NONE, "Wait for no-fetch release by CPL...\n");
  while (true) {
    // Wait for 16 clock ticks
    for (int i=0; i<50; ++i)
      co_await tick();

    auto data = co_await read(rst_ctl_nofetch, SZ_4B, boot_interface);
    if ((data & 0xFF) != 0xFF) {
      cvm::log(cvm::NONE, "Wait for no-fetch release by CPL... {} \n",data);
      break;
    }
  }

  co_return;
}

cvm::messenger::task<void> reset_sequence::program_fuses() {
  co_await tick();

  uint64_t fuse = fuse_val();

  uint32_t ncores = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;

  cvm::log(cvm::HIGH, "[pwrmgmt] Programming fuse MMRs\n", trace_fuse_mmr);
  for (uint32_t i = 0; i < ncores; ++i) {
    if (!FLAGS_fuse_based_clock_gating) {
      uint64_t data = co_await read(cr_chicken_bits_mmr + i * core_fuse_offset, SZ_8B, boot_interface);
      co_await write(cr_chicken_bits_mmr + i * core_fuse_offset, SZ_8B, data | !FLAGS_fuse_based_clock_gating, boot_interface);
    }
    co_await write(core_fuse_mmr + i * core_fuse_offset,   SZ_8B, fuse);
  }   
  co_await write(trace_fuse_mmr, SZ_8B, fuse, boot_interface );
  co_await write(aclint_fuse_mmr, SZ_8B, fuse);
  co_await write(dm_fuse_mmr,     SZ_8B, fuse);
  co_await write(sc_fuse_mmr,     SZ_8B, fuse);
  
  if (FLAGS_rand_core_harvest || FLAGS_sw_fuse_program_enable)
    co_await write(sw_fuse_mmr,     SZ_8B, fuse);


  co_return;
}

cvm::messenger::task<void> reset_sequence::disable_clc4_entry() {
  co_await tick();
  auto data = co_await read(cpl_sram_cstate_limit_offset, SZ_4B, boot_interface);
  data = data & 0xFFFFFF00; 
  co_await write(cpl_sram_cstate_limit_offset, SZ_4B, data);
  co_return;
}

cvm::messenger::task<void> reset_sequence::release_cpl_nofetch() {
  co_await tick();
  co_await write(rst_ctl_nofetch, SZ_4B, ((~FLAGS_hart_enable_mask << cpl_cl_no_fetch) & 0xff), boot_interface);

  co_return;
}

cvm::messenger::task<void> reset_sequence::cold_reset_ack() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::pwrmgmt::m_cold_reset_ack<>>(loc_);
  co_return;
}

cvm::messenger::task<void> reset_sequence::force_ref_clk_ack() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::pwrmgmt::m_force_ref_clk_ack<>>(loc_);
  co_return;
}

cvm::messenger::task<void> reset_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::pwrmgmt::m_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> reset_sequence::trigger() {
  co_return;
}

cvm::messenger::task<uint64_t> reset_sequence::read(uint64_t addr, size_t sz, interface_t interface, bool exp_err_rsp /* = false */) {
  assert(sz <= 8);

  cvm::log(cvm::MEDIUM, "[pwrmgmt] {} : read req - addr={:#x}, sz={}, location={}\n", get_intf_name(interface), addr, sz, axi_loc_[interface]);  

  unsigned id;
  if (interface == SMC) {
    if (!cvm::registry::messenger.call<smc_mst_t::push_ar_no_id_rpc>(axi_loc_[interface], axi::a_no_id_t{addr, log2(sz), exp_err_rsp, RESET_SEQ_ID}, id)) {
      auto axi_idalloc_done = co_await check_axi_rresp_timeout(interface, id, addr, sz, exp_err_rsp);
      if(!axi_idalloc_done) {
        co_return 0;
      }
    }
  } else {
    if (!cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_loc_[interface], axi::a_no_id_t{addr, log2(sz), uint8_t(0xF), exp_err_rsp, RESET_SEQ_ID}, id)) {
      auto axi_idalloc_done = co_await check_axi_rresp_timeout(interface, id, addr, sz, exp_err_rsp);
      if (!axi_idalloc_done) {
        co_return 0;
      }
    }
  }
  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(r_channel_[interface], [&id](const auto& r) { return r.id == id; });
  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  uint64_t dword = 0;
  auto data = convert_to_dword_array(resp.data);
  if (interface == SMC) { 
    // FIXME - check why this alignment is needed
    dword = (addr % 8) ? (data[0] >> 32) : data[0];
  } else {
    uint64_t offset = (addr&0x3f)/8; 
    dword = data[offset];
  }
  dword &= mask;
  cvm::log(cvm::MEDIUM, "[pwrmgmt] {} : read resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", get_intf_name(interface), resp.id, addr, sz, data[0], dword, mask);
  co_return dword;
}

cvm::messenger::task<void> reset_sequence::write(uint64_t addr, size_t sz, uint64_t data, interface_t interface, bool exp_err_rsp /* = false */) {
  assert(sz <= 8);

  uint64_t dword = (addr % 8) ? (data << 32) : data;
  auto byte_array_tmp = convert_to_byte_array({dword});
  std::vector<uint8_t> byte_array(64, uint8_t(0));

  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  mask = (addr % 8) ? (mask << 32) : mask;
  std::vector<bool> strb(64, false);

  if (interface == SMC)
  {
    for(int i=0; i < 8; ++i)
    {
      strb[i] = (mask & (0xFFull << (i*8))) != 0;
      byte_array[i] = byte_array_tmp[i];
    }
    byte_array.resize(8);
    strb.resize(8);
  }
  else
  {
    for(int i=0; i < 8; ++i)
    {
      strb[8*((int)((addr & 0x3F) /8)) + i] = (mask & (0xFFull << (i*8))) != 0;
      byte_array[8*((int)((addr & 0x3F)/8)) + i] = byte_array_tmp[i];
    }
  }

  cvm::log(cvm::MEDIUM, "[pwrmgmt] {} : write req - addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", get_intf_name(interface), addr, sz, data, dword, mask);
  
  unsigned id;
  if (interface == SMC)
  {
    if (!cvm::registry::messenger.call<smc_mst_t::push_aw_no_id_rpc>(axi_loc_[interface], axi::a_no_id_t{addr, log2(sz), exp_err_rsp, RESET_SEQ_ID}, id)) {
      auto axi_idalloc_done = co_await check_axi_bresp_timeout(interface, id, addr, sz, exp_err_rsp);
      if (!axi_idalloc_done) {
        co_return;
      }
    }
    cvm::registry::messenger.call<smc_mst_t::push_w_rpc>(axi_loc_[interface], axi::w_t{byte_array, strb, 1});
  }
  else
  {
    if (!cvm::registry::messenger.call<overlay_mst_t::push_aw_no_id_rpc>(axi_loc_[interface], axi::a_no_id_t{addr, log2(sz), uint8_t(0xF), exp_err_rsp, RESET_SEQ_ID}, id)) {
      auto axi_idalloc_done = co_await check_axi_bresp_timeout(interface, id, addr, sz, exp_err_rsp);
      if (!axi_idalloc_done) {
        co_return;
      }
    }
    cvm::registry::messenger.call<overlay_mst_t::push_w_rpc>(axi_loc_[interface], axi::w_t{byte_array, strb, 1});
  }

  auto resp = co_await cvm::registry::messenger.wait<axi::b_t>(b_channel_[interface], [&id](const auto& b) { return b.id == id; });
  cvm::log(cvm::MEDIUM, "[pwrmgmt] {} : write resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", get_intf_name(interface), resp.id, addr, sz, data, dword, mask);
  co_return;
}

cvm::messenger::task<void> reset_sequence::write(uint64_t addr, size_t sz, const std::vector<uint64_t>& data, bool exp_err_rsp /* = false */ ) {
  assert(sz <= 8);

  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  mask = (addr % 8) ? (mask << 32) : mask;
  std::vector<bool> strb(8, false);
  for(int i=0; i<8; ++i)
    strb[i] = (mask & (0xFFull << (i*8))) != 0;

  unsigned id;
  std::vector<unsigned> ids;
  int size = data.size();
  for(int i=0; i < size; i++){
    uint64_t addr_n = addr + i*sz;
    uint64_t dword = (addr_n % 8) ? (data[i] << 32) : data[i];
    auto byte_array = convert_to_byte_array({dword});

    cvm::log(cvm::MEDIUM, "[pwrmgmt] batch write req : {} - addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", i, addr_n, sz, data[i], dword, mask);
    if (!cvm::registry::messenger.call<smc_mst_t::push_aw_no_id_rpc>(axi_loc_[SMC], axi::a_no_id_t{addr_n, log2(sz), exp_err_rsp, RESET_SEQ_ID}, id)) {
      auto axi_idalloc_done = co_await check_axi_bresp_timeout(SMC, id, addr, sz, exp_err_rsp);
      if (!axi_idalloc_done) {
        co_return;
      }
    }
    cvm::registry::messenger.call<smc_mst_t::push_w_rpc>(axi_loc_[SMC], axi::w_t{byte_array, strb, 1});
    ids.push_back(id);
  };

  // Note - simultaneous burst write calls might result in interleaved resposes
  for(int i=0; i < size; i++){
    id = ids[i];
    uint64_t addr_n = addr + i*sz;
    uint64_t dword = (addr_n % 8) ? (data[i] << 32) : data[i];
    auto resp = co_await cvm::registry::messenger.wait<axi::b_t>(b_channel_[SMC], [&id](const auto& b) { return b.id == id; });
    cvm::log(cvm::MEDIUM, "[pwrmgmt] batch write resp : {} - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", i, resp.id, addr_n, sz, data[i], dword, mask);
  };
  co_return;
};

cvm::messenger::task<void> reset_sequence::batch_write(uint64_t addr, size_t sz, const std::vector<uint64_t>& data, bool exp_err_rsp /* = false */ ) {

  size_t batch_size = 1 << (id_width_[SMC] - seqid_width_ - 1);
  size_t batches_count = std::ceil(static_cast<double>(data.size()) / batch_size);
  for (size_t i = 0; i < batches_count; i++) {
    auto batch_start_indx = i * batch_size;
    uint64_t addr_n = addr + (batch_start_indx * sz);
    batch_size = std::min(batch_size, data.size() - batch_start_indx);
    std::vector<uint64_t> batch_data(batch_size);
    std::copy(data.begin() + batch_start_indx, data.begin() + batch_start_indx + batch_size, batch_data.begin());
    co_await write(addr_n, sz, batch_data, exp_err_rsp);
  }

  co_return;
};

cvm::messenger::task<void> reset_sequence::csr_write(uint32_t core_id, uint32_t unit, uint64_t addr, uint64_t data) {
  uint64_t cmd = 0;
  uint32_t offset = core_id * core_fuse_offset;
  cvm::log(cvm::NONE, "[pwrmgmt] csr write req - core_id = {}, addr={:#x}, data={:#x} \n", core_id, addr, data );
  uint64_t wr = 0x1;
  uint64_t en = 0x1;
  cmd = en<<62 | wr << 61 | unit<<12|addr;
  co_await write(core_crCsrDataPort + offset, SZ_8B, data);
  co_await write(core_crCsrCommandPort + offset, SZ_8B, cmd);
  do { 
    cmd = co_await read(core_crCsrCommandPort + offset, SZ_8B);
  } while ((cmd>>63) != 0x0 );
  co_return;
}


cvm::messenger::task<uint64_t> reset_sequence::csr_read(uint32_t core_id, uint32_t unit,uint64_t addr) {
  uint64_t cmd = 0;
  uint32_t offset = core_id * core_fuse_offset;
  cvm::log(cvm::NONE, "[pwrmgmt] csr read req - core_id = {}, addr={:#x} \n", core_id, addr );
  uint64_t wr = 0;
  uint64_t en = 0x1;
  cmd = en <<62 | wr << 61 |unit<<12|addr;
  co_await write(core_crCsrCommandPort + offset, SZ_8B, cmd);
  do { 
    cmd = co_await read(core_crCsrCommandPort + offset, SZ_8B);
  } while ((cmd>>63) != 0x0 );
  cvm::log(cvm::NONE, "[pwrmgmt] cr read res - core_id = {}, addr={:#x} \n", core_id, addr );
  auto data = co_await read(core_crCsrDataPort + offset, SZ_8B);
  co_return data;
}

std::vector<uint64_t> reset_sequence::convert_to_dword_array(const std::vector<uint8_t>& byte_array) {
  std::vector<uint64_t> result(byte_array.size() / sizeof(uint64_t));
  std::copy(reinterpret_cast<const uint64_t*>(byte_array.data()),
            reinterpret_cast<const uint64_t*>(byte_array.data() + byte_array.size()),
            result.begin());
  return result;
}

std::vector<uint8_t> reset_sequence::convert_to_byte_array(const std::vector<uint64_t>& dword_array) {
  std::vector<uint8_t> result(dword_array.size() * sizeof(uint64_t));
  std::copy(reinterpret_cast<const uint8_t*>(dword_array.data()),
            reinterpret_cast<const uint8_t*>(dword_array.data()) + dword_array.size() * sizeof(uint64_t),
            result.begin());
  return result;
}

uint64_t reset_sequence::core_fuse_val() {
  uint64_t core_fuse = 0;
  std::vector<uint64_t> id = mhartid();
  for (uint32_t i=0; i<id.size(); ++i)
    core_fuse |= (((i << 1u) | 1u) << (4 * id[i]));
  core_fuse = core_fuse << core_fuse_idx;
  return core_fuse;
}

uint64_t reset_sequence::core_en(uint32_t c) {
  return static_cast<uint64_t>((FLAGS_hart_enable_mask & (1u << c)) >> c);
}

std::vector<uint64_t> reset_sequence::mhartid() {
  std::vector<uint64_t> core_mhartid {};
  std::istringstream ss(FLAGS_hart_enable_id);
  std::string token;
  while (std::getline(ss, token, ',')) {
    core_mhartid.push_back(std::stoull(token));
  }
  return core_mhartid;
}

uint64_t reset_sequence::trace_fuse_val() {
  return static_cast<uint64_t>(FLAGS_ntrace_enable) << trace_fuse_idx;
}

uint64_t reset_sequence::dm_fuse_val() {
  return static_cast<uint64_t>(FLAGS_debug_enable) << dm_fuse_idx;
}

uint64_t reset_sequence::export_control_fuse_val() {
  return static_cast<uint64_t>(FLAGS_export_control_en) << exp_ctrl_fuse_idx;
}

uint64_t reset_sequence::cla_fuse_val() {
  return static_cast<uint64_t>(FLAGS_cla_enable) << cla_fuse_idx;
}

uint64_t reset_sequence::io_coherency_fuse_val() {
  return static_cast<uint64_t>(FLAGS_io_coherency_disable) << io_cohr_fuse_idx;
}

uint64_t reset_sequence::dst_fuse_val() {
  return static_cast<uint64_t>(FLAGS_dst_enable) << dst_fuse_idx;
}

uint64_t reset_sequence::sc_fuse_val() {
  uint64_t sc_fuse = 0;

  int32_t nways = cvm::topology::attr(cvm::topology::get_from_type("SC", 0), "SC_NUM_WAYS").second;
  for (int i=0; i<nways/4; ++i) {
    uint32_t segment = (~FLAGS_sc_dis_ways_mask >> (4*i)) & 0xf;
    if (segment == 0xf)
      sc_fuse |= (1ull << i);
  }
  return sc_fuse;
}

uint64_t reset_sequence::fuse_val() {
  return core_fuse_val() | trace_fuse_val() | dm_fuse_val() | sc_fuse_val() | export_control_fuse_val() |  cla_fuse_val() | io_coherency_fuse_val() | dst_fuse_val() | (1ull << lock_idx);
}


cvm::messenger::task<void> reset_sequence::program_patch() {
  cvm::log(cvm::HIGH, "[pwrmgmt] ========== Starting Patch Programming ==========\n");
  co_await tick();
  
  // Process patch hex file and corresponding assembly file
  if (!FLAGS_patch_ucode_path.empty()) {
    cvm::log(cvm::HIGH, "[pwrmgmt] Reading and processing patch hex file with corresponding assembly\n");
    PatchUtils::read_hex_and_assembly(FLAGS_patch_ucode_path);
  } else {
    cvm::log(cvm::ERROR, "[pwrmgmt] No patch hex file specified. Use +patch_ucode_path to provide hex file path\n");
    co_return;
  }
  
  // Program the hex data directly with proper byte ordering and zero-skipping
  co_await write_hex_patches_directly();
  
  if (!FLAGS_patch_cpl_filter_dis) { 
    cvm::log(cvm::HIGH, "[pwrmgmt] Programming CPL AXI filters for patch access\n");
    //CPL AXI in filter programming
    co_await write(cpl_in_filter0_addr_l, SZ_8B, 0x4C000);
    co_await write(cpl_in_filter0_addr_h, SZ_8B, 0x4EFFF);
    co_await write(cpl_in_filter0_config, SZ_8B, 0x8000000001010113);
    cvm::log(cvm::HIGH, "[pwrmgmt] CPL AXI in filter: addr_l=0x4C000, addr_h=0x4EFFF, config=0x8000000001010113\n");
    
    //CPL AXI out filter programming
    co_await write(cpl_out_filter0_addr_l, SZ_8B, 0x4C000);
    co_await write(cpl_out_filter0_addr_h, SZ_8B, 0x4EFFF);
    co_await write(cpl_out_filter0_config, SZ_8B, 0x8000000001010113);
    cvm::log(cvm::HIGH, "[pwrmgmt] CPL AXI out filter: addr_l=0x4C000, addr_h=0x4EFFF, config=0x8000000001010113\n");
  } else {
    cvm::log(cvm::HIGH, "[pwrmgmt] CPL filter programming disabled via FLAGS_patch_cpl_filter_dis\n");
  };  

  cvm::log(cvm::HIGH, "[pwrmgmt] Processing patch instruction selection\n");
  std::string token;
  std::vector<std::string> disable_patch_instr = {"patch_prolouge","patch_epilouge"};
  std::istringstream ss1(FLAGS_disable_patches);
  while (std::getline(ss1, token, ',')) {
    disable_patch_instr.push_back(token);
  }
  cvm::log(cvm::HIGH, "[pwrmgmt] Disabled patches: {}\n", FLAGS_disable_patches);
  
  std::vector<std::string> patch_instr = {} ;
  if (FLAGS_rand_patch) {
    cvm::log(cvm::HIGH, "[pwrmgmt] Using random patch selection mode\n");
    for (auto const& [key, value] : PatchUtils::patches) 
      if (std::find(disable_patch_instr.begin(), disable_patch_instr.end(), key) == disable_patch_instr.end())
        patch_instr.push_back(key);
    std::shuffle(std::begin(patch_instr), std::end(patch_instr), cvm::rand::gen);
    cvm::log(cvm::HIGH, "[pwrmgmt] Available patches for random selection: {}\n", patch_instr.size());
  } else {
    cvm::log(cvm::HIGH, "[pwrmgmt] Using specified patch selection: {}\n", FLAGS_patches);
    std::istringstream ss(FLAGS_patches);
    while (std::getline(ss, token, ',')) {
      if (std::find(disable_patch_instr.begin(), disable_patch_instr.end(), token) == disable_patch_instr.end())
        patch_instr.push_back(token);
    }
  }
  cvm::log(cvm::HIGH, "[pwrmgmt] Final patch instruction list size: {}\n", patch_instr.size());

  cvm::log(cvm::HIGH, "[pwrmgmt] Setting up patch configuration registers\n");
  std::unordered_map<uint32_t, uint64_t> patch_cfg;
  patch_cfg[core_pversion_mmr] = rand()%0xFF;
  cvm::log(cvm::HIGH, "[pwrmgmt] Patch version register: 0x{:x}\n", patch_cfg[core_pversion_mmr]);


  cvm::log(cvm::HIGH, "[pwrmgmt] Configuring patch registers for {} instructions\n", patch_instr.size());
  for (int i = 0; i < (int)patch_instr.size(); ++i) { 
    std::string patchTag = patch_instr[i];
    cvm::log(cvm::HIGH, "[pwrmgmt] Patch[{}]: {} - patchMask: 0x{:x}, Opcode: 0x{:x}, enableMask: 0x{:x}\n", 
             i, patchTag, PatchUtils::patches[patchTag].patchMask, PatchUtils::patches[patchTag].patchInstruction, PatchUtils::patches[patchTag].enableMask);
    patch_cfg[core_preg0_mmr+(i*8)] = ((uint64_t)PatchUtils::patches[patchTag].patchMask<<32 | PatchUtils::patches[patchTag].patchInstruction); 
    pcontrol_data =  pcontrol_data | (((uint64_t)PatchUtils::patches[patchTag].enableMask | 1) << i*16); // enable patch 
    cvm::log(cvm::HIGH, "[pwrmgmt] Patch register[{}] addr: 0x{:x}, data: 0x{:x}\n", i, core_preg0_mmr+(i*8), patch_cfg[core_preg0_mmr+(i*8)]);
    if (i == 3) break;
  }

  if (FLAGS_patch_cfg_lock) {
    pcontrol_data = pcontrol_data | 0x8000800080008000;
    cvm::log(cvm::HIGH, "[pwrmgmt] Patch configuration lock enabled\n");
  }
  cvm::log(cvm::HIGH, "[pwrmgmt] Final pcontrol_data: 0x{:x}\n", pcontrol_data);

  cvm::log(cvm::HIGH, "[pwrmgmt] Programming patch registers to {} cores\n", FLAGS_num_harts);
  for (uint32_t i=0; i< FLAGS_num_harts; i++) {
    uint32_t offset = i * core_fuse_offset;
    cvm::log(cvm::HIGH, "[pwrmgmt] Programming core[{}] with offset: 0x{:x}\n", i, offset);
    for (auto j : patch_cfg) {
      co_await write(j.first + offset, SZ_8B, j.second);
      cvm::log(cvm::HIGH, "[pwrmgmt] Core[{}]: wrote 0x{:x} to addr 0x{:x}\n", i, j.second, j.first + offset);
    }
    //co_await csr_write(i, core_ptvec_csr , 0x4210C000); FIXME : Enable CSR write once its fixed
    co_await write(core_pcontrol_mmr + offset, SZ_8B, pcontrol_data);
    cvm::log(cvm::HIGH, "[pwrmgmt] Core[{}]: enabled patches via pcontrol at 0x{:x} = 0x{:x}\n", i, core_pcontrol_mmr + offset, pcontrol_data);
  };

  if (FLAGS_patch_mmr_check) {
    patch_cfg[core_pcontrol_mmr] = pcontrol_data;
    uint64_t data;
    int random_cid = rand()%FLAGS_num_harts;
    uint32_t offset = random_cid * core_fuse_offset;
    for (auto i : patch_cfg) { 
      if (i.first != core_pversion_mmr && FLAGS_patch_cfg_lock)
        co_await write(i.first + offset, SZ_8B, rand()%0xFFFFFFFFFFFFFFFF); //Writes to check lock-ability
    };
    for (auto i : patch_cfg) {
      data = co_await read(i.first + offset, SZ_8B);
      if (data != i.second)
        cvm::log(cvm::ERROR, "Error: [pwrmgmt] patch registers check addr 0x{:x} ,  Expected :0x{:x}, Actual : 0x{:x} \n", i.first, i.second, data );
      else
        cvm::log(cvm::NONE, "[pwrmgmt]  patch registers check : addr 0x{:x} , data 0x{:x} \n", i.first, data );
    };
  };

  co_return;
};

cvm::messenger::task<void> reset_sequence::write_thub_reg(uint8_t addr, uint32_t data, uint8_t satellite_num, uint8_t mbox_num) {

    uint64_t w_data;
    uint8_t pkt_type = 0x8;
    uint8_t ext_pkt = 0x3;
    uint8_t int_addr;
    
    // Divide address by 4
    int_addr = addr >> 2;
    w_data = ((uint64_t)int_addr << 28) | ((0xF << 24) | (data >> 8 ) );
    co_await tick();
    // Write to REGDATA
    co_await write(pm_mbox_regdata, SZ_8B, w_data);

    w_data = 0;
    w_data |= (0 << 26) | (1 << 23) | (ext_pkt << 20) | (mbox_num << 16) | ((data & 0xff)<<8) | (satellite_num << 4) | pkt_type;
    co_await write(pm_mbox_reg, SZ_8B, w_data);
}

cvm::messenger::task<void> reset_sequence::program_thub_max_threshold() {
  co_await tick();
  if(FLAGS_tj_max){
      switch (num_cores_-1) {
          case 0:
              co_await write_thub_reg(thub_threhold_param_reg,0x8FFF0400,12,0);   // PMNW ID for THUB-0 
              break;
          case 1:
              co_await write_thub_reg(thub_threhold_param_reg,0x8FFF0400,12,0);   // PMNW ID for THUB-0 
              break;
          case 2:
              co_await write_thub_reg(thub_threhold_param_reg,0x8FFF0400,12,0);   // PMNW ID for THUB-0 
              co_await write_thub_reg(thub_threhold_param_reg,0x8FFF0400,11,1);   // PMNW ID for THUB-1 
              break;
          case 7:
              co_await write_thub_reg(thub_threhold_param_reg,0x8FFF0400,9,0);   // PMNW ID for THUB-0 
              co_await write_thub_reg(thub_threhold_param_reg,0x8FFF0400,10,1);   // PMNW ID for THUB-1 
              co_await write_thub_reg(thub_threhold_param_reg,0x8FFF0400,11,2);   // PMNW ID for THUB-2 
              co_await write_thub_reg(thub_threhold_param_reg,0x8FFF0400,12,3);   // PMNW ID for THUB-3 
              break;
          default:
              cvm::log(cvm::ERROR, "Error: [tj_max] Invalid NHARTS seen.. {} .... \n",num_cores_);
      }
  }
  co_return;

 
};



cvm::messenger::task<void> reset_sequence::patch_ram_check() {
  uint64_t actual_data, exp_data;
  uint32_t addr;
  co_await tick();
  for( int i = 0; i<20;i++ ){
    addr = cpl_patch_ram_base + (rand()%512)*8;
    actual_data = co_await read(addr, SZ_8B);
    if (PatchUtils::patch_ram.find(addr) == PatchUtils::patch_ram.end())
      exp_data = 0;
    else
      exp_data = PatchUtils::patch_ram[addr];
  if (exp_data != actual_data)
      cvm::log(cvm::ERROR, "Error: [pwrmgmt] patch ram check addr 0x{:x} ,  Expected :0x{:x}, Actual : 0x{:x} \n", addr, exp_data, actual_data );
    else
      cvm::log(cvm::NONE, "[pwrmgmt]  patch ram check : addr 0x{:x} , data 0x{:x} \n", addr, actual_data );
  };  
  co_return;
};

cvm::messenger::task<void> reset_sequence::init_smc_filters() {

  co_await tick();

  //CPL AXI in filter programming
  co_await write(cpl_in_filter1_addr_l ,SZ_8B , 0x41000, boot_interface);
  co_await write(cpl_in_filter1_addr_h ,SZ_8B , 0x41FFF, boot_interface);
  co_await write(cpl_in_filter1_config ,SZ_8B , 0x8000000000010113, boot_interface);
  co_await write(cpl_in_filter2_addr_l ,SZ_8B , 0x42000, boot_interface);
  co_await write(cpl_in_filter2_addr_h ,SZ_8B , 0x42FFF, boot_interface);
  co_await write(cpl_in_filter2_config ,SZ_8B , 0x8000000000020113, boot_interface);
  co_await write(cpl_in_filter3_addr_l ,SZ_8B , 0x41000, boot_interface);
  co_await write(cpl_in_filter3_addr_h ,SZ_8B , 0x4EFFF, boot_interface);
  co_await write(cpl_in_filter3_config ,SZ_8B , 0x8000000000030113, boot_interface);

  // CPL SRAM infilter (SRC-ID) programming for Overlay transactions
  // CPL AXI in filter programming:- With SRC-ID = MMODE_ID (0xC)
  co_await write(cpl_in_filter4_addr_l, SZ_8B, 0x40000, boot_interface);
  co_await write(cpl_in_filter4_addr_h, SZ_8B, 0x4FFFF, boot_interface);
  co_await write(cpl_in_filter4_config, SZ_8B, 0x80000000000C0113, boot_interface);
  // CPL AXI in filter programming:- With SRC-ID = SEP_ID (0xF)
  co_await write(cpl_in_filter5_addr_l, SZ_8B, 0x40000, boot_interface);
  co_await write(cpl_in_filter5_addr_h, SZ_8B, 0x4FFFF, boot_interface);
  co_await write(cpl_in_filter5_config, SZ_8B, 0x80000000000F0113, boot_interface);

  co_return;
};

cvm::messenger::task<void> reset_sequence::init_smc_ras_ibf_filters() {

  co_await tick();
  // CPL AXI in filter programming for accessing RAS CPL MMRs
  co_await write(cpl_in_filter6_addr_l, SZ_8B, 0x3A000, boot_interface);
  co_await write(cpl_in_filter6_addr_h, SZ_8B, 0x3A0E0, boot_interface);
  co_await write(cpl_in_filter6_config, SZ_8B, 0x8000000000003113, boot_interface);

  co_return;
};

cvm::messenger::task<void> reset_sequence::fuse_mmr_check(rst_t rst_type) {
  co_await tick();
  cvm::log(cvm::MEDIUM, "[pwrmgmt]  Fuse MMR check at {} reset\n", rst_type );

  uint64_t fuse =  fuse_val();
  uint32_t ncores = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  //if (rst_type == COLD)
  //  fuse = fuse_val();
  //else
  //  fuse = co_await read(dm_fuse_mmr, SZ_8B, boot_interface);
  std::vector<uint64_t> registers = { 
    sw_fuse_mmr,
    //trace_fuse_mmr,
    aclint_fuse_mmr,
    dm_fuse_mmr,
    sc_fuse_mmr,
    dst_control_mmr,
    trace_control_mmr,
    core_cla_ctrl_status_mmr
  };

  for (uint32_t i=0; i<ncores; ++i)
    registers.push_back(core_fuse_mmr + i * core_fuse_offset);
  uint64_t actual_data, exp_data;
  for (auto addr : registers) {
    bool exp_err_rsp = false;
    exp_err_rsp = (addr == dst_control_mmr) ? !FLAGS_dst_enable : exp_err_rsp;
    exp_err_rsp = (addr == trace_control_mmr) ? !FLAGS_ntrace_enable : exp_err_rsp;
    exp_err_rsp = (addr == core_cla_ctrl_status_mmr) ? !FLAGS_cla_enable : exp_err_rsp;
    exp_err_rsp = (addr >= (core_fuse_mmr + FLAGS_num_harts * core_fuse_offset) && addr < (core_fuse_mmr + ncores * core_fuse_offset)) ? true : exp_err_rsp;
    actual_data = co_await read(addr, SZ_8B, boot_interface, exp_err_rsp);
    bool ignore_check = (addr==dst_control_mmr) || (addr==trace_control_mmr) || (addr==core_cla_ctrl_status_mmr);
    if (!exp_err_rsp && !ignore_check) {
      exp_data = (addr == sw_fuse_mmr)? ((rst_type == COLD) ? sw_fuse_default_val: fuse): fuse;
      if ((exp_data != actual_data))
        cvm::log(cvm::ERROR, "Error: [pwrmgmt] Fuse reg read check addr 0x{:x} ,  Expected :0x{:x}, Actual : 0x{:x} \n", addr, exp_data, actual_data );
      else
        cvm::log(cvm::NONE, "[pwrmgmt]  Fuse reg read check : addr 0x{:x} , data 0x{:x} \n", addr, actual_data );
    }
  };
  std::vector<uint64_t> fuse_registers = { 
    sw_fuse_mmr,
    trace_fuse_mmr,
    aclint_fuse_mmr,
    dm_fuse_mmr,
    sc_fuse_mmr,
  };
  for (uint32_t i=0; i<ncores; ++i)
    fuse_registers.push_back(core_fuse_mmr + i * core_fuse_offset);  
  for (auto addr : fuse_registers) {
    bool exp_err_rsp = false;
    exp_err_rsp = (addr == dst_control_mmr) ? !FLAGS_dst_enable : exp_err_rsp;
    exp_err_rsp = (addr == trace_control_mmr) ? !FLAGS_ntrace_enable : exp_err_rsp;
    exp_err_rsp = (addr == core_cla_ctrl_status_mmr) ? !FLAGS_cla_enable : exp_err_rsp;
    exp_err_rsp = (addr >= (core_fuse_mmr + FLAGS_num_harts * core_fuse_offset) && addr < (core_fuse_mmr + ncores * core_fuse_offset)) ? true : exp_err_rsp;
    co_await write(addr, SZ_8B, rand()%0xFFFF'FFFF'FFFF'FFFF, boot_interface, exp_err_rsp);
    if (!exp_err_rsp) {
      actual_data = co_await read(addr, SZ_8B, boot_interface);
      exp_data = (addr == sw_fuse_mmr)? ((rst_type == COLD) ? sw_fuse_default_val: fuse): fuse;
      if (exp_data != actual_data)
        cvm::log(cvm::ERROR, "Error:[pwrmgmt] Fuse reg lock check addr 0x{:x} ,  Expected :0x{:x}, Actual : 0x{:x} \n", addr, exp_data, actual_data );
      else
        cvm::log(cvm::MEDIUM, "[pwrmgmt]  Fuse reg lock check : addr 0x{:x} , data 0x{:x} \n", addr, actual_data );
    } 
  };
  if (rst_type == COLD)
    co_return;

  std::vector<uint64_t> id = mhartid();
  uint64_t physical_id = 0;
  std::vector<interface_t> interfaces = {SMC, OVERLAY};
  for ( auto interface : interfaces) {
    for (uint32_t i=0; i<FLAGS_num_harts; ++i) { 
        physical_id = co_await read(core_physical_id_mmr + i * core_fuse_offset, SZ_8B, interface);
        if (id[i] != (physical_id & 0x7))
          cvm::log(cvm::ERROR, "Error: [pwrmgmt] Core ID to Virtual ID mapping Virtual id 0x{:x} ,  Expected Core ID :0x{:x}, Actual Core ID : 0x{:x} \n", i, id[i], (physical_id & 0x7) );
        else
          cvm::log(cvm::NONE, "[pwrmgmt]  Core ID to Virtual ID mapping is correct : Virtual id 0x{:x} , Core ID : 0x{:x}  \n", i, id[i] );
    };
  }
  
  co_return;
};


cvm::messenger::task<void> reset_sequence::disabled_mmr_csr_check() {
  co_await tick();
  uint32_t ncores = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  std::vector<interface_t> interfaces = {SMC, OVERLAY};

  for ( auto interface : interfaces) {
    cvm::log(cvm::MEDIUM, "[pwrmgmt]  Disabled MMR check from {} interface  \n", get_intf_name(interface) );
    bool exp_err_rsp = false;
    for (uint32_t i = 0; i < ncores; ++i) {
      exp_err_rsp = i >= FLAGS_num_harts;
      mmr_read_write_check(cr_scratchpad + i * core_fuse_offset, interface, exp_err_rsp);
      co_await read(core_fuse_mmr + i * core_fuse_offset,   SZ_8B, interface, exp_err_rsp);
    }

    exp_err_rsp = !FLAGS_dst_enable;
    mmr_read_write_check(tr_scratchpad, interface, exp_err_rsp);

    exp_err_rsp = FLAGS_debug_enable<=1;
    mmr_read_write_check(dm_scratchpad, interface, exp_err_rsp);
  }

  co_return;
};

cvm::messenger::task<void> reset_sequence::mmr_read_write_check(uint64_t addr, interface_t interface, bool exp_err_rsp) {
  uint64_t data, wr_data;
  data = co_await read(addr, SZ_8B, interface, exp_err_rsp);
  wr_data = (data & 0xFFFF'FFFF'FFFF'FF00) | rand()%0xFF;
  co_await write(addr, SZ_8B, wr_data, interface, exp_err_rsp);
  data = co_await read(addr, SZ_8B, interface, exp_err_rsp);
  if ((wr_data!=data) & !exp_err_rsp)
    cvm::log(cvm::ERROR, "[pwrmgmt] Error: MMR read to addr 0x{:x} failed.  Expected :0x{:x}, Actual : 0x{:x} \n", addr, wr_data, data );
  else
    cvm::log(cvm::NONE, "[pwrmgmt]  MMR read to addr 0x{:x}, data 0x{:x}\n", addr, data );
};

//-----------------------------------------------------------------------------------------------
// Drivers
//-----------------------------------------------------------------------------------------------
void reset_sequence::init() {
  cvm::registry::callbacks.push(
    scope_,
    []() {
      cvm::log(cvm::MEDIUM, "[pwrmgmt] assert cold_reset, force_ref_clk\n");
      pwrmgmt_init();
    });
}

void reset_sequence::cold_reset(uint8_t assert) {
  cvm::registry::callbacks.push(
    scope_,
    [assert]() {
      cvm::log(cvm::MEDIUM, "[pwrmgmt] {} cold reset\n", assert ? "assert" : "deassert");
      pwrmgmt_cold_reset(assert);
    });
}

void reset_sequence::warm_reset(uint8_t assert) {
  cvm::registry::callbacks.push(
    scope_,
    [assert]() {
      cvm::log(cvm::MEDIUM, "[pwrmgmt] {} warm reset\n", assert ? "assert" : "deassert");
      pwrmgmt_warm_reset(assert);
    });
};

void reset_sequence::reset_hold(uint8_t sram, uint8_t debug, uint8_t critical) {
  cvm::registry::callbacks.push(
    scope_,
    [sram,debug,critical]() {
      cvm::log(cvm::NONE, "[pwrmgmt] reset holds [sram={}, debug={}, critical={}]\n", sram, debug, critical);
      pwrmgmt_reset_hold(sram, debug, critical);
    });
}

void reset_sequence::force_ref_clk(uint8_t assert) {
  cvm::registry::callbacks.push(
    scope_,
    [assert]() {
      cvm::log(cvm::MEDIUM, "[pwrmgmt] {} force_ref_clk\n", assert ? "assert" : "deassert");
      pwrmgmt_force_ref_clk(assert);
    });
}

// Helper function to trim whitespace from a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return ""; 
    }
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

std::string reset_sequence::get_intf_name(interface_t value) {
  switch (value) {
      case interface_t::SMC: return "SMC";
      case interface_t::OVERLAY: return "OVERLAY";
      default: return "Unknown"; 
  }
}








cvm::messenger::task<void> reset_sequence::write_hex_patches_directly() {
    cvm::log(cvm::HIGH, "[pwrmgmt] ========== Direct Hex Patch Programming ==========\n");
    
    if (PatchUtils::hex_address_data.empty()) {
        cvm::log(cvm::ERROR, "[pwrmgmt] No hex data available for patch programming\n");
        co_return;
    }
    
    // Sort addresses for sequential programming
    std::vector<std::pair<uint64_t, uint32_t>> sorted_hex_data;
    for (const auto& pair : PatchUtils::hex_address_data) {
        sorted_hex_data.push_back(pair);
    }
    std::sort(sorted_hex_data.begin(), sorted_hex_data.end());
    
    // Group consecutive addresses for efficient batch writing
    std::vector<uint32_t> current_batch;
    uint64_t batch_start_addr = 0;
    uint64_t expected_addr = 0;
    
    for (size_t i = 0; i < sorted_hex_data.size(); ++i) {
        uint64_t offset = sorted_hex_data[i].first;  // This is now an offset from hex parsing
        uint64_t addr = cpl_patch_ram_base + offset; // Apply base address for programming
        uint32_t data = sorted_hex_data[i].second;
        
        // Start new batch or continue current batch
        if (current_batch.empty() || addr == expected_addr) {
            if (current_batch.empty()) {
                batch_start_addr = addr;
            }
            current_batch.push_back(data);
            expected_addr = addr + 4;
        } else {
            // Write current batch and start new one
            if (!current_batch.empty()) {
                co_await write_hex_batch(batch_start_addr, current_batch);
                current_batch.clear();
            }
            
            // Start new batch
            batch_start_addr = addr;
            current_batch.push_back(data);
            expected_addr = addr + 4;
        }
    }
    
    // Write final batch
    if (!current_batch.empty()) {
        co_await write_hex_batch(batch_start_addr, current_batch);
    }
    
    cvm::log(cvm::HIGH, "[pwrmgmt] ========== Direct Hex Patch Programming Complete ==========\n");
    cvm::log(cvm::HIGH, "[pwrmgmt] Total hex entries programmed: {}\n", PatchUtils::hex_address_data.size());
    co_return;
}

cvm::messenger::task<void> reset_sequence::write_hex_batch(uint64_t start_addr, const std::vector<uint32_t>& data) {
    if (data.empty()) {
        co_return;
    }
    std::vector<uint64_t> write_data = PatchUtils::concatenate_uint32_to_uint64(data);
    co_await batch_write(start_addr, SZ_8B, write_data);
    // Store for RAM checking if enabled
    if (FLAGS_patch_ram_check) {
        PatchUtils::populate_patch_ram(start_addr, write_data);
    }
    co_return;
}

cvm::messenger::task<void> reset_sequence::init_mmr()
{
  if (FLAGS_init_mmr_resetseq == "")
      co_return;

  cvm::log(cvm::HIGH, "Backdoor writes to MMRs\n");
  try { // parse and process the +init_mmr_resetseq and report any errors
    std::string delimiter = ",";
    std::vector<std::string> mmr_num_val = cosim_util::split_string(FLAGS_init_mmr_resetseq, delimiter);
    for (const auto& entry : mmr_num_val) {
      delimiter = ":";
      std::vector<std::string> num_val = cosim_util::split_string(entry, delimiter);
      auto mmr_addr = std::stoull(num_val.at(0), nullptr, 0);
      size_t size = (size_t)std::stoull(num_val.at(1), nullptr, 0);
      auto mmr_value = std::stoull(num_val.at(2), nullptr, 0);
      cvm::log(cvm::HIGH, "MMR addr {} = {} ({} bytes)\n", mmr_addr, mmr_value, size);
      for (uint32_t i = 0; i < FLAGS_num_harts; i++) {
        co_await write(mmr_addr, size, mmr_value);
      }
    }
  }
  catch (...) {
    cvm::log(cvm::ERROR, "Error: unable to parse +init_mmr_resetseq={}\n", FLAGS_init_mmr_resetseq);
    co_return;
  }

  co_return;
}

cvm::messenger::task<void> reset_sequence::rmw_mmr()
{
  if (FLAGS_rmw_mmr_resetseq == "")
      co_return;

  cvm::log(cvm::HIGH, "Backdoor RMW to MMRs\n");

  try { // parse and process the +rmw_mmr_resetseq and report any errors
    std::string delimiter = ",";
    std::vector<std::string> mmr_num_val = cosim_util::split_string(FLAGS_rmw_mmr_resetseq, delimiter);
    for (const auto& entry : mmr_num_val) {
      delimiter = ":";
      std::vector<std::string> num_val = cosim_util::split_string(entry, delimiter);
      auto mmr_addr = std::stoull(num_val.at(0), nullptr, 0);
      size_t size = (size_t)std::stoull(num_val.at(1), nullptr, 0);
      auto mmr_value = std::stoull(num_val.at(2), nullptr, 0);
      auto mmr_mask = std::stoull(num_val.at(3), nullptr, 0);
      cvm::log(cvm::HIGH, "MMR addr {} = {}:{} ({} bytes)\n", mmr_addr, mmr_value, mmr_mask, size);
      for (uint32_t i = 0; i < FLAGS_num_harts; i++) {
        auto v = (co_await read(mmr_addr, size) & ~mmr_mask) | mmr_value;
        co_await write(mmr_addr, size, v);
      }
    }
  }
  catch (...) {
    cvm::log(cvm::ERROR, "Error: unable to parse +rmw_mmr_resetseq={}\n", FLAGS_rmw_mmr_resetseq);
    co_return;
  }

  co_return;
}

cvm::messenger::task<void> reset_sequence::init_csr()
{
  if (FLAGS_init_csr_resetseq == "")
      co_return;

  std::map<std::string, uint64_t> csr_name_to_addr_map;
  for (const auto& csr : csrs) {
    csr_name_to_addr_map[csr.second.name] = csr.second.addr;
  }

  cvm::log(cvm::HIGH, "Backdoor writes to CSRs\n");
  try { // parse and process the +init_csr_resetseq and report any errors
    std::string delimiter = ",";
    std::vector<std::string> csr_num_val = cosim_util::split_string(FLAGS_init_csr_resetseq, delimiter);
    for (const auto& entry : csr_num_val) {
      delimiter = ":";
      std::vector<std::string> num_val = cosim_util::split_string(entry, delimiter);
      auto unit = std::stoull(num_val.at(0), nullptr, 0);
      auto csr_value = std::stoull(num_val.at(2), nullptr, 0);
      uint64_t csr_addr;
      auto csr = num_val.at(1); // expect both csr address("0x301") as well as string("misa")
      if (csr_name_to_addr_map.count(csr)) {
        csr_addr = csr_name_to_addr_map[csr];
      } else {
        char* p;
        uint64_t csrn = std::strtoul(csr.c_str(), &p, 0);
        if (*p == 0)
          csr_addr = csrn;
        else {
          cvm::log(cvm::ERROR, "Error: csr_name:{} undefined see +init_csr_resetseq switch\n", csr);
          co_return;
        }
      }
      cvm::log(cvm::HIGH, "Unit = {}: CSR addr {} = {}\n", unit, csr_addr, csr_value);
      for (uint32_t i = 0; i < FLAGS_num_harts; i++) {
        co_await csr_write(i, (uint32_t)unit, csr_addr, csr_value);
      }
    }
  }
  catch (...) {
    cvm::log(cvm::ERROR, "Error: unable to parse +init_csr_resetseq={}\n", FLAGS_init_csr_resetseq);
    co_return;
  }

  co_return;
}

cvm::messenger::task<void> reset_sequence::rmw_csr()
{
  if (FLAGS_rmw_csr_resetseq == "")
      co_return;

  std::map<std::string, uint64_t> csr_name_to_addr_map;
  for (const auto& csr : csrs) {
    csr_name_to_addr_map[csr.second.name] = csr.second.addr;
  }

  cvm::log(cvm::HIGH, "Backdoor RMW to CSRs\n");
  try { // parse and process the +rmw_csr_resetseq and report any errors
    std::string delimiter = ",";
    std::vector<std::string> csr_num_val = cosim_util::split_string(FLAGS_rmw_csr_resetseq, delimiter);
    for (const auto& entry : csr_num_val) {
      delimiter = ":";
      std::vector<std::string> num_val = cosim_util::split_string(entry, delimiter);
      auto unit = std::stoull(num_val.at(0), nullptr, 0);
      auto csr_value = std::stoull(num_val.at(2), nullptr, 0);
      auto csr_mask = std::stoull(num_val.at(3), nullptr, 0);
      uint64_t csr_addr;
      auto csr = num_val.at(1); // expect both csr address("0x301") as well as string("misa")
      if (csr_name_to_addr_map.count(csr)) {
        csr_addr = csr_name_to_addr_map[csr];
      } else {
        char* p;
        uint64_t csrn = std::strtoul(csr.c_str(), &p, 0);
        if (*p == 0)
          csr_addr = csrn;
        else {
          cvm::log(cvm::ERROR, "Error: csr_name:{} undefined see +rmw_csr_resetseq switch\n", csr);
          co_return;
        }
      }
      cvm::log(cvm::HIGH, "Unit = {}: CSR addr {} = {}:{}\n", unit, csr_addr, csr_value, csr_mask);
      for (uint32_t i = 0; i < FLAGS_num_harts; i++) {
        auto v = (co_await csr_read(i, (uint32_t)unit, csr_addr) & ~csr_mask) | csr_value;
        co_await csr_write(i, (uint32_t)unit, csr_addr, v);
      }
    }
  }
  catch (...) {
    cvm::log(cvm::ERROR, "Error: unable to parse +rmw_csr_resetseq={}\n", FLAGS_rmw_csr_resetseq);
    co_return;
  }

  co_return;
}

cvm::messenger::task<void> reset_sequence::enable_ntrace() {
  // NTrace UseCase Scenario: Added support for NTrace Enable in boot [Stop-on-wrap only]
  uint64_t tr_ram_start_addr      = 0x0;
  uint64_t tr_ram_end_addr        = 0x1000;
  uint32_t tr_ram_mem_mode        = 0x1;    // SMEM
  uint32_t tr_ram_wrap_mode       = 0x1;    // Stop-on-wrap
  uint32_t tr_ram_mem_mode_idx    = 4;
  uint32_t tr_ram_wrap_mode_idx   = 8;
  uint32_t tr_ram_on              = 0x3;
  uint32_t tr_funnel_disinput_val = 0x0;
  uint32_t tr_funnel_on           = 0x3;
  uint32_t tr_te_on               = 0x7;
  uint32_t tr_te_frame_cfg        = 0x102FF0;

  // Configure NTrace RAM
  co_await write(tr_ram_start_low, SZ_8B, tr_ram_start_addr);
  co_await write(tr_ram_rp_low, SZ_8B, tr_ram_start_addr);
  co_await write(tr_ram_wp_low, SZ_8B, tr_ram_start_addr);
  co_await write(tr_ram_limit_low, SZ_8B, tr_ram_end_addr);
  co_await write(tr_ram_control, SZ_4B, (tr_ram_wrap_mode << tr_ram_wrap_mode_idx) | (tr_ram_mem_mode << tr_ram_mem_mode_idx) | tr_ram_on);

  // Configure Trace Funnel
  co_await write(tr_funnel_disinput, SZ_4B, tr_funnel_disinput_val);
  co_await write(tr_funnel_control, SZ_4B, tr_funnel_on);

  // Configure NTrace Encoder
  co_await write(cdbg_tr_frame_cfg, SZ_4B, tr_te_frame_cfg);
  co_await write(tr_te_control, SZ_4B, tr_te_on);

  co_return;
}

cvm::messenger::task<bool> reset_sequence::check_axi_bresp_timeout(interface_t interface, unsigned& id, uint64_t addr, size_t sz, bool exp_err_rsp) {

  uint32_t axi_bresp_cycle_cnt = 0;

  while (true) {
    co_await tick();
    
    if (axi_bresp_cycle_cnt >= FLAGS_axi_resp_timeout) {
      cvm::log(cvm::ERROR, "[pwrmgmt] [{}] Error: No free id's remaining for {} axi master\n", interface==SMC?"smc_axi_mst":"axi_mst", interface==SMC?"smc":"");
      co_return false;
    }
    axi_bresp_cycle_cnt++;

    if (interface == SMC) {
      if (cvm::registry::messenger.call<smc_mst_t::push_aw_no_id_rpc>(axi_loc_[interface], axi::a_no_id_t{addr, log2(sz), exp_err_rsp, RESET_SEQ_ID}, id)) {
        co_return true;
      }
    }
    else {
      if (cvm::registry::messenger.call<overlay_mst_t::push_aw_no_id_rpc>(axi_loc_[interface], axi::a_no_id_t{addr, log2(sz), uint8_t(0xF), exp_err_rsp, RESET_SEQ_ID}, id)) {
        co_return true;
      }
    }
  }

  co_return true;

}

cvm::messenger::task<bool> reset_sequence::check_axi_rresp_timeout(interface_t interface, unsigned& id, uint64_t addr, size_t sz, bool exp_err_rsp) {

  uint32_t axi_rresp_cycle_cnt = 0;

  while (true) {
    co_await tick();

    if (axi_rresp_cycle_cnt >= FLAGS_axi_resp_timeout) {
      cvm::log(cvm::ERROR, "[pwrmgmt] [{}] Error: No free id's remaining for {} axi master\n", interface==SMC?"smc_axi_mst":"axi_mst", interface==SMC?"smc":"");
      co_return false;
    }
    axi_rresp_cycle_cnt++;

    if (interface == SMC) {
      if (cvm::registry::messenger.call<smc_mst_t::push_ar_no_id_rpc>(axi_loc_[interface], axi::a_no_id_t{addr, log2(sz), exp_err_rsp, RESET_SEQ_ID}, id)) {
        co_return true;
      }
    }
    else {
      if (cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_loc_[interface], axi::a_no_id_t{addr, log2(sz), uint8_t(0xF), exp_err_rsp, RESET_SEQ_ID}, id)) {
        co_return true;
      }
    }
  }

  co_return true;
}
