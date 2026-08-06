// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "cvm/messenger.hpp"
#include "src/transactors/axi_sw/axi.h"

// AXI master RPC tags, at namespace scope rather than nested in axi_sw_mst<...>.
// cvm::messenger keys procedures on typeid, so caller and registrant must name
// the same type -- and naming the nested one forces a caller to spell out the
// full template instantiation, which pulls in the project-generated
// rv_tester_transactions.hpp. A caller that only pushes a transaction (a sysmod
// device rerouting to the overlay master, say) should not need that dependency.
//
// axi_sw_mst re-exports these as member aliases, so existing
// axi_sw_mst<...>::push_ar_no_id_rpc spellings still name these same types.
//
// try_lock_rpc stays nested: it returns axi_sw_mst<...>::lock_t.

CVM_MESSENGER_procedure_call(push_ar_no_id_rpc, bool(const axi::a_no_id_t& ar, axi::id_t& id));
CVM_MESSENGER_procedure_call(push_aw_no_id_rpc, bool(const axi::a_no_id_t& aw, axi::id_t& id));
CVM_MESSENGER_procedure_call(push_w_rpc, void(const axi::w_t& w));
CVM_MESSENGER_procedure_call(free_aw_ids_rpc, unsigned());
