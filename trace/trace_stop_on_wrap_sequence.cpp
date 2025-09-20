#include "trace_stop_on_wrap_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "fmt/ranges.h"

REGISTRY_register(trace_stop_on_wrap_sequence, TRACE, cvm::registry::all);

DEFINE_bool(ntrace_stop_on_wrap_seq_en, false, "Enable ntrace_stop_on_wrap_seq_en_sequence in the sim");
DEFINE_bool(dst_stop_on_wrap_seq_en, false, "Enable dst_stop_on_wrap_seq_en_sequence in the sim");

extern "C" {
  void terminate_ntrace_test_func(uint8_t val);

  uint8_t trace_stop_on_wrap_en_func() {
    return FLAGS_ntrace_stop_on_wrap_seq_en || FLAGS_dst_stop_on_wrap_seq_en || FLAGS_enable_ntrace_in_boot;
  }
}

trace_stop_on_wrap_sequence::trace_stop_on_wrap_sequence
  (cvm::topology::loc_t loc, unsigned) : 
  loc_(loc), scope_(nullptr) {

  // Topology
  axi_mst_loc_ = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST", 0);

  // Scope
  cvm::registry::messenger.connect<svScope>(
    loc_, 
    [this](svScope s) { return this->set_scope(s); }
  );

  if (FLAGS_ntrace_stop_on_wrap_seq_en || FLAGS_enable_ntrace_in_boot) {
    ntrace_main_thread();
  }

  if (FLAGS_dst_stop_on_wrap_seq_en) {
    dst_main_thread();
  }

}

trace_stop_on_wrap_sequence::~trace_stop_on_wrap_sequence() {
  // cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_ntrace_stop_on_wrap_count\": \"{}\"}}\n", id_, ntrace_stop_on_wrap_count_);
  // cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_dst_trace_stop_on_wrap_count\": \"{}\"}}\n", id_, dst_trace_stop_on_wrap_count_);
}

void trace_stop_on_wrap_sequence::ntrace_main_thread()
{
  auto *task = +[] (trace_stop_on_wrap_sequence* m) -> cvm::messenger::task<void> {
    co_await m->ntrace_main();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void trace_stop_on_wrap_sequence::dst_main_thread()
{
  auto *task = +[] (trace_stop_on_wrap_sequence* m) -> cvm::messenger::task<void> {
    co_await m->dst_main();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> trace_stop_on_wrap_sequence::tick()
{
  co_await cvm::registry::messenger.wait<rv_tester_transactions::trace::m_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::core_no_fetch()
{
  co_await cvm::registry::messenger.wait<rv_tester_transactions::trace::m_core_no_fetch<>>(loc_);
  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::ntrace_main() {

  // Wait for no fetch
  co_await core_no_fetch();

  cvm::log(cvm::MEDIUM, "[trace] Starting NTrace stop-on-wrap sequence\n");

  while (true) {
    co_await poll_ntrace_ram_en(ENABLE);
    co_await poll_ntrace_ram_en(DISABLE);
    co_await disable_ntrace_encoder();
    co_await disable_trace_funnel();
    co_await reset_ntrace_ram();

    auto is_sram_mode = !((co_await read(tr_ram_control, SZ_4B)) & ~tr_ram_mem_mode_mask);
    if (is_sram_mode) {
      co_await read_ntrace_ram();
    }

    if (FLAGS_enable_ntrace_in_boot) {
      co_await reset_trace_funnel();
      co_return;
    }

    for (int cnt_loop=0; cnt_loop<800; cnt_loop++) {
      co_await tick();
    }

    auto tr_end_status = (co_await read(tr_funnel_disinput, SZ_4B)) & 0xFFFF'FFFF;
    cvm::log(cvm::MEDIUM, "[trace] Trace Test end status = {} : tr_end_status = {:#x} ....\n", (tr_end_status == tr_end_indicator_val), tr_end_status);

    if (tr_end_status != tr_end_indicator_val) {
      co_await enable_ntrace_ram();
      co_await enable_trace_funnel();
      co_await enable_ntrace_encoder();
    }
    else {
      reset_trace_funnel();
      break;
    }
  }

  for (int cnt_loop=0; cnt_loop<50; cnt_loop++) {
    co_await tick();
  }

  terminate_test(1);

  cvm::log(cvm::MEDIUM, "[trace] NTrace stop-on-wrap sequence completed\n");

  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::dst_main() {

  // Wait for no fetch
  co_await core_no_fetch();

  cvm::log(cvm::MEDIUM, "[dst_trace] Starting DST Trace stop-on-wrap sequence\n");

  while (true) {

    co_await check_dst_trace_ram_en();
    co_await poll_dst_trace_ram_en();
    co_await disable_dst_trace();
    co_await disable_trace_funnel();
    co_await disable_dst_trace_ram();
    co_await read_dst_trace_ram();
    
    auto dis_input = co_await read(tr_funnel_disinput, SZ_4B);
    if(dis_input != 0xABCD) {
      co_await enable_dst_trace_ram();
      co_await enable_trace_funnel();
      co_await enable_dst_trace();
    }
    else {
      co_await reset_trace_funnel();
      break;
    }

  }

  co_return;
}

// NTrace Functions
cvm::messenger::task<void> trace_stop_on_wrap_sequence::poll_ntrace_ram_en(trace_ram_status_t trace_ram_status /* = ENABLE */) {

  cvm::log(cvm::MEDIUM, "[trace] Checking NTrace RAM {} status....\n", (trace_ram_status == ENABLE) ? "Enable" : "Disable");

  while (true) {

    for(int cnt_loop=0; cnt_loop<800; cnt_loop++) {
      co_await tick();
    }

    auto data = co_await read(tr_ram_control, SZ_4B);
    uint32_t tr_ram_en = data & (1 << tr_ram_enable_idx);
    uint32_t tr_ram_active = data & (1 << tr_ram_active_idx);
    
    if ((trace_ram_status == ENABLE && tr_ram_en) || (trace_ram_status == DISABLE && !tr_ram_en && tr_ram_active))
    {
      cvm::log(cvm::MEDIUM, "[trace] NTrace RAM {} : tr_ram_control = {:#x}\n", (trace_ram_status == ENABLE) ? "Enabled" : "Disabled", data);
      break;
    }

  }

  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::disable_ntrace_encoder() {

  cvm::log(cvm::MEDIUM, "[trace] Disabling NTrace Encoder....\n");

  auto data = co_await read(tr_te_control, SZ_4B);
  data &= FLAGS_enable_ntrace_in_boot ? tr_te_control_active_mask : tr_te_control_enable_mask;
  co_await write(tr_te_control, SZ_4B, data);

  while(true) {
    auto data = co_await read(tr_te_control, SZ_4B);
    uint32_t tr_te_empty = data & (1 << tr_te_control_empty_idx);
    if (tr_te_empty) {
      cvm::log(cvm::MEDIUM, "[trace] NTrace Packetizer flush observed... \n");
      break;
    }
  }

  cvm::log(cvm::MEDIUM, "[trace] Disabled NTrace Encoder....\n");

  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::disable_trace_funnel() {

  cvm::log(cvm::MEDIUM, "[trace] Disabling Trace Funnel....\n");
  co_await write(tr_funnel_control, SZ_4B, 0x1);

  cvm::log(cvm::MEDIUM, "[trace] Disabled Trace Funnel....\n");

  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::disable_ntrace_ram() {

  cvm::log(cvm::MEDIUM, "[trace] Disabling NTrace RAM....\n");
  auto data = co_await read(tr_ram_control, SZ_4B);
  data &= tr_ram_enable_mask;
  co_await write(tr_ram_control, SZ_4B, data);
  
  cvm::log(cvm::MEDIUM, "[trace] Disabling NTrace RAM Completed....\n");  

  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::reset_ntrace_ram() {

  cvm::log(cvm::MEDIUM, "[trace] Resetting NTrace RAM....\n");
  auto data = co_await read(tr_ram_control, SZ_4B);
  data &= tr_ram_active_mask;
  co_await write(tr_ram_control, SZ_4B, data);

  cvm::log(cvm::MEDIUM, "[trace] Resetting NTrace RAM Completed....\n");

  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::read_ntrace_ram() {

  cvm::log(cvm::MEDIUM, "[trace] Started reading NTrace SRAM data... \n");

  auto WritePointer = co_await read(tr_ram_wp_low, SZ_4B);  
  cvm::log(cvm::MEDIUM, "[trace] NTrace SRAM WritePointer value: {:#x}\n", WritePointer);

  auto ReadPointer = co_await read(tr_ram_rp_low, SZ_4B);
  cvm::log(cvm::MEDIUM, "[trace] NTrace SRAM ReadPointer data: {:#x}\n", ReadPointer);

  auto ram_data_DW_count = (WritePointer - ReadPointer)/4;
  cvm::log(cvm::MEDIUM, "[trace] NTrace SRAM Number of DWs to read: {:#x}\n", ram_data_DW_count);

  for (uint32_t i=0; i<uint32_t(ram_data_DW_count); i++)
  {
    co_await read(tr_ram_data, SZ_4B);
  }

  cvm::log(cvm::MEDIUM, "[trace] Completed reading NTrace SRAM data... \n");

  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::enable_ntrace_ram() {

  cvm::log(cvm::MEDIUM, "[trace] Re-enabling NTrace RAM....\n");

  co_await write(tr_ram_start_low, SZ_4B, 0x0);
  co_await write(tr_ram_rp_low, SZ_4B, 0x0);
  co_await write(tr_ram_wp_low, SZ_4B, 0x0);
  co_await write(tr_ram_limit_low, SZ_4B, 0x400);
  
  auto data = co_await read(tr_ram_control, SZ_4B);
  data |= (~tr_ram_active_mask);
  co_await write(tr_ram_control, SZ_4B, data);
  
  cvm::log(cvm::MEDIUM, "[trace] Re-enabled NTrace RAM....\n");
  
  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::enable_trace_funnel() {

  // Configure Funnel disable inputs
  cvm::log(cvm::MEDIUM, "[trace] Configured Trace Funnel Disable Inputs....\n");
  co_await write(tr_funnel_disinput, SZ_4B, 0xFE);

  cvm::log(cvm::MEDIUM, "[trace] Re-enabling Trace Funnel....\n");  
  co_await write(tr_funnel_control, SZ_4B, 0x3);
  cvm::log(cvm::MEDIUM, "[trace] Re-enabled Trace Funnel....\n");

  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::enable_ntrace_encoder() {

  cvm::log(cvm::MEDIUM, "[trace] Re-enabling NTrace Encoder....\n");

  auto data = co_await read(tr_te_control, SZ_4B);
  data |= (~tr_te_control_active_mask);
  co_await write(tr_te_control, SZ_4B, data);

  cvm::log(cvm::MEDIUM, "[trace] Re-enabled NTrace Encoder....\n");

  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::reset_trace_funnel() {

  cvm::log(cvm::MEDIUM, "[trace] Resetting Trace Funnel....\n");
  
  co_await write(tr_funnel_control, SZ_4B, 0x0);  
  cvm::log(cvm::MEDIUM, "[trace] Resetting Trace Funnel Completed....\n");
  
  co_return;
}

// DST Functions
cvm::messenger::task<void> trace_stop_on_wrap_sequence::check_dst_trace_ram_en() {

  cvm::log(cvm::NONE, "[trace] Check DST RAM ENABLE \n");
  while (true) {

    auto data = co_await read(tr_dst_ram_control, SZ_4B);
    if (data & (1 << tr_ram_enable_idx)){
      cvm::log(cvm::NONE, "[trace] Observed DST RAM ENABLE data: {:#x}\n", data);
      break;
    }

    for(int cnt_loop=0;cnt_loop < 800;cnt_loop ++){
      co_await tick();
    }
  }

  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::poll_dst_trace_ram_en() {

  cvm::log(cvm::NONE, "[trace] Checking DST Trace RAM Enable....\n");
  while (true) {

    for(int cnt_loop=0;cnt_loop < 800;cnt_loop ++){
      co_await tick();
    }

    auto data = co_await read(tr_dst_ram_control, SZ_4B);
    if ((data & (1 << tr_ram_enable_idx)) == 0){
      cvm::log(cvm::NONE, "[trace] Observed DST Trace RAM Enable ....\n");
      break;
    }
  }

  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::disable_dst_trace() {

  cvm::log(cvm::NONE, "[trace] Disabling DST Trace Generation....\n");

  auto data = co_await read(cdbg_cla_ctrl_status, SZ_8B);
  data = data & 0xFFFF'FF9F;
  co_await write(cdbg_cla_ctrl_status,SZ_8B,data);

  data = co_await read(tr_dst_control, SZ_4B);
  data = data & 0xFFFF'FFF9;
  co_await write(tr_dst_control,SZ_4B,data);

  while(1){
    auto data = co_await read(tr_dst_control, SZ_4B);
    if (data & (1 << tr_dst_control_empty_idx)){
      cvm::log(cvm::NONE, "[trace] DST Packetizer flush observed... \n");
      break;
    }
  }

  cvm::log(cvm::NONE, "[trace] Disabling DST Trace Generation Completed....\n");
  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::disable_dst_trace_ram() {

  cvm::log(cvm::NONE, "[trace] Disabling DST Trace RAM....\n");
  auto data = co_await read(tr_dst_ram_control, SZ_4B);
  data = data & 0xFFFF'FFFC;
  co_await write(tr_dst_ram_control,SZ_4B,data);

  cvm::log(cvm::NONE, "[trace] Disabling DST Trace RAM Completed....\n");
  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::read_dst_trace_ram() {

  cvm::log(cvm::NONE, "[trace] Starting DST SRAM Reading... \n");

  auto WritePointer = co_await read(tr_dst_ram_wp_low, SZ_4B);
  cvm::log(cvm::NONE, "[trace] DST SRAM WritePointer data: {:#x}\n", WritePointer);

  auto ReadPointer = co_await read(tr_dst_ram_rp_low, SZ_4B);
  cvm::log(cvm::NONE, "[trace] DST SRAM ReadPointer data: {:#x}\n", ReadPointer);

  ReadPointer = (WritePointer - ReadPointer)/4;
  cvm::log(cvm::NONE, "[trace] DST SRAM Number of DWs to read: {:#x}\n", ReadPointer);

  for(uint32_t i=0; i< uint32_t(ReadPointer) ; i++){
    WritePointer = co_await read(tr_dst_ram_data, SZ_4B);
  }

  cvm::log(cvm::NONE, "[trace] Completed DST SRAM Reading... \n");
  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::enable_dst_trace_ram() {
  cvm::log(cvm::NONE, "[trace] Re-enabling DST Trace RAM....\n");

  co_await write(tr_dst_ram_limit_low,SZ_4B,0x1000);
  co_await write(tr_dst_ram_rp_low,SZ_4B,0x0);
  auto data = co_await read(tr_dst_ram_control, SZ_4B);
  data = data | 0x3;
  co_await write(tr_dst_ram_control,SZ_4B,data);

  cvm::log(cvm::NONE, "[trace] Re-enabling DST Trace RAM Completed....\n");
  co_return;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::enable_dst_trace() {

  cvm::log(cvm::NONE, "[trace] Re-enabling DST Trace....\n");
  co_await write(cdbg_cla_counter0,SZ_8B,0x3A00'0000);

  // Enable CLA CLK
  auto data = co_await read(cdbg_cla_ctrl_status, SZ_8B);
  data = data | 0x40; 
  co_await write(cdbg_cla_ctrl_status,SZ_8B,data);
  
  // Enable DST Packetizer
  auto dst_cntrl = co_await read(tr_dst_control, SZ_4B);
  dst_cntrl = dst_cntrl | 0x7;
  co_await write(tr_dst_control,SZ_4B,dst_cntrl);

  // Enable CLA EAP
  data = data | 0x60; 
  co_await write(cdbg_cla_ctrl_status,SZ_8B,data);

  cvm::log(cvm::NONE, "[trace] Re-enabling DST Trace Completed....\n");
  co_return;
}

cvm::messenger::task<uint64_t> trace_stop_on_wrap_sequence::read(uint64_t addr, size_t sz, block_t block /* = BLOCK */) {

  assert(sz <= 8);

  uint64_t rdata = 0;
  uint8_t offset = static_cast<uint8_t>(addr & 0x3f);
  
  cvm::log(cvm::MEDIUM, "[trace] read req - addr={:#x}, sz={}\n", addr, sz);
  cvm::registry::messenger.signal(axi_mst_loc_, transactor::read_request_t{addr, sz});

  if (!block)
    co_return rdata;

  auto resp = co_await cvm::registry::messenger.wait<transactor::read_response_t>(axi_mst_loc_);
  rdata = convert_to_dword_array(resp.data,offset,sz);
  
  cvm::log(cvm::MEDIUM, "[trace] read resp - id={}, addr={:#x}, sz={}, data={:#x}\n", resp.id, addr, sz, rdata);

  co_return rdata;
}

cvm::messenger::task<void> trace_stop_on_wrap_sequence::write(uint64_t addr, size_t sz, uint64_t data, block_t block /* = BLOCK */) {

  assert(sz <= 8);

  uint8_t offset = static_cast<uint8_t>(addr & 0x3f);
  uint64_t aligned_addr = addr & ~0x3full;
  auto byte_array = convert_to_byte_array(data, offset);

  uint64_t mask = (sz == 64) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;

  std::vector<bool> strb(64, false);
  for (int i = 0; i < static_cast<int>(sz); ++i) {
      if (offset + i < 64) {
          cvm::log(cvm::MEDIUM, "[trace] write strb index={:#x}\n", (offset + i));
          strb[offset + i] = 1;
      }
  }

  cvm::log(cvm::MEDIUM, "[trace] write req - addr={:#x}, sz={}, data={:#x}, mask={:#x}\n", aligned_addr, sz, data, mask);
  cvm::registry::messenger.signal(axi_mst_loc_, transactor::write_request_t{aligned_addr, sz, byte_array, strb});

  if (!block)
    co_return;

  auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(axi_mst_loc_);
  cvm::log(cvm::MEDIUM, "[trace] write resp - id={}, addr={:#x}, sz={}, data={:#x}, mask={:#x}\n", resp.id, aligned_addr, sz, data, mask);

  co_return;
}

uint64_t trace_stop_on_wrap_sequence::convert_to_dword_array(const std::vector<uint8_t>& byte_array, uint8_t shift, size_t sz) {

  uint64_t result=0;
  for (int i = 0; i < static_cast<int>(sz); ++i) {
     result = result | static_cast<uint64_t>(byte_array[shift+i]) << (i*8);
  }

  return result;
}

std::vector<uint8_t> trace_stop_on_wrap_sequence::convert_to_byte_array(uint64_t data, uint8_t shift) {
  
  std::vector<uint8_t> byte_vector(64, 0); // Initialize a 64-byte vector with zeros
  for (int i = 0; i < 8; ++i) {
      if (shift + i < 64) {
          byte_vector[shift + i] = static_cast<uint8_t>((data >> (i * 8)) & 0xFF);
      }
  }

  return byte_vector;
}

void trace_stop_on_wrap_sequence::terminate_test(uint8_t terminate_test)
{
  cvm::registry::callbacks.push(
    scope_,
    [terminate_test]() {
      cvm::log(cvm::MEDIUM, "[trace] Test {} \n", terminate_test ? " terminated" : "not terminated");
      terminate_ntrace_test_func(terminate_test);
    });
}
