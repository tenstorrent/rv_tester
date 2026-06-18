#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"

// FIXME Temporary solution; need to revisit
DEFINE_bool(vip, false, "Set for vip builds");
DEFINE_bool(vip_axi_dpi, false, "C++ vip <-> DPI <-> SV ports <-> DPI <-> C++ axi or C++ vip <-> C++ axi");

DEFINE_uint64(max_stall_cycle, 40000, "Max stall cycle limit to terminate the sim");
DEFINE_uint64(max_stall_cycle_base, 40000, "Base value for max_stall_cycle calculation when scaling with number of cores");
DEFINE_uint64(max_stall_cycle_per_core_increment, 2000, "Increment value per additional core for max_stall_cycle calculation");
DEFINE_uint64(max_cycle_base, 10000000, "Base value for max_cycle calculation when scaling with number of cores");
DEFINE_uint64(max_cycle_per_core_increment, 75000, "Increment value per additional core for max_cycle calculation");
DEFINE_uint64(max_instr_base, 100000, "Base value for max_instr calculation when scaling with number of cores");
DEFINE_uint64(max_instr_per_core_increment, 20000, "Increment value per additional core for max_instr calculation");
DEFINE_double(ext_mem_stall_factor, 0.8, "Apply mult factor for shared cache to max_stall_cycle");
DEFINE_bool(timeout_scale_en, true, "Enable timeout scaling via DPI calls (default on for simulation, off for emulation)");

extern "C" {
std::uint32_t ext_mem_rv_tester_get_stall_timeout() { return FLAGS_ext_mem_stall_factor * FLAGS_max_stall_cycle; }
}

extern "C" void stall_checker_rv_tester_error(const char* error, std::uint32_t threshold) {
  cvm::log(cvm::ERROR, "Error: {}: {} cycles\n", error, threshold);
}
