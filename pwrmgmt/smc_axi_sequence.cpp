#include "smc_axi_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "rv_tester/rv_tester_plusargs.h"
#include "fmt/ranges.h"

REGISTRY_register(smc_axi_sequence, PWRMGMT, cvm::registry::all);

DEFINE_bool(smc_axi_sp_rand_en, false, "Enable random smc axi accesses in the sim");
DEFINE_string(smc_axi_sp_count, "5:5", "Specifies the number of sets of SMC AXI accesses in the simulation.");
DEFINE_string(smc_axi_sp_interval, "100:100", "Specifies the SOC cycle interval between each set of SMC AXI transactions in the simulation.");
DEFINE_string(smc_axi_sp_width, "2:2", "Specifies the number of SMC AXI transactions per set in the simulation.");

DEFINE_bool(smc_axi_csr_rand_en, false, "Enable random smc axi accesses in the sim");
DEFINE_string(smc_axi_csr_count, "5:5", "Specifies the number of sets of SMC AXI accesses in the simulation.");
DEFINE_string(smc_axi_csr_interval, "100:100", "Specifies the SOC cycle interval between each set of SMC AXI transactions in the simulation.");
DEFINE_string(smc_axi_csr_width, "1:1", "Specifies the number of SMC AXI transactions per set in the simulation.");

extern "C" {
  void smc_axi_blocking_sequence_tick(uint8_t val);
}

smc_axi_sequence::smc_axi_sequence
  (cvm::topology::loc_t loc, unsigned) : 
  loc_(loc), scope_(nullptr), smc_axi_read_count_(0), smc_axi_write_count_(0) {

  // Topology
  smc_axi_loc_ = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_SMC_MST", 0);

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });

  // Channels
  r_channel_ = cvm::registry::messenger.channel<axi::r_t>(smc_axi_loc_);
  b_channel_ = cvm::registry::messenger.channel<axi::b_t>(smc_axi_loc_);

  // smc axi sequence threads
  if (FLAGS_smc_axi_sp_rand_en) {
    scratchpad_write_thread();
    scratchpad_read_thread();
  }

  if (FLAGS_smc_axi_csr_rand_en) {
    csr_access_thread();
  }
}

smc_axi_sequence::~smc_axi_sequence() {
  if (FLAGS_metrics) {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"smc_axi_read_count\": \"{}\"}}\n", smc_axi_read_count_);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"smc_axi_write_count\": \"{}\"}}\n", smc_axi_write_count_);
  }
}

void smc_axi_sequence::scratchpad_write_thread() {
  auto *task = +[] (smc_axi_sequence* m) -> cvm::messenger::task<void> {
    co_await m->scratchpad_write();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void smc_axi_sequence::scratchpad_read_thread() {
  auto *task = +[] (smc_axi_sequence* m) -> cvm::messenger::task<void> {
    co_await m->scratchpad_read();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void smc_axi_sequence::csr_access_thread() {
  auto *task = +[] (smc_axi_sequence* m) -> cvm::messenger::task<void> {
    co_await m->csr_access();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> smc_axi_sequence::scratchpad_write() {
  while (true) {
    co_await scratchpad_tick();

    cvm::rand::uniform_dist<uint32_t> smc_mmr_index_dist(0, smc_scratchpad_info.size() - 1);
    uint32_t sp_indx = smc_mmr_index_dist();
    uint32_t addr = smc_scratchpad_info[sp_indx].addr;
    uint64_t data = smc_scratchpad_info[sp_indx].data; //FIXME Shouldn't this be random?
    size_t sz = smc_scratchpad_info[sp_indx].sz;

    cvm::log(cvm::FULL, "[smc_axi] Scratchpad-MMR write req - addr={:#x}, data={:#x} \n", addr, data);

    unsigned id;
    co_await write(id, addr, sz, data, NO_BLOCK);

    // Save sp_idx to trigger read
    sp_inflight_writes_[id] = sp_indx;
  }
  co_return;
}

cvm::messenger::task<void> smc_axi_sequence::scratchpad_read() {
  while (true) {
    auto resp = co_await cvm::registry::messenger.wait<axi::b_t>(b_channel_, [this](const auto& b) { return sp_inflight_writes_.find(b.id) != sp_inflight_writes_.end(); });

    // Wait on write response and trigger read
    uint32_t sp_indx = sp_inflight_writes_[resp.id];
    uint32_t addr = smc_scratchpad_info[sp_indx].addr;
    size_t sz = smc_scratchpad_info[sp_indx].sz;

    cvm::log(cvm::FULL, "[smc_axi] Scratchpad-MMR read req - addr={:#x}\n", addr);

    unsigned id;
    co_await read(id, addr, sz, NO_BLOCK);

    // Erase the id
    sp_inflight_writes_.erase(resp.id);
  }
  co_return;
}

cvm::messenger::task<void> smc_axi_sequence::csr_access() {
  while (true) {
    co_await csr_tick();

    // Since the sequence needs blocking reads, use the blocking
    // function markers to inhibit random tick count
    blocking_seq_tick(1);

    cvm::rand::uniform_dist<uint32_t> smc_csr_index_dist(0, 1);
    uint32_t csr_indx = smc_csr_index_dist();
    uint32_t addr = smc_csr_info[csr_indx].addr;
    size_t sz = smc_csr_info[csr_indx].sz;

    co_await csr_write(addr, sz);
    co_await csr_read(addr, sz);

    blocking_seq_tick(0);
  }
}

cvm::messenger::task<void> smc_axi_sequence::csr_write(uint32_t addr, size_t sz_CsrDataPort) {
  size_t sz_CsrCommandPort = SZ_8B;
  uint64_t data = 0; //FIXME Shouldn't this be random?

  uint32_t core_id = 0;
  uint32_t unit = 0x8;
  uint64_t cmd = 0;
  uint32_t offset = core_id * core_fuse_offset;
  uint64_t wr = 0x1;
  uint64_t en = 0x1;
  bool CommandPort_Busy = false;

  cvm::log(cvm::FULL, "[smc_axi] csr write req - core_id = {}, addr={:#x}, data={:#x} \n", core_id, addr, data);

  unsigned id;
  co_await write(id, core_crCsrDataPort + offset, sz_CsrDataPort, data);
  cmd = en<<62 | wr<<61 | unit<<12 | addr;
  co_await write(id, core_crCsrCommandPort + offset, sz_CsrCommandPort, cmd);

  do {
    cmd = co_await read(id, core_crCsrCommandPort + offset, sz_CsrCommandPort);
    CommandPort_Busy = (cmd>>63) == 1;
  } while (CommandPort_Busy);

  co_return;
}

cvm::messenger::task<uint64_t> smc_axi_sequence::csr_read(uint32_t addr, size_t sz_CsrDataPort) {
  size_t sz_CsrCommandPort = SZ_8B;
  uint32_t core_id = 0;
  uint32_t unit = 0x8;
  uint64_t cmd = 0;
  uint32_t offset = core_id * core_fuse_offset;
  uint64_t wr = 0x1;
  uint64_t en = 0x1;
  bool CommandPort_Busy = false;

  cvm::log(cvm::FULL, "[smc_axi] csr read req - core_id = {}, addr={:#x} \n", core_id, addr);
  cmd = en<<62 | wr<<61 | unit<<12 | addr;

  unsigned id;
  co_await write(id, core_crCsrCommandPort + offset, sz_CsrCommandPort, cmd);
  do {
    cmd = co_await read(id, core_crCsrCommandPort + offset, sz_CsrCommandPort);
    CommandPort_Busy = (cmd>>63) == 1;
  } while (CommandPort_Busy);

  auto data = co_await read(id, core_crCsrDataPort + offset, sz_CsrDataPort);
  cvm::log(cvm::FULL, "[smc_axi] csr read resp - core_id = {}, addr={:#x} data={:#x}\n", core_id, addr, data);
  co_return data;
}

cvm::messenger::task<void> smc_axi_sequence::scratchpad_tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::pwrmgmt::m_smc_axi_sp_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> smc_axi_sequence::csr_tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::pwrmgmt::m_smc_axi_csr_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<std::pair<uint32_t, uint64_t>> smc_axi_sequence::cpl_sram_write() {
  cvm::rand::uniform_dist<uint32_t> cpl_sram_addr_dist(cpl_sram_base, cpl_sram_limit);
  uint32_t rand_sram_addr = cpl_sram_addr_dist() & 0xFFFFFF8;
  uint64_t rand_sram_data = 0xA5A5A5A5A5A5A5A5;

  cvm::log(cvm::FULL, "[smc_axi] CPL-SRAM write req - addr={:#x}, data={:#x} \n", rand_sram_addr, rand_sram_data);

  unsigned id;
  co_await write(id, rand_sram_addr, SZ_8B, rand_sram_data);

  co_return std::make_pair(rand_sram_addr, rand_sram_data);
}

cvm::messenger::task<void> smc_axi_sequence::smc_trns_read_check(smc_dest_path_t smc_dest_path, uint32_t addr, uint64_t exp_data, size_t sz, block_t block /* = BLOCK */) {
  uint64_t actual_data;
  unsigned id;
  if (smc_dest_path==CORE_CSR)
    actual_data = co_await csr_read(addr, sz);
  else
    actual_data = co_await read(id, addr, sz, block);

  if (exp_data == actual_data)
    cvm::log(cvm::FULL, "[smc_axi] {} read_data check :- PASS  : Addr = 0x{:x}, Data = 0x{:x} \n", (smc_dest_path==CPL_SRAM ? "CPL_SRAM" : (smc_dest_path==CORE_CSR ? "CSR" : "Scratchpad-MMR")), addr, actual_data);
  else
    cvm::log(cvm::ERROR,  "[smc_axi] {} read_data check :- ERROR : Addr = 0x{:x}, Expected Data = 0x{:x}, Actual Data = 0x{:x} \n", (smc_dest_path==CPL_SRAM ? "CPL_SRAM" : (smc_dest_path==CORE_CSR ? "CSR" : "Scratchpad-MMR")), addr, exp_data, actual_data);

  co_return;
};

cvm::messenger::task<uint64_t> smc_axi_sequence::read(unsigned& id, uint64_t addr, size_t sz, block_t block /* = BLOCK */) {
  assert(sz <= 8);

  if (!cvm::registry::messenger.call<smc_axi_mst_t::push_ar_no_id_rpc>(smc_axi_loc_, axi::a_no_id_t{addr, log2(sz)}, id))
    co_return 0;
  smc_axi_read_count_++;
  cvm::log(cvm::MEDIUM, "[smc_axi] read req - id={}, addr={:#x}, sz={}\n", id, addr, sz);

  if (!block)
    co_return 0;

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(r_channel_, [&id](const auto& r) { return r.id == id; });
  auto data = convert_to_dword_array(resp.data);
  // FIXME - check why this alignment is needed
  uint64_t dword = (addr % 8) ? (data[0] >> 32) : data[0];
  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  dword &= mask;
  cvm::log(cvm::MEDIUM, "[smc_axi] read resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data[0], dword, mask);
  co_return dword;
}

cvm::messenger::task<void> smc_axi_sequence::write(unsigned& id, uint64_t addr, size_t sz, uint64_t data, block_t block /* = BLOCK */) {
  assert(sz <= 8);
  // FIXME - check why this alignment is needed
  uint64_t dword = (addr % 8) ? (data << 32) : data;
  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  mask = (addr % 8) ? (mask << 32) : mask;
  auto byte_array = convert_to_byte_array({dword});
  std::vector<bool> strb(8, false);
  for(int i=0; i<8; ++i)
    strb[i] = (mask & (0xFFull << (i*8))) != 0;

  if (!cvm::registry::messenger.call<smc_axi_mst_t::push_aw_no_id_rpc>(smc_axi_loc_, axi::a_no_id_t{addr, 3}, id)) //FIXME size
    co_return;
  cvm::registry::messenger.call<smc_axi_mst_t::push_w_rpc>(smc_axi_loc_, axi::w_t{byte_array, strb, 1});
  smc_axi_write_count_++;
  cvm::log(cvm::MEDIUM, "[smc_axi] write req - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", id, addr, sz, data, dword, mask);

  if (!block)
    co_return;

  auto resp = co_await cvm::registry::messenger.wait<axi::b_t>(b_channel_, [&id](const auto& b) { return b.id == id; });
  cvm::log(cvm::MEDIUM, "[smc_axi] write resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data, dword, mask);
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

// // PREV VERSION CODE
// cvm::messenger::task<void> reset_sequence::smc_scratchpad_default_access() {
//   co_await tick();
//   // Read reset values  
//   co_await smc_read_access_check(mb_scratchpad, mb_scratchpad_rst,SZ_8B);
//   co_await smc_read_access_check(cc_scratchpad, cc_scratchpad_rst,SZ_4B);
//   co_await smc_read_access_check(rc_scratchpad, rc_scratchpad_rst,SZ_4B);
//   co_await smc_read_access_check(dm_scratchpad, dm_scratchpad_rst,SZ_8B);
//   co_await smc_read_access_check(cr_scratchpad, cr_scratchpad_rst,SZ_8B);
//   co_await smc_read_access_check(sw_scratchpad, sw_scratchpad_rst,SZ_8B);
//   co_await smc_read_access_check(ac_scratchpad, ac_scratchpad_rst,SZ_8B);
//   co_return;
// };


void smc_axi_sequence::blocking_seq_tick(uint8_t val) {
  cvm::registry::callbacks.push(
    scope_,
    [val]() {
      cvm::log(cvm::FULL, "[smc_axi] {} blocking seq \n", val ? "start" : "end");
      smc_axi_blocking_sequence_tick(val);
    });
}

