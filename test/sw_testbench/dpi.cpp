#include <iostream>
#include <cinttypes>

extern "C" void write_rvfi(uint8_t valid, uint32_t order, uint32_t hartid, uint32_t nretid, uint32_t insn, uint64_t pc);

extern "C" void get_1c_stimulus(uint8_t reset, uint32_t order) {
  static uint64_t pc = 0x80000000;
  static uint32_t insn = 0x6f;
  write_rvfi(!reset, order, 0, 0, insn, pc);
}

extern "C" void get_2c_stimulus(uint8_t reset, uint32_t order) {
  static uint64_t pc = 0x80000000;
  static uint32_t insn = 0x6f;
  write_rvfi(!reset, order, 0, 0, insn, pc);
  write_rvfi(!reset, order, 1, 0, insn, pc);
}
