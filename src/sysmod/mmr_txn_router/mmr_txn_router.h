// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "src/sysmod/device.h"
#include "cvm/topology.hpp"
#include <string>
#include <queue>
#include "cvm/registry.hpp"
#include "transactor.h"
#include "src/transactors/axi_sw/axi.h"
#include "cvm/random.hpp"

class mmr_txn_router : public device {
public:
  cvm::messenger::task<std::uint8_t> write(const transactor::write_t& w);
  cvm::messenger::task<std::uint8_t> read(const read_t& r, data_t& data);

  mmr_txn_router(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);

  void configure() override;

private:
  // Attribute randomisation for rerouted requests (see +rg_attr_* plusargs).
  bool attr_random_ = false;
  uint32_t attr_fields_ = 0;
  uint32_t write_count_ = 0;
  // >8B size injection for rerouted MMR reads (see +fsim_rg_size_gt8_every); SCB NDERRs these.
  uint32_t size_gt8_every_ = 0;
  uint32_t read_count_ = 0;
  // >8B size injection for rerouted MMR writes (see +fsim_rg_aw_size_gt8_every); SCB NDERRs these.
  uint32_t aw_size_gt8_every_ = 0;
  uint32_t aw_size_count_ = 0;
  cvm::rand::uniform_dist<uint32_t> attr_rng_;
  transactor::axi_attr_t pick_attr(bool is_write);

  cvm::topology::loc_t axi_mst_loc_l;
  // Data-bus width (bytes) of the reroute master; an AxSIZE may not exceed it,
  // so wider transfers are expressed as full-width bursts in make_ar.
  size_t axi_mst_data_bytes_ = 0;
  // Maximum AXI beats per transaction the reroute master accepts (MAX_AXLEN+1
  // from topology); 0 means unbounded. AXI-Lite-style ports such as the CPL
  // utility bus cap AxLEN at 0, so a transfer that would otherwise burst is
  // reissued as a sequence of single bus-width-beat transactions and reassembled.
  size_t axi_mst_max_beats_ = 0;
  cvm::messenger::pool<transactor::read_response_t>::channel_info read_resp_channel_;
  cvm::messenger::pool<transactor::write_response_t>::channel_info write_resp_channel_;

  // Burst-limited reroute masters: fan a wider transfer out into single
  // bus-width-beat (AxLEN=0) transactions and reassemble the read payload.
  cvm::messenger::task<std::uint8_t> read_split(uint64_t addr, size_t length, const axi::a_no_id_t& tmpl, data_t& data);
  cvm::messenger::task<std::uint8_t> write_split(const transactor::write_t& w, const transactor::axi_attr_t& attr);
  // Copy n bytes from the given integer, x, to the data iterator
  // following little endian convention. If n is larger than the size
  // of x, then copy zero bytes after copying the bytes of x.
  template <typename INT>
  void serializeInt(INT x, size_t n, data_t& data) {
    for (unsigned i = 0; i < n; ++i, x >>= 8)
      data[i] = x & 0xff;
  }

  // Copy bytes from data iterator into the given integer following
  // lilttle endian convention.
  template <typename INT>
  void deserializeInt(const data_t& data, INT& x) {
    x = 0;
    for (unsigned i = 0; i < sizeof(x); ++i)
      x |= INT(data[i]) << i * 8;
  }
};
