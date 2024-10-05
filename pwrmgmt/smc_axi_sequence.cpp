#include "smc_axi_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "fmt/ranges.h"

REGISTRY_register(smc_axi_sequence, PWRMGMT, cvm::registry::all);

DEFINE_bool(rand_smc_axi, false, "Enable random smc axi accesses in the sim");
DEFINE_string(smc_axi_count, "5:5", "Number of smc axi sequences in the sim");
DEFINE_string(smc_axi_interval, "100:100", "soc cycle interval between smc axi sequences in the sim");
DEFINE_string(smc_axi_width, "2:2", "soc cycle width of smc axi sequences in the sim");

smc_axi_sequence::smc_axi_sequence
  (cvm::topology::loc_t loc, unsigned) : 
  loc_(loc), smc_axi_read_count_(0), smc_axi_write_count_(0) {

  // Topology
  smc_axi_loc_ = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_SMC_MST", 0);

  if (!FLAGS_rand_smc_axi)
    return;

  // smc_rand sequence thread
  main_thread();
}

smc_axi_sequence::~smc_axi_sequence() {
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"rand_smc_axi_read_count\": \"{}\"}}\n", smc_axi_read_count_);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"rand_smc_axi_write_count\": \"{}\"}}\n", smc_axi_write_count_);
}

void smc_axi_sequence::main_thread() {
  auto *task = +[] (smc_axi_sequence* m) -> cvm::messenger::task<void> {
    co_await m->main();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> smc_axi_sequence::main() {
  cvm::rand::uniform_dist<int> smc_dist(0, 1);

  while (true) {
    co_await tick();

    switch (smc_dist()) {
      case 0: co_await scratchpad_write(); break;
      case 1: co_await sram_write(); break;
    }
  }
  co_return;
}

cvm::messenger::task<void> smc_axi_sequence::scratchpad_write() {
  cvm::rand::uniform_dist<uint32_t> index_dist(0, smc_scratchpad_address.size() - 1);
  uint32_t rand_scratchpad_addr = smc_scratchpad_address[index_dist()];
  uint64_t data = 0xA5A5A5A5;
  for (const auto& addr : {rc_scratchpad, cc_scratchpad}) {
    if (rand_scratchpad_addr == addr) {
      co_await write(addr, SZ_4B, data, NO_BLOCK);
      co_return;
    }
  }
  data = 0xA5A5A5A5A5A5A5A5;
  for (const auto& addr : {dm_scratchpad, cr_scratchpad, sw_scratchpad, ac_scratchpad, mb_scratchpad}) {
    if (rand_scratchpad_addr == addr) {
      co_await write(addr, SZ_8B, data, NO_BLOCK);
      co_return;
    }
  }
}

cvm::messenger::task<void> smc_axi_sequence::sram_write() {
  cvm::rand::uniform_dist<uint32_t> addr_dist(0x40000, 0x4BFFF);
  uint32_t rand_sram_addr = 0x42100000 + (addr_dist() & 0xFFFF8);
  uint64_t data = 0xA5A5A5A5A5A5A5A5;
  co_await write(rand_sram_addr, SZ_8B, data, NO_BLOCK);
  co_return;
}

cvm::messenger::task<void> smc_axi_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::pwrmgmt::m_rand_smc_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<uint64_t> smc_axi_sequence::read(uint64_t addr, size_t sz, block_t block /* = BLOCK */) {
  assert(sz <= 8);
  cvm::log(cvm::MEDIUM, "[smc] read req - addr={:#x}, sz={}\n", addr, sz);
  cvm::registry::messenger.signal(smc_axi_loc_, transactor::read_request_t{addr, sz});
  smc_axi_read_count_++;

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

cvm::messenger::task<void> smc_axi_sequence::write(uint64_t addr, size_t sz, uint64_t data, block_t block /* = BLOCK */) {
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
  smc_axi_write_count_++;

  if (!block)
    co_return;

  auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(smc_axi_loc_);
  cvm::log(cvm::MEDIUM, "[smc] write resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data, dword, mask);
  co_return;
}

std::vector<uint64_t> smc_axi_sequence::convert_to_dword_array(const std::vector<uint8_t>& byte_array) {
  std::vector<uint64_t> result(byte_array.size() / sizeof(uint64_t));
  std::copy(reinterpret_cast<const uint64_t*>(byte_array.data()),
            reinterpret_cast<const uint64_t*>(byte_array.data() + byte_array.size()),
            result.begin());
  return result;
}

std::vector<uint8_t> smc_axi_sequence::convert_to_byte_array(const std::vector<uint64_t>& dword_array) {
  std::vector<uint8_t> result(dword_array.size() * sizeof(uint64_t));
  std::copy(reinterpret_cast<const uint8_t*>(dword_array.data()),
            reinterpret_cast<const uint8_t*>(dword_array.data()) + dword_array.size() * sizeof(uint64_t),
            result.begin());
  return result;
}
