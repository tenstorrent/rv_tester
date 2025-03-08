#include "patch_control_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "rv_tester/rv_tester_plusargs.h"
#include "fmt/ranges.h"

REGISTRY_register(patch_control_sequence, PWRMGMT, cvm::registry::all);

DEFINE_bool(pcontrol_rand_en, false, "Enable random smc axi accesses in the sim");
DEFINE_string(pcontrol_count, "200:250", "Number of smc axi sequences in the sim");
DEFINE_string(pcontrol_interval, "10000:10000", "soc cycle interval between smc axi sequences in the sim");
DEFINE_string(pcontrol_width, "1:1", "soc cycle width of smc axi sequences in the sim");

patch_control_sequence::patch_control_sequence
  (cvm::topology::loc_t loc, unsigned) : 
  loc_(loc), pcontrol_read_count_(0), pcontrol_write_count_(0) {

  // Topology
  smc_axi_loc_ = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_SMC_MST", 0);
  
  uint32_t num_harts = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  FLAGS_pcontrol_width = fmt::format("{}:{}",num_harts, num_harts);

  // main sequence thread
  if (FLAGS_pcontrol_rand_en)
    main_thread();
}

patch_control_sequence::~patch_control_sequence() {
  if (FLAGS_metrics) {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"rand_pcontrol_read_count\": \"{}\"}}\n", pcontrol_read_count_);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"rand_pcontrol_write_count\": \"{}\"}}\n", pcontrol_write_count_);
  }

}

void patch_control_sequence::main_thread() {
  auto *task = +[] (patch_control_sequence* m) -> cvm::messenger::task<void> {
    co_await m->main();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> patch_control_sequence::main() {
  co_await tick();
  pcontrol_data = co_await read(core_pcontrol_mmr , SZ_8B, BLOCK);
  pcontrol_enable_mask = pcontrol_data & 0x1000100010001;
  while (true) {
    co_await pcontrol_write();
    co_await tick();
  }
  co_return;
}

cvm::messenger::task<void> patch_control_sequence::pcontrol_write() {
  cvm::log(cvm::MEDIUM, "pcontrol_data = 0x{:x}, pcontrol_enable_mask = 0x{:x}\n", pcontrol_data, pcontrol_enable_mask);
  pcontrol_data = (pcontrol_data ^ pcontrol_enable_mask);
  for (uint32_t i=0; i< FLAGS_num_harts; i++) {
    uint32_t offset = i * core_fuse_offset;
    co_await write(core_pcontrol_mmr + offset, SZ_8B, pcontrol_data , NO_BLOCK);
  };
  co_return;
}

cvm::messenger::task<void> patch_control_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::pwrmgmt::m_pcontrol_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<uint64_t> patch_control_sequence::read(uint64_t addr, size_t sz, block_t block /* = BLOCK */) {
  assert(sz <= 8);
  cvm::log(cvm::MEDIUM, "[smc] read req - addr={:#x}, sz={}\n", addr, sz);
  cvm::registry::messenger.signal(smc_axi_loc_, transactor::read_request_t{addr, sz});
  pcontrol_read_count_++;

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

cvm::messenger::task<void> patch_control_sequence::write(uint64_t addr, size_t sz, uint64_t data, block_t block /* = BLOCK */) {
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
  cvm::registry::messenger.signal(smc_axi_loc_, transactor::write_request_t{addr, SZ_8B, byte_array, strb});
  pcontrol_write_count_++;

  if (!block)
    co_return;

  auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(smc_axi_loc_);
  cvm::log(cvm::MEDIUM, "[smc] write resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data, dword, mask);
  co_return;
}

std::vector<uint64_t> patch_control_sequence::convert_to_dword_array(const std::vector<uint8_t>& byte_array) {
  std::vector<uint64_t> result(byte_array.size() / sizeof(uint64_t));
  std::copy(reinterpret_cast<const uint64_t*>(byte_array.data()),
            reinterpret_cast<const uint64_t*>(byte_array.data() + byte_array.size()),
            result.begin());
  return result;
}

std::vector<uint8_t> patch_control_sequence::convert_to_byte_array(const std::vector<uint64_t>& dword_array) {
  std::vector<uint8_t> result(dword_array.size() * sizeof(uint64_t));
  std::copy(reinterpret_cast<const uint8_t*>(dword_array.data()),
            reinterpret_cast<const uint8_t*>(dword_array.data()) + dword_array.size() * sizeof(uint64_t),
            result.begin());
  return result;
}
