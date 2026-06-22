// plusargs for rvfi so packages that depend on them can include them
#include "cvm/plusargs.hpp"

DECLARE_bool(rvfi);
DECLARE_bool(cov);
DECLARE_uint64(debug_entry_pc_offset);
DECLARE_uint64(debug_exit_pc_offset);
DECLARE_uint64(debug_mem_base_offset);
DECLARE_uint64(debug_mem_size);
DECLARE_bool(cosim);
DECLARE_bool(cache_model_en);
DECLARE_bool(emulate_amo_arithmetic);
DECLARE_bool(mcm);
