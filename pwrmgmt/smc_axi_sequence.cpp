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

  // smc axi sequence thread
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
  uint32_t rand_indx = 0;
  std::pair<uint32_t, uint64_t> sram_transaction_info;
  size_t smc_axi_trns_width = 2;

  while (true) {
    co_await tick();

    for (size_t trns_indx = 0; trns_indx < smc_axi_trns_width; trns_indx++)
    {
      cvm::rand::uniform_dist<int> smc_trns_path_dist(0, 1);
      switch (smc_trns_path_dist()) {
        case 0:
          // SMC --> MMR/PMNW path
          rand_indx = co_await scratchpad_write();
          co_await smc_trns_read_check(MMR_PMNW, smc_scratchpad_info[rand_indx].addr, smc_scratchpad_info[rand_indx].data, smc_scratchpad_info[rand_indx].sz);
          break;
        case 1: 
          // SMC --> CSR path
          rand_indx = co_await csr_write();
          co_await smc_trns_read_check(CORE_CSR, smc_csr_info[rand_indx].addr, smc_csr_info[rand_indx].data, smc_csr_info[rand_indx].sz);
          break;
      }

      // SMC --> CPL SRAM path
      sram_transaction_info = co_await cpl_sram_write();
      co_await smc_trns_read_check(CPL_SRAM, sram_transaction_info.first, sram_transaction_info.second, SZ_8B);
    }
  }

  co_return;
}

cvm::messenger::task<void> smc_axi_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::pwrmgmt::m_smc_axi_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<uint32_t> smc_axi_sequence::scratchpad_write() {
  cvm::rand::uniform_dist<uint32_t> smc_mmr_index_dist(0, smc_scratchpad_info.size() - 1);
  uint32_t rand_indx = smc_mmr_index_dist();
  uint32_t addr = smc_scratchpad_info[rand_indx].addr;
  uint64_t data = smc_scratchpad_info[rand_indx].data;
  size_t sz = smc_scratchpad_info[rand_indx].sz;

  cvm::log(cvm::MEDIUM, "[smc-axi] Scratchpad-MMR write req - addr={:#x}, data={:#x} \n", addr, data);
  co_await write(addr, sz, data);
  
  cvm::log(cvm::MEDIUM, "[smc-axi] Scratchpad-MMR write done - addr={:#x}, data={:#x} \n", addr, data);
  co_return rand_indx;
}

cvm::messenger::task<std::pair<uint32_t, uint64_t>> smc_axi_sequence::cpl_sram_write() {
  cvm::rand::uniform_dist<uint32_t> cpl_sram_addr_dist(cpl_sram_base, cpl_sram_limit);
  uint32_t rand_sram_addr = cpl_sram_addr_dist() & 0xFFFFFF8;
  uint64_t rand_sram_data = 0xA5A5A5A5A5A5A5A5;

  cvm::log(cvm::MEDIUM, "[smc-axi] CPL-SRAM write req - addr={:#x}, data={:#x} \n", rand_sram_addr, rand_sram_data);
  co_await write(rand_sram_addr, SZ_8B, rand_sram_data);

  cvm::log(cvm::MEDIUM, "[smc-axi] CPL-SRAM write done - addr={:#x}, data={:#x} \n", rand_sram_addr, rand_sram_data);
  co_return std::make_pair(rand_sram_addr, rand_sram_data);
}

cvm::messenger::task<uint32_t> smc_axi_sequence::csr_write() {
  cvm::rand::uniform_dist<uint32_t> smc_csr_index_dist(0, 1);
  uint32_t rand_indx = smc_csr_index_dist();
  uint32_t addr = smc_csr_info[rand_indx].addr;
  size_t sz_CsrDataPort = smc_csr_info[rand_indx].sz;
  size_t sz_CsrCommandPort = SZ_8B;
  uint64_t data = smc_csr_info[rand_indx].data;

  uint32_t core_id = 0;
  uint32_t unit = 0x8;
  uint64_t cmd = 0;
  uint32_t offset = core_id * core_fuse_offset;
  uint64_t wr = 0x1;
  uint64_t en = 0x1;
  bool CommandPort_Busy = false;

  cvm::log(cvm::MEDIUM, "[smc-axi] csr write req - core_id = {}, addr={:#x}, data={:#x} \n", core_id, addr, data);
  co_await write(core_crCsrDataPort + offset, sz_CsrDataPort, data);
  cmd = en<<62 | wr<<61 | unit<<12 | addr;
  co_await write(core_crCsrCommandPort + offset, sz_CsrCommandPort, cmd);
  do {
    cmd = co_await read(core_crCsrCommandPort + offset, sz_CsrCommandPort);
    CommandPort_Busy = (cmd>>63) == 1;
  } while (CommandPort_Busy);

  cvm::log(cvm::MEDIUM, "[smc-axi] csr write done - core_id = {}, addr={:#x}, data={:#x} \n", core_id, addr, data);
  co_return rand_indx;
}

cvm::messenger::task<uint64_t> smc_axi_sequence::csr_read(uint32_t addr, size_t sz) {
  size_t sz_CsrDataPort = sz;
  size_t sz_CsrCommandPort = SZ_8B;

  uint32_t core_id = 0;
  uint32_t unit = 0x8;
  uint64_t cmd = 0;
  uint32_t offset = core_id * core_fuse_offset;
  uint64_t wr = 0x1;
  uint64_t en = 0x1;
  bool CommandPort_Busy = false;

  cvm::log(cvm::MEDIUM, "[smc-axi] csr read req - core_id = {}, addr={:#x} \n", core_id, addr);
  cmd = en<<62 | wr<<61 | unit<<12 | addr;
  co_await write(core_crCsrCommandPort + offset, sz_CsrCommandPort, cmd);
  do {
    cmd = co_await read(core_crCsrCommandPort + offset, sz_CsrCommandPort);
    CommandPort_Busy = (cmd>>63) == 1;
  } while (CommandPort_Busy);

  cvm::log(cvm::MEDIUM, "[smc-axi] csr read resp - core_id = {}, addr={:#x} \n", core_id, addr);
  auto data = co_await read(core_crCsrDataPort + offset, sz_CsrDataPort);
  co_return data;
}

cvm::messenger::task<void> smc_axi_sequence::smc_trns_read_check(smc_dest_path_t smc_dest_path, uint32_t addr, uint64_t exp_data, size_t sz, block_t block /* = BLOCK */) {
  uint64_t actual_data;
  if (smc_dest_path==CORE_CSR)
    actual_data = co_await csr_read(addr, sz);
  else
    actual_data = co_await read(addr, sz, block);

  if (exp_data == actual_data)
    cvm::log(cvm::MEDIUM, "[smc-axi] {} read_data check :- PASS  : Addr = 0x{:x}, Data = 0x{:x} \n", (smc_dest_path==CPL_SRAM ? "CPL_SRAM" : (smc_dest_path==CORE_CSR ? "CSR" : "Scratchpad-MMR")), addr, actual_data);
  else
    cvm::log(cvm::ERROR,  "[smc-axi] {} read_data check :- ERROR : Addr = 0x{:x}, Expected Data = 0x{:x}, Actual Data = 0x{:x} \n", (smc_dest_path==CPL_SRAM ? "CPL_SRAM" : (smc_dest_path==CORE_CSR ? "CSR" : "Scratchpad-MMR")), addr, exp_data, actual_data);
  co_return;
};

cvm::messenger::task<uint64_t> smc_axi_sequence::read(uint64_t addr, size_t sz, block_t block /* = BLOCK */) {
  assert(sz <= 8);
  cvm::log(cvm::MEDIUM, "[smc-axi] read req - addr={:#x}, sz={}\n", addr, sz);
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
  cvm::log(cvm::MEDIUM, "[smc-axi] read resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data[0], dword, mask);
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
  cvm::log(cvm::MEDIUM, "[smc-axi] write req - addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", addr, sz, data, dword, mask);
  cvm::registry::messenger.signal(smc_axi_loc_, transactor::write_request_t{addr, SZ_8B, byte_array, strb});
  smc_axi_write_count_++;

  if (!block)
    co_return;

  auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(smc_axi_loc_);
  cvm::log(cvm::MEDIUM, "[smc-axi] write resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data, dword, mask);
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