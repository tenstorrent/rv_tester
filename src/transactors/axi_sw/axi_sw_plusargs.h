// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once
// declares plusargs for any packages that depends on them
#include "cvm/plusargs.hpp"

DECLARE_int32(axi_sw_read_latency_max);
DECLARE_int32(axi_sw_read_latency_timeout_threshold);
DECLARE_int32(axi_sw_read_latency_fifo_threshold);
DECLARE_int32(axi_sw_read_latency_fixed);
DECLARE_bool(axi_sw_read_no_callbacks);
DECLARE_int32(axi_sw_read_consecutive_spurious_calls_allowed);
DECLARE_uint32(axi_sw_reorder_window);
DECLARE_uint32(axi_sw_reorder_timeout);
DECLARE_bool(axi_sw_fast_write_response);
DECLARE_uint32(axi_sw_lfsr_seed_aw_rdy);
DECLARE_uint32(axi_sw_lfsr_mask_aw_rdy);
DECLARE_uint32(axi_sw_lfsr_seed_ar_rdy);
DECLARE_uint32(axi_sw_lfsr_mask_ar_rdy);
DECLARE_uint32(axi_sw_lfsr_seed_w_rdy);
DECLARE_uint32(axi_sw_lfsr_mask_w_rdy);
DECLARE_string(axi_sw_add_response_latency_range);
