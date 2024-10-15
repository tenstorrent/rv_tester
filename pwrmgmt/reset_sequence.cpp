#include "reset_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "pmu/pmu_plusargs.h"
#include <sstream>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <vector>

REGISTRY_register(reset_sequence, PWRMGMT, cvm::registry::all);

DEFINE_bool(pwrmgmt, false, "Runtime disable for pwrmgmt");
DEFINE_uint32(pll_pwrup_timeout, 50, "Number of soc cycles expected for pll power up to complete");
DEFINE_bool(pll_dfs, false, "Enable dfs sequence during cold boot");
DEFINE_uint32(pll_dfs_freq, 1200, "Clock freq for dfs");
DEFINE_uint32(pll_dfs_timeout, 50, "Number of soc cycles expected for pll dfs to complete");
DEFINE_uint32(num_thubs, 4, "Number of temprature hubs");
DEFINE_uint32(smc_max_snippets, 2, "Maximum number of random continuous AXI access from SMC ");
DEFINE_uint32(smc_snippet_size, 10, "Maximum number of random continuous AXI access from SMC ");
DEFINE_uint32(smc_max_ticks, 100, "Maximum delay/ticks between SMC randomm access snippets");
DEFINE_string(warm_reset, "off", "Enable warm resets in the sim - off/random/trigger");
DEFINE_string(warm_reset_count, "0:4", "Number of warm resets in the sim if random mode enabled");
DEFINE_string(warm_reset_interval, "2000:10000", "TB cycle interval between warm resets in the sim if random mode enabled");
DEFINE_string(warm_reset_trigger_type, "", "Send warm reset on a trigger");
DEFINE_string(warm_reset_trigger_interval, "rand:0:100", "TB cycle interval from trigger to warm reset");
DEFINE_string(warm_reset_sram_hold, "0:0", "Sram hold");
DEFINE_string(warm_reset_debug_hold, "0:1", "Debug hold");
DEFINE_string(warm_reset_critical_hold, "0:1", "Critical hold");
DEFINE_bool(patch_en, false, "Enable instruction patching");
DEFINE_bool(tj_max, false, "Program lower TJ Max Threshold");
DEFINE_bool(temp_throttle, false, "Program lower Temp throttle for core");
DEFINE_bool(patch_cpl_filter_dis, false, "Disable programming of inbound and outbound filters in core");
DEFINE_bool(patch_mmr_check, false, "Enable read write checking of patch related registers");
DEFINE_bool(patch_ram_check, false, "Enable read write checking of patch ram region");
DEFINE_bool(patch_cfg_lock, false, "Lock the patch mmrs while boot programming ");
DEFINE_bool(fuse_mmr_check, false, "Check RW and lockability of fuses ");
DEFINE_bool(init_smc_infilters, false, "Enable filter programming for JTAG and Overlay to access SRAM ");
DEFINE_bool(smc_axi_access, false, "Enable random AXI access from SMC ");
DEFINE_string(patch_ucode_input_file_path, "", "Path to file containing patch ucode routine");
DEFINE_string(patches, "WFI,SUB,BLT,AMOSWAP", "+patches=<instr1>,<instr2>,<instr3>,<instr4>; default will be picked if not specified ");
DEFINE_string(disable_patches, "AMOSWAP", "+disable_patches=<instr1>,<instr2>,<instr3>,<instr4>; default will be picked if not specified ");
DEFINE_bool(rand_patch, false, "Randomly pick 4 instructions available in the CSV to be patched");


extern "C" {
  void pwrmgmt_init();
  void pwrmgmt_cold_reset(uint8_t val);
  void pwrmgmt_warm_reset(uint8_t val);
  void pwrmgmt_reset_hold(uint8_t sram, uint8_t debug, uint8_t critical);
  void pwrmgmt_force_ref_clk(uint8_t val);

  uint8_t pwrmgmt_get_warm_reset_en(const char* mode) {
    return (std::string(mode) != "off");
  }
}

reset_sequence::reset_sequence(cvm::topology::loc_t loc, unsigned) : loc_(loc), scope_(nullptr) {

  // Topology
  smc_axi_loc_ = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_SMC_MST", 0);
  num_cores_ = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });

  // Reset count
  cvm::registry::messenger.connect<int>(loc_, [this](int c) { return this->start(c); });
}

void reset_sequence::start(int reset_count) {

  reset_count_ = reset_count;

  cvm::log(cvm::HIGH, "[reset_sequence] count = {}\n", reset_count_);

  // Sequence threads
  if (reset_count_ <= 0)
    cold_reset_sequence_thread();

  if (reset_count_ > 0)
    warm_reset_sequence_thread();


  smc_random_sequence_thread();  
}

reset_sequence::~reset_sequence() {
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

void reset_sequence::smc_random_sequence_thread() {
  auto *task = +[] (reset_sequence* m) -> cvm::messenger::task<void> {
    co_await m->smc_axi_random_access();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void reset_sequence::temp_throttle_release_thread() {
  auto *task = +[] (reset_sequence* m) -> cvm::messenger::task<void> {
    co_await m->temp_throttle_disable();
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
  co_await pll_startup_sequence();

  // PLL dfs sequence
  if (FLAGS_pll_dfs)
    co_await pll_dfs_sequence();

  // Reset controller sequence
  co_await cpl_reset_sequence(COLD);

  // Wait for next tick
  co_await tick();

  co_return;
}

cvm::messenger::task<void> reset_sequence::warm_reset_sequence() {
  // Assert force_ref_clk
  force_ref_clk(1);

  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
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

  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
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

  co_await cpl_reset_sequence(WARM);

  // Wait for next tick
  co_await tick();

  co_return;
}

cvm::messenger::task<void> reset_sequence::cpl_reset_sequence(rst_t rst_type) {
  co_await release_cpl_reset();
  if (rst_type == COLD)  
    co_await program_fuses();
  if (FLAGS_fuse_mmr_check)
    co_await fuse_mmr_check();
  co_await program_thub_threshold();
  if(FLAGS_init_smc_infilters) {
   co_await init_smc_filters();
  }

  if (FLAGS_patch_en && rst_type == COLD) { 
    co_await program_patch();
  } else if (FLAGS_patch_ram_check) {
    co_await patch_ram_check();
  };
  co_await release_cpl_nofetch();
  if (FLAGS_fuse_mmr_check)
    co_await disabled_mmr_csr_check();

  co_return;
}

cvm::messenger::task<void> reset_sequence::pll_startup_sequence() {
  co_await check_pll_status();
  co_await clear_pll_status();

  co_return;
}

cvm::messenger::task<void> reset_sequence::check_pll_status() {
  uint32_t count = 0;
  while (true) {
    co_await tick();
    auto data = co_await read(pll_interrupts, SZ_4B);
    if (data & (1 << cold_powerup_idx))
      break;

    count++;
    if (count > FLAGS_pll_pwrup_timeout)
      cvm::log(cvm::ERROR, "Error: PLL cold power up not done after {} soc clocks\n", FLAGS_pll_pwrup_timeout);
  }

  co_return;
}

cvm::messenger::task<void> reset_sequence::clear_pll_status() {
  co_await tick();
  co_await write(pll_interrupts, SZ_4B, (1 << cold_powerup_idx));

  co_return;
}

cvm::messenger::task<void> reset_sequence::pll_dfs_sequence() {
  uint32_t freq_ratio = 2400 / FLAGS_pll_dfs_freq;
  co_await write(pll_parameters0, SZ_4B, (1 << scalar_div_idx | 52 << main_divider_div_idx | freq_ratio << pre_divider_div_idx));
  co_await write(pll_control, SZ_4B, (1 << dfs_req_idx));

  uint32_t count = 0;
  while (true) {
    co_await tick();
    auto data = co_await read(pll_interrupts, SZ_4B);
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
  co_await write(rst_ctl_warm, SZ_4B, (1 << cpl_cl_warm_reset_n));

  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
  co_await tick();

  co_await write(rst_ctl_warm, SZ_4B, (1 << cpl_force_ss_to_ref_clock_n | 1 << cpl_cl_warm_reset_n));

  co_return;
}

cvm::messenger::task<void> reset_sequence::program_fuses() {
  co_await tick();

  uint64_t fuse = fuse_val();

  co_await write(sw_fuse_mmr,     SZ_8B, fuse);

  for (uint32_t i = 0; i < FLAGS_num_harts; ++i)
    co_await write(core_fuse_mmr + i * core_fuse_offset,   SZ_8B, fuse);
  
  //co_await write(trace_fuse_mmr+4,  SZ_4B, (fuse>>32) & 0xFFFFFFFF);
  //co_await write(trace_fuse_mmr, SZ_4B, fuse & 0xFFFFFFFF ); //FIXME: Enable 8B transaction after RVDE-17674 is fixed 
  co_await write(trace_fuse_mmr, SZ_8B, fuse & 0xFFFFFFFFFFF7FFF ); //FIXME: Enable 8B transaction after RVDE-17674 is fixed 
  co_await write(trace_fuse_mmr, SZ_8B, fuse );  
  co_await write(aclint_fuse_mmr, SZ_8B, fuse);
  co_await write(dm_fuse_mmr,     SZ_8B, fuse);
  co_await write(sc_fuse_mmr,     SZ_8B, fuse);

  co_return;
}

cvm::messenger::task<void> reset_sequence::release_cpl_nofetch() {
  co_await tick();
  co_await write(rst_ctl_nofetch, SZ_4B, ((~FLAGS_hart_enable_mask << cpl_cl_no_fetch) & 0xff));

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

cvm::messenger::task<uint64_t> reset_sequence::read(uint64_t addr, size_t sz) {
  assert(sz <= 8);
  cvm::log(cvm::MEDIUM, "[pwrmgmt] read req - addr={:#x}, sz={}\n", addr, sz);
  cvm::registry::messenger.signal(smc_axi_loc_, transactor::read_request_t{addr, sz});
  auto resp = co_await cvm::registry::messenger.wait<transactor::read_response_t>(smc_axi_loc_);
  auto data = convert_to_dword_array(resp.data);
  // FIXME - check why this alignment is needed
  uint64_t dword = (addr % 8) ? (data[0] >> 32) : data[0];
  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  dword &= mask;
  cvm::log(cvm::MEDIUM, "[pwrmgmt] read resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data[0], dword, mask);
  co_return dword;
}

cvm::messenger::task<void> reset_sequence::write(uint64_t addr, size_t sz, uint64_t data) {
  assert(sz <= 8);
  // FIXME - check why this alignment is needed
  uint64_t dword = (addr % 8) ? (data << 32) : data;
  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  mask = (addr % 8) ? (mask << 32) : mask;
  auto byte_array = convert_to_byte_array({dword});
  std::vector<bool> strb(8, false);
  for(int i=0; i<8; ++i)
    strb[i] = (mask & (0xFFull << (i*8))) != 0;
  cvm::log(cvm::MEDIUM, "[pwrmgmt] write req - addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", addr, sz, data, dword, mask);
  cvm::registry::messenger.signal(smc_axi_loc_, transactor::write_request_t{addr, SZ_8B, byte_array, strb});
  auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(smc_axi_loc_);
  cvm::log(cvm::MEDIUM, "[pwrmgmt] write resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data, dword, mask);
  co_return;
}

cvm::messenger::task<void> reset_sequence::write(uint64_t addr, size_t sz, const std::vector<uint64_t>& data ) {
  assert(sz <= 8);
  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  mask = (addr % 8) ? (mask << 32) : mask;
  std::vector<bool> strb(8, false);
  for(int i=0; i<8; ++i)
    strb[i] = (mask & (0xFFull << (i*8))) != 0;
  int size = data.size();
  for(int i=0; i < size; i++){
    uint64_t addr_n = addr + i*sz;
    uint64_t dword = (addr_n % 8) ? (data[i] << 32) : data[i];
    auto byte_array = convert_to_byte_array({dword});
    cvm::log(cvm::MEDIUM, "[pwrmgmt] batch write req : {} - addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", i, addr_n, sz, data[i], dword, mask);
    cvm::registry::messenger.signal(smc_axi_loc_, transactor::write_request_t{addr_n, SZ_8B, byte_array, strb});
  };
  // Note - simultaneous burst write calls might result in interleaved resposes
  for(int i=0; i < size; i++){
    uint64_t addr_n = addr + i*sz;
    uint64_t dword = (addr_n % 8) ? (data[i] << 32) : data[i];
    auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(smc_axi_loc_);
    cvm::log(cvm::MEDIUM, "[pwrmgmt] batch write resp : {} - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", i, resp.id, addr_n, sz, data[i], dword, mask);
  };
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
  return static_cast<uint64_t>(FLAGS_trace_enable << trace_fuse_idx);
}

uint64_t reset_sequence::dm_fuse_val() {
  return static_cast<uint64_t>(FLAGS_debug_enable << dm_fuse_idx);
}

uint64_t reset_sequence::export_control_fuse_val() {
  return static_cast<uint64_t>(FLAGS_export_control_en << exp_ctrl_fuse_idx);
}

uint64_t reset_sequence::sc_fuse_val() {
  uint64_t sc_fuse = 0;

  int32_t nways = cvm::topology::attr(cvm::topology::get_from_type("CORE", 0), "SC_NUM_WAYS").second;
  for (int i=0; i<nways/4; ++i) {
    uint32_t segment = (~FLAGS_sc_dis_ways_mask >> (4*i)) & 0xf;
    if (segment == 0xf)
      sc_fuse |= (1ull << i);
  }
  return sc_fuse;
}

uint64_t reset_sequence::fuse_val() {
  return core_fuse_val() | trace_fuse_val() | dm_fuse_val() | sc_fuse_val() | export_control_fuse_val() | (1ull << lock_idx);
}


cvm::messenger::task<void> reset_sequence::program_patch() {
  co_await tick();
  read_patch_csv();
  if (!FLAGS_patch_cpl_filter_dis) { 
    //CPL AXI in filter programming
    co_await write(cpl_in_filter0_addr_l ,SZ_8B , 0x4C000);
    co_await write(cpl_in_filter0_addr_h ,SZ_8B , 0x4EFFF);
    co_await write(cpl_in_filter0_config ,SZ_8B , 0x81010113);      
    //CPL AXI out filter programming
    co_await write(cpl_out_filter0_addr_l ,SZ_8B , 0x4C000);
    co_await write(cpl_out_filter0_addr_h ,SZ_8B , 0x4EFFF);
    co_await write(cpl_out_filter0_config ,SZ_8B , 0x81010113);
  };  

  std::string token;
  std::vector<std::string> disable_patch_instr = {"patch_prolouge","patch_epilouge"};
  std::istringstream ss1(FLAGS_disable_patches);
  while (std::getline(ss1, token, ',')) {
    disable_patch_instr.push_back(token);
  }
  
  std::vector<std::string> patch_instr = {} ;
  if (FLAGS_rand_patch) {
    for (auto const& [key, value] : patches) 
      if (std::find(disable_patch_instr.begin(), disable_patch_instr.end(), key) == disable_patch_instr.end())
        patch_instr.push_back(key);
    std::shuffle(std::begin(patch_instr), std::end(patch_instr), cvm::rand::gen);
  } else {
    std::istringstream ss(FLAGS_patches);
    while (std::getline(ss, token, ',')) {
      if (std::find(disable_patch_instr.begin(), disable_patch_instr.end(), token) == disable_patch_instr.end())
        patch_instr.push_back(token);
    }
  }
  

  std::unordered_map<uint32_t, uint64_t> patch_cfg;
  patch_cfg[core_pversion_mmr] = rand()%0xFF;


  std::vector<uint32_t> patch_header = patches["patch_prolouge"].ucodes;

  patch_header.insert(patch_header.end(), patches["patch_epilouge"].ucodes.begin(), patches["patch_epilouge"].ucodes.end()); 

  co_await write(cpl_patch_ram_base, SZ_8B, concatenate_uint32_to_uint64(patch_header) );
  co_await write(cpl_patch_ram_ptrig_0, SZ_8B, concatenate_uint32_to_uint64(patch_trig_0) );
  co_await write(cpl_patch_ram_ptrig_1, SZ_8B, concatenate_uint32_to_uint64(patch_trig_1) );
  co_await write(cpl_patch_ram_ptrig_2, SZ_8B, concatenate_uint32_to_uint64(patch_trig_2) );
  co_await write(cpl_patch_ram_ptrig_3, SZ_8B, concatenate_uint32_to_uint64(patch_trig_3) );

  uint64_t pcontrol_data = 0;

  for (int i = 0; i < (int)patch_instr.size(); ++i) { 
    std::string patchTag = patch_instr[i];
    cvm::log(cvm::MEDIUM, "[pwrmgmt] Patching instruction: {} , patchMask: 0x{:x}, Opcode: 0x{:x}, enableMask: 0x{:x}\n", patchTag, patches[patchTag].patchMask, patches[patchTag].patchInstruction, patches[patchTag].enableMask);
    patch_cfg[core_preg0_mmr+(i*8)] = ((uint64_t)patches[patchTag].patchMask<<32 | patches[patchTag].patchInstruction); 
    std::vector<uint32_t> ucode_body = patches[patchTag].ucodes;
    co_await write(cpl_patch_ram_pbody_0+(i*0x400), SZ_8B, concatenate_uint32_to_uint64(ucode_body) );
    if (FLAGS_patch_ram_check) populate_patch_ram(cpl_patch_ram_pbody_0+(i*0x400), concatenate_uint32_to_uint64(ucode_body));
    pcontrol_data =  pcontrol_data | (((uint64_t)patches[patchTag].enableMask | 1) << i*16); // enable patch 
    cvm::log(cvm::MEDIUM, "[pwrmgmt] pcontrol_data : 0x{:x}\n", pcontrol_data);
    if (i == 3) break;
  }
  if (FLAGS_patch_ram_check) {
    populate_patch_ram(cpl_patch_ram_base, concatenate_uint32_to_uint64(patch_header) );
    populate_patch_ram(cpl_patch_ram_ptrig_0, concatenate_uint32_to_uint64(patch_trig_0) );
    populate_patch_ram(cpl_patch_ram_ptrig_1, concatenate_uint32_to_uint64(patch_trig_1) );
    populate_patch_ram(cpl_patch_ram_ptrig_2, concatenate_uint32_to_uint64(patch_trig_2) );
    populate_patch_ram(cpl_patch_ram_ptrig_3, concatenate_uint32_to_uint64(patch_trig_3) );
    co_await patch_ram_check();
  }

  if (FLAGS_patch_cfg_lock) pcontrol_data = pcontrol_data | 0x8000800080008000;


  for (uint32_t i=0; i< FLAGS_num_harts; i++) {
    uint32_t offset = i * core_fuse_offset;
    for (auto j : patch_cfg)
      co_await write(j.first + offset, SZ_8B, j.second);
    //co_await csr_write(i, core_ptvec_csr , 0x4210C000); FIXME : Enable CSR write once its fixed
    co_await write(core_pcontrol_mmr + offset, SZ_8B, pcontrol_data);
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
        cvm::log(cvm::ERROR, "[pwrmgmt] patch registers check ERROR : addr 0x{:x} ,  Expected :0x{:x}, Actual : 0x{:x} \n", i.first, i.second, data );
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

cvm::messenger::task<void> reset_sequence::program_thub_threshold() {
  co_await tick();
  if(FLAGS_tj_max){
    for (uint8_t i=0; i<FLAGS_num_thubs; ++i) {
      co_await write_thub_reg(thub_threhold_param_reg,0x05400640,i+9,i);
    };
  };
  // Enable MC throttling when Temprature crosses threshold
  if(FLAGS_temp_throttle)
  {
    for(uint32_t p =0; p < FLAGS_num_harts; ++p) // Fixed for 8 core config as THUB is only in 8c
    {
      // Write to MC power config
      co_await csr_write(p, 0x8,core_pwr_throttle_cfg_0 , 0x000078830372a211);
      co_await csr_write(p, 0x8,core_pwr_throttle_cfg_1 , 0x1041017ecb594129);
    };
    // Disable temp throttle
    temp_throttle_release_thread();
  };
 
};

cvm::messenger::task<void> reset_sequence::temp_throttle_disable()
{

  //Delay before temp throttle is disabled
  for(uint32_t i =0; i< 5000; i++)
  {
    co_await tick();
  };

  for(uint32_t p =0; p < FLAGS_num_harts; ++p) // Fixed for 8 core config as THUB is only in 8c
  {
      // Write to MC power config
      co_await csr_write(p, 0x8,core_pwr_throttle_cfg_0 , 0x000078830372a211);
      co_await csr_write(p, 0x8,core_pwr_throttle_cfg_1 , 0x11ff017ecb594129);
  };
};
 

std::vector<uint64_t> reset_sequence::concatenate_uint32_to_uint64(const std::vector<uint32_t>& input) {
      std::vector<uint64_t> result;
      int size = input.size();
      // Loop through input array and concatenate pairs
      for (int i = 0; i < size; i += 2) {
          uint32_t low = input[i];
          uint32_t high = (i + 1 < size) ? input[i + 1] : 0;  // Use 0 if no pair available
          uint64_t concatenated = static_cast<uint64_t>(high) << 32 | low;
          result.push_back(concatenated);
      }
      return result;
  };

void reset_sequence::populate_patch_ram(uint64_t addr, const std::vector<uint64_t>& data ) {
  int size = data.size();
  for(int i=0; i < size; i++){
    uint64_t addr_n = addr + i*8;
    patch_ram[addr_n] = data[i];
    //cvm::log(cvm::NONE, "[pwrmgmt]  populate_patch_ram : addr 0x{:x} , data 0x{:x} \n", addr_n, data[i] );
  }
};

cvm::messenger::task<void> reset_sequence::patch_ram_check() {
  uint64_t actual_data, exp_data;
  uint32_t addr;
  co_await tick();
  for( int i = 0; i<20;i++ ){
    addr = cpl_patch_ram_base + (rand()%512)*8;
    actual_data = co_await read(addr, SZ_8B);
    if (patch_ram.find(addr) == patch_ram.end())
      exp_data = 0;
    else
      exp_data = patch_ram[addr];
  if (exp_data != actual_data)
      cvm::log(cvm::ERROR, "[pwrmgmt] patch ram check ERROR : addr 0x{:x} ,  Expected :0x{:x}, Actual : 0x{:x} \n", addr, exp_data, actual_data );
    else
      cvm::log(cvm::NONE, "[pwrmgmt]  patch ram check : addr 0x{:x} , data 0x{:x} \n", addr, actual_data );
  };  
  co_return;
};

cvm::messenger::task<void> reset_sequence::init_smc_filters() {
  
  co_await tick();
    //CPL AXI in filter programming
    co_await write(cpl_in_filter1_addr_l ,SZ_8B , 0x41000);
    co_await write(cpl_in_filter1_addr_h ,SZ_8B , 0x41FFF);
    co_await write(cpl_in_filter1_config ,SZ_8B , 0x8000000000010113);      
    //CPL AXI in filter programming
    co_await write(cpl_in_filter2_addr_l ,SZ_8B , 0x42000);
    co_await write(cpl_in_filter2_addr_h ,SZ_8B , 0x42FFF);
    co_await write(cpl_in_filter2_config ,SZ_8B , 0x8000000000020113);  

    co_await write(cpl_in_filter2_addr_l+0x20 ,SZ_8B , 0x41000);
    co_await write(cpl_in_filter2_addr_h+0x20 ,SZ_8B , 0x4EFFF);
    co_await write(cpl_in_filter2_config+0x20 ,SZ_8B , 0x8000000000030113);   

  co_return;
};

cvm::messenger::task<void> reset_sequence::fuse_mmr_check() {
  co_await tick();

  uint64_t fuse = fuse_val();
  std::vector<uint64_t> fuse_registers = { 
    sw_fuse_mmr,
    trace_fuse_mmr,
    aclint_fuse_mmr,
    dm_fuse_mmr,
    sc_fuse_mmr
  };
  
  for (uint32_t i=0; i<FLAGS_num_harts; ++i)
    fuse_registers.push_back(core_fuse_mmr + i * core_fuse_offset);
  uint64_t actual_data;
  for (auto addr : fuse_registers) {
    actual_data = co_await read(addr, SZ_8B);
    if (fuse != actual_data)
      cvm::log(cvm::ERROR, "[pwrmgmt] Fuse reg read check ERROR : addr 0x{:x} ,  Expected :0x{:x}, Actual : 0x{:x} \n", addr, fuse, actual_data );
    else
      cvm::log(cvm::NONE, "[pwrmgmt]  Fuse reg read check : addr 0x{:x} , data 0x{:x} \n", addr, actual_data );
  };

    for (auto addr : fuse_registers) {
      co_await write(addr, SZ_8B, rand()%0xFFFF'FFFF'FFFF'FFFF);
      actual_data = co_await read(addr, SZ_8B);
      if (fuse != actual_data)
        cvm::log(cvm::ERROR, "[pwrmgmt] Fuse reg lock check ERROR : addr 0x{:x} ,  Expected :0x{:x}, Actual : 0x{:x} \n", addr, fuse, actual_data );
      else
        cvm::log(cvm::NONE, "[pwrmgmt]  Fuse reg lock check : addr 0x{:x} , data 0x{:x} \n", addr, actual_data );
    };

  std::vector<uint64_t> id = mhartid();
  uint64_t physical_id = 0;
  for (uint32_t i=0; i<FLAGS_num_harts; ++i) { 
      physical_id = co_await read(core_physical_id_mmr + i * core_fuse_offset, SZ_8B);
      //FIXME : Add a check for mhartid csr
      if (id[i] != (physical_id & 0x7))
        cvm::log(cvm::ERROR, "[pwrmgmt] Core ID to Virtual ID mapping ERROR : Virtual id 0x{:x} ,  Expected Core ID :0x{:x}, Actual Core ID : 0x{:x} \n", i, id[i], (physical_id & 0x7) );
  };
  
  co_return;
};


cvm::messenger::task<void> reset_sequence::disabled_mmr_csr_check() {
  co_await tick();
  //FIXME : ADD read write accesses to disabled cores
  co_return;
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
      cvm::log(cvm::MEDIUM, "[pwrmgmt] reset holds [sram={}, debug={}, critical={}]\n", sram, debug, critical);
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

cvm::messenger::task<void> reset_sequence::smc_scratchpad_default_access() {
  co_await tick();
    // Read reset values  
    co_await smc_read_access_check(mb_scratchpad, mb_scratchpad_rst,SZ_8B);
    co_await smc_read_access_check(cc_scratchpad, cc_scratchpad_rst,SZ_4B);
    co_await smc_read_access_check(rc_scratchpad, rc_scratchpad_rst,SZ_4B);
    co_await smc_read_access_check(dm_scratchpad, dm_scratchpad_rst,SZ_8B);
    co_await smc_read_access_check(cr_scratchpad, cr_scratchpad_rst,SZ_8B);
    co_await smc_read_access_check(sw_scratchpad, sw_scratchpad_rst,SZ_8B);
    co_await smc_read_access_check(ac_scratchpad, ac_scratchpad_rst,SZ_8B);
  co_return;
  };

 cvm::messenger::task<void> reset_sequence::smc_read_access_check(uint32_t addr, uint64_t exp_data, size_t sz){
  uint64_t actual_data;

  actual_data = co_await read(addr, sz);
  if (exp_data != actual_data)
    cvm::log(cvm::ERROR, "[pwrmgmt] SMC Scratchpad access check ERROR : addr 0x{:x} ,  Expected :0x{:x}, Actual : 0x{:x} \n",addr , exp_data, actual_data );
  else
    cvm::log(cvm::NONE, "[pwrmgmt] SMC Scratchpad access check : addr 0x{:x} , data 0x{:x} \n",addr , actual_data );

  co_return;
 };

 cvm::messenger::task<void> reset_sequence::smc_axi_random_access()
 {
  uint64_t data;
  uint32_t randomAddress;
  uint32_t SRAMAddress;
  uint32_t data2;
  int randomIndex;

    co_await tick();
  if(FLAGS_smc_axi_access){
    // Default value check for scratch pad registers
    co_await smc_scratchpad_default_access(); 

    data  = 0xA5A5A5A5A5A5A5A5; 
    data2 = 0xA5A5A5A5; 
    for(uint32_t i = 0; i < FLAGS_smc_max_snippets ; ++i)
    {
      // Delay between each iteration
      co_await delay_counters();
      for(uint32_t p = 0; p < FLAGS_smc_snippet_size ; ++p)
      {
        std::random_device rd;
        std::mt19937 gen(rd());

        // Create a uniform distribution to select random indices
        std::uniform_int_distribution<> dist(0,smc_dest_address.size() - 1);
        std::uniform_int_distribution<uint32_t> sram_dist(0x40000,0x4BFFF);

        // Pick a random address from the pool of scratchpad registers
        randomIndex = dist(gen);
        randomAddress = smc_dest_address[randomIndex];
        SRAMAddress   = sram_dist(gen) & 0xFFFF8;

        if((randomAddress == rc_scratchpad)|| (randomAddress == cc_scratchpad))
        {
        co_await write(randomAddress, SZ_4B, data2);
        co_await smc_read_access_check(randomAddress, data2,SZ_4B);
        }
        else if((randomAddress == core_pwr_throttle_cfg_0)|| (randomAddress == core_pwr_throttle_cfg_1))
        {
          co_await csr_read(0, 0x8,randomAddress);
        }
        else{
         co_await write(randomAddress, SZ_8B, data);
         co_await smc_read_access_check(randomAddress, data,SZ_8B);
        };

        // Random SRAM access
         co_await write(SRAMAddress, SZ_8B, data);
         co_await smc_read_access_check(SRAMAddress, data,SZ_8B);
        data = ~data;
        data2 = ~data2;
  
      };
    };
  };
  co_return;
};

cvm::messenger::task<void> reset_sequence::delay_counters(){
  for(uint32_t i =0; i< FLAGS_smc_max_ticks; i++)
  {
    co_await tick();
  };

  co_return;
 };
 
// Helper function to trim whitespace from a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return ""; 
    }
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

void reset_sequence::read_patch_csv() {
    std::ifstream file(FLAGS_patch_ucode_input_file_path); //"@chips//dv/tb/patch/patch_ucode.csv"
    cvm::log(cvm::MEDIUM, "Patch Ucode CSV : {}\n" , FLAGS_patch_ucode_input_file_path);

    std::string line;
    // Skip header row
    std::getline(file, line); 
    std::string currentPatchTag; 

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string patchTag, ucode, patchInstruction, patchMask, enableMask;
        std::getline(ss, patchTag, ',');
        std::getline(ss, ucode, ',');
        std::getline(ss, patchInstruction, ',');
        std::getline(ss, patchMask, ',');
        std::getline(ss, enableMask, ',');

        // Trim leading/trailing whitespace
        patchTag = trim(patchTag);
        ucode = trim(ucode);
        patchInstruction = trim(patchInstruction);
        patchMask = trim(patchMask);
        enableMask = trim(enableMask);

        // Convert to hexadecimal and discard "0x" prefix
        if (ucode.substr(0, 2) == "0x") {
            ucode = ucode.substr(2); 
        }
        if (patchInstruction.substr(0, 2) == "0x") {
            patchInstruction = patchInstruction.substr(2); 
        }
        if (patchMask.substr(0, 2) == "0x") {
            patchMask = patchMask.substr(2); 
        }
        if (enableMask.substr(0, 2) == "0x") {
            enableMask = enableMask.substr(2); 
        }

        // If patchTag is not empty, it's a new instruction
        if (!patchTag.empty()) {
            currentPatchTag = patchTag;
            patches[currentPatchTag] = {
                patchTag, 
                {}, 
                static_cast<uint32_t>(std::stoul(patchInstruction, nullptr, 16)), 
                static_cast<uint32_t>(std::stoul(patchMask, nullptr, 16)),
                static_cast<uint32_t>(std::stoul(enableMask, nullptr, 16))
            }; 
        } 
        else if (!ucode.empty()) { // If ucode is not empty and we have a current patch
            patches[currentPatchTag].ucodes.push_back(static_cast<uint32_t>(std::stoul(ucode, nullptr, 16)));
        } 
    }

    // Output the parsed patches, formatting hex values

    for (const auto& pair : patches) {
        cvm::log(cvm::HIGH, "Patch Tag: {} \n" ,pair.second.patchTag);
        cvm::log(cvm::HIGH, "Patch Instruction: 0x{:x}\n",pair.second.patchInstruction);
        cvm::log(cvm::HIGH, "Patch Mask: 0x{:x}\n", pair.second.patchMask);
        cvm::log(cvm::HIGH, "Enable Mask: 0x{:x}\n", pair.second.enableMask);
        cvm::log(cvm::HIGH, "Ucodes: ");
        for (const auto& ucode : pair.second.ucodes) {
            cvm::log(cvm::HIGH, "0x{:x}, ",ucode); 
        }
        cvm::log(cvm::HIGH, "\n***********************\n"); 

      
    }
}
