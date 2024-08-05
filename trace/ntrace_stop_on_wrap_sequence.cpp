#include "ntrace_stop_on_wrap_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"

REGISTRY_register(ntrace_stop_on_wrap_sequence, TRACE, cvm::registry::all);

DEFINE_bool(ntrace_stop_on_wrap, true, "Enable ntrace_stop_on_wrap_sequence in the sim");

extern "C" {
  //void trace_init();
}

ntrace_stop_on_wrap_sequence::ntrace_stop_on_wrap_sequence
  (cvm::topology::loc_t loc, unsigned) : 
  loc_(loc), scope_(nullptr) {

  // Topology
  axi_mst_loc_ = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST", 0);

  // Scope
  cvm::registry::messenger.connect<svScope>(
    loc_, 
    [this](svScope s) { return this->set_scope(s); }
  );

  if (!FLAGS_ntrace_stop_on_wrap)
    return;

  // ntrace_stop_on_wrap sequence thread
  main_thread();
}

ntrace_stop_on_wrap_sequence::~ntrace_stop_on_wrap_sequence() {
  //cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_trace_ntrace_stop_on_wrap_count\": \"{}\"}}\n", id_, ntrace_stop_on_wrap_count_);
}

void ntrace_stop_on_wrap_sequence::main_thread() {
  auto *task = +[] (ntrace_stop_on_wrap_sequence* m) -> cvm::messenger::task<void> {
    co_await m->main();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> ntrace_stop_on_wrap_sequence::main() {
  // Wait for no fetch
  co_await core_no_fetch();
  cvm::log(cvm::NONE, "[trace] Starting trace sequence\n");

  while (true) {
    co_await tick();
    co_await poll_ntrace_ram_en();
    co_await disable_ntrace_ram();
    co_await read_ntrace_ram();
    co_await enable_ntrace_ram();
    co_await enable_ntrace();
  }

  co_return;
}

cvm::messenger::task<void> ntrace_stop_on_wrap_sequence::poll_ntrace_ram_en() {
  while (true) {
    co_await tick();
    cvm::log(cvm::NONE, "[trace] read tr_ram_control\n");
    auto data = co_await read(tr_ram_control, SZ_4B);
    cvm::log(cvm::NONE, "[trace] read tr_ram_control data: {:#x}\n", data);
    if (data & (1 << tr_ram_enable_idx))
      break;
  }

  co_return;
}

cvm::messenger::task<void> ntrace_stop_on_wrap_sequence::disable_ntrace_ram() {
  co_return;
}

cvm::messenger::task<void> ntrace_stop_on_wrap_sequence::read_ntrace_ram() {
  co_return;
}

cvm::messenger::task<void> ntrace_stop_on_wrap_sequence::enable_ntrace_ram() {
  co_return;
}

cvm::messenger::task<void> ntrace_stop_on_wrap_sequence::enable_ntrace() {
  co_return;
}

cvm::messenger::task<void> ntrace_stop_on_wrap_sequence::core_no_fetch() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::trace::m_core_no_fetch<>>(loc_);
  co_return;
}

cvm::messenger::task<void> ntrace_stop_on_wrap_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::trace::m_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<uint64_t> ntrace_stop_on_wrap_sequence::read(uint64_t addr, size_t sz) {
  assert(sz <= 8);
  cvm::log(cvm::NONE, "[trace] read req - addr={:#x}, sz={}\n", addr, sz);
  cvm::registry::messenger.signal(axi_mst_loc_, transactor::read_request_t{addr, sz});
  auto resp = co_await cvm::registry::messenger.wait<transactor::read_response_t>(axi_mst_loc_);
  auto data = convert_to_dword_array(resp.data);
  cvm::log(cvm::NONE, "[trace] read resp - id={}, addr={:#x}, sz={}, data={:#x}\n", resp.id, addr, sz, data[0]);
  co_return data[0];
}

cvm::messenger::task<void> ntrace_stop_on_wrap_sequence::write(uint64_t addr, size_t sz, uint64_t data, bool block /* = true */) {
  assert(sz <= 8);
  uint64_t mask = (sz == 8) ? ~uint64_t(0) : ((uint64_t)1 << (sz*8)) - 1;
  auto byte_array = convert_to_byte_array({data});
  std::vector<bool> strb(8, false);
  for(int i=0; i<8; ++i)
    strb[i] = (mask & (0xFFull << (i*8))) != 0;
  cvm::log(cvm::NONE, "[trace] write req - addr={:#x}, sz={}, data={:#x}, strb={:#x}\n", addr, sz, data, mask);
  cvm::registry::messenger.signal(axi_mst_loc_, transactor::write_request_t{addr, sz, byte_array, strb});
  if (!block)
    co_return;
  auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(axi_mst_loc_);
  cvm::log(cvm::NONE, "[trace] write resp - id={}, addr={:#x}, sz={}, data={:#x}, strb={:#x}\n", resp.id, addr, sz, data, mask);
  co_return;
}

std::vector<uint64_t> ntrace_stop_on_wrap_sequence::convert_to_dword_array(const std::vector<uint8_t>& byte_array) {
  std::vector<uint64_t> result(byte_array.size() / sizeof(uint64_t));
  std::copy(reinterpret_cast<const uint64_t*>(byte_array.data()),
            reinterpret_cast<const uint64_t*>(byte_array.data() + byte_array.size()),
            result.begin());
  return result;
}

std::vector<uint8_t> ntrace_stop_on_wrap_sequence::convert_to_byte_array(const std::vector<uint64_t>& dword_array) {
  std::vector<uint8_t> result(dword_array.size() * sizeof(uint64_t));
  std::copy(reinterpret_cast<const uint8_t*>(dword_array.data()),
            reinterpret_cast<const uint8_t*>(dword_array.data()) + dword_array.size() * sizeof(uint64_t),
            result.begin());
  return result;
}

//void ntrace_stop_on_wrap_sequence::init() {
//  cvm::registry::callbacks.push(
//    scope_,
//    []() {
//      trace_init();
//    });
//}
