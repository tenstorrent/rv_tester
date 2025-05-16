#include "thub_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "fmt/ranges.h"

REGISTRY_register(thub_sequence, PWRMGMT, cvm::registry::all);

DEFINE_bool(thub_rand_en, false, "Enable random smc axi accesses in the sim");
DEFINE_string(thub_count, "10000:10000", "Number of smc axi sequences in the sim");
DEFINE_string(thub_interval, "5:5", "soc cycle interval between smc axi sequences in the sim");
DEFINE_string(thub_width, "1:1", "soc cycle width of smc axi sequences in the sim");
DEFINE_bool(temp_throttle, false, "Program lower Temp throttle for core");

extern "C" {
  void thub_blocking_sequence_tick(uint8_t val);
}

thub_sequence::thub_sequence
  (cvm::topology::loc_t loc, unsigned) : 
  scope_(nullptr) , loc_(loc)  {

  // Topology
  smc_axi_loc_ = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_SMC_MST", 0);
  
  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });

  // main sequence thread
  if (FLAGS_thub_rand_en)
    main_thread();
}

thub_sequence::~thub_sequence() {
}

void thub_sequence::main_thread() {
  auto *task = +[] (thub_sequence* m) -> cvm::messenger::task<void> {
    co_await m->main();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> thub_sequence::main() {
  co_await tick();
  while (true) {
    co_await tick();

    // Enable MC throttling when Temprature crosses threshold
    if(FLAGS_temp_throttle)
    {
      cvm::log(cvm::NONE, "[THUB THROTTLE]  Throttle Enabled ...... \n");
      co_await temp_throttle_configuration();
    };
  }
  co_return;
}

cvm::messenger::task<void> thub_sequence::wait_for_ticks()
{
  cvm::rand::uniform_dist<uint32_t> smc_mmr_index_dist(400, 500);
  uint32_t thub_throttle_on = smc_mmr_index_dist();

  for(uint32_t i =0; i< thub_throttle_on; i++)
  {
    co_await tick();
  };
};

cvm::messenger::task<void> thub_sequence::temp_throttle_configuration()
{
  cvm::rand::uniform_dist<uint32_t> smc_mmr_index_dist(0, FLAGS_num_harts-1);
  core_throttle = smc_mmr_index_dist();
  cvm::log(cvm::NONE, "[THUB THROTTLE]  Entered into throttle for Core {} ...... \n", core_throttle);
  co_await temp_throttle_enable();
  //Delay before temp throttle is disabled
  co_await wait_for_ticks();
  co_await temp_throttle_disable();
  co_await wait_for_ticks();
  cvm::log(cvm::NONE, "[THUB THROTTLE]  Completed into throttle for Core {} ...... \n", core_throttle);
};

cvm::messenger::task<void> thub_sequence::temp_throttle_enable()
{
    uint64_t cntr_data;

    // Make sure MCsrInhibit for Counter10 is 0
    cntr_data = co_await csr_read(core_throttle, 0x4, core_mcountinhibit); // MS inhibit
    cntr_data = cntr_data & 0xFFFFFFFFFFFFFBFF;
    co_await csr_write(core_throttle, 0x4, core_mcountinhibit , cntr_data); // MS Event
    cntr_data = co_await csr_read(core_throttle, 0x8, core_mcountinhibit); // MC inhibit
    cntr_data = cntr_data & 0xFFFFFFFFFFFFFBFF;
    co_await csr_write(core_throttle, 0x8, core_mcountinhibit , cntr_data); // MC Event

    // Configure HPMEVENT10/HPMCOUNTER10 to monitor throttle
    co_await csr_write(core_throttle, 0x4, core_mhpmevent10 , 0x94400000); // MS Event
    co_await csr_write(core_throttle, 0x8, core_mhpmevent10 , 0x94400000); // MC Event
    cntr_data = co_await csr_read(core_throttle, 0x8, core_mhpmcounter10);
    if(cntr_data != 0)
      cvm::log(cvm::ERROR, "[THROTTLE] Before Enabling Throttle HPMCOUNTER is non-zero {} .... \n",cntr_data);
    else
      cvm::log(cvm::NONE, "[THROTTLE] Before Enabling Throttle HPMCOUNTER is {} .... \n",cntr_data);

    // Write to MC/MS power config
    co_await csr_write(core_throttle, 0x8,core_pwr_throttle_cfg_0 , 0x000078830372a211);
    co_await csr_write(core_throttle, 0x8,core_pwr_throttle_cfg_1 , 0x1041017ecb594129);
};

cvm::messenger::task<void> thub_sequence::temp_throttle_disable()
{
  uint64_t cntr_data;

  cntr_data = co_await csr_read(core_throttle, 0x4, core_mhpmcounter10);
  if(cntr_data == 0)
    cvm::log(cvm::ERROR, "[THROTTLE] After Enabling Throttle HPMCOUNTER is zero {} .... \n",cntr_data);
  else
  cvm::log(cvm::NONE, "[THROTTLE] After Enabling Throttle HPMCOUNTER is {} .... \n",cntr_data);

  // Write to MC/MS power config
  co_await csr_write(core_throttle, 0x8,core_pwr_throttle_cfg_0 , 0x000078830372a211);
  co_await csr_write(core_throttle, 0x8,core_pwr_throttle_cfg_1 , 0x11ff017ecb594129);

  co_await csr_write(core_throttle, 0x4, core_mhpmevent10 , 0x0); // MS Event
  co_await csr_write(core_throttle, 0x8, core_mhpmevent10 , 0x0); // MC Event
  co_await csr_write(core_throttle, 0x4, core_mhpmcounter10 , 0x0); // MS Event
};

cvm::messenger::task<void> thub_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::pwrmgmt::m_thub_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<uint64_t> thub_sequence::read(uint64_t addr, size_t sz, block_t block /* = BLOCK */) {
  assert(sz <= 8);
  cvm::log(cvm::MEDIUM, "[smc] read req - addr={:#x}, sz={}\n", addr, sz);
  cvm::registry::messenger.signal(smc_axi_loc_, transactor::read_request_t{addr, sz});

  if (!block)
    co_return 0;

  auto resp = co_await cvm::registry::messenger.wait<transactor::read_response_t>(smc_axi_loc_);
  auto data = convert_to_dword_array(resp.data);
  // FIXME - check why this alignment is needed
  uint64_t dword = (addr % 8) ? (data[0] >> 32) : data[0];
  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  dword &= mask;
  cvm::log(cvm::MEDIUM, "[thub] read resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data[0], dword, mask);
  co_return dword;
}

cvm::messenger::task<void> thub_sequence::write(uint64_t addr, size_t sz, uint64_t data, block_t block /* = BLOCK */) {
  assert(sz <= 8);
  // FIXME - check why this alignment is needed
  uint64_t dword = (addr % 8) ? (data << 32) : data;
  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  mask = (addr % 8) ? (mask << 32) : mask;
  auto byte_array = convert_to_byte_array({dword});
  std::vector<bool> strb(8, false);
  for(int i=0; i<8; ++i)
    strb[i] = (mask & (0xFFull << (i*8))) != 0;
  cvm::log(cvm::MEDIUM, "[thub] write req - addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", addr, sz, data, dword, mask);
  cvm::registry::messenger.signal(smc_axi_loc_, transactor::write_request_t{addr, SZ_8B, byte_array, strb});

  if (!block)
    co_return;

  auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(smc_axi_loc_);
  cvm::log(cvm::MEDIUM, "[thub] write resp - id={}, addr={:#x}, sz={}, data={:#x}, dword={:#x} mask={:#x}\n", resp.id, addr, sz, data, dword, mask);
  co_return;
}

cvm::messenger::task<void> thub_sequence::csr_write(uint32_t core_id, uint32_t unit, uint64_t addr, uint64_t data) {
  uint64_t cmd = 0;
  uint32_t offset = core_id * core_fuse_offset;
  cvm::log(cvm::NONE, "[thub] csr write req - core_id = {}, addr={:#x}, data={:#x} \n", core_id, addr, data );
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


cvm::messenger::task<uint64_t> thub_sequence::csr_read(uint32_t core_id, uint32_t unit,uint64_t addr) {
  uint64_t cmd = 0;
  uint32_t offset = core_id * core_fuse_offset;
  cvm::log(cvm::NONE, "[thub] csr read req - core_id = {}, addr={:#x} \n", core_id, addr );
  uint64_t wr = 0;
  uint64_t en = 0x1;
  cmd = en <<62 | wr << 61 |unit<<12|addr;
  co_await write(core_crCsrCommandPort + offset, SZ_8B, cmd);
  do { 
    cmd = co_await read(core_crCsrCommandPort + offset, SZ_8B);
  } while ((cmd>>63) != 0x0 );
  cvm::log(cvm::NONE, "[thub] cr read res - core_id = {}, addr={:#x} \n", core_id, addr );
  auto data = co_await read(core_crCsrDataPort + offset, SZ_8B);
  co_return data;
}

std::vector<uint64_t> thub_sequence::convert_to_dword_array(const std::vector<uint8_t>& byte_array) {
  std::vector<uint64_t> result(byte_array.size() / sizeof(uint64_t));
  std::copy(reinterpret_cast<const uint64_t*>(byte_array.data()),
            reinterpret_cast<const uint64_t*>(byte_array.data() + byte_array.size()),
            result.begin());
  return result;
}

std::vector<uint8_t> thub_sequence::convert_to_byte_array(const std::vector<uint64_t>& dword_array) {
  std::vector<uint8_t> result(dword_array.size() * sizeof(uint64_t));
  std::copy(reinterpret_cast<const uint8_t*>(dword_array.data()),
            reinterpret_cast<const uint8_t*>(dword_array.data()) + dword_array.size() * sizeof(uint64_t),
            result.begin());
  return result;
}

void thub_sequence::blocking_seq_tick(uint8_t val) {
  cvm::registry::callbacks.push(
    scope_,
    [val]() {
      cvm::log(cvm::FULL, "[thub] {} blocking seq \n", val ? "start" : "end");
      thub_blocking_sequence_tick(val);
    });
}