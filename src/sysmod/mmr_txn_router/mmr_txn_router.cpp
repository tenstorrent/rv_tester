// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "src/sysmod/mmr_txn_router/mmr_txn_router.h"
#include "src/transactors/axi_sw/axi_sw_mst_rpc.h"
#include "cvm/plusargs.hpp"

#include <algorithm>

// Rerouted hart MMR requests normally carry the master defaults on every AXI
// attribute (cache/prot/qos/region/user = 0, allocator id). In the production
// cluster these fields are driven by the bridge, switch, CPL and chiplet-fabric
// masters; randomising them exercises the ring-to-MS interface-parity lanes
// that the CCX bench otherwise never toggles. Intended for fault-simulation
// campaigns only: everything below is inert unless +fsim_rg_attr_randomize is
// set, and the rg_attr_* knobs are ignored without it.
DEFINE_bool(fsim_rg_attr_randomize, false, "fault-sim only: randomise AXI attributes on rerouted MMR requests");
DEFINE_uint32(rg_attr_fields, 0x1F, "bitmask of fields to randomise: 1=cache 2=prot 4=qos 8=region 16=user");
DEFINE_bool(rg_attr_user_codepoints, true, "user drawn from the production code points {0x0,0x1,0x3}; false = full 8 bits");
DEFINE_uint32(rg_attr_srcid01_every, 0, "every Nth rerouted write carries a manual id with ring SrcId 01 (0 = never)");

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

transactor::axi_attr_t mmr_txn_router::pick_attr(bool is_write) {
  transactor::axi_attr_t attr;
  if (!attr_random_)
    return attr;
  const uint32_t r = attr_rng_();
  if (attr_fields_ & 0x1)
    attr.cache = uint8_t(r & 0xF);
  if (attr_fields_ & 0x2)
    attr.prot = uint8_t((r >> 4) & 0x7);
  if (attr_fields_ & 0x4)
    attr.qos = uint8_t((r >> 8) & 0xF);
  if (attr_fields_ & 0x8)
    attr.region = uint8_t((r >> 12) & 0xF);
  if (attr_fields_ & 0x10) {
    static constexpr uint8_t codepoints[] = {0x0, 0x1, 0x3};
    attr.user = FLAGS_rg_attr_user_codepoints ? codepoints[(r >> 16) % 3] : uint8_t((r >> 16) & 0xFF);
  }
  // Ring SrcId 01 is the ACLINT/local source: its B is suppressed at the core
  // node and completed by the ring's synthetic-B shim, so the id returns to
  // the master normally. Bit 14 keeps the id clear of the allocator range.
  if (is_write && FLAGS_rg_attr_srcid01_every && (++write_count_ % FLAGS_rg_attr_srcid01_every) == 0) {
    attr.is_manual_id = true;
    attr.manual_id = 0x4000u | (write_count_ & 0x7u);
  }
  return attr;
}

void mmr_txn_router::configure() {
  device::configure();
  attr_random_ = FLAGS_fsim_rg_attr_randomize;
  attr_fields_ = FLAGS_rg_attr_fields;
  if (attr_random_)
    cvm::log(cvm::NONE, "[mmr_txn_router] AXI attribute randomisation on, fields={:#x} user_codepoints={} srcid01_every={}\n",
             attr_fields_, FLAGS_rg_attr_user_codepoints, FLAGS_rg_attr_srcid01_every);
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
  {
    const transactor::axi_attr_t attr = pick_attr(false);
    ar.cache = axi::cache_mem_attr_t(attr.cache);
    ar.prot = attr.prot;
    ar.qos = attr.qos;
    ar.region = attr.region;
    ar.user = attr.user;
  }

  axi::id_t axi_id;
  if (!cvm::registry::messenger.call<axi_sw_mst_push_ar_no_id_rpc>(axi_mst_loc_l, ar, axi_id)) {
    cvm::log(cvm::ERROR, "[mmr_txn_router] failed to allocate AXI id for read addr={:#x} len={}\n", addr, length);
    co_return axi::RESP_DECERR;
  }

  // Drain orphans from recycled AXI ids. Any orphan in the queue is from
  // a prior transaction; we drop it so the wait only matches this transaction's
  // own completion. This is a simpler/safer alternative to token tagging when
  // response structures cannot be extended.
  cvm::registry::messenger.clear_channel<transactor::read_response_t>(read_resp_channel_);

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
  req.attr = pick_attr(true);

  axi::id_t axi_id;
  if (!cvm::registry::messenger.call<axi_sw_mst_push_write_request_rpc>(axi_mst_loc_l, req, axi_id)) {
    cvm::log(cvm::ERROR, "[mmr_txn_router] failed to allocate AXI id for write addr={:#x} len={}\n", addr, length);
    co_return axi::RESP_DECERR;
  }

  // Drain orphans from recycled AXI ids. Any orphan in the queue is from
  // a prior transaction; we drop it so the wait only matches this transaction's
  // own completion. This is a simpler/safer alternative to token tagging when
  // response structures cannot be extended.
  cvm::registry::messenger.clear_channel<transactor::write_response_t>(write_resp_channel_);

  auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(
      write_resp_channel_,
      [&axi_id](const transactor::write_response_t& wr) { return wr.id == axi_id; });

  cvm::log(cvm::HIGH, "[mmr_txn_router] routing mmr write back to overlay: addr={:#x} resp={}\n", addr, resp.resp);
  co_return resp.resp;
}
