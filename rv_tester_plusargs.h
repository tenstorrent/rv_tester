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
