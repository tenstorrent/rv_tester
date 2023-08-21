#ifndef _RISCV_DECODE_H
#define _RISCV_DECODE_H

#include <algorithm>
#include <cstdint>
#include <string.h>
#include <strings.h>
#include <cinttypes>
#include <type_traits>

typedef int64_t sreg_t;
typedef uint64_t reg_t;

const int NXPR = 32;
const int NFPR = 32;
const int NVPR = 32;
const int NCSR = 4096;

#define insn_length(x) \
  (((x) & 0x03) < 0x03 ? 2 : \
   ((x) & 0x1f) < 0x1f ? 4 : \
   ((x) & 0x3f) < 0x3f ? 6 : \
   8)
#define MAX_INSN_LENGTH 8
#define PC_ALIGN 2

#define Sn(n) ((n) < 2 ? X_S0 + (n) : X_Sn + (n))

#define get_field(reg, mask) \
  (((reg) & (std::remove_cv<decltype(reg)>::type)(mask)) / ((mask) & ~((mask) << 1)))

#define set_field(reg, mask, val) \
  (((reg) & ~(std::remove_cv<decltype(reg)>::type)(mask)) | (((std::remove_cv<decltype(reg)>::type)(val) * ((mask) & ~((mask) << 1))) & (std::remove_cv<decltype(reg)>::type)(mask)))

#define DEBUG_START             0x0
#define DEBUG_END               (0x1000 - 1)

#endif