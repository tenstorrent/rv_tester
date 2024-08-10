#pragma once
// declares plusargs for any packages that depends on them
#include "cvm/plusargs.hpp"

DECLARE_string(load);
DECLARE_string(hex);
DECLARE_string(load_lz4);
DECLARE_bool(bootrom);
DECLARE_string(bootrom_path);
DECLARE_string(cplfw_path);
DECLARE_bool(enable_sp_init);
DECLARE_uint64(seed);
DECLARE_uint32(num_harts);
DECLARE_uint32(hart_enable_mask);
DECLARE_bool(hart_sync_en);
DECLARE_string(hart_enable_id);
DECLARE_int32(num_sc_dis_ways);
DECLARE_int32(sc_dis_ways_mask);
DECLARE_uint32(trace_enable);
DECLARE_uint32(debug_enable);
