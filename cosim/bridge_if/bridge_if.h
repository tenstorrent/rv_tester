#pragma once

#include <vector>
#include <bitset>
#include "bridge_params.h"

// This file defines the structs used by the bridge to communicate
// with dut interfaces such as rvfi and whisper interface
//
// dut <-- rv_instr_t --> bridge <-- whisper_state_t --> whisper

typedef int hart_id_t;

// bridge <-> whisper
typedef struct whisper_state_s {
  //used in whisperStep
  uint64_t tag;
  uint64_t time;
  uint64_t pc;
  uint32_t opcode;
  unsigned change_count;
  char buffer[128];
  unsigned buffer_size = 128;
  //Used in whisperChange
  uint32_t resource;
  uint64_t address;
  uint64_t value;
  bool valid;
  uint32_t priv_mode;
  uint32_t fp_flags;
  bool trap;
  bool stop;
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
  uint64_t vrd_wdata[vlen/64];
 
  vr_s() {
    clear();
  }

  void clear() {
    valid = false;
  }
} vr_t;

typedef struct csr_s {
  bool valid;
  uint32_t csr_addr;
  uint64_t csr_wmask;
  uint64_t csr_wdata;

  csr_s() {
    clear();
  }

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
  uint64_t addr;
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
  bool valid;
  uint8_t hart;
  uint64_t id;
  uint64_t cycle;
  uint64_t tag;
  uint32_t opcode;
  bool trap;
  uint8_t priv;
  bool intr;
  bool excp;
  uint64_t icause;
  uint64_t ecause;
  uint64_t mem_va;
  uint64_t mem_pa;

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
    pc.clear();
    gpr.clear();
    fpr.clear();
    vr.clear();
    mem_read.clear();
    mem_write.clear();
    csr.clear();
  }
} rv_instr_t;
