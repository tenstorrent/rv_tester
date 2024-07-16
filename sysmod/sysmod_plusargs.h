#pragma once
// declares plusargs for any packages that depends on them
#include "cvm/plusargs.hpp"

DECLARE_string(load);
DECLARE_string(hex);
DECLARE_string(load_lz4);
DECLARE_bool(bootrom);
DECLARE_string(bootrom_path);
DECLARE_string(cplfw_path);
DECLARE_uint64(seed);
DECLARE_uint32(num_harts);
DECLARE_uint32(hart_enable_mask);
DECLARE_string(hart_enable_id);
DECLARE_uint32(num_sc_ways);
DECLARE_uint32(sc_way_enable_mask);
DECLARE_uint32(trace_enable);
DECLARE_uint32(debug_enable);
