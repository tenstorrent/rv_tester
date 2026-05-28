#pragma once
// declares plusargs for any packages that depends on them
#include "cvm/plusargs.hpp"

DECLARE_bool(axi_allow_slverr_resp);
DECLARE_bool(axi_allow_decerr_resp);
DECLARE_bool(axi_rand_id_alloc);
DECLARE_bool(axi_disable_seqid_alloc);
DECLARE_bool(axi_sw_mst_greedy_queue);
DECLARE_bool(axi_sw_rsp_toggle_en);
DECLARE_int32(axi_mst_brdy_high);
DECLARE_int32(axi_mst_brdy_low);
DECLARE_int32(axi_mst_rrdy_high);
DECLARE_int32(axi_mst_rrdy_low);
DECLARE_int64(axi_sw_rsp_toggle_start);
DECLARE_uint32(axi_resp_timeout);
