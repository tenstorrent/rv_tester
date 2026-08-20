// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "src/sysmod/mmr_txn_router/mmr_txn_router.h"
#include "src/transactors/axi_sw/axi_sw_mst_rpc.h"

#include <algorithm>

namespace {

bool is_hardware_axi_source(cvm::topology::loc_t source) {
  static const auto sources = [] {
    std::vector<cvm::topology::loc_t> locs;
    for (const auto& loc : cvm::topology::get_from_type("PLATFORM_TRANSACTOR"))
      locs.push_back(loc);
    return locs;
  }();
  return std::find(sources.begin(), sources.end(), source) != sources.end();
}

axi::a_no_id_t make_ar(uint64_t addr, size_t length, bool upsize_narrow) {
  axi::a_no_id_t ar(false, addr, 0);
  // ms_mmr rejects ring AR with AXI size < 32b. Upsize narrow non-hardware
  // reads on the reroute path; hardware reads stay narrow so ms_mmr can DECERR.
  size_t axi_length = length;
  if (upsize_narrow && axi_length > 0 && axi_length < 4)
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
  write_resp_channel_ = cvm::registry::messenger.channel<transactor::write_response_t>(axi_mst_loc_l);
}

cvm::messenger::task<std::uint8_t> mmr_txn_router::read(const read_t& dr, data_t& data) {
  const auto& r = dr.r;
  auto& addr = r.addr;
  auto& length = r.length;

  const bool hardware_read = is_hardware_axi_source(dr.source);
  axi::a_no_id_t ar = make_ar(addr, length, !hardware_read);
  if (hardware_read)
    ar.allow_decerr_resp = true;

  axi::id_t axi_id;
  if (!cvm::registry::messenger.call<axi_sw_mst_push_ar_no_id_rpc>(axi_mst_loc_l, ar, axi_id)) {
    cvm::log(cvm::ERROR, "[mmr_txn_router] failed to allocate AXI id for read addr={:#x} len={}\n", addr, length);
    co_return axi::RESP_DECERR;
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

  cvm::log(cvm::HIGH, "[mmr_txn_router] routing mmr read back to overlay: src={} hw={} addr={:#x} resp={}\n",
           dr.source, hardware_read, addr, resp.resp);
  co_return resp.resp;
}

cvm::messenger::task<std::uint8_t> mmr_txn_router::write(const transactor::write_t& w) {
  uint64_t addr = w.addr;
  size_t length = w.length;
  auto& data = w.data;
  auto& strb = w.strb;
  cvm::log(cvm::HIGH, "[mmr_txn_router] routing mmr write back to overlay: Addr = {:#x}\n", addr);
  // Allow + propagate the B-channel resp: a DUT MMR store may legitimately
  // DECERR (chicken bit / unmapped), which the DUT turns into the async DERR
  // interrupt. Mirrors the read reroute path.
  transactor::write_request_t req{addr, length, data, strb};
  req.allow_decerr_resp = true;

  axi::id_t axi_id;
  if (!cvm::registry::messenger.call<axi_sw_mst_push_write_request_rpc>(axi_mst_loc_l, req, axi_id)) {
    cvm::log(cvm::ERROR, "[mmr_txn_router] failed to allocate AXI id for write addr={:#x} len={}\n", addr, length);
    co_return axi::RESP_DECERR;
  }

  auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(
      write_resp_channel_,
      [&axi_id](const transactor::write_response_t& wr) { return wr.id == axi_id; });

  cvm::log(cvm::HIGH, "[mmr_txn_router] routing mmr write back to overlay: addr={:#x} resp={}\n", addr, resp.resp);
  co_return resp.resp;
}
