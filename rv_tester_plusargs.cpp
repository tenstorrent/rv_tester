#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"

// FIXME Temporary solution; need to revisit
DEFINE_bool(vip, false, "Set for vip builds");
DEFINE_bool(vip_axi_dpi, false, "C++ vip <-> DPI <-> SV ports <-> DPI <-> C++ axi or C++ vip <-> C++ axi");

DEFINE_uint64(max_stall_cycle, 20000, "Max stall cycle limit to terminate the sim");
DEFINE_double(ext_mem_stall_factor, 0.8, "Apply mult factor for shared cache to max_stall_cycle");

extern "C" {
    std::uint32_t ext_mem_rv_tester_get_stall_timeout() { return FLAGS_ext_mem_stall_factor * FLAGS_max_stall_cycle; }
}

extern "C" void stall_checker_rv_tester_error(const char* error, std::uint32_t threshold) {
    cvm::log(cvm::ERROR, "Error: {}: {} cycles\n", error, threshold);
}
