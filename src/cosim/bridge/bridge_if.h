#pragma once

#include <vector>
#include <bitset>
#include <string>
#include "bridge_params.h"
#include "src/cac_lib.h"

// This file defines the structs used by the bridge to communicate
// with dut interfaces such as rvfi and whisper interface
//
// dut <-- rv_instr_t --> bridge <-- whisper_state_t --> whisper

using hart_id_t = cac::hart_t;

// bridge <-> whisper
typedef struct whisper_state_s {
  //used in whisperStep
  uint64_t tag = 0;
  uint64_t time = 0;
  uint64_t pc = 0;
  uint32_t opcode = 0;
  unsigned change_count = 0;
  std::string disasm = std::string(128, ' ');
  //Used in whisperChange
  uint32_t resource = 0;
  uint64_t address = 0;
  uint64_t value = 0;
  bool valid = false;
  uint32_t priv_mode = 0;
  uint32_t fp_flags = 0;
  bool trap = false;
  bool stop = false;
  bool is_load = false;
  bool is_cancelled = false;
} whisper_state_t;

// dut <-> bridge
typedef struct pc_s {
  bool valid = false;
  uint64_t pc_rdata = 0;
  bool error = false;

  pc_s() {
    clear();
  }

  void clear() {
    valid = false;
  }
} pc_t;

typedef struct gpr_s {
  bool valid;
  uint32_t rd_addr;
  uint64_t rd_wdata;

  gpr_s() {
    clear();
  }

  constexpr gpr_s(bool valid, uint32_t rd_addr, uint64_t rd_wdata) : valid(valid),
                                                                     rd_addr(rd_addr),
                                                                     rd_wdata(rd_wdata) {}

  void clear() {
    valid = false;
  }
} gpr_t;

typedef struct fpr_s {
  bool valid;
  uint32_t frd_addr;
  uint64_t frd_wdata;

  fpr_s() {
    clear();
  }

  constexpr fpr_s(bool valid, uint32_t frd_addr, uint64_t frd_wdata) : valid(valid),
                                                                       frd_addr(frd_addr),
                                                                       frd_wdata(frd_wdata) {}

  void clear() {
    valid = false;
  }
} fpr_t;

typedef struct vr_s {
  bool valid;
  uint32_t vrd_addr;
  std::bitset<256> vrd_wdata;

  vr_s() {
    clear();
  }

  constexpr vr_s(bool valid, uint32_t vrd_addr, std::bitset<256> vrd_wdata) : valid(valid),
                                                                              vrd_addr(vrd_addr),
                                                                              vrd_wdata(vrd_wdata) {}

  void clear() {
    valid = false;
  }
} vr_t;

typedef struct csr_s {
  bool valid;
  uint32_t hart;
  uint64_t cycle;
  uint32_t csr_addr;
  uint64_t csr_wmask;
  uint64_t csr_wdata;

  csr_s() {
    clear();
  }

  constexpr csr_s(bool valid, uint32_t hart, uint64_t cycle, uint32_t addr, uint64_t mask, uint64_t data) : valid(valid),
                                                                                                            hart(hart),
                                                                                                            cycle(cycle),
                                                                                                            csr_addr(addr),
                                                                                                            csr_wmask(mask),
                                                                                                            csr_wdata(data) {}

  void clear() {
    valid = false;
  }
} csr_t;

typedef struct mem_s {
  bool valid;
  uint32_t hart;
  uint64_t cycle;
  uint64_t opcode;
  uint64_t tag;
  uint64_t va;
  uint64_t pa;
  uint8_t size;
  uint64_t data;
  std::bitset<256> data_vec;
  bool amo;
  uint8_t amo_op;
  bool v_ext;
  uint32_t attr;
  bool page4kX;
  uint32_t page4kX_attr;
  bool error;
  uint8_t field;
  uint8_t elem_idx;
  bool splat;
  uint8_t elem_size;
  bool mtime_valid;
  uint64_t mtime;
  bool trap_intr;

  mem_s() {
    clear();
  }

  void clear() {
    valid = false;
  }

  bool operator==(const mem_s& other) const {
    return pa == other.pa;
  }
} mem_t;

typedef struct mem_cl_s {
  bool valid;
  uint64_t cycle;
  uint64_t va;
  uint64_t pa;
  uint8_t size;
  std::bitset<512> data;
  uint64_t mask;
  bool error;

  mem_cl_s() {
    clear();
  }

  void clear() {
    valid = false;
  }
} mem_cl_t;

typedef struct rv_steps_s {
  // Metadata
  bool valid = false;
  uint64_t cycle = 0;
  uint32_t steps = 0;
  uint32_t skips = 0;
} rv_steps_t;

typedef struct rv_instr_s {
  // Metadata
  bool valid = false;
  bool first_uop = false;
  bool last_uop = false;
  bool comp = false;
  bool ucode = false;
  bool opcode_modified = false;
  uint8_t hart = 0;
  uint64_t id = 0;
  uint64_t cycle = 0;
  uint64_t core_cycle = 0;
  uint64_t tag = 0;
  uint64_t branch_tag = 0;
  uint32_t opcode = 0;
  std::string disasm = std::string(128, ' ');
  uint64_t uop = 0;
  bool cracked = false;
  bool trap = false;
  bool trap_valid = false;
  uint32_t trap_opcode = 0;
  uint64_t trap_addr = 0;
  uint8_t priv = 3;
  bool nmi = false;
  bool intr = false;
  bool virt_mode = false;
  bool excp = false;
  uint64_t ncause = 0;
  uint64_t icause = 0;
  uint64_t ecause = 0;
  uint64_t mem_va = 0;
  uint64_t mem_pa = 0;
  uint32_t flags = 0;

  // Registers
  pc_t pc;
  std::vector<gpr_t> gpr;
  std::vector<fpr_t> fpr;
  std::vector<vr_t> vr;
  std::vector<csr_t> csr;

  // Memory
  mem_t mem_read;
  mem_t mem_write;

  // Special values
  bool mtime_valid;
  uint64_t mtime;

  rv_instr_s() {
    clear();
  }

  void clear() {
    valid = false;
    first_uop = true;
    last_uop = true;
    comp = false;
    ucode = false;
    opcode_modified = false;
    trap = false;
    trap_valid = false;
    nmi = false;
    intr = false;
    excp = false;
    mtime_valid = false;
    pc.clear();
    gpr.clear();
    fpr.clear();
    vr.clear();
    mem_read.clear();
    mem_write.clear();
    csr.clear();
  }
} rv_instr_t;

typedef struct rv_instr_group_s {
  uint64_t cycle = 0;
  std::vector<rv_instr_t> instrs;
  std::vector<csr_t> csrs;

  rv_instr_group_s() {
    clear();
  }

  void clear() {
    instrs.clear();
    csrs.clear();
  }
} rv_instr_group_t;

typedef struct rv_debug_s {
  bool enter;
  bool exit;
  uint64_t cycle;
  uint64_t hart;
} rv_debug_t;

typedef struct rv_intr_s {
  uint64_t cycle;
  uint64_t core_cycle;
  bool hw;
  std::bitset<64> mip;
  std::bitset<64> mip_set;
  std::bitset<64> mip_clr;
  bool seip;
  bool seip_set;
  bool seip_clr;
  uint64_t mtime;
  uint32_t size;
  uint8_t buserr_bit;
  uint64_t timeCsr;
  bool trap_intr;
} rv_intr_t;

typedef struct rv_nmi_s {
  uint64_t cycle;
  bool valid;
  uint64_t cause;
} rv_nmi_t;
