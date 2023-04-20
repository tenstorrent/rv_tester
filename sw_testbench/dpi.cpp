#include <iostream>
#include <cinttypes>

// temporary defines to remove later
DEFINE_string(riscv_trace_path, "", "");
DEFINE_string(trace_csv_path, "", "");
DEFINE_string(whisper_csv_path, "", "");

extern "C" void write_rvfi(uint8_t valid, uint32_t core, uint32_t insn, uint64_t pc);

extern "C" void get_stimulus(uint8_t reset, std::uint64_t clocks) {
    static uint64_t pc = 0x80000000;
    write_rvfi(!reset, 0, 0, pc);
    if (!reset) {
        // pc += 4;
    }
}
