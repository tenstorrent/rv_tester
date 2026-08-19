// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "src/sysmod/mmr_txn_router/mmr_txn_router.h"
#include "src/transactors/axi_sw/axi_sw_mst_rpc.h"

#include <algorithm>

namespace {

axi::a_no_id_t make_ar(uint64_t addr, size_t length) {
  axi::a_no_id_t ar(false, addr, 0);
  // ms_mmr rejects ring AR with AXI size < 32b. Upsize narrow cosim reads
  // on the reroute path; mmr_txn_router::read() still returns only the
  // requested byte count to the memory model.
  size_t axi_length = length;
  if (axi_length > 0 && axi_length < 4)
    axi_length = 4;
  // Mirror axi_sw_mst::a_wrapper burst sizing for power-of-two lengths.
  switch (axi_length) {
  case 1:
    ar.size = 0;
    ar.len = 0;
    break;
  case 2:
    ar.size = 1;
    ar.len = 0;
    break;
  case 4:
    ar.size = 2;
    ar.len = 0;
    break;
  case 8:
    ar.size = 3;
    ar.len = 0;
    break;
  case 16:
    ar.size = 4;
    ar.len = 0;
    break;
  case 32:
    ar.size = 5;
    ar.len = 0;
    break;
  case 64:
    ar.size = 6;
    ar.len = 0;
    break;
  default:
    ar.len = length - 1;
    ar.size = 0;
    break;
  }
  return ar;
}

} // namespace

mmr_txn_router::mmr_txn_router(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
    : device(tag, addr, size, loc, &mmr_txn_router::write, &mmr_txn_router::read, this), axi_mst_loc_l(axi_mst_loc) {
  cvm::log(cvm::HIGH, " [mmr_txn_router] Constructor \n");
}

void mmr_txn_router::configure() {
  device::configure();
  read_resp_channel_ = cvm::registry::messenger.channel<transactor::read_response_t>(axi_mst_loc_l);
}

cvm::messenger::task<void> mmr_txn_router::read(const transactor::read_t& r, data_t& data) {
  auto& addr = r.addr;
  auto& length = r.length;

  axi::id_t axi_id;
  if (!cvm::registry::messenger.call<axi_sw_mst_push_ar_no_id_rpc>(axi_mst_loc_l, make_ar(addr, length), axi_id)) {
    cvm::log(cvm::ERROR, "[mmr_txn_router] failed to allocate AXI id for read addr={:#x} len={}\n", addr, length);
    co_return;
  }

  auto resp = co_await cvm::registry::messenger.wait<transactor::read_response_t>(
      read_resp_channel_,
      [&axi_id](const transactor::read_response_t& rr) { return rr.id == axi_id; });

  // resp.data is a full bus-width beat from the AXI master; a narrow read's
  // payload sits on the byte lanes addressed by addr (AXI lane placement).
  // The device read contract wants the addressed bytes LSB-first.
  if (!resp.data.empty() && length < resp.data.size()) {
    size_t lane = addr % resp.data.size();
    size_t n = std::min(length, resp.data.size() - lane);
    data.assign(resp.data.begin() + lane, resp.data.begin() + lane + n);
  } else {
    data = resp.data;
    if (data.size() > length)
      data.resize(length);
  }

  cvm::log(cvm::HIGH, "[mmr_txn_router] routing mmr read back to overlay: Addr = {:#x}\n", addr);
  co_return;
}

void mmr_txn_router::write(const transactor::write_t& w) {
  uint64_t addr = w.addr;
  size_t length = w.length;
  uint32_t value;
  auto& data = w.data;
  auto& strb = w.strb;
  deserializeInt(w.data, value);
  cvm::log(cvm::HIGH, "[mmr_txn_router] routing mmr write back to overlay: Addr = {:#x}\n", addr);
  //re route mmr write
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
}
