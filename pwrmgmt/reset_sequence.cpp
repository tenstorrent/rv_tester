#include "reset_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include <sstream>

REGISTRY_register(reset_sequence, PWRMGMT, cvm::registry::all);

DEFINE_bool(pwrmgmt, false, "Runtime disable for pwrmgmt");
DEFINE_uint32(pll_pwrup_timeout, 50, "Number of soc cycles expected for pll power up to complete");
DEFINE_bool(pll_dfs, false, "Enable dfs sequence during cold boot");
DEFINE_uint32(pll_dfs_freq, 1200, "Clock freq for dfs");
DEFINE_uint32(pll_dfs_timeout, 50, "Number of soc cycles expected for pll dfs to complete");
DEFINE_string(warm_reset, "off", "Enable warm resets in the sim - off/random/trigger");
DEFINE_string(warm_reset_count, "0:4", "Number of warm resets in the sim if random mode enabled");
DEFINE_string(warm_reset_interval, "5000:50000", "TB cycle interval between warm resets in the sim if random mode enabled");
DEFINE_string(warm_reset_trigger_type, "", "Send warm reset on a trigger");
DEFINE_string(warm_reset_trigger_interval, "rand:0:100", "TB cycle interval from trigger to warm reset");
DEFINE_string(warm_reset_sram_hold, "0:1", "Sram hold");
DEFINE_string(warm_reset_debug_hold, "0:1", "Debug hold");
DEFINE_string(warm_reset_critical_hold, "0:1", "Critical hold");
DEFINE_bool(patch_en, false, "Enable instruction patching");

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

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });

  // Sequence threads
  cold_reset_sequence_thread();
  if (FLAGS_warm_reset == "random") {
    warm_reset_random_mode_sequence_thread();
  } else if (FLAGS_warm_reset == "trigger") {
    warm_reset_trigger_mode_sequence_thread();
  }
}

void reset_sequence::check() {
  // Called just before destruction
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"pwrmgmt_warm_reset_count\": \"{}\"}}\n", warm_reset_count_);
}

void reset_sequence::cold_reset_sequence_thread() {
  auto *task = +[] (reset_sequence* m) -> cvm::messenger::task<void> {
    co_await m->cold_reset_sequence();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void reset_sequence::warm_reset_random_mode_sequence_thread() {
  auto *task = +[] (reset_sequence* m) -> cvm::messenger::task<void> {
    co_await m->warm_reset_random_mode_sequence();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void reset_sequence::warm_reset_trigger_mode_sequence_thread() {
  auto *task = +[] (reset_sequence* m) -> cvm::messenger::task<void> {
    co_await m->warm_reset_trigger_mode_sequence();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> reset_sequence::cold_reset_sequence() {
  // Wait for first couple clock ticks
  for (int i=0; i<2; ++i)
    co_await tick();

  // Init values for all pins
  // Assert cold reset
  init();

  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
    co_await tick();

  // Deassert cold reset
  cold_reset(0);

  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
    co_await tick();

  if (!FLAGS_pwrmgmt)
    co_return;

  // PLL cold powerup sequence
  co_await pll_startup_sequence();

  // PLL dfs sequence
  if (FLAGS_pll_dfs)
    co_await pll_dfs_sequence();

  // Reset controller sequence
  co_await cpl_reset_sequence(COLD);

  // Deassert force_ref_clk
  force_ref_clk(0);

  // Wait for next tick
  co_await tick();

  co_return;
}

cvm::messenger::task<void> reset_sequence::warm_reset_random_mode_sequence() {
  // Wait on force_ref_clk deassertion
  co_await force_ref_clk();

  while (true) {
    // Wait for next tick generated after a random interval "warm_reset_interval"
    co_await tick();

    warm_reset_count_++;
    cvm::log(cvm::NONE, "[pwrmgmt] Starting warm reset sequence - count = {}\n", warm_reset_count_);

    co_await warm_reset_sequence();
  }
}

cvm::messenger::task<void> reset_sequence::warm_reset_trigger_mode_sequence() {
  // Wait on force_ref_clk deassertion
  co_await force_ref_clk();

  // Wait for next selected trigger
  co_await trigger();

  // Wait for tick after trigger
  co_await tick();

  co_await warm_reset_sequence();
}

cvm::messenger::task<void> reset_sequence::warm_reset_sequence() {
  // Assert force_ref_clk
  force_ref_clk(1);

  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
    co_await tick();

  // Assert holds
  reset_hold(
    cvm::rand::get(FLAGS_warm_reset_sram_hold),
    cvm::rand::get(FLAGS_warm_reset_debug_hold),
    cvm::rand::get(FLAGS_warm_reset_critical_hold)
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

  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
    co_await tick();

  co_await cpl_reset_sequence(WARM);

  // Deassert force_ref_clk
  force_ref_clk(0);

  // Wait for next tick
  co_await tick();

  co_return;
}

cvm::messenger::task<void> reset_sequence::cpl_reset_sequence(rst_t ) {
  co_await release_cpl_reset();
  co_await program_fuses();
  if (FLAGS_patch_en) { 
    co_await program_patch();
  };
  co_await release_cpl_nofetch();

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

  co_return;
}

cvm::messenger::task<void> reset_sequence::program_fuses() {
  co_await tick();

  uint64_t fuse = fuse_val();

  co_await write(sw_fuse_mmr,     SZ_8B, fuse);

  for (uint32_t i=0; i<FLAGS_num_harts; ++i)
    co_await write(core_fuse_mmr + i * core_fuse_offset,   SZ_8B, fuse);

  co_await write(trace_fuse_mmr,  SZ_8B, fuse);
  co_await write(aclint_fuse_mmr, SZ_8B, fuse);
  co_await write(dm_fuse_mmr,     SZ_8B, fuse);
  co_await write(sc_fuse_mmr,     SZ_8B, fuse);

  co_return;
}

cvm::messenger::task<void> reset_sequence::release_cpl_nofetch() {
  co_await tick();
  co_await write(rst_ctl_nofetch, SZ_4B, (0 << cpl_cl_no_fetch));

  co_return;
}

cvm::messenger::task<void> reset_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::pwrmgmt::m_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> reset_sequence::force_ref_clk() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::pwrmgmt::m_force_ref_clk<>>(loc_);
  co_return;
}

cvm::messenger::task<void> reset_sequence::trigger() {
  co_return;
}

cvm::messenger::task<uint64_t> reset_sequence::read(uint64_t addr, size_t sz) {
  assert(sz <= 8);
  cvm::log(cvm::NONE, "[pwrmgmt] read req - addr={:#x}, sz={}\n", addr, sz);
  cvm::registry::messenger.signal(smc_axi_loc_, transactor::read_request_t{addr, sz});
  auto resp = co_await cvm::registry::messenger.wait<transactor::read_response_t>(smc_axi_loc_);
  auto data = convert_to_dword_array(resp.data);
  // FIXME - check why this alignment is needed
  uint64_t dword = (addr % 8) ? (data[0] >> 32) : data[0];
  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  dword &= mask;
  cvm::log(cvm::NONE, "[pwrmgmt] read resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data[0], dword, mask);
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
  cvm::log(cvm::NONE, "[pwrmgmt] write req - addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", addr, sz, data, dword, mask);
  cvm::registry::messenger.signal(smc_axi_loc_, transactor::write_request_t{addr, SZ_8B, byte_array, strb});
  auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(smc_axi_loc_);
  cvm::log(cvm::NONE, "[pwrmgmt] write resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data, dword, mask);
  co_return;
}

void reset_sequence::init() {
  cvm::registry::callbacks.push(
    scope_,
    []() {
      cvm::log(cvm::NONE, "[pwrmgmt] assert cold_reset, force_ref_clk\n");
      pwrmgmt_init();
    });
}

void reset_sequence::cold_reset(uint8_t assert) {
  cvm::registry::callbacks.push(
    scope_,
    [assert]() {
      cvm::log(cvm::NONE, "[pwrmgmt] {} cold reset\n", assert ? "assert" : "deassert");
      pwrmgmt_cold_reset(assert);
    });
}

void reset_sequence::warm_reset(uint8_t assert) {
  cvm::registry::callbacks.push(
    scope_,
    [assert]() {
      cvm::log(cvm::NONE, "[pwrmgmt] {} warm reset\n", assert ? "assert" : "deassert");
      pwrmgmt_warm_reset(assert);
    });
}

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
      cvm::log(cvm::NONE, "[pwrmgmt] {} force_ref_clk\n", assert ? "assert" : "deassert");
      pwrmgmt_force_ref_clk(assert);
    });
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
  std::vector<uint64_t> core_mhartid = mhartid();
  for (uint32_t i=0; i<FLAGS_num_harts; ++i)
    core_fuse |= (((core_mhartid[i] << 1) | core_en(i)) << (4*i));
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

uint64_t reset_sequence::sc_fuse_val() {
  uint64_t sc_fuse = 0;

  for (uint32_t i=0; i<FLAGS_num_sc_ways/4; ++i) {
    uint32_t segment = (FLAGS_sc_way_enable_mask >> (4*i)) & 0xf;
    if (segment == 0xf)
      sc_fuse |= (1ull << i);
  }
  return sc_fuse;
}

uint64_t reset_sequence::fuse_val() {
  return core_fuse_val() | trace_fuse_val() | dm_fuse_val() | sc_fuse_val() | (1ull << lock_idx);
}


cvm::messenger::task<void> reset_sequence::program_patch() {
  co_await tick();
  uint64_t patch_header[110] = {
    0x7c906ff3,        //0x4214c000 :    	csrrsi	t6,0x7c9,0
    0x002fff93,        //0x4214c004 :    	andi	t6,t6,2
    0x7c9f9073,        //0x4214c008 :    	csrw	0x7c9,t6
    0x7b209073,        //0x4214c00c :    	csrw	dscratch0,ra
    0x7b311073,        //0x4214c010 :    	csrw	dscratch1,sp
    0x7b506ff3,        //0x4214c014 :    	csrrsi	t6,0x7b5,0
    0x00cfd093,        //0x4214c018 :    	srli	ra,t6,0xc
    0x00c09093,        //0x4214c01c :    	slli	ra,ra,0xc
    0xf1406173,        //0x4214c020 :    	csrrsi	sp,mhartid,0
    0x00717113,        //0x4214c024 :    	andi	sp,sp,7
    0x00811113,        //0x4214c028 :    	slli	sp,sp,0x8
    0x00001fb7,        //0x4214c02c :    	lui	t6,0x1
    0x600f8f9b,        //0x4214c030 :    	addiw	t6,t6,1536 # 1600 <tohost-0x6fffea00>
    0x00110133,        //0x4214c034 :    	add	sp,sp,ra
    0x01f10133,        //0x4214c038 :    	add	sp,sp,t6
    0x00312c23,        //0x4214c03c :    	sw	gp,24(sp)
    0x02412023,        //0x4214c040 :    	sw	tp,32(sp)
    0x02512423,        //0x4214c044 :    	sw	t0,40(sp)
    0x02612823,        //0x4214c048 :    	sw	t1,48(sp)
    0x02712c23,        //0x4214c04c :    	sw	t2,56(sp)
    0x04812023,        //0x4214c050 :    	sw	s0,64(sp)
    0x04912423,        //0x4214c054 :    	sw	s1,72(sp)
    0x04a12823,        //0x4214c058 :    	sw	a0,80(sp)
    0x04b12c23,        //0x4214c05c :    	sw	a1,88(sp)
    0x06c12023,        //0x4214c060 :    	sw	a2,96(sp)
    0x06d12423,        //0x4214c064 :    	sw	a3,104(sp)
    0x06e12823,        //0x4214c068 :    	sw	a4,112(sp)
    0x06f12c23,        //0x4214c06c :    	sw	a5,120(sp)
    0x09012023,        //0x4214c070 :    	sw	a6,128(sp)
    0x09112423,        //0x4214c074 :    	sw	a7,136(sp)
    0x09212823,        //0x4214c078 :    	sw	s2,144(sp)
    0x09312c23,        //0x4214c07c :    	sw	s3,152(sp)
    0x0b412023,        //0x4214c080 :    	sw	s4,160(sp)
    0x0b512423,        //0x4214c084 :    	sw	s5,168(sp)
    0x0b712823,        //0x4214c088 :    	sw	s7,176(sp)
    0x0b812c23,        //0x4214c08c :    	sw	s8,184(sp)
    0x0d912023,        //0x4214c090 :    	sw	s9,192(sp)
    0x0da12423,        //0x4214c094 :    	sw	s10,200(sp)
    0x0db12823,        //0x4214c098 :    	sw	s11,208(sp)
    0x0dc12c23,        //0x4214c09c :    	sw	t3,216(sp)
    0x0fd12023,        //0x4214c0a0 :    	sw	t4,224(sp)
    0x0fe12823,        //0x4214c0a4 :    	sw	t5,240(sp)
    0x0ff12c23,        //0x4214c0a8 :    	sw	t6,248(sp)
    0x7b206f73,        //0x4214c0ac :    	csrrsi	t5,dscratch0,0
    0x7b306ff3,        //0x4214c0b0 :    	csrrsi	t6,dscratch1,0
    0x01e12423,        //0x4214c0b4 :    	sw	t5,8(sp)
    0x01f12823,        //0x4214c0b8 :    	sw	t6,16(sp)
    0x7c8061f3,        //0x4214c0bc :    	csrrsi	gp,0x7c8,0
    0x7b006273,        //0x4214c0c0 :    	csrrsi	tp,dcsr,0
    0x1c027213,        //0x4214c0c4 :    	andi	tp,tp,448
    0x00625213,        //0x4214c0c8 :    	srli	tp,tp,0x6
    0x00621213,        //0x4214c0cc :    	slli	tp,tp,0x6
    0x00408233,        //0x4214c0d0 :    	add	tp,ra,tp
    0x40020213,        //0x4214c0d4 :    	addi	tp,tp,1024 # 400 <tohost-0x6ffffc00>
    0x7b106f73,        //0x4214c0d8 :    	csrrsi	t5,dpc,0
    0x00020067,        //0x4214c0dc :    	jr	tp # 0 <tohost-0x70000000>
    0x7c906ff3,        //0x4214c0e0 :    	csrrsi	t6,0x7c9,0
    0x002fff93,        //0x4214c0e4 :    	andi	t6,t6,2
    0x7c9f9073,        //0x4214c0e8 :    	csrw	0x7c9,t6
    0x7b509073,        //0x4214c0ec :    	csrw	0x7b5,ra
    0x00c0d093,        //0x4214c0f0 :    	srli	ra,ra,0xc
    0x00c09093,        //0x4214c0f4 :    	slli	ra,ra,0xc
    0xf1406173,        //0x4214c0f8 :    	csrrsi	sp,mhartid,0
    0x00717113,        //0x4214c0fc :    	andi	sp,sp,7
    0x00811113,        //0x4214c100 :    	slli	sp,sp,0x8
    0x00110133,        //0x4214c104 :    	add	sp,sp,ra
    0x00001fb7,        //0x4214c108 :    	lui	t6,0x1
    0x600f8f9b,        //0x4214c10c :    	addiw	t6,t6,1536 # 1600 <tohost-0x6fffea00>
    0x01f10133,        //0x4214c110 :    	add	sp,sp,t6
    0x00812f03,        //0x4214c114 :    	lw	t5,8(sp)
    0x01012f83,        //0x4214c118 :    	lw	t6,16(sp)
    0x7b2f1073,        //0x4214c11c :    	csrw	dscratch0,t5
    0x7b3f9073,        //0x4214c120 :    	csrw	dscratch1,t6
    0x01812183,        //0x4214c124 :    	lw	gp,24(sp)
    0x02012203,        //0x4214c128 :    	lw	tp,32(sp)
    0x02812283,        //0x4214c12c :    	lw	t0,40(sp)
    0x03012303,        //0x4214c130 :    	lw	t1,48(sp)
    0x03812383,        //0x4214c134 :    	lw	t2,56(sp)
    0x04012403,        //0x4214c138 :    	lw	s0,64(sp)
    0x04812483,        //0x4214c13c :    	lw	s1,72(sp)
    0x05012503,        //0x4214c140 :    	lw	a0,80(sp)
    0x05812583,        //0x4214c144 :    	lw	a1,88(sp)
    0x06012603,        //0x4214c148 :    	lw	a2,96(sp)
    0x06812683,        //0x4214c14c :    	lw	a3,104(sp)
    0x07012703,        //0x4214c150 :    	lw	a4,112(sp)
    0x07812783,        //0x4214c154 :    	lw	a5,120(sp)
    0x08012803,        //0x4214c158 :    	lw	a6,128(sp)
    0x08812883,        //0x4214c15c :    	lw	a7,136(sp)
    0x09012903,        //0x4214c160 :    	lw	s2,144(sp)
    0x09812983,        //0x4214c164 :    	lw	s3,152(sp)
    0x0a012a03,        //0x4214c168 :    	lw	s4,160(sp)
    0x0a812a83,        //0x4214c16c :    	lw	s5,168(sp)
    0x0b012b03,        //0x4214c170 :    	lw	s6,176(sp)
    0x0b812b83,        //0x4214c174 :    	lw	s7,184(sp)
    0x0c012c03,        //0x4214c178 :    	lw	s8,192(sp)
    0x0c812c83,        //0x4214c17c :    	lw	s9,200(sp)
    0x0d012d03,        //0x4214c180 :    	lw	s10,208(sp)
    0x0d812d83,        //0x4214c184 :    	lw	s11,216(sp)
    0x0e012e03,        //0x4214c188 :    	lw	t3,224(sp)
    0x0e812e83,        //0x4214c18c :    	lw	t4,232(sp)
    0x0f012f03,        //0x4214c190 :    	lw	t5,240(sp)
    0x0f812f83,        //0x4214c194 :    	lw	t6,248(sp)
    0x7b2060f3,        //0x4214c198 :    	csrrsi	ra,dscratch0,0
    0x7b306173,        //0x4214c19c :    	csrrsi	sp,dscratch1,0
    0x7c906ff3,        //0x4214c1a0 :    	csrrsi	t6,0x7c9,0
    0x003fef93,        //0x4214c1a4 :    	ori	t6,t6,3
    0x7c9f9073,        //0x4214c1a4 :    	csrw	0x7c9,t6
    0x7b200073,        //0x4214c1a8 :    	dret
    0x00000013,        //0x4214c1ac :    	nop
    0x00000013,        //0x4214c1b0 :    	nop
  };
        

  uint64_t patch_trig_0[16] = {
        0x4214cfb7,        	//lui	t6,0x4214c
        0x500f8f9b,        	//addiw	t6,t6,1280 # 4214c500 <tohost-0x2deb3b00>
        0x000f8067,        	//jr	t6
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013         //nop
  };
  uint64_t patch_trig_1[16] = {
        0x4214dfb7,        	//lui	t6,0x4214d
        0x900f8f9b,        	//addiw	t6,t6,-1792 # 4214c900 <tohost-0x2deb3700>
        0x000f8067,        	//jr	t6
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
  };
  uint64_t patch_trig_2[16] = {
        0x4214dfb7,         //lui	t6,0x4214d
        0xd00f8f9b,         //addiw	t6,t6,-768 # 4214cd00 <tohost-0x2deb3300>
        0x000f8067,         //jr	t6
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
  };
  uint64_t patch_trig_3[16] = {
        0x4214dfb7,         //lui	t6,0x4214d
        0x100f8f9b,         //addiw	t6,t6,256 # 4214d100 <tohost-0x2deb2f00>
        0x000f8067,         //jr	t6
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
        0x00000013,         //nop
  };
  uint64_t patch_body_sub[36] = {
                0x00f1d213,          	//srli	tp,gp,0xf
                0x01f27213,          	//andi	tp,tp,31
                0x0141d293,          	//srli	t0,gp,0x14
                0x01f2f293,          	//andi	t0,t0,31
                0x0071d313,          	//srli	t1,gp,0x7
                0x01f37313,          	//andi	t1,t1,31
                0x00321213,          	//slli	tp,tp,0x3
                0x00220233,          	//add	tp,tp,sp
                0x00023203,          	//ld	tp,0(tp) # 0 <tohost-0x70000000>
                0x00329293,          	//slli	t0,t0,0x3
                0x002282b3,          	//add	t0,t0,sp
                0x0002b283,          	//ld	t0,0(t0)
                0x00300f93,          	//li	t6,3
                0x7c9f9073,          	//csrw	0x7c9,t6
                0xfff2c213,          	//not	tp,t0
                0x00100f93,          	//li	t6,1
                0x01f20233,          	//add	tp,tp,t6
                0x00331313,          	//slli	t1,t1,0x3
                0x00230333,          	//add	t1,t1,sp
                0x00432023,          	//sw	tp,0(t1)
                0x004f0093,          	//addi	ra,t5,4
                0x4214cfb7,          	//lui	t6,0x4214c
                0x0e0f8f9b,          	//addiw	t6,t6,224 # 4214c0e0 <tohost-0x2deb3f20>
                0x000f8067,          	//jr	t6
                0x00000013,         //nop
                0x00000013,         //nop
                0x00000013,         //nop
                0x00000013,         //nop
                0x00000013,         //nop
                0x00000013,         //nop
                0x00000013,         //nop
                0x00000013,         //nop
                0x00000013,         //nop
                0x00000013,         //nop
              
  };
  uint64_t patch_body_subw[14] = {
      0x7b106ff3,          	//csrrsi	t6,dpc,0
      0x004f8f93,          	//addi	t6,t6,4
      0x41498933,          	//sub	s2,s3,s4
      0x7b1f9073,          	//csrw	dpc,t6
      0x00000f93,          	//li	t6,0
      0x7b200073,          	//dret
      0x00000013,         //nop
      0x00000013,         //nop
      0x00000013,         //nop
      0x00000013,         //nop
      0x00000013,         //nop
      0x00000013,         //nop
  };

  co_await batch_write(cpl_patch_ram_base, patch_header, sizeof(patch_header)/sizeof(uint64_t));
  co_await batch_write(cpl_patch_ram_ptrig_0, patch_trig_0, sizeof(patch_trig_0)/sizeof(uint64_t));
  co_await batch_write(cpl_patch_ram_ptrig_1, patch_trig_1, sizeof(patch_trig_1)/sizeof(uint64_t));
  co_await batch_write(cpl_patch_ram_ptrig_2, patch_trig_2, sizeof(patch_trig_2)/sizeof(uint64_t));
  co_await batch_write(cpl_patch_ram_ptrig_3, patch_trig_3, sizeof(patch_trig_3)/sizeof(uint64_t));
  co_await batch_write(cpl_patch_ram_pbody_0, patch_body_subw, sizeof(patch_body_subw)/sizeof(uint64_t));
  co_await batch_write(cpl_patch_ram_pbody_1, patch_body_sub, sizeof(patch_body_sub)/sizeof(uint64_t));


  for (uint32_t i=0; i<FLAGS_num_harts; ++i) {
    //co_await write(core_pversion_mmr + i * core_fuse_offset, SZ_8B, 0xD); //pversion
    co_await write(core_preg0_mmr + i * core_fuse_offset, SZ_8B, 0xFFFFFFFF4149893b);//preg0 :subw x18, x19, x20
    co_await write(core_preg1_mmr + i * core_fuse_offset, SZ_8B, 0xFE00707F40000033);//preg1 :sub
    co_await write(core_pcontrol_mmr + i * core_fuse_offset, SZ_8B, 0x83FF83FF);//pcontrol
    //co_await write(core_pcontrol_mmr + i * core_fuse_offset, SZ_8B, 0x8001);//pcontrol
    co_await write(core_ptvec_csr + i * core_fuse_offset, SZ_8B, 0x210C00000); //Program PtVec register
  };

  co_return;
}

cvm::messenger::task<void> reset_sequence::batch_write(uint32_t start_addr, uint64_t data[], int size) {
    cvm::log(cvm::HIGH, "[SMC_XTOR] Programming Patch RAM : Address {:x}, instr count {} \n", start_addr, size);  
    for (int i=0;i< size;i=i+2){
        co_await write(start_addr+(i*4), SZ_8B, (data[i+1]<<32)|data[i]);
    }   
  co_return;
};


