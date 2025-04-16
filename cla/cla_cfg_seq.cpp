#include "cla_cfg_seq.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "fmt/ranges.h"

REGISTRY_register(cla_cfg_seq, CLA, cvm::registry::all);

DEFINE_bool(cla_clk_halt, false, "Enable CLA clk halt events");
DEFINE_bool(cla_nmi, false, "Enable CLA NMI events");
DEFINE_bool(cla_rand_nmi_trig_en, false, "Enable CLA NMI/XTrigger events");

bool elf_completed;

extern "C" {
  void terminate_cla_cfg_seq_func(uint8_t val);
  uint8_t cla_cfg_seq_en_func() {
    return (FLAGS_cla_nmi || FLAGS_cla_rand_nmi_trig_en || FLAGS_cla_clk_halt);
  }
  void cla_send_elf_terminate() {
    elf_completed = 1;
  }
}


cla_cfg_seq::cla_cfg_seq
  (cvm::topology::loc_t loc, unsigned) : 
  loc_(loc), scope_(nullptr) {

  // Topology
  axi_mst_loc_ = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST", 0);
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_);

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });

  if(FLAGS_cla_nmi || FLAGS_cla_rand_nmi_trig_en || FLAGS_cla_clk_halt) {
    cla_main_thread();
  }

}

cla_cfg_seq::~cla_cfg_seq() {
}

void cla_cfg_seq::cla_main_thread()
{
  auto *task = +[] (cla_cfg_seq* m) -> cvm::messenger::task<void> {
    co_await m->cla_main();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> cla_cfg_seq::tick()
{
  co_await cvm::registry::messenger.wait<rv_tester_transactions::cla::m_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::core_no_fetch()
{
  co_await cvm::registry::messenger.wait<rv_tester_transactions::cla::m_core_no_fetch<>>(loc_);
  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::cla_main() {

  cvm::rand::uniform_dist<uint32_t> cnt_loop(20000, 21000);
  cvm::rand::uniform_dist<uint32_t> nmi_rand_cnt(400, 500);
  cvm::rand::uniform_dist<uint32_t> trig_rand_cnt(400, 500);

  cnt_loop_max = cnt_loop();
  nmi_total_cnt = nmi_rand_cnt();              // NMI total enable count
  trig_total_cnt = trig_rand_cnt();            // Xtrigger/rand NMI total count

  // Wait for no fetch
  co_await core_no_fetch();
  mask = FLAGS_hart_enable_mask;
  nmi_event = false;

  cvm::log(cvm::NONE, "[cla] Starting CLA CFG sequence nmi_cnt {} trig_cnt {} \n",nmi_total_cnt, trig_total_cnt);

  while (true) {

    co_await configure_cla();
    cnt_loop_max = cnt_loop();
    co_await wait_for_clocks(cnt_loop_max);
    co_await disable_cla();
    cvm::rand::uniform_dist<uint32_t> again_loop(300, 500);
    cnt_loop_max = again_loop();
    co_await wait_for_clocks(cnt_loop_max);

  }

  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::wait_for_clocks(uint32_t max) {

  for(uint32_t cnt_loop=0;cnt_loop < max;cnt_loop ++){
    if(elf_completed && (FLAGS_cla_nmi || FLAGS_cla_rand_nmi_trig_en)) {
      co_await clear_pend_nmi_on_terminate();
      terminate_test(1);
      elf_completed = 0;
    }
    co_await tick();
  }
}

cvm::messenger::task<void> cla_cfg_seq::configure_cla() {

  if(FLAGS_cla_clk_halt){
    co_await configure_cla_clk_halt();
  }
  if(FLAGS_cla_nmi){
    co_await configure_cla_nmi();
  }
  if(FLAGS_cla_rand_nmi_trig_en){
    co_await configure_cla_rand_nmi_trig_en();
  }
  cvm::log(cvm::NONE, "[cla] Configuration Completed....\n");

  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::disable_cla() {

  if(FLAGS_cla_nmi){
    co_await disable_cla_nmi();
  }
  if(FLAGS_cla_rand_nmi_trig_en){
    co_await disable_cla_rand_nmi_trig_en();
  }
  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::configure_cla_clk_halt() {

  for(uint32_t i=0; i< 8 ; i++){
    if((mask & (1 << i))){
      cvm::log(cvm::NONE, "[cla] CLA HALT Configs for Core-{} \n",i);
      core_offset = 0x10000 * i;
      cntr_data = (rng() % 0x2000) + 0x4000;
      cntr_data = cntr_data << 16;
      
      co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, 0x40);
      co_await write((cdbg_cla_counter0 + core_offset), SZ_8B, cntr_data);
      co_await write((cdbg_node0_eap0_cfg + core_offset), SZ_8B, 0x10049);
      co_await write((cdbg_node1_eap0_cfg + core_offset), SZ_8B, 0x101306);
      co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, 0x60);
    }
  }
  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::configure_cla_nmi() {

  for(uint32_t i=0; i< 8 ; i++){
    if((mask & (1 << i))){
      cvm::log(cvm::NONE, "[cla] CLA NMI Configs for Core-{} \n",i);
      core_offset = 0x10000 * i;
      cntr_data = rng()%0x2000 + 0x2000;
      cntr_data = cntr_data << 16;
      if(reenable_nmi){
        co_await write((cdbg_cla_counter0 + core_offset), SZ_8B, cntr_data);
        co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, (0x1B00 | 0x60));
      }
      else{
        co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, (0x1B00 | 0x40));
        co_await write((cdbg_cla_counter0 + core_offset), SZ_8B, cntr_data);
        co_await write((cdbg_node0_eap0_cfg + core_offset), SZ_8B, 0x10049);
        co_await write((cdbg_node1_eap0_cfg + core_offset), SZ_8B, 0x10110A);
        co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, (0x1B00 | 0x60));
      }
    }
  }

  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::configure_cla_rand_nmi_trig_en() {
  uint32_t wait_on_count,wait_off_count,event_count;
  uint32_t wdata;

  wait_on_count = (rng()% 201) + 1000;    // On Delay 1000-1200 CLK cycle
  wait_off_count = (rng()% 101) + 300;    // Off Delay 300-400 CLK cycle
  event_count = (rng()% 71) + 200;       // Event on Delay 200-270 CLK cycle
  eap_ctrl = (54 << 7);                   // Considering 15 value as per waves
  active_core = (FLAGS_num_harts == 1) ? 0 : (rng() % FLAGS_num_harts);
  reenable_rand_trig = 0;
  core_offset = (0x10000 * active_core);

  cvm::log(cvm::NONE, "[cla] NMI/Trigger Configs for Core - {} nmi_event {} \n", active_core, nmi_event);

  if(reenable_rand_trig) {
    if(nmi_event){
      co_await write((cdbg_node1_eap1_cfg + core_offset), SZ_8B, 0x10009);// ALWAYS ON, NMI
    }
    else {
      co_await write((cdbg_node1_eap1_cfg + core_offset), SZ_8B, 0x1081D);// ALWAYS ON, TRIGGER-0,1
    }
    co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, (eap_ctrl | 0x60));
  }
  else {
    co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, (eap_ctrl | 0x40));
    wdata = 0; wdata = (wait_on_count << 16);
    co_await write((cdbg_cla_counter0 + core_offset), SZ_8B, wdata); // CNT0 - On count
    wdata = 0; wdata = (event_count << 16);
    co_await write((cdbg_cla_counter1 + core_offset), SZ_8B, wdata); // CNT1 - event count
    wdata = 0; wdata = (wait_off_count << 16);
    co_await write((cdbg_cla_counter2 + core_offset), SZ_8B, wdata); // CNT2 - Off count

    co_await write((cdbg_node0_eap1_cfg + core_offset), SZ_8B, 0x10040); // ALWAYS ON, AUTOINCR0
    co_await write((cdbg_node0_eap0_cfg + core_offset), SZ_8B, 0x101645); // TARGET MATCH-0, CLRCNT0, AUTOINCR1, DEST-1
    if(nmi_event){
      co_await write((cdbg_node1_eap1_cfg + core_offset), SZ_8B, 0x10009); // ALWAYS ON, NMI
    }
    else {
      co_await write((cdbg_node1_eap1_cfg + core_offset), SZ_8B, 0x1081D); // ALWAYS ON, TRIGGER-0,1
    }
    co_await write((cdbg_node1_eap0_cfg + core_offset), SZ_8B, 0x131A56); // TRAGET MATCH-1. CLRCNT1, AUTOINCR2, DEST-2
    co_await write((cdbg_node2_eap0_cfg + core_offset), SZ_8B, 0x161900); // TRAGET MATCH-2. CLRCNT2, DEST-0
    co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, (eap_ctrl | 0x60));
  }
  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::disable_cla_nmi() {
  reenable_nmi = 1;

  for(uint32_t i=0; i< 8 ; i++){
    if((mask & (1 << i))){
      cvm::log(cvm::NONE, "[cla] Push CLA NMI Configs for Core-{} \n",i);
      core_offset = 0x10000 * i;
      co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, (0x1B00 | 0x40));  // Disable EAP, CLA enabled
    }
  }
  nmi_total_cnt = nmi_total_cnt - 1;
  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::disable_cla_rand_nmi_trig_en() {
  cvm::log(cvm::NONE, "[cla] NMI/Trigger Disable ..... \n");
  core_offset = (0x10000 * active_core);
  co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, ((eap_ctrl | 0x40) & 0x3FC0));  // Disable EAP, CLA enabled
  reenable_rand_trig = 1;
  nmi_event = !nmi_event;
  trig_total_cnt = trig_total_cnt - 1;
  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::clear_pend_nmi_on_terminate() {
  cvm::log(cvm::NONE, "[cla] Terminate condition detected \n");
  for(uint32_t i=0; i< 8 ; i++){
    if((mask & (1 << i))){
      cvm::log(cvm::NONE, "[cla] Clearing Any pending NMI for Core {} \n",i);
      core_offset = 0x10000 * i;
      co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, (0x1B00 | 0x40));
      co_await write((cdbg_cla_dbg_eap_sts + core_offset), SZ_8B, 0xFFFF'FFFF'0000'0000);
    }
  }
  co_return;
}

cvm::messenger::task<uint64_t> cla_cfg_seq::read(uint64_t addr, size_t sz, block_t block /* = BLOCK */) {

  assert(sz <= 8);

  uint64_t rdata = 0;
  uint8_t offset = static_cast<uint8_t>(addr & 0x3f);
  
  cvm::log(cvm::MEDIUM, "[cla] read req - addr={:#x}, sz={}\n", addr, sz);
  cvm::registry::messenger.signal(axi_mst_loc_, transactor::read_request_t{addr, sz});

  if (!block)
    co_return rdata;

  auto resp = co_await cvm::registry::messenger.wait<transactor::read_response_t>(axi_mst_loc_);
  rdata = convert_to_dword_array(resp.data,offset,sz);
  
  cvm::log(cvm::MEDIUM, "[cla] read resp - id={}, addr={:#x}, sz={}, data={:#x}\n", resp.id, addr, sz, rdata);

  co_return rdata;
}

cvm::messenger::task<void> cla_cfg_seq::write(uint64_t addr, size_t sz, uint64_t data, block_t block /* = BLOCK */) {

  assert(sz <= 8);

  uint8_t offset = static_cast<uint8_t>(addr & 0x3f);
  uint64_t aligned_addr = addr & ~0x3full;
  auto byte_array = convert_to_byte_array(data, offset);

  uint64_t mask = (sz == 64) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;

  std::vector<bool> strb(64, false);
  for (int i = 0; i < static_cast<int>(sz); ++i) {
      if (offset + i < 64) {
          strb[offset + i] = 1;
      }
  }

  // Check for AXI transactor lock before driving
  while(1) {
    auto lock = cvm::registry::messenger.call<overlay_mst_t::try_lock_rpc>(axi_mst_loc_);
    if(lock) { break; }
    co_await tick();
  }

  cvm::log(cvm::MEDIUM, "[cla] write req - addr={:#x}, sz={}, data={:#x}, mask={:#x}\n", aligned_addr, sz, data, mask);
  cvm::registry::messenger.signal(axi_mst_loc_, transactor::write_request_t{aligned_addr, 64, byte_array, strb});

  if (!block)
    co_return;

  auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(axi_mst_loc_);
  cvm::log(cvm::MEDIUM, "[cla] write resp - id={}, addr={:#x}, sz={}, data={:#x}, mask={:#x}\n", resp.id, aligned_addr, sz, data, mask);

  co_return;
}


cvm::messenger::task<void> cla_cfg_seq::csr_write(uint32_t core_id, uint32_t unit, uint64_t addr, uint64_t data) {
  transactor::read_t read_req ;
  uint64_t cmd = 0;
  uint32_t offset = core_id * core_fuse_offset;
  cvm::log(cvm::MEDIUM, "[cla] csr write req - core_id = {}, addr={:#x}, data={:#x} \n", core_id, addr, data );
  uint64_t wr = 0x1;
  uint64_t en = 0x1;
  cmd = en<<62 | wr << 61 | unit<<12 | addr;
  
  co_await axi_write_mmr_granular(core_crCsrDataPort + offset);
  co_await axi_write_mmr_data_granular(core_crCsrDataPort + offset, data);

  co_await axi_write_mmr_granular(core_crCsrCommandPort + offset);
  co_await axi_write_mmr_data_granular(core_crCsrCommandPort + offset, cmd);

  read_req.addr = (core_crCsrCommandPort + offset);
  read_req.length = 8;
  do { 
    cmd = co_await axi_read_mmr_granular(read_req);
  } while ((cmd>>63) != 0x0 );
  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::axi_write_mmr_granular(uint64_t addr) {

  axi::a_no_id_t aw_txn;
  uint64_t aligned_addr = addr & ~0x3full;
  unsigned id;
  aw_txn.w    = true;
  aw_txn.addr = aligned_addr;
  aw_txn.len  = 0;
  aw_txn.size = 6;
  aw_txn.burst = axi::burst_t(0);
  aw_txn.lock  =0;
  aw_txn.cache  =axi::cache_mem_attr_t(0);
  aw_txn.prot  =2;
  aw_txn.qos  =0;
  aw_txn.region  =0;
  aw_txn.atop  =0;
  aw_txn.user  =3;
  
  cvm::log(cvm::MEDIUM, "[cla] cla_cfg_seq WRITE GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", aw_txn.addr);

  if (!cvm::registry::messenger.call<overlay_mst_t::push_aw_no_id_rpc>(axi_mst_loc_, aw_txn, id))
    co_return;
  co_return;
 
}

cvm::messenger::task<void> cla_cfg_seq::axi_write_mmr_data_granular(uint64_t addr, uint64_t data) {
 
  axi::w_t w_txn;
  std::vector<bool> strb(64, false);
  uint8_t offset = static_cast<uint8_t>(addr & 0x3f);
  w_txn.data = convert_to_byte_array(data, offset);

  for (int i = 0; i < 8; ++i) {
      if (offset + i < 64) {
          strb[offset + i] = 1;
      }
  } 
  w_txn.strb = strb;
  w_txn.last = 1;
  
  cvm::log(cvm::MEDIUM, "[cla] cla_cfg_seq WRITE DATA GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", addr);
  cvm::registry::messenger.call<overlay_mst_t::push_w_rpc>(axi_mst_loc_, w_txn);

  co_return;
}

cvm::messenger::task<uint64_t> cla_cfg_seq::axi_read_mmr_granular(const transactor::read_t& r ) {
  uint64_t rdata = 0;
  uint8_t offset = static_cast<uint8_t>(r.addr & 0x3f);
  uint64_t aligned_addr = r.addr & ~0x3full;
  axi::a_no_id_t ar_txn;
  unsigned id;
  ar_txn.w    = false;
  ar_txn.addr = aligned_addr;
  ar_txn.len  = 0;
  ar_txn.size = 6;
  ar_txn.burst = axi::burst_t(0);
  ar_txn.lock  =0;
  ar_txn.cache  =axi::cache_mem_attr_t(0);
  ar_txn.prot  =2;
  ar_txn.qos  =0;
  ar_txn.region  =0;
  ar_txn.atop  =0;
  ar_txn.user  =3;
  
  cvm::log(cvm::FULL, "[cla] cla_cfg_seq AXI READ GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", ar_txn.addr);

   if (!cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_mst_loc_, ar_txn , id))
     co_return rdata;

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);

  rdata = convert_to_dword_array(resp.data,offset,SZ_8B);

  cvm::log(cvm::FULL, "[cla] cla_cfg_seq AXI READ GRANULAR read addr {:#X} completed\n",ar_txn.addr);
  co_return rdata;
}

uint64_t cla_cfg_seq::convert_to_dword_array(const std::vector<uint8_t>& byte_array, uint8_t shift, size_t sz) {

  uint64_t result=0;
  for (int i = 0; i < static_cast<int>(sz); ++i) {
     result = result | static_cast<uint64_t>(byte_array[shift+i]) << (i*8);
  }

  return result;
}

std::vector<uint8_t> cla_cfg_seq::convert_to_byte_array(uint64_t data, uint8_t shift) {
  
  std::vector<uint8_t> byte_vector(64, 0); // Initialize a 64-byte vector with zeros
  for (int i = 0; i < 8; ++i) {
      if (shift + i < 64) {
          byte_vector[shift + i] = static_cast<uint8_t>((data >> (i * 8)) & 0xFF);
      }
  }

  return byte_vector;
}

void cla_cfg_seq::terminate_test(uint8_t terminate_test)
{
  cvm::registry::callbacks.push(
    scope_,
    [terminate_test]() {
      cvm::log(cvm::NONE, "[cla] Test {} \n", terminate_test ? " terminated" : "not terminated");
      terminate_cla_cfg_seq_func(terminate_test);
    });
}
