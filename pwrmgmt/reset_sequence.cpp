#include "reset_sequence.hpp"
#include "rand_gflags.h"

REGISTRY_register(reset_sequence, PWRMGMT, cvm::registry::all);

DEFINE_uint32(pll_pwrup_timeout, 50, "Number of soc cycles expected for pll to be stable");
DEFINE_string(warm_reset, "off", "Enable warm resets in the sim - off/directed/random/trigger");
DEFINE_string(warm_reset_count, "1", "Number of warm resets in the sim if random mode enabled");
DEFINE_string(warm_reset_interval, "5000", "TB cycle interval between warm resets in the sim if random mode enabled");
DEFINE_string(warm_reset_trigger_type, "", "Send warm reset on a trigger");
DEFINE_string(warm_reset_trigger_interval, "0:100", "TB cycle interval from trigger to warm reset");
DEFINE_string(warm_reset_sram_hold, "0:1", "Sram hold");
DEFINE_string(warm_reset_debug_hold, "0:1", "Debug hold");
DEFINE_string(warm_reset_critical_hold, "0:1", "Critical hold");

namespace gflags {
  std::vector<std::string> warm_reset_rand_flags = {
    "warm_reset_count", "warm_reset_interval", "warm_reset_trigger_interval", "warm_reset_sram_hold", "warm_reset_debug_hold", "warm_reset_critical_hold"
  };

  rand warm_reset_rand(warm_reset_rand_flags);
}

extern "C" {
  std::uint32_t warm_reset_rand_get(const char* p) {
    return gflags::warm_reset_rand.get(p);
  }
  void pwrmgmt_init();
  void pwrmgmt_cold_reset(uint8_t val);
  void pwrmgmt_warm_reset(uint8_t val);
  void pwrmgmt_reset_hold(uint8_t sram, uint8_t debug, uint8_t critical);
  void pwrmgmt_force_ref_clk(uint8_t val);
}

reset_sequence::reset_sequence(cvm::topology::loc_t loc, unsigned) : loc_(loc), scope_(nullptr) {
  cvm::log(cvm::NONE, "[pwrmgmt] loc={}\n", loc_);

  // Topology
  nharts_ = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  smc_axi_loc_ = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_SMC_MST", 0);

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });

  // Sequence threads
  cold_reset_sequence_thread();
  if (FLAGS_warm_reset == "random") {
    gflags::warm_reset_rand.randomize();
    warm_reset_random_mode_sequence_thread();
  } else if (FLAGS_warm_reset == "trigger") {
    warm_reset_trigger_mode_sequence_thread();
  }
}

void reset_sequence::cold_reset_sequence_thread() {
  auto *task = +[] (reset_sequence* m) -> cvm::messenger::task<void> {
    cvm::log(cvm::NONE, "[pwrmgmt] spawning cold reset sequence\n");
    co_await m->cold_reset_sequence();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void reset_sequence::warm_reset_random_mode_sequence_thread() {
  auto *task = +[] (reset_sequence* m) -> cvm::messenger::task<void> {
    cvm::log(cvm::NONE, "[pwrmgmt] spawning warm reset sequence\n");
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
  // Wait for first clock tick
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

  // Check and clear pll status
  co_await check_pll_status();
  co_await clear_pll_status();

  co_await cpl_reset_sequence(COLD);

  co_return;
}

cvm::messenger::task<void> reset_sequence::warm_reset_random_mode_sequence() {
  for (uint32_t i = 0; i < gflags::warm_reset_rand.get("warm_reset_count"); ++i) {
    // Wait on nofetch deassertion
    co_await nofetch();

    // Randomize state
    gflags::warm_reset_rand.randomize();

    // Wait for next tick
    co_await tick();

    co_await warm_reset_sequence();
  }
}

cvm::messenger::task<void> reset_sequence::warm_reset_trigger_mode_sequence() {
  // Wait on nofetch deassertion
  co_await nofetch();

  // Wait for next selected trigger
  co_await trigger();

  // Wait for tick after trigger
  co_await tick();

  co_await warm_reset_sequence();
}

cvm::messenger::task<void> reset_sequence::warm_reset_sequence() {
  // Wait for next clock tick
  co_await tick();

  // Assert force_ref_clk
  force_ref_clk(1);

  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
    co_await tick();

  // Assert holds
  reset_hold(
    gflags::warm_reset_rand.get("warm_reset_sram_hold"),
    gflags::warm_reset_rand.get("warm_reset_debug_hold"),
    gflags::warm_reset_rand.get("warm_reset_critical_hold")
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

  co_return;
}

cvm::messenger::task<void> reset_sequence::cpl_reset_sequence(rst_t ) {
  // Program reset controller
  co_await release_cpl_reset();
  co_await program_fuses();
  co_await release_cpl_nofetch();

  // Deassert force_ref_clk
  force_ref_clk(0);

  // Wait for 16 clock ticks
  for (int i=0; i<16; ++i)
    co_await tick();

  co_return;
}

cvm::messenger::task<void> reset_sequence::check_pll_status() {
  uint32_t count = 0;
  while (true) {
    co_await tick();
    auto data = co_await read(pll_interrupts, SZ_4B);
    if (data & (1 << cold_powerup_done))
      break;

    count++;
    if (count > FLAGS_pll_pwrup_timeout)
      cvm::log(cvm::ERROR, "Error: PLL cold power up not done after {} soc clocks\n", FLAGS_pll_pwrup_timeout);
  }

  co_return;
}

cvm::messenger::task<void> reset_sequence::clear_pll_status() {
  co_await tick();
  co_await write(pll_interrupts, SZ_4B, (1 << cold_powerup_done));
  co_return;
}

cvm::messenger::task<void> reset_sequence::release_cpl_reset() {
  co_await tick();
  co_await write(rst_ctl_warm, SZ_4B, (1 << cpl_cl_warm_reset_n));

  co_return;
}

cvm::messenger::task<void> reset_sequence::program_fuses() {
  co_await tick();

  for (uint32_t i=0; i<nharts_; ++i)
    co_await write(fuse_core_mmr + i * fuse_hart_offset,   SZ_8B, 0x18700);

  co_await write(fuse_trace_mmr,  SZ_8B, 0x18700);
  co_await write(fuse_aclint_mmr, SZ_8B, 0x18700);
  co_await write(fuse_dm_mmr,     SZ_8B, 0x18700);
  co_await write(fuse_sc_mmr,     SZ_8B, 0x18700);
  //co_await write(fuse_sw_mmr,     SZ_8B, 0x18700);

  co_return;
}

cvm::messenger::task<void> reset_sequence::release_cpl_nofetch() {
  co_await tick();
  co_await write(rst_ctl_nofetch, SZ_4B, (0 << cpl_cl_no_fetch));
}

cvm::messenger::task<void> reset_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::pwrmgmt::m_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> reset_sequence::nofetch() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::pwrmgmt::m_nofetch<>>(loc_);
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
      cvm::log(cvm::NONE, "[pwrmgmt] assert cold_reset\n");
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

