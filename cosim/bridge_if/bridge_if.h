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
} whisper_state_t;

// dut <-> bridge
typedef struct pc_s {
  bool valid;
  uint64_t pc_rdata;

  pc_s() {
    clear();
  }

  void clear() {
    valid = false;
  }
} pc_t;

typedef struct gpr_s {
  bool valid;
  uint64_t rd_addr;
  uint64_t rd_wdata;

  gpr_s() {
    clear();
  }

  void clear() {
    valid = false;
  }
} gpr_t;
 
typedef struct fpr_s {
  bool valid;
  uint64_t frd_addr;
  bool frd_wvalid;
  uint64_t frd_wdata;

  fpr_s() {
    clear();
  }

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

  void clear() {
    valid = false;
  }
} vr_t;

typedef struct csr_s {
  bool valid;
  uint64_t cycle;
  uint32_t csr_addr;
  uint64_t csr_wmask;
  uint64_t csr_wdata;

  csr_s() {
    clear();
  }

  constexpr csr_s(bool valid, uint64_t cycle, uint32_t addr, uint64_t mask, uint64_t data) :
    valid(valid),
    cycle(cycle),
    csr_addr(addr),
    csr_wmask(mask),
    csr_wdata(data)
  {}

  void clear() {
    valid = false;
  }
} csr_t;

typedef struct mem_s {
  bool valid;
  uint64_t cycle;
  uint64_t tag;
  uint64_t va;
  uint64_t pa;
  uint8_t size;
  uint64_t data;

  mem_s() {
    clear();
  }

  void clear() {
    valid = false;
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

  mem_cl_s() {
    clear();
  }

  void clear() {
    valid = false;
  }
} mem_cl_t;

typedef struct rv_instr_s {
  // Metadata
  bool valid = false;
  bool last_uop = false;
  bool comp = false;
  bool ucode = false;
  uint8_t hart = 0;
  uint64_t id = 0;
  uint64_t cycle = 0;
  uint64_t tag = 0;
  uint32_t opcode = 0;
  uint32_t uop = 0;
  bool trap = false;
  uint8_t priv = 0;
  bool intr = false;
  bool excp = false;
  uint64_t icause = 0;
  uint64_t ecause = 0;
  uint64_t mem_va = 0;
  uint64_t mem_pa = 0;

  // Registers
  pc_t pc;
  gpr_t gpr;
  fpr_t fpr;
  vr_t vr;
  std::vector<csr_t> csr;

  // Memory
  mem_t mem_read;
  mem_t mem_write;

  rv_instr_s() {
    clear();
  }

  void clear() {
    valid = false;
    last_uop = true;
    comp = false;
    ucode = false;
    trap = false;
    intr = false;
    excp = false;
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
  bool mip_posedge;
  uint64_t mip;
  bool seip_posedge;
  bool seip_negedge;
  bool seip;
  bool stip_negedge;
} rv_intr_t;  
