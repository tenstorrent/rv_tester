// plusargs for rvfi so packages that depend on them can include them
#include "cvm/plusargs.hpp"

DECLARE_bool(mcm);
DECLARE_bool(cov);
DECLARE_uint64(debug_entry_pc); 
DECLARE_uint64(debug_exit_pc);
DECLARE_uint64(debug_mem_base); 
DECLARE_uint64(debug_mem_size);
DECLARE_bool(cosim);