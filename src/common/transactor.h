// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "fmt/format.h"
#include "axi_seqids.hpp"

class transactor {

public:
  struct write_t {
    uint64_t addr;
    size_t length;
    std::vector<uint8_t> data;
    std::vector<bool> strb;
    uint32_t id = 0;
  };

  struct read_t {
    uint32_t id;
    uint64_t addr;
    size_t length;
  };

  struct read_response_t {
    uint32_t id;
    std::vector<uint8_t> data;
    uint8_t resp = 0; // AXI resp_t; 0 == RESP_OKAY
  };

  struct read_request_t {
    uint64_t addr;
    size_t length;
    bool exp_err_rsp = false;
  };

  struct write_response_t {
    uint32_t id;
    uint8_t resp = 0; // AXI resp_t; 0 == RESP_OKAY
  };

  struct write_request_t {
    uint64_t addr;
    size_t length;
    std::vector<uint8_t> data;
    std::vector<bool> strb;
    bool exp_err_rsp = false;
    bool allow_decerr_resp = false;
  };

  transactor(cvm::topology::loc_t loc, const std::string& tag)
      : loc_(loc), tag_(tag),
        resp_channel_(cvm::registry::messenger.channel<read_response_t>(loc_)),
        wresp_channel_(cvm::registry::messenger.channel<write_response_t>(loc_)) {};

  virtual ~transactor() = default;

  // Synchronous write: waits for the write_response_t so the B-channel resp
  // (e.g. DECERR from a rerouted MMR store) reaches the DUT. Mirrors read().
  cvm::messenger::task<write_response_t> write(uint64_t addr, size_t length, const std::vector<uint8_t>& data, const std::vector<bool>& strb) {
    uint32_t id = ++id_;

    cvm::registry::messenger.signal<write_t>(
        loc_,
        write_t{addr, length, data, strb, id});

    auto response = co_await cvm::registry::messenger.wait<write_response_t>(wresp_channel_, [&id](const write_response_t& w) { return w.id == id; });
    cvm::log(cvm::FULL, "[transactor] loc={}: b: id={}, addr={:#x}, len={}, resp={}\n", loc_, response.id, addr, length, response.resp);
    co_return response;
  }

  cvm::messenger::task<read_response_t> read(uint64_t addr, size_t length /*, bool block = false */) {
    uint32_t id = ++id_;

    cvm::registry::messenger.signal<read_t>(
        loc_,
        read_t{id, addr, length});

    auto response = co_await cvm::registry::messenger.wait<read_response_t>(resp_channel_, [&id](const read_response_t& r) { return r.id == id; });
    std::string d;
    for (size_t i = 0; i < response.data.size(); i++)
      d += fmt::format("{:02x}", response.data[i]);
    cvm::log(cvm::FULL, "[transactor] loc={}: r: id={}, addr={:#x}, len={}, size={}, resp={}, data={}\n", loc_, response.id, addr, length, response.data.size(), response.resp, d);
    co_return response;
  }

  std::string tag() { return tag_; }

private:
  const cvm::topology::loc_t loc_;
  const std::string tag_;
  cvm::messenger::pool<read_response_t>::channel_info resp_channel_;
  cvm::messenger::pool<write_response_t>::channel_info wresp_channel_;
  uint32_t id_ = 0;
};
