#include "cla_cfg_seq.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "fmt/ranges.h"

REGISTRY_register(cla_cfg_seq, CLA, cvm::registry::all);

DEFINE_bool(cla_clk_halt, false, "Enable CLA clk halt events");
DEFINE_bool(cla_nmi, false, "Enable CLA NMI events");
DEFINE_bool(cla_rand_nmi_trig_en, false, "Enable CLA NMI/XTrigger events");
DEFINE_bool(cla_custom_action_en, false, "CLA custom actions enable");

bool elf_completed;
bool cpl_xtrigger_ack=0;

extern "C" {
  void terminate_cla_cfg_seq_func(uint8_t val);
  uint8_t cla_cfg_seq_en_func() {
    return (FLAGS_cla_nmi || FLAGS_cla_rand_nmi_trig_en || FLAGS_cla_clk_halt || FLAGS_cla_custom_action_en);
  }
  void cla_send_elf_terminate() {
    elf_completed = 1;
  }
  void send_cpl_xtrigger_ack() {
    cpl_xtrigger_ack = 1;
  }
}


cla_cfg_seq::cla_cfg_seq
  (cvm::topology::loc_t loc, unsigned) : 
  loc_(loc), scope_(nullptr) {

  // Topology
  axi_mst_loc_ = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST", 0);
  smc_loc_     = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_SMC_MST", 0);
  channel      = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_);
  smc_b_channel_ = cvm::registry::messenger.channel<axi::b_t>(smc_loc_);

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });

  if(FLAGS_cla_nmi || FLAGS_cla_rand_nmi_trig_en || FLAGS_cla_clk_halt) {
    cla_main_thread();
  }
  if(FLAGS_cla_custom_action_en) {
    cla_custom_action_thread();
  }

}

cla_cfg_seq::~cla_cfg_seq() {
  if(expect_cpl_xtrigger && !cpl_xtrigger_ack && FLAGS_cla_rand_nmi_trig_en) {
    cvm::log(cvm::ERROR, "[cla] Error: Expected CPL-XTrigger but not received....\n");
  }
}

void cla_cfg_seq::cla_main_thread()
{
  auto *task = +[] (cla_cfg_seq* m) -> cvm::messenger::task<void> {
    co_await m->cla_main();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void cla_cfg_seq::cla_custom_action_thread()
{
  auto *task = +[] (cla_cfg_seq* m) -> cvm::messenger::task<void> {
    co_await m->cla_custom_action_main();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> cla_cfg_seq::tick()
{
  co_await cvm::registry::messenger.wait<rv_tester_transactions::cla::m_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::smc_tick()
{
  co_await cvm::registry::messenger.wait<rv_tester_transactions::cla_smc::m_smc_tick<>>(loc_);
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

  co_await smc_tick();
  co_await smc_axi_write(smc_cpu_debug_ctrl, 0x3F83); // bit[22:7] Xtrig/ClkHalt enable, bit [1:0] Chiplet Enable, enable CPL-CLA
  while (true) {

    co_await configure_cla();
    cnt_loop_max = cnt_loop();
    co_await wait_for_clocks(cnt_loop_max);
    co_await disable_cla();
    cvm::rand::uniform_dist<uint32_t> again_loop(300, 500);
    cnt_loop_max = again_loop();
    co_await wait_for_clocks(cnt_loop_max);
    if(end_cla_cfg_seq) { break; }

  }

  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::wait_for_clocks(uint32_t max) {

  for(uint32_t cnt_loop=0;cnt_loop < max;cnt_loop ++){
    if(elf_completed && (FLAGS_cla_nmi || FLAGS_cla_rand_nmi_trig_en)) {
      co_await clear_pend_nmi_on_terminate();
      terminate_test(1);
      end_cla_cfg_seq = 1;
      elf_completed = 0;
      cnt_loop = max;
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
  cvm::rand::uniform_dist<uint32_t> cntr_rnd(16400, 24000);

  for(uint32_t i=0; i< 8 ; i++){
    if((mask & (1 << i))){
      cvm::log(cvm::NONE, "[cla] CLA HALT Configs for Core-{} \n",i);
      core_offset = 0x10000 * i;
      cntr_data = cntr_rnd();
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
  cvm::rand::uniform_dist<uint32_t> cntr_rnd(16400, 24000);

  for(uint32_t i=0; i< 8 ; i++){
    if((mask & (1 << i))){
      cvm::log(cvm::NONE, "[cla] CLA NMI Configs for Core-{} \n",i);
      core_offset = 0x10000 * i;
      cntr_data = cntr_rnd();
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
  cvm::rand::uniform_dist<uint32_t> wait_on_rnd(1000, 1200);
  cvm::rand::uniform_dist<uint32_t> wait_off_rnd(300, 400);
  cvm::rand::uniform_dist<uint32_t> event_rnd(200, 280);
  cvm::rand::uniform_dist<uint32_t> rand_harts(0, FLAGS_num_harts-1); // RVDE-27599, enable it once Issue resolved
  uint32_t wait_on_count,wait_off_count,event_count;
  uint32_t wdata;

  wait_on_count = wait_on_rnd();    // On Delay 1000-1200 CLK cycle
  wait_off_count = wait_off_rnd();  // Off Delay 300-400 CLK cycle
  event_count = event_rnd();        // Event on Delay 200-270 CLK cycle
  eap_ctrl = (54 << 7);
  // active_core = rand_harts(); RVDE-27599, enable it once Issue resolved
  active_core = (FLAGS_num_harts == 1) ? 0 : rand_harts();
  core_offset = (0x10000 * active_core);

  cvm::log(cvm::NONE, "[cla] NMI/Trigger Configs for Core - {} nmi_event {} \n", active_core, nmi_event);

  if(active_core != FLAGS_num_harts) {
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
  else {    // CPL-CLA Configuration
    expect_cpl_xtrigger = 1;
    co_await smc_axi_write(cpl_cla_ctrl_status, (eap_ctrl | 0x40));
    wdata = 0; wdata = (wait_on_count << 16);
    co_await smc_axi_write(cpl_cla_counter0, wdata); // CNT0 - On count
    wdata = 0; wdata = (event_count << 16);
    co_await smc_axi_write(cpl_cla_counter1, wdata); // CNT1 - event count
    wdata = 0; wdata = (wait_off_count << 16);
    co_await smc_axi_write(cpl_cla_counter2, wdata); // CNT2 - Off count

    co_await smc_axi_write(cpl_node0_eap1_cfg, 0x10040);   // ALWAYS ON, AUTOINCR0
    co_await smc_axi_write(cpl_node0_eap0_cfg, 0x101645);  // TARGET MATCH-0, CLRCNT0, AUTOINCR1, DEST-1
    co_await smc_axi_write(cpl_node1_eap1_cfg, 0x1081D);   // ALWAYS ON, TRIGGER-0,1
    co_await smc_axi_write(cpl_node1_eap0_cfg, 0x131A56);  // TRAGET MATCH-1. CLRCNT1, AUTOINCR2, DEST-2
    co_await smc_axi_write(cpl_node2_eap0_cfg, 0x161900);  // TRAGET MATCH-2. CLRCNT2, DEST-0
    co_await smc_axi_write(cpl_cla_ctrl_status, (eap_ctrl | 0x60));
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
  if(active_core != FLAGS_num_harts) {  // For non-CPL-CLA active core
    co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, ((eap_ctrl | 0x40) & 0x3FC0));  // Disable EAP, CLA enabled
  }
  else {  // For CPL-CLA active core
    co_await smc_axi_write(cpl_cla_ctrl_status, ((eap_ctrl | 0x40) & 0x3FC0));  // Disable EAP, CLA enabled
  }
  nmi_event = !nmi_event;
  trig_total_cnt = trig_total_cnt - 1;
  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::clear_pend_nmi_on_terminate() {
  cvm::log(cvm::NONE, "[cla] Terminate condition detected \n");
  for(uint32_t i=0; i< FLAGS_num_harts ; i++){
    if(i == (FLAGS_num_harts-1)){
      cvm::log(cvm::NONE, "[cla] Clearing Any pending NMI for Core with blocking writes{} \n",i);
      core_offset = 0x10000 * i;
      co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, (0x1B00 | 0x40));
      co_await write((cdbg_cla_dbg_eap_sts + core_offset), SZ_8B, 0xFFFF'FFFF'0000'0000);
    }
    else {
      cvm::log(cvm::NONE, "[cla] Clearing Any pending NMI for Core with posted writes {} \n",i);
      core_offset = 0x10000 * i;
      co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, (0x1B00 | 0x40), NO_BLOCK);
      co_await write((cdbg_cla_dbg_eap_sts + core_offset), SZ_8B, 0xFFFF'FFFF'0000'0000, NO_BLOCK);
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

cvm::messenger::task<void> cla_cfg_seq::smc_axi_write(uint64_t addr, uint64_t data, block_t block /* = BLOCK */) {

  axi::a_no_id_t aw_txn;
  uint64_t aligned_addr = addr & ~0x7ull;
  auto byte_array = convert_to_smc_byte_array(data);
  std::vector<bool> strb(8, true);
  for(int i=0; i<8; ++i)
    strb[i] = 1;

  unsigned id;
  aw_txn.w      =  true;  // Write transaction
  aw_txn.addr   = aligned_addr;
  aw_txn.size   = 3;
  aw_txn.user   = 3;
  aw_txn.exp_err_rsp   = 0;
  aw_txn.seqid  = CLA_SEQ_ID;
  
  cvm::log(cvm::MEDIUM, "[cla] SMC CLA Access Write - addr={:#x} data={:#x} \n", aw_txn.addr, data);

  if (!cvm::registry::messenger.call<smc_mst_t::push_aw_no_id_rpc>(smc_loc_, aw_txn, id)) {
    auto axi_idalloc_done = co_await check_smc_axi_bresp_timeout(id, aw_txn.addr);
    if (!axi_idalloc_done) {
      co_return;
    }
  }
  cvm::registry::messenger.call<smc_mst_t::push_w_rpc>(smc_loc_, axi::w_t{byte_array, strb, 1});

  if(block == BLOCK) {
    auto resp = co_await cvm::registry::messenger.wait<axi::b_t>(smc_b_channel_, [&id](const auto& b) { return b.id == id; });
    cvm::log(cvm::MEDIUM, "[cla] SMC CLA Access Write resp - id={}, addr={:#x}, sz={}, data={:#x}\n", resp.id, aw_txn.addr, aw_txn.size, data);
  }
  co_return;
 
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

cvm::messenger::task<void> cla_cfg_seq::cla_custom_action_main() {

  cvm::rand::uniform_dist<uint32_t> total_action_cnt(20, 50);
  cvm::rand::uniform_dist<uint32_t> cnt_loop(20000, 25000);

  custom_action_cnt = total_action_cnt();
  cnt_loop_max = cnt_loop();
  // Wait for no fetch
  co_await core_no_fetch();
  cvm::log(cvm::NONE, "[cla] Starting CLA Custom Action sequence Total Count {} \n", custom_action_cnt);

  for(uint32_t i=0; i< custom_action_cnt; i++) {
    co_await configure_cla_custom_action();
    co_await wait_for_clocks(cnt_loop_max);
    co_await disable_cla_custom_action();
  }
  co_return;
}

/************************************************************
                    CLA Action Sequence
NODE0_EAP0: CNTR0_TGT_MATCH , CLEAR_CNT0 moved to NODE1
NODE0_EAP1: ALWAYS ON , INCR_CNT0 , Custom action for FE 0xA [1010]
NODE1_EAP0: CNTR1_TGT_MATCH , CLEAR_CNT1 moved to NODE2
NODE1_EAP1: ALWAYS ON , INCR_CNT1 , Custom action for FE 0x5 [0101]
NODE2_EAP0: CNTR2_TGT_MATCH , CLEAR_CNT2 moved to NODE0
NODE2_EAP1: ALWAYS ON , INCR_CNT2
************************************************************/

cvm::messenger::task<void> cla_cfg_seq::configure_cla_custom_action() {
  cvm::rand::uniform_dist<uint32_t> trig_cnt(1, 200);
  cvm::rand::uniform_dist<uint32_t> off_cnt(400, 500);

  for(uint32_t i=0; i< FLAGS_num_harts ; i++){
    cvm::log(cvm::NONE, "[cla] CLA Custom Action Configs for Core-{} \n",i);
    core_offset = 0x10000 * i;
    co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, (0x1B00 | 0x40));
    cntr_data = trig_cnt();
    cntr_data = cntr_data << 16;
    co_await write((cdbg_cla_counter0 + core_offset), SZ_8B, cntr_data);
    cntr_data = trig_cnt();
    cntr_data = cntr_data << 16;
    co_await write((cdbg_cla_counter1 + core_offset), SZ_8B, cntr_data);
    cntr_data = off_cnt();
    cntr_data = cntr_data << 16;
    co_await write((cdbg_cla_counter2 + core_offset), SZ_8B, cntr_data);
    co_await write((cdbg_node0_eap0_cfg + core_offset), SZ_8B, 0x100045);
    co_await write((cdbg_node0_eap1_cfg + core_offset), SZ_8B, 0x3310010040);
    co_await write((cdbg_node1_eap0_cfg + core_offset), SZ_8B, 0x130056);
    co_await write((cdbg_node1_eap1_cfg + core_offset), SZ_8B, 0x3200010051);
    co_await write((cdbg_node2_eap0_cfg + core_offset), SZ_8B, 0x160064);
    co_await write((cdbg_node2_eap1_cfg + core_offset), SZ_8B, 0x10062);
    co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, (0x1B00 | 0x60));
  }

  co_return;
}

cvm::messenger::task<void> cla_cfg_seq::disable_cla_custom_action() {

  for(uint32_t i=0; i< FLAGS_num_harts ; i++){
    cvm::log(cvm::NONE, "[cla] Disable CLA Custom Action Configs for Core-{} \n",i);
    core_offset = 0x10000 * i;
    co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, (0x1B00 | 0x40));
  }
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
  aw_txn.seqid  =CLA_SEQ_ID;
  
  cvm::log(cvm::MEDIUM, "[cla] cla_cfg_seq WRITE GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", aw_txn.addr);

  if (!cvm::registry::messenger.call<overlay_mst_t::push_aw_no_id_rpc>(axi_mst_loc_, aw_txn, id)) {
    auto axi_idalloc_done = co_await check_axi_bresp_timeout(aw_txn, id);
    if (!axi_idalloc_done) {
      co_return;
    }
  }
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
  ar_txn.seqid  =CLA_SEQ_ID;

  
  cvm::log(cvm::FULL, "[cla] cla_cfg_seq AXI READ GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", ar_txn.addr);

  if (!cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_mst_loc_, ar_txn , id)) {
    auto axi_idalloc_done = co_await check_axi_rresp_timeout(ar_txn, id);
    if (!axi_idalloc_done) {
      co_return 0;
    }
  }

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

uint64_t cla_cfg_seq::convert_to_smc_dword_array(const std::vector<uint8_t>& byte_array) {

  uint64_t result=0;
  for (int i = 0; i < 8; ++i) {
     result = result | static_cast<uint64_t>(byte_array[i]) << (i*8);
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

std::vector<uint8_t> cla_cfg_seq::convert_to_smc_byte_array(uint64_t data) {
  
  std::vector<uint8_t> byte_vector(8, 0); // Initialize a 64-byte vector with zeros
  for (int i = 0; i < 8; ++i) {
    byte_vector[i] = static_cast<uint8_t>((data >> (i * 8)) & 0xFF);
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

cvm::messenger::task<bool> cla_cfg_seq::check_axi_bresp_timeout(axi::a_no_id_t aw_txn, unsigned& id) {

  uint32_t axi_bresp_cycle_cnt = 0;

  while (true) {
    co_await tick();

    if (axi_bresp_cycle_cnt >= FLAGS_axi_resp_timeout) {
      cvm::log(cvm::ERROR, "[cla] [axi_mst] Error: No free id's remaining for axi master\n");
      co_return false;
    }
    axi_bresp_cycle_cnt++;

    if (cvm::registry::messenger.call<overlay_mst_t::push_aw_no_id_rpc>(axi_mst_loc_, aw_txn, id)) {
      co_return true;
    }
  }

  co_return true;
}

cvm::messenger::task<bool> cla_cfg_seq::check_axi_rresp_timeout(axi::a_no_id_t ar_txn, unsigned& id) {

  uint32_t axi_rresp_cycle_cnt = 0;

  while (true) {
    co_await tick();

    if (axi_rresp_cycle_cnt >= FLAGS_axi_resp_timeout) {
      cvm::log(cvm::ERROR, "[cla] [axi_mst] Error: No free id's remaining for axi master\n");
      co_return false;
    }
    axi_rresp_cycle_cnt++;

    if (cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_mst_loc_, ar_txn, id)) {
      co_return true;
    }
  }

  co_return true;
}

cvm::messenger::task<bool> cla_cfg_seq::check_smc_axi_bresp_timeout(unsigned& id, uint64_t addr) {

  uint32_t smc_axi_bresp_cycle_cnt = 0;
  axi::a_no_id_t aw_txn;
  aw_txn.w      =  true;  // Write transaction
  aw_txn.addr   = addr;
  aw_txn.len    = 0;
  aw_txn.size   = 3;
  aw_txn.burst  = axi::burst_t(0);
  aw_txn.lock   = 0;
  aw_txn.cache  = axi::cache_mem_attr_t(0);
  aw_txn.prot   = 2;
  aw_txn.qos    = 0;
  aw_txn.region = 0;
  aw_txn.atop   = 0;
  aw_txn.user   = 3;
  aw_txn.seqid  = CLA_SEQ_ID;
  while (true) {
    co_await smc_tick();
    
    if (smc_axi_bresp_cycle_cnt >= FLAGS_axi_resp_timeout) {
      cvm::log(cvm::ERROR, "[cla] Error: No free id's remaining for smc axi master\n");
      co_return false;
    }
    smc_axi_bresp_cycle_cnt++;

    if (cvm::registry::messenger.call<smc_mst_t::push_aw_no_id_rpc>(smc_loc_, aw_txn, id)) {
      co_return true;
    }

  }
  co_return true;

}