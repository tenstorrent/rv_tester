// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "src/transactors/axi_sw/axi.h"
#include "axi_sw_mst.h"
#include "cvm/messenger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "rv_tester_transactions.hpp"

DECLARE_string(memdump);

class memdump {
public:
  memdump(cvm::topology::loc_t loc, unsigned id);
  memdump(cvm::topology::loc_t loc) : memdump(loc, 0) {}
  ~memdump() = default;

  // Called by cvm::registry after all module constructors have run.
  // Sysmod's RPCs (block/unblock terminate) are guaranteed registered by then.
  void configure();

  using overlay_mst_t = axi_sw_mst<
      rv_tester_transactions::axi_sw_mst::b<>,
      rv_tester_transactions::axi_sw_mst::r<>,
      rv_tester_transactions::axi_sw_mst::ar_q_ptr<>,
      rv_tester_transactions::axi_sw_mst::aw_q_ptr<>,
      rv_tester_transactions::axi_sw_mst::w_q_ptr<>>;

private:
  struct region {
    std::string filename;
    uint64_t start;
    uint64_t end;
  };

  cvm::topology::loc_t loc_;
  cvm::topology::loc_t axi_mst_loc_;
  cvm::topology::loc_t sysmod_loc_;
  cvm::messenger::pool<axi::r_t>::channel_info channel_;
  std::vector<region> regions_;
  std::map<std::string, uint64_t> symbol_cache_;

  void parse_plusarg();
  bool evaluate_expr(const std::string& expr, uint64_t& out);
  bool resolve_token(const std::string& tok, uint64_t& out);
  bool lookup_symbol(const std::string& sym, uint64_t& out);

  cvm::messenger::task<void> dump_all();
  cvm::messenger::task<void> read_line(uint64_t addr, std::vector<uint8_t>& out);
};
