// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include <iostream>
#include <cinttypes>
#include <vector>

extern "C" void write_rvfi(uint8_t valid, uint32_t order, uint32_t hartid, uint32_t nretid, uint32_t insn, uint64_t pc,
                           uint8_t rd_addr, uint64_t rd_wdata);
extern "C" void write_rvfi_trap(uint32_t order, uint32_t hartid, uint64_t pc, uint64_t cause);
extern "C" void write_rvfi_store(uint32_t hartid, uint64_t paddr, uint8_t wmask, uint64_t wdata, uint32_t attr);
extern "C" void write_msi(uint32_t hartid, uint8_t valid, uint64_t addr, uint32_t data);

namespace {

// One retired instruction, with the rd value whisper computes.
struct Retire {
  uint64_t pc;
  uint32_t insn;
  uint8_t rd;
  uint64_t rd_wdata;
};

// testlists/imsic_ipi.elf (see imsic_ipi.S): tester hart h has mhartid
// IPI_MHARTID[h], and sends an MSI to the other hart.
constexpr std::array<uint64_t, 2> IPI_MHARTID = {2, 5};
constexpr uint8_t T0 = 5, T1 = 6, T2 = 7, T3 = 28, T4 = 29;
constexpr uint32_t J_SELF = 0x6f; // j .
constexpr uint64_t IPI_SPIN_PC = 0x80000068;
constexpr uint64_t IPI_HANDLER_PC = 0x80000100;
constexpr uint32_t IPI_CLAIM = 0x35c012f3;    // csrrw t0, mtopei, zero
constexpr uint64_t IPI_TOPEI = (1 << 16) | 1; // identity 1, priority 1
constexpr uint64_t IPI_DONE_PC = 0x80000104;
// whisper.json
constexpr uint64_t IMSIC_MBASE = 0x24000000;
constexpr uint64_t IMSIC_MSTRIDE = 0x1000;
constexpr uint64_t MACHINE_EXTERNAL_INTR = (1ull << 63) | 11;
constexpr uint8_t WORD_MASK = 0xf;
constexpr uint32_t IO_ATTR = 0x800; // rvfi.cpp mem_attr_to_string(): io, not cacheable
// Stimulus calls from the store to the MSI at the other hart, from the MSI to
// its trap, and after the trap, how many the hart idles for cosim.sv's 3-cycle
// cause delay.
constexpr uint32_t IPI_MSI_DELAY = 2;
constexpr uint32_t IPI_TRAP_DELAY = 4;
constexpr uint32_t IPI_TRAP_IDLE = 4;
// Handler retirements hart 0 makes before it idles, so that hart 1 reaches
// +max_instr and ends the test.
constexpr uint32_t IPI_HART0_DONE = 2;

// The instructions hart `self` retires up to its store.
std::vector<Retire> ipi_program(size_t self) {
  const uint64_t mhartid = IPI_MHARTID[self];
  const uint64_t other = IPI_MHARTID[1 - self];
  std::vector<Retire> program = {
      {0x80000000, 0x00100293, T0, 1},
      {0x80000004, 0x01f29293, T0, 0x80000000},
      {0x80000008, 0x10028293, T0, 0x80000100},
      {0x8000000c, 0x30529073, 0, 0},
      {0x80000010, 0x07000293, T0, 0x70},
      {0x80000014, 0x35029073, 0, 0},
      {0x80000018, 0x00100293, T0, 1},
      {0x8000001c, 0x35129073, 0, 0},
      {0x80000020, 0x0c000293, T0, 0xc0},
      {0x80000024, 0x35029073, 0, 0},
      {0x80000028, 0x00200293, T0, 2},
      {0x8000002c, 0x35129073, 0, 0},
      {0x80000030, 0x000012b7, T0, 0x1000},
      {0x80000034, 0x8002829b, T0, 0x800},
      {0x80000038, 0x30429073, 0, 0},
      {0x8000003c, 0x30046073, 0, 0},
      {0x80000040, 0xf1402373, T1, mhartid}, // csrr t1, mhartid
      {0x80000044, 0x00200393, T2, IPI_MHARTID[0]},
      {0x80000048, 0x00500e13, T3, IPI_MHARTID[1]},
      {0x8000004c, 0x00730463, 0, 0}, // beq t1, t2, send
  };
  if (mhartid != IPI_MHARTID[0])
    program.push_back({0x80000050, 0x00038e13, T3, IPI_MHARTID[0]}); // mv t3, t2
  program.insert(program.end(), {
                                    {0x80000054, 0x00ce1e13, T3, other * IMSIC_MSTRIDE},
                                    {0x80000058, 0x24000eb7, T4, IMSIC_MBASE},
                                    {0x8000005c, 0x01ce8eb3, T4, IMSIC_MBASE + other * IMSIC_MSTRIDE},
                                    {0x80000060, 0x00100293, T0, 1},
                                    {0x80000064, 0x005ea023, 0, 0}, // sw t0, 0(t4): setipnum = 1
                                });
  return program;
}

} // namespace

extern "C" void get_1c_stimulus(uint8_t reset, uint32_t order) {
  static uint64_t pc = 0x80000000;
  static uint32_t insn = 0x6f;
  write_rvfi(!reset, order, 0, 0, insn, pc, 0, 0);
}

extern "C" void get_2c_stimulus(uint8_t reset, uint32_t order) {
  static uint64_t pc = 0x80000000;
  static uint32_t insn = 0x6f;
  write_rvfi(!reset, order, 0, 0, insn, pc, 0, 0);
  write_rvfi(!reset, order, 1, 0, insn, pc, 0, 0);
}

// Harts 0 and 1 run imsic_ipi.elf and each store an MSI to the other's IMSIC
// file. As the DUT, the stimulus then reports the MSI at the other hart, which
// takes the interrupt and claims it in the handler.
extern "C" void get_8c_stimulus(uint8_t reset, uint32_t order) {
  struct Hart {
    std::vector<Retire> program;
    uint32_t msi_at = 0; // step at which the MSI from the other hart arrives
  };
  static std::array<Hart, 2> harts = {Hart{ipi_program(0)}, Hart{ipi_program(1)}};
  static bool reset_seen = false;
  static uint32_t step = 0;
  reset_seen = reset_seen || reset;
  const bool running = reset_seen && !reset;
  write_msi(0, 0, 0, 0);
  write_msi(1, 0, 0, 0);
  if (!running) {
    write_rvfi(0, order, 0, 0, 0, 0, 0, 0);
    write_rvfi(0, order, 1, 0, 0, 0, 0, 0);
    return;
  }

  for (uint32_t h = 0; h < harts.size(); ++h) {
    Hart& hart = harts[h];
    const uint32_t other = 1 - h;
    if (step < hart.program.size()) {
      const Retire& r = hart.program[step];
      write_rvfi(1, order, h, 0, r.insn, r.pc, r.rd, r.rd_wdata);
      if (step + 1 == hart.program.size()) { // the store: the MSI reaches the other hart later
        write_rvfi_store(h, IMSIC_MBASE + IPI_MHARTID[other] * IMSIC_MSTRIDE, WORD_MASK, 1, IO_ATTR);
        harts[other].msi_at = step + IPI_MSI_DELAY;
      }
      continue;
    }
    const uint32_t trap_at = hart.msi_at + IPI_TRAP_DELAY;
    const uint32_t interrupt_at = trap_at + IPI_TRAP_IDLE + 1;
    if (hart.msi_at == 0 || step < trap_at) {
      write_rvfi(1, order, h, 0, J_SELF, IPI_SPIN_PC, 0, 0);
      if (hart.msi_at != 0 && step == hart.msi_at)
        write_msi(h, 1, IMSIC_MBASE + IPI_MHARTID[h] * IMSIC_MSTRIDE, 1);
    } else if (step == trap_at) {
      write_rvfi_trap(order, h, IPI_SPIN_PC, MACHINE_EXTERNAL_INTR);
    } else if (step < interrupt_at) {
      write_rvfi(0, order, h, 0, 0, 0, 0, 0);
    } else if (step == interrupt_at) {
      // The record that takes the interrupt carries the interrupted PC, as
      // whisper reports the interrupt step; the handler follows.
      write_rvfi(1, order, h, 0, J_SELF, IPI_SPIN_PC, 0, 0);
    } else if (step == interrupt_at + 1) {
      write_rvfi(1, order, h, 0, IPI_CLAIM, IPI_HANDLER_PC, T0, IPI_TOPEI);
    } else {
      const bool idle = h == 0 && step > interrupt_at + 1 + IPI_HART0_DONE;
      write_rvfi(!idle, order, h, 0, J_SELF, IPI_DONE_PC, 0, 0);
    }
  }
  ++step;
}
