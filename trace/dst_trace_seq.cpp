#include "dst_trace_seq.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "fmt/ranges.h"

REGISTRY_register(dst_trace_seq, TRACE, cvm::registry::all);

extern "C" {
  void terminate_dst_trace_seq_func(uint8_t val);
  uint8_t dst_trace_seq_en_func() {
    return FLAGS_trace_en;
  }
}

dst_trace_seq::dst_trace_seq
  (cvm::topology::loc_t loc, unsigned) : 
  loc_(loc), scope_(nullptr) {

  // Topology
  axi_mst_loc_ = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST", 0);
  
  // Channels - create dedicated channels for read/write responses to avoid cross-consumption
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_);
  b_channel_ = cvm::registry::messenger.channel<axi::b_t>(axi_mst_loc_);

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });

  if(FLAGS_trace_en) {
    dst_main_thread();
  }

}

dst_trace_seq::~dst_trace_seq() {
}

void dst_trace_seq::dst_main_thread()
{
  auto *task = +[] (dst_trace_seq* m) -> cvm::messenger::task<void> {
    co_await m->dst_main();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> dst_trace_seq::tick()
{
  co_await cvm::registry::messenger.wait<rv_tester_transactions::trace::m_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> dst_trace_seq::core_no_fetch()
{
  co_await cvm::registry::messenger.wait<rv_tester_transactions::trace::m_core_no_fetch<>>(loc_);
  co_return;
}

cvm::messenger::task<void> dst_trace_seq::dst_main() {

  cvm::rand::uniform_dist<uint32_t> rand_core_num(0, FLAGS_num_harts-1);
  
  // Wait for no fetch
  co_await core_no_fetch();
  
  mask = FLAGS_hart_enable_mask;
  if(FLAGS_num_harts > 1) {
    while(1){
      enabled_core = rand_core_num();
      if(mask & static_cast<uint32_t>(1 << enabled_core)) break;
    }
  }
  core_offset = 0x10000 * enabled_core;
  cvm::log(cvm::NONE, "[dst_trace_seq] Starting DST Trace sequence on Core - {:#x}\n",enabled_core);

  //while (true) {

    smem_mode = 1;
    //for(int cnt_loop=0;cnt_loop < 100;cnt_loop ++){
    //  co_await tick();
    //}
    co_await configure_dst_ram_funnel();
    co_await configure_cla();
    co_await check_dst_counter_value();
    co_await disable_dst_trace();
    co_await disable_trace_funnel();
    co_await disable_dst_trace_ram();
    if(smem_mode) {
      co_await poll_dst_ram_empty();
    }
    else {
      co_await read_dst_trace_ram();
    }
    terminate_test(1);

  //}

  co_return;
}

cvm::messenger::task<void> dst_trace_seq::configure_dst_ram_funnel() {
  uint64_t ram_start_addr, ram_limit_addr;
  uint32_t dst_ram_control;

  cvm::log(cvm::MEDIUM, "[dst_trace_seq] Trace Funnel Configuration Started....\n");
  if(smem_mode){
    ram_start_addr = 0; // Replace with any random value
    ram_limit_addr = 0x8000'0000; // Replace with any random value
    dst_ram_control = 0x13;
  }
  else {
    ram_start_addr = 0x0; // Replace with any random value
    ram_limit_addr = 0x8000; // Replace with any random value
    dst_ram_control = 0x3;
  }
  co_await write(tr_dst_ram_start_low, SZ_8B, ram_start_addr);
  co_await write(tr_dst_ram_limit_low, SZ_8B, ram_limit_addr);
  co_await write(tr_dst_ram_rp_low, SZ_8B, ram_start_addr);
  co_await write(tr_dst_ram_control, SZ_4B, dst_ram_control);
  co_await enable_trace_funnel();
  co_await write(tr_funnel_control, SZ_4B, 0x3);

  cvm::log(cvm::MEDIUM, "[dst_trace_seq] Trace Funnel Configuration Completed....\n");

  co_return;
}

cvm::messenger::task<void> dst_trace_seq::configure_cla() {
  uint64_t on_time;

  cvm::log(cvm::MEDIUM, "[dst_trace_seq] CLA Configuration Started....\n");
  co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, 0x40);
  co_await write((tr_dst_inst_feature + core_offset), SZ_4B, (0x40000000 | (enabled_core  << 16)));
  co_await write((tr_dst_control + core_offset), SZ_4B, 0x3000007);
  co_await configure_fe_dbm();
  co_await write((cdbg_dbg_mux_sel_cfg + core_offset), SZ_8B, 0x1);
  co_await write((cdbg_dbg_mux_sel_cfg + core_offset), SZ_8B, 0x5);
  co_await write((cdbg_dbg_mux_sel_cfg + core_offset), SZ_8B, 0x9);
  on_time = 0x10000000 + (smem_mode * 0x80000000);
  co_await write((cdbg_cla_counter0 + core_offset), SZ_8B, on_time);
  co_await write((cdbg_node0_eap0_cfg + core_offset), SZ_8B, 0x11211);
  co_await write((cdbg_node1_eap0_cfg + core_offset), SZ_8B, 0x101316);
  co_await write((cdbg_node0_eap1_cfg + core_offset), SZ_8B, 0x1001D);
  co_await write((cdbg_node1_eap1_cfg + core_offset), SZ_8B, 0x100022);
  co_await write((cdbg_cla_ctrl_status + core_offset), SZ_8B, 0x60);

  cvm::log(cvm::MEDIUM, "[dst_trace_seq] CLA Configuration Completed....\n");

  co_return;
}

cvm::messenger::task<void> dst_trace_seq::configure_fe_dbm() {

  cvm::log(cvm::MEDIUM, "[dst_trace_seq] FE DBM Configuration Started....\n");
  co_await csr_write(enabled_core, 0x2, fe_dbg_mux_sel, 0x1);
  co_await csr_write(enabled_core, 0x2, fe_dbg_mux_sel, 0x5);
  co_await csr_write(enabled_core, 0x2, fe_dbg_mux_sel, 0x9);
  co_await csr_write(enabled_core, 0x2, fe_dbg_mux_sel, 0xd);
  co_await csr_write(enabled_core, 0x2, fe_dbg_mux_sel, 0x11);
  co_await csr_write(enabled_core, 0x2, fe_dbg_mux_sel, 0x15);
  co_await csr_write(enabled_core, 0x2, fe_dbg_mux_sel, 0x19);
  co_await csr_write(enabled_core, 0x2, fe_dbg_mux_sel, 0x1d);
  co_await csr_write(enabled_core, 0x2, fe_dbg_mux_sel, 0x21);
  co_await csr_write(enabled_core, 0x2, fe_dbg_mux_sel, 0x25);
  co_await csr_write(enabled_core, 0x2, fe_dbg_mux_sel, 0x29);
  co_await csr_write(enabled_core, 0x2, fe_dbg_mux_sel, 0x35);
  co_await csr_write(enabled_core, 0x2, fe_dbg_mux_sel, 0x39);
  cvm::log(cvm::MEDIUM, "[dst_trace_seq] FE DBM Configuration Completed....\n");
  co_return;
}

cvm::messenger::task<void> dst_trace_seq::disable_trace_funnel() {

  cvm::log(cvm::MEDIUM, "[dst_trace_seq] Disabling Trace Funnel....\n");
  co_await write(tr_funnel_control, SZ_4B, 0x1);
  cvm::log(cvm::MEDIUM, "[dst_trace_seq] Disabled Trace Funnel....\n");
  co_return;
}

cvm::messenger::task<void> dst_trace_seq::enable_trace_funnel() {
  uint32_t dis_input;
  uint8_t dst_core_dis;

  dst_core_dis = 1 << enabled_core;
  dst_core_dis = ~dst_core_dis;
  dis_input = dst_core_dis << 8;

  // Configure Funnel disable inputs
  co_await write(tr_funnel_disinput, SZ_4B, dis_input);
  cvm::log(cvm::MEDIUM, "[dst_trace_seq] enabling Trace Funnel....\n");  
  co_await write(tr_funnel_control, SZ_4B, 0x3);
  cvm::log(cvm::MEDIUM, "[dst_trace_seq] enabled Trace Funnel....\n");

  co_return;
}


cvm::messenger::task<void> dst_trace_seq::reset_trace_funnel() {

  cvm::log(cvm::MEDIUM, "[dst_trace_seq] Resetting Trace Funnel....\n");
  co_await write(tr_funnel_control, SZ_4B, 0x0);  
  cvm::log(cvm::MEDIUM, "[dst_trace_seq] Resetting Trace Funnel Completed....\n");
  
  co_return;
}

// DST Functions
cvm::messenger::task<void> dst_trace_seq::check_dst_counter_value() {
  uint32_t target_cnt, value_cnt;

  cvm::log(cvm::NONE, "[dst_trace_seq] Check DST Counter0 Values \n");
  while (true) {
    for(int cnt_loop=0;cnt_loop < 50;cnt_loop ++){
      co_await tick();
    }
    auto data = co_await read(cdbg_cla_counter0 + core_offset, SZ_8B);
    value_cnt = 0xFFFF & data;
    target_cnt = (0xFFFF0000 & data) >> 16;
    if (value_cnt > target_cnt) {
      break;
    }
  }

  co_return;
}

cvm::messenger::task<void> dst_trace_seq::poll_dst_ram_empty() {

  cvm::log(cvm::NONE, "[dst_trace_seq] Poll DST Trace RAM Empty....\n");
  while (true) {

    for(int cnt_loop=0;cnt_loop < 50;cnt_loop ++){
      co_await tick();
    }

    auto data = co_await read(tr_dst_ram_control, SZ_4B);
    if ((data & (1 << tr_ram_empty_idx))){
      cvm::log(cvm::NONE, "[dst_trace_seq] Observed DST Trace RAM Empty ....\n");
      break;
    }
  }

  co_return;
}
cvm::messenger::task<void> dst_trace_seq::disable_dst_trace() {

  cvm::log(cvm::NONE, "[dst_trace_seq] Disabling DST Trace Generation....\n");

  auto data = co_await read(cdbg_cla_ctrl_status + core_offset, SZ_8B);
  data = data & 0xFFFF'FF9F;
  co_await write(cdbg_cla_ctrl_status + core_offset,SZ_8B,data);

  data = co_await read(tr_dst_control + core_offset, SZ_4B);
  data = data & 0xFFFF'FFF9;
  co_await write(tr_dst_control + core_offset,SZ_4B,data);

  while(1){
    auto data = co_await read(tr_dst_control + core_offset, SZ_4B);
    if (data & (1 << tr_dst_control_empty_idx)){
      cvm::log(cvm::NONE, "[dst_trace_seq] DST Packetizer flush observed... \n");
      break;
    }
  }

  cvm::log(cvm::NONE, "[dst_trace_seq] Disabling DST Trace Generation Completed....\n");
  co_return;
}

cvm::messenger::task<void> dst_trace_seq::disable_dst_trace_ram() {

  cvm::log(cvm::NONE, "[dst_trace_seq] Disabling DST Trace RAM....\n");
  auto data = co_await read(tr_dst_ram_control, SZ_4B);
  data = data & 0xFFFF'FFFD;
  co_await write(tr_dst_ram_control,SZ_4B,data);

  cvm::log(cvm::NONE, "[dst_trace_seq] Disabling DST Trace RAM Completed....\n");
  co_return;
}

cvm::messenger::task<void> dst_trace_seq::read_dst_trace_ram() {

  cvm::log(cvm::NONE, "[dst_trace_seq] Starting DST SRAM Reading... \n");

  auto WritePointer = co_await read(tr_dst_ram_wp_low, SZ_4B);
  cvm::log(cvm::FULL, "[dst_trace_seq] DST SRAM WritePointer data: {:#x}\n", WritePointer);

  auto ReadPointer = co_await read(tr_dst_ram_rp_low, SZ_4B);
  cvm::log(cvm::FULL, "[dst_trace_seq] DST SRAM ReadPointer data: {:#x}\n", ReadPointer);

  ReadPointer = (WritePointer - ReadPointer)/4;
  cvm::log(cvm::NONE, "[dst_trace_seq] DST SRAM Number of DWs to read: {:#x}\n", ReadPointer);

  for(uint32_t i=0; i< uint32_t(ReadPointer) ; i++){
    WritePointer = co_await read(tr_dst_ram_data, SZ_4B);
  }

  cvm::log(cvm::NONE, "[dst_trace_seq] Completed DST SRAM Reading... \n");
  co_return;
}

cvm::messenger::task<void> dst_trace_seq::enable_dst_trace_ram() {
  cvm::log(cvm::NONE, "[dst_trace_seq] Re-enabling DST Trace RAM....\n");

  //co_await write(tr_dst_ram_limit_low,SZ_4B,0x1000);
  //co_await write(tr_dst_ram_rp_low,SZ_4B,0x0);
  auto data = co_await read(tr_dst_ram_control, SZ_4B);
  data = data | 0x3;
  co_await write(tr_dst_ram_control,SZ_4B,data);

  cvm::log(cvm::NONE, "[dst_trace_seq] Re-enabling DST Trace RAM Completed....\n");
  co_return;
}

cvm::messenger::task<uint64_t> dst_trace_seq::read(uint64_t addr, size_t sz, block_t block /* = BLOCK */) {

  assert(sz <= 8);

  uint64_t rdata = 0;
  uint8_t offset = static_cast<uint8_t>(addr & 0x3f);
  
  // Clear channel to remove any stale responses
  cvm::registry::messenger.clear_channel<axi::r_t>(channel);
  
  // Use RPC to allocate transaction ID
  axi::a_no_id_t ar_txn;
  ar_txn.w    = false;
  ar_txn.addr = addr;
  ar_txn.size = log2(sz);
  
  unsigned id;
  if (!cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_mst_loc_, ar_txn, id)) {
    cvm::log(cvm::ERROR, "[dst_trace_seq] read req - addr={:#x}, sz={} failed to allocate axi ID\n", addr, sz);
    co_return 0;
  }
  
  cvm::log(cvm::MEDIUM, "[dst_trace_seq] read req - addr={:#x}, sz={}\n", addr, sz);

  if (!block)
    co_return rdata;

  // Wait for response on dedicated channel with ID filtering
  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel, [&id](const auto& r) { return r.id == id; });
  rdata = convert_to_dword_array(resp.data, offset, sz);
  
  cvm::log(cvm::MEDIUM, "[dst_trace_seq] read resp - id={}, addr={:#x}, sz={}, data={:#x}\n", resp.id, addr, sz, rdata);

  co_return rdata;
}

cvm::messenger::task<void> dst_trace_seq::write(uint64_t addr, size_t sz, uint64_t data, block_t block /* = BLOCK */) {

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
  
  // Clear channel to remove any stale responses
  cvm::registry::messenger.clear_channel<axi::b_t>(b_channel_);

  // Check for AXI transactor lock before driving
  while(1) {
    auto lock = cvm::registry::messenger.call<overlay_mst_t::try_lock_rpc>(axi_mst_loc_);
    if(lock) { break; }
    co_await tick();
  }
  
  // Use RPC to allocate transaction ID
  axi::a_no_id_t aw_txn;
  aw_txn.w    = true;
  aw_txn.addr = aligned_addr;
  aw_txn.size = log2(64);
  
  unsigned id;
  if (!cvm::registry::messenger.call<overlay_mst_t::push_aw_no_id_rpc>(axi_mst_loc_, aw_txn, id)) {
    cvm::log(cvm::ERROR, "[dst_trace_seq] write req - addr={:#x}, sz={} failed to allocate axi ID\n", aligned_addr, sz);
    co_return;
  }
  
  // Send write data
  cvm::registry::messenger.call<overlay_mst_t::push_w_rpc>(axi_mst_loc_, axi::w_t{byte_array, strb, 1});

  cvm::log(cvm::MEDIUM, "[dst_trace_seq] write req - addr={:#x}, sz={}, data={:#x}, mask={:#x}\n", aligned_addr, sz, data, mask);

  if (!block)
    co_return;

  // Wait for response on dedicated channel with ID filtering
  auto resp = co_await cvm::registry::messenger.wait<axi::b_t>(b_channel_, [&id](const auto& b) { return b.id == id; });
  cvm::log(cvm::MEDIUM, "[dst_trace_seq] write resp - id={}, addr={:#x}, sz={}, data={:#x}, mask={:#x}\n", resp.id, aligned_addr, sz, data, mask);

  co_return;
}


cvm::messenger::task<void> dst_trace_seq::csr_write(uint32_t core_id, uint32_t unit, uint64_t addr, uint64_t data) {
  transactor::read_t read_req ;
  uint64_t cmd = 0;
  uint32_t offset = core_id * core_fuse_offset;
  cvm::log(cvm::MEDIUM, "[dst_trace_seq] csr write req - core_id = {}, addr={:#x}, data={:#x} \n", core_id, addr, data );
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

cvm::messenger::task<void> dst_trace_seq::axi_write_mmr_granular(uint64_t addr) {

  axi::a_no_id_t aw_txn;
  uint64_t aligned_addr = addr & ~0x3full;
  unsigned id;
  aw_txn.w    = true;
  //aw_txn.id   = 1;
  //aw_txn.addr = 0x60000000;
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
  aw_txn.seqid  =DST_TRACE_SEQ_ID;
  
  cvm::log(cvm::MEDIUM, "[dst_trace_seq] dst_trace_seq WRITE GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", aw_txn.addr);

  if (!cvm::registry::messenger.call<overlay_mst_t::push_aw_no_id_rpc>(axi_mst_loc_, aw_txn, id)) {
    auto axi_idalloc_done = co_await check_axi_bresp_timeout(aw_txn, id);
    if (!axi_idalloc_done) {
      co_return;
    }
  }
  co_return;
 
}

cvm::messenger::task<void> dst_trace_seq::axi_write_mmr_data_granular(uint64_t addr, uint64_t data) {
 
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
  
  cvm::log(cvm::MEDIUM, "[dst_trace_seq] dst_trace_seq WRITE DATA GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", addr);
  cvm::registry::messenger.call<overlay_mst_t::push_w_rpc>(axi_mst_loc_, w_txn);

  co_return;
}

cvm::messenger::task<uint64_t> dst_trace_seq::axi_read_mmr_granular(const transactor::read_t& r ) {
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
  ar_txn.seqid  =DST_TRACE_SEQ_ID;
  
  cvm::log(cvm::MEDIUM, "[dst_trace_seq] dst_trace_seq AXI READ GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", ar_txn.addr);

  if (!cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_mst_loc_, ar_txn , id)) {
    auto axi_idalloc_done = co_await check_axi_rresp_timeout(ar_txn, id);
    if (!axi_idalloc_done) {
      co_return 0;
    }
  }

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);

  rdata = convert_to_dword_array(resp.data,offset,SZ_8B);

  cvm::log(cvm::MEDIUM, "[dst_trace_seq] dst_trace_seq AXI READ GRANULAR read addr {:#X} completed\n",ar_txn.addr);
  co_return rdata;
}

uint64_t dst_trace_seq::convert_to_dword_array(const std::vector<uint8_t>& byte_array, uint8_t shift, size_t sz) {

  uint64_t result=0;
  for (int i = 0; i < static_cast<int>(sz); ++i) {
     result = result | static_cast<uint64_t>(byte_array[shift+i]) << (i*8);
  }

  return result;
}

std::vector<uint8_t> dst_trace_seq::convert_to_byte_array(uint64_t data, uint8_t shift) {
  
  std::vector<uint8_t> byte_vector(64, 0); // Initialize a 64-byte vector with zeros
  for (int i = 0; i < 8; ++i) {
      if (shift + i < 64) {
          byte_vector[shift + i] = static_cast<uint8_t>((data >> (i * 8)) & 0xFF);
      }
  }

  return byte_vector;
}

void dst_trace_seq::terminate_test(uint8_t terminate_test)
{
  cvm::registry::callbacks.push(
    scope_,
    [terminate_test]() {
      cvm::log(cvm::NONE, "[dst_trace_seq] Test {} \n", terminate_test ? " terminated" : "not terminated");
      terminate_dst_trace_seq_func(terminate_test);
    });
}

cvm::messenger::task<bool> dst_trace_seq::check_axi_bresp_timeout(axi::a_no_id_t aw_txn, unsigned& id) {

  uint32_t axi_bresp_cycle_cnt = 0;

  while (true) {
    co_await tick();

    if (axi_bresp_cycle_cnt >= FLAGS_axi_resp_timeout) {
      cvm::log(cvm::ERROR, "[dst_trace_seq] [axi_mst] Error: No free id's remaining for axi master\n");
      co_return false;
    }
    axi_bresp_cycle_cnt++;

    if (cvm::registry::messenger.call<overlay_mst_t::push_aw_no_id_rpc>(axi_mst_loc_, aw_txn, id)) {
      co_return true;
    }
  }

  co_return true;
}

cvm::messenger::task<bool> dst_trace_seq::check_axi_rresp_timeout(axi::a_no_id_t ar_txn, unsigned& id) {

  uint32_t axi_rresp_cycle_cnt = 0;

  while (true) {
    co_await tick();

    if (axi_rresp_cycle_cnt >= FLAGS_axi_resp_timeout) {
      cvm::log(cvm::ERROR, "[dst_trace_seq] [axi_mst] Error: No free id's remaining for axi master\n");
      co_return false;
    }
    axi_rresp_cycle_cnt++;

    if (cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_mst_loc_, ar_txn, id)) {
      co_return true;
    }
  }
  
  co_return true;
}
