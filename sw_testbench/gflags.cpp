#include "cvm/plusargs.hpp"

DEFINE_string(cm, "", "command line flag : cm");
DEFINE_string(cm_dir, "", "command line flag : cm_dir");
DEFINE_string(cm_log, "", "command line flag : cm_log");
DEFINE_string(cm_name, "", "command line flag : cm_name");
DEFINE_string(cg_coverage_control, "", "command line flag : cg_coverage_control");
DEFINE_string(covg_disable_cg, "", "command line flag : covg_disable_cg");
DEFINE_int64(vcd_cycle_on, 0, "vcd_cycle_on");
DEFINE_int64(vcd_cycle_off, 0, "vcd_cycle_off");
DEFINE_bool(vip, false, "Set for vip builds");
DEFINE_bool(vip_axi_dpi, false, "C++ vip <-> DPI <-> SV ports <-> DPI <-> C++ axi or C++ vip <-> C++ axi");
