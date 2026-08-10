// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "src/sysmod/mmr_txn_router/mmr_txn_router.h"

mmr_txn_router::mmr_txn_router(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
    : device(tag, addr, size, loc, &mmr_txn_router::write, &mmr_txn_router::read, this), axi_mst_loc_l(axi_mst_loc) {
  cvm::log(cvm::HIGH, " [mmr_txn_router] Constructor \n");
}

void mmr_txn_router::configure() {
  device::configure();
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
  data_width_ = cvm::topology::attr(axi_mst_loc_l, "DATA_WIDTH").second;
}

cvm::messenger::task<void> mmr_txn_router::read(const transactor::read_t& r, data_t& data) {
  auto& addr = r.addr;
  auto& length = r.length;

  const size_t beat_bytes = data_width_ / 8;
  if (length == 0 || beat_bytes == 0) {
    cvm::log(cvm::ERROR, "Error: [mmr_txn_router] bad mmr read: Addr = {:#x} len={} data_width={}\n", addr, length, data_width_);
    co_return;
  }

  // Push the AR ourselves rather than signalling read_request_t: the RPC returns
  // the id the master allocated, which is what lets the wait below filter. Every
  // device rerouting through this master has its own r_t channel, so an
  // unfiltered wait takes whichever beat lands first -- including another
  // device's. Mirrors axi_sw_mst::a_wrapper's length -> (len, size) mapping.
  const bool single_beat = ((length & (length - 1)) == 0) && (length <= beat_bytes);

  axi::a_no_id_t ar{};
  ar.w = false;
  ar.addr = addr;
  ar.len = single_beat ? axi::len_t(0) : axi::len_t(length - 1);
  ar.size = single_beat ? axi::sz_t(__builtin_ctz(unsigned(length))) : axi::sz_t(0);
  ar.burst = axi::BURST_INCR;

  axi::id_t id = 0;
  if (!cvm::registry::messenger.call<push_ar_no_id_rpc>(axi_mst_loc_l, ar, id)) {
    cvm::log(cvm::ERROR, "Error: [mmr_txn_router] no free axi master id, dropping mmr read: Addr = {:#x} len={}\n", addr, length);
    co_return;
  }

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(
      channel,
      [id](const axi::r_t& rsp) { return rsp.id == id; });
  data = resp.data;
  cvm::log(cvm::HIGH, "[mmr_txn_router] routing mmr read back to overlay: Addr = {:#x} len={} id={} resp={}\n",
           addr, length, id, resp.resp);
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
