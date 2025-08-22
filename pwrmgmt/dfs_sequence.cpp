#include "dfs_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "rv_tester/rv_tester_plusargs.h"
#include "fmt/ranges.h"

REGISTRY_register(dfs_sequence, PWRMGMT, cvm::registry::all);

DEFINE_bool(dfs_rand_en, false, "Enable random dfs injection in the sim");
DEFINE_string(dfs_count, "200:250", "Number of dfs sequences in the sim");
DEFINE_string(dfs_interval, "5000:10000", "soc cycle interval between dfs sequences in the sim");
DEFINE_string(dfs_range_mhz, "1000:2700", "Range of core clock frequency supported");
DEFINE_string(dfs_width, "1:1", "soc cycle width of dfs sequences in the sim");


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
  FLAGS_pll_dfs_timeout = 5;
  while (true) {
    co_await dfs_write();
    co_await tick();
  }
  co_return;
}

cvm::messenger::task<void> dfs_sequence::dfs_write() {
  cvm::log(cvm::HIGH, "Initialing DFS ");
  pll_status_reg_s reg_status;
  pll_control_reg_s reg_control;
  pll_interrupts_reg_s reg_interrupts;
  auto data = co_await read(pll_status, SZ_4B);
  uint64_t pll_parameter_reg = 0;
  reg_status.unpack(static_cast<uint32_t>(data));
  uint8_t pll_under_dfs = 0;
  if (reg_status.pll0_locked) {
    cvm::log(cvm::MEDIUM, "[dfs_sequence] PLL0 is active\n");
    pll_parameter_reg = pll_parameters1;
    pll_under_dfs = 1;
  } else if (reg_status.pll1_locked){
    cvm::log(cvm::MEDIUM, "[dfs_sequence] PLL1 is active\n");
    pll_parameter_reg = pll_parameters0;
    pll_under_dfs = 0;
  } else {
    cvm::log(cvm::ERROR, "Error: No PLLs active\n");
  }

  uint32_t clock_profile = (rand()%6)+1;
  auto loc = cvm::topology::get_from_type("CLKI", 0);
  uint32_t pll_dfs_freq = cvm::topology::list_attr(loc, fmt::format("PROFILE{}_CLOCK_FREQ_MHZ",clock_profile)).second[1];
  cvm::log(cvm::MEDIUM, "[dfs_sequence] Core frequency is being changed to {} based on clk_profile {}\n", pll_dfs_freq, clock_profile);
  
  uint32_t freq_ratio = 2400 / pll_dfs_freq;
  co_await write(pll_parameter_reg, SZ_4B, (1 << scalar_div_idx | 52 << main_divider_div_idx | freq_ratio << pre_divider_div_idx));
  reg_control.pll_dfs_req = 1;
  reg_control.pll_sel_override = 1;
  reg_control.pll_sel = 4 + pll_under_dfs;
  reg_control.dis_inactive_pll_shutdown = 0;
  co_await write(pll_control, SZ_4B, static_cast<uint64_t>(reg_control.pack()));

  uint32_t count = 0;
  while (true) {
    co_await tick();
    auto data = co_await read(pll_interrupts, SZ_4B);
    reg_interrupts.unpack(static_cast<uint32_t>(data));
    if (reg_interrupts.dfs_done)
      break;

    count++;
    if (count > FLAGS_pll_dfs_timeout)
      cvm::log(cvm::ERROR, "PLL dfs not done after {} soc clocks\n", FLAGS_pll_dfs_timeout);
  }
  cvm::log(cvm::MEDIUM, "[dfs_sequence] Core frequency change successful. DFS complete\n");
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
  auto data = convert_to_dword_array(resp.data);
  // FIXME - check why this alignment is needed
  uint64_t dword = (addr % 8) ? (data[0] >> 32) : data[0];
  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  dword &= mask;
  cvm::log(cvm::MEDIUM, "[smc] read resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data[0], dword, mask);
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
