#pragma once

#include "cvm/plusargs.hpp"

DECLARE_uint64(resetpc);
DECLARE_uint64(resetpcfw);
DECLARE_uint64(max_stall_cycle);
DECLARE_uint64(max_stall_cycle_base);
DECLARE_uint64(max_stall_cycle_per_core_increment);
DECLARE_uint64(max_cycle_base);
DECLARE_uint64(max_cycle_per_core_increment);
DECLARE_uint64(max_instr_base);
DECLARE_uint64(max_instr_per_core_increment);
DECLARE_bool(timeout_scale_en);
DECLARE_bool(rv_tester_mem_bypass_cache);
DECLARE_bool(rv_tester_enable_llc);
DECLARE_bool(metrics);
DECLARE_bool(monitor);
DECLARE_string(axi_resp_slverr_addr);
DECLARE_string(axi_resp_decerr_addr);
DECLARE_int32(axi_resp_slverr_threshold);
DECLARE_int32(axi_resp_decerr_threshold);
DECLARE_bool(offline_dpi);
DECLARE_bool(offline_dpi_test);
DECLARE_string(axi_resp_slverr_pattern);
DECLARE_string(axi_resp_decerr_pattern);
DECLARE_string(test_start_label);
DECLARE_uint64(pa_mask);
// FIXME Temporary solution; need to revisit
DECLARE_bool(vip);
DECLARE_bool(vip_axi_dpi);
DECLARE_bool(warm_reset_directed_en);
