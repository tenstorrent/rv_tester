#include "dfs_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "rv_tester/rv_tester_plusargs.h"
#include "fmt/ranges.h"
#include "pwrmgmt.hpp"

// DPI import to control clock mode from SystemVerilog
// extern "C" void rv_tester_set_clock_mode(unsigned int new_clock_mode); // TODO: Fix DPI export visibility issue

// DPI import to control clock mode from SystemVerilog
// extern "C" void rv_tester_set_clock_mode(unsigned int new_clock_mode); // TODO: Fix DPI export visibility issue

REGISTRY_register(dfs_sequence, PWRMGMT, cvm::registry::all);

DEFINE_bool(dfs_rand_en, false, "Enable random dfs injection in the sim");
DEFINE_string(dfs_count, "200:250", "Number of dfs sequences in the sim");
DEFINE_string(dfs_interval, "5000:10000", "soc cycle interval between dfs sequences in the sim");
DEFINE_string(dfs_range_mhz, "1000:2700", "Range of core clock frequency supported");
DEFINE_string(dfs_width, "20:20", "soc cycle width of dfs sequences in the sim");


dfs_sequence::dfs_sequence
  (cvm::topology::loc_t loc, unsigned) : 
  loc_(loc), dfs_read_count_(0), dfs_write_count_(0) {

  // Topology
  smc_axi_loc_ = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_SMC_MST", 0);
  
  // main sequence thread
  if (FLAGS_dfs_rand_en)
    main_thread();
}

dfs_sequence::~dfs_sequence() {
  if (FLAGS_metrics) {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"rand_dfs_read_count\": \"{}\"}}\n", dfs_read_count_);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"rand_dfs_write_count\": \"{}\"}}\n", dfs_write_count_);
  }

}

void dfs_sequence::main_thread() {
  auto *task = +[] (dfs_sequence* m) -> cvm::messenger::task<void> {
    co_await m->main();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> dfs_sequence::main() {
  co_await tick();
  while (true) {
    co_await dfs_write();
    co_await tick();
  }
  co_return;
}

cvm::messenger::task<void> dfs_sequence::dfs_write() {
  cvm::log(cvm::HIGH, "Initialing DFS ");
  
  // Check PLL status using direct bit manipulation
  auto pll_status_data = co_await read(pll_status, SZ_4B);
  uint32_t status_reg = static_cast<uint32_t>(pll_status_data);
  
  // Check if PLLs are both active and locked for DFS operation
  bool pll0_active = (status_reg >> PLL0_ACTIVE_BIT) & 1;
  bool pll0_locked = (status_reg >> PLL0_LOCKED_BIT) & 1;
  bool pll1_active = (status_reg >> PLL1_ACTIVE_BIT) & 1;
  bool pll1_locked = (status_reg >> PLL1_LOCKED_BIT) & 1;
  
  bool pll0_usable = pll0_active && pll0_locked;
  bool pll1_usable = pll1_active && pll1_locked;

  if (pll0_usable) {
    cvm::log(cvm::MEDIUM, "[dfs_sequence] PLL0 is active and locked\n");
  } else if (pll1_usable) {
    cvm::log(cvm::MEDIUM, "[dfs_sequence] PLL1 is active and locked\n");
  } else {
    cvm::log(cvm::ERROR, "[dfs_sequence] Error: No PLLs are active and locked (PLL0: active=%d locked=%d, PLL1: active=%d locked=%d)\n", 
             static_cast<int>(pll0_active), static_cast<int>(pll0_locked), 
             static_cast<int>(pll1_active), static_cast<int>(pll1_locked));
    co_return; // Early return to prevent DFS operation when no usable PLLs
  }

  uint32_t clock_profile = (rand()%6)+1;
  auto loc = cvm::topology::get_from_type("CLKI", 0);
  uint32_t pll_dfs_freq = cvm::topology::list_attr(loc, fmt::format("PROFILE{}_CLOCK_FREQ_MHZ",clock_profile)).second[1];
  uint32_t ref_clk_freq = cvm::topology::list_attr(loc, fmt::format("PROFILE{}_CLOCK_FREQ_MHZ",clock_profile)).second[4];
  cvm::log(cvm::MEDIUM, "[dfs_sequence] Core frequency is being changed to {} MHz based on clk_profile {} and ref_clk_freq {}\n", pll_dfs_freq, clock_profile, ref_clk_freq);
  
  // Set the clock mode in SystemVerilog to match the selected profile
  // rv_tester_set_clock_mode(clock_profile); // TODO: Fix DPI export visibility issue
  
  // Calculate PLL parameters using common utility function
  auto params = pll_utils::calculate_pll_params(pll_dfs_freq, ref_clk_freq);
  
#ifdef MOVELLUS_PLL_MODEL
  cvm::log(cvm::MEDIUM, "[dfs_sequence] Using Movellus PLL: fcw_int={}, postdiv={} for {} MHz\n", 
           params.fcw_int, params.postdiv, pll_dfs_freq);
  
  // Configure PLL Parameters1 register (fcw_int)
  auto rdata1 = co_await read(pll_parameters1, SZ_4B);
  auto new_params1 = pll_utils::configure_pll_parameters1(static_cast<uint32_t>(rdata1), params);
  co_await write(pll_parameters1, SZ_4B, new_params1);
  
  // Configure PLL Parameters0 register (postdiv)
  auto rdata0 = co_await read(pll_parameters0, SZ_4B);
  auto new_params0 = pll_utils::configure_pll_parameters0(static_cast<uint32_t>(rdata0), params);
  co_await write(pll_parameters0, SZ_4B, new_params0);
  
#else
  // Generic PLL model
  cvm::log(cvm::MEDIUM, "[dfs_sequence] Using Generic PLL: freq_ratio={} for {} MHz\n", 
           params.freq_ratio, pll_dfs_freq);
  
  auto rdata0 = co_await read(pll_parameters0, SZ_4B);
  auto new_params0 = pll_utils::configure_pll_parameters0(static_cast<uint32_t>(rdata0), params);
  co_await write(pll_parameters0, SZ_4B, new_params0);
#endif

  // Set up control register for DFS request using direct bit manipulation
  uint32_t control_reg = 0;
  control_reg |= (1 << PLL_DFS_REQ_BIT);  // Set DFS request bit
  // dis_inactive_pll_shutdown is 0 by default (bit not set)
  co_await write(pll_control, SZ_4B, static_cast<uint64_t>(control_reg));

  for (uint32_t i=0; i<FLAGS_pll_dfs_timeout; i++) {
    co_await tick();
  }  
  
  // Check DFS completion using direct bit manipulation
  auto interrupt_data = co_await read(pll_interrupts, SZ_4B);
  uint32_t interrupt_reg = static_cast<uint32_t>(interrupt_data);
  bool dfs_done = (interrupt_reg >> DFS_DONE_BIT) & 1;
  
  if (!dfs_done) {
    cvm::log(cvm::ERROR, "Error: PLL dfs not done after {} soc clocks\n", FLAGS_pll_dfs_timeout);
  } else {
    cvm::log(cvm::MEDIUM, "[dfs_sequence] Core frequency change successful. DFS complete\n");
    
    // Clear pll_dfs_req bit after successful DFS completion
    uint32_t clear_control_reg = control_reg & ~(1 << PLL_DFS_REQ_BIT);
    // pll_dfs_req bit is already 0 in clear_control_reg, so just write it
    co_await write(pll_control, SZ_4B, static_cast<uint64_t>(clear_control_reg));
    cvm::log(cvm::MEDIUM, "[dfs_sequence] Cleared pll_dfs_req bit after DFS completion\n");
  }
  co_return;
}

cvm::messenger::task<void> dfs_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::pwrmgmt::m_dfs_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<uint64_t> dfs_sequence::read(uint64_t addr, size_t sz, block_t block /* = BLOCK */) {
  assert(sz <= 8);
  cvm::log(cvm::MEDIUM, "[smc] read req - addr={:#x}, sz={}\n", addr, sz);
  cvm::registry::messenger.signal(smc_axi_loc_, transactor::read_request_t{addr, sz});
  dfs_read_count_++;

  if (!block)
    co_return 0;

  auto resp = co_await cvm::registry::messenger.wait<transactor::read_response_t>(smc_axi_loc_);
  auto data1 = convert_to_dword_array(resp.data);
  // FIXME - check why this alignment is needed
  uint64_t dword = (addr % 8) ? (data1[0] >> 32) : data1[0];
  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  dword &= mask;
  cvm::log(cvm::MEDIUM, "[smc] read resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data1[0], dword, mask);
  co_return dword;
}

cvm::messenger::task<void> dfs_sequence::write(uint64_t addr, size_t sz, uint64_t data, block_t block /* = BLOCK */) {
  assert(sz <= 8);
  // FIXME - check why this alignment is needed
  uint64_t dword = (addr % 8) ? (data << 32) : data;
  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  mask = (addr % 8) ? (mask << 32) : mask;
  auto byte_array = convert_to_byte_array({dword});
  std::vector<bool> strb(8, false);
  for(int i=0; i<8; ++i)
    strb[i] = (mask & (0xFFull << (i*8))) != 0;
  cvm::log(cvm::MEDIUM, "[smc] write req - addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", addr, sz, data, dword, mask);
  cvm::registry::messenger.signal(smc_axi_loc_, transactor::write_request_t{addr, sz, byte_array, strb});
  dfs_write_count_++;

  if (!block)
    co_return;

  auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(smc_axi_loc_);
  cvm::log(cvm::MEDIUM, "[smc] write resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data, dword, mask);
  co_return;
}

std::vector<uint64_t> dfs_sequence::convert_to_dword_array(const std::vector<uint8_t>& byte_array) {
  std::vector<uint64_t> result(byte_array.size() / sizeof(uint64_t));
  std::copy(reinterpret_cast<const uint64_t*>(byte_array.data()),
            reinterpret_cast<const uint64_t*>(byte_array.data() + byte_array.size()),
            result.begin());
  return result;
}

std::vector<uint8_t> dfs_sequence::convert_to_byte_array(const std::vector<uint64_t>& dword_array) {
  std::vector<uint8_t> result(dword_array.size() * sizeof(uint64_t));
  std::copy(reinterpret_cast<const uint8_t*>(dword_array.data()),
            reinterpret_cast<const uint8_t*>(dword_array.data()) + dword_array.size() * sizeof(uint64_t),
            result.begin());
  return result;
}
