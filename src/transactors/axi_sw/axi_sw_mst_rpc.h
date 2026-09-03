// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "src/transactors/axi_sw/axi.h"
#include "transactor.h"
#include "cvm/messenger.hpp"

// Type-erased RPCs so sysmod devices (e.g. mmr_txn_router) can issue AR/AW on any
// axi_sw_mst without depending on rv_tester_transactions.hpp.
CVM_MESSENGER_procedure_call(axi_sw_mst_push_ar_no_id_rpc, bool(const axi::a_no_id_t& ar, axi::id_t& id));
// Pushes a rerouted write (AW + W beats) and returns the allocated AXI id so the
// caller can correlate the B-response and recover its resp code.
CVM_MESSENGER_procedure_call(axi_sw_mst_push_write_request_rpc, bool(const transactor::write_request_t& req, axi::id_t& id));
