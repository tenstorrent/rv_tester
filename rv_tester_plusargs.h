#pragma once

#include "cvm/plusargs.hpp"

DECLARE_uint64(resetpc);
DECLARE_uint64(resetpcfw);
DECLARE_int32(max_stall_cycle);
DECLARE_bool(bypass_cache);
DECLARE_bool(bypass_mem);
DECLARE_bool(rand_snoop_en);
DECLARE_string(set_mmr);
DECLARE_bool(metrics);
DECLARE_bool(monitor);
DECLARE_string(axi_resp_slverr_addr);
DECLARE_string(axi_resp_decerr_addr);
DECLARE_int32(axi_resp_slverr_threshold);
DECLARE_int32(axi_resp_decerr_threshold);
DECLARE_string(axi_resp_slverr_pattern);
DECLARE_string(axi_resp_decerr_pattern);
DECLARE_string(test_start_label);
// FIXME Temporary solution; need to revisit
DECLARE_bool(vip);
DECLARE_bool(vip_axi_dpi);
