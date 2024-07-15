#include "rv_tester_plusargs.h"
#include "cvm/logger.hpp"

DEFINE_uint64(seed, 1, "Simulation seed passed down for randomization");
DEFINE_int32(max_stall_cycle, 20000, "Max stall cycle limit to terminate the sim");
DEFINE_uint32(num_harts, 1, "Number of enabled harts - upto 8");
DEFINE_uint32(hart_enable_mask, 0x1, "Hart enable mask. Ex: With 2 enabled harts in a 8-hart system, could be 0x18. Should match num_harts.");
DEFINE_string(hart_enable_id, "0", "Hart id sequence corresponding to physical cores. Ex: With 2 enabled harts in a 8-hart system, could be 4,3 i.e. hart0=core4, hart1=core3.");
DEFINE_uint32(num_sc_ways, 24, "Number of enabled SC ways - upto 24 in multiples of 4");
DEFINE_uint32(sc_way_enable_mask, 0xFFFFFF, "SC way enable mask. Ex: With 20 enabled ways out of 24, could be 0xF0_FFFF.");
DEFINE_uint32(trace_enable, 1, "Trace enable fuse");
DEFINE_uint32(debug_enable, 3, "Debug enable fuse");

DEFINE_double(ext_mem_stall_factor, 0.8, "Apply mult factor for shared cache to max_stall_cycle");

extern "C" {
    std::uint32_t ext_mem_rv_tester_get_stall_timeout() { return FLAGS_ext_mem_stall_factor * FLAGS_max_stall_cycle; }
}

extern "C" void stall_checker_rv_tester_error(const char* error, std::uint32_t threshold) {
    cvm::log(cvm::ERROR, "Error: {}: {} cycles\n", error, threshold);
}
