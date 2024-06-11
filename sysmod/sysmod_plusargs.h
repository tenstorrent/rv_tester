// declares plusargs for any packages that depends on them
#include "cvm/plusargs.hpp"

DECLARE_string(load);
DECLARE_string(hex);
DECLARE_string(load_lz4);
DECLARE_bool(bootrom);
DECLARE_string(bootrom_path);
DECLARE_string(cplfw_path);
DECLARE_uint64(hart_enable_mask);
DECLARE_int32(seed);