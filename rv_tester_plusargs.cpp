#include "rv_tester_plusargs.h"
#include "cvm/logger.hpp"

DEFINE_int32(max_stall_cycle, 20000, "Max stall cycle limit to terminate the sim");
DEFINE_uint32(num_harts, 1, "Max number of harts to enable");
DEFINE_uint64(hart_enable_mask, 0x1, "Hart enable mask. Ex: To enable 2 harts in a 8-hart system, use 0x3. Should match num_harts.");

DEFINE_double(ext_mem_stall_factor, 0.8, "Apply mult factor for shared cache to max_stall_cycle");

extern "C" {
    std::uint32_t ext_mem_rv_tester_get_stall_timeout() { return FLAGS_ext_mem_stall_factor * FLAGS_max_stall_cycle; }
}

extern "C" void stall_checker_rv_tester_error(const char* error, std::uint32_t threshold) {
    cvm::log(cvm::ERROR, "Error: {}: {} cycles\n", error, threshold);
}
