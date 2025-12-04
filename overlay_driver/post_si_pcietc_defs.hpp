#pragma once

#include <cstdint>
#include "cvm/logger.hpp"

#define SERIALIZE_INT(x, n, data) \
  do { \
    for (unsigned i = 0; i < n; ++i, x >>= 8) \
      data[i] = x & 0xff; \
  } while (0)

#define DESERIALIZE_INT(data, n, x) \
  do { \
    x = 0; \
    for (int i = n - 1; i >= 0; --i) \
      x = (x << 8) | data[i]; \
  } while (0)

#define is_in_range(offset, length, start, size) \
  ( (static_cast<uint32_t>(offset) >= static_cast<uint32_t>(start)) && \
    ((static_cast<uint32_t>(offset) + static_cast<uint32_t>(length)) <= (static_cast<uint32_t>(start) + static_cast<uint32_t>(size))) )

struct post_si_pcietc_helper_rpc_data_t {
  uint64_t addr;
  uint64_t offset;
  size_t length;
  std::vector<uint8_t> data;
};

// Address space definitions for PCIe test controller
namespace post_si_pcietc {
    // Address region patterns
    enum class address_region_t : uint32_t {
        GLOBAL_REGISTERS    = 0x0000,    // [0x0000, 0x1000) - Global node registers
        CE_REGISTERS        = 0x1000,    // [0x1000, 0x2000) - Completer Engine registers
        RESERVED_1          = 0x2000,    // [0x2000, 0x4000) - Reserved space
        EE_REGISTERS        = 0x4000,    // [0x4000, 0x8000) - Execution Engine registers
        RESERVED_2          = 0x8000,    // [0x8000, 0x100000) - Reserved space
        EE0_INSTR_BUFFER    = 0x100000,  // [0x100000, 0x120000) - EE0 instruction buffer
        RESERVED_3          = 0x120000,  // [0x120000, 0x140000) - Reserved space
        EE1_INSTR_BUFFER    = 0x140000,  // [0x140000, 0x160000) - EE1 instruction buffer
        RESERVED_4          = 0x160000,  // [0x160000, 0x180000) - Reserved space
        EE2_INSTR_BUFFER    = 0x180000,  // [0x180000, 0x1a0000) - EE2 instruction buffer
        RESERVED_5          = 0x1a0000,  // [0x1a0000, 0x1c0000) - Reserved space
        EE3_INSTR_BUFFER    = 0x1c0000,  // [0x1c0000, 0x1e0000) - EE3 instruction buffer
        RESERVED_6          = 0x1e0000,  // [0x1e0000, 0x400000) - Reserved space
        CE_MEMORY           = 0x400000   // [0x400000, 0xc00000) - Completer Engine memory
    };

    // Size constants
    namespace size_constants {
        constexpr uint32_t GLOBAL_REGION_SIZE       = 0x1000;    // 4KB
        constexpr uint32_t CE_REGISTER_REGION_SIZE  = 0x1000;    // 4KB
        constexpr uint32_t EE_REGISTER_REGION_SIZE  = 0x1000;    // 4KB
        constexpr uint32_t INSTR_BUFFER_SIZE        = 0x20000;   // 128KB
        constexpr uint32_t CE_MEMORY_SIZE           = 0x800000;  // 8MB
    };

    // Data patterns for testing
    enum class data_pattern_t : uint8_t {
        ZEROS               = 0x00,
        ADDR_BASED          = 0x01,
        INV_ADDR_BASED      = 0x02,
        LOOP_COUNT          = 0x04,
        CUSTOM_DW0          = 0x08,
        CUSTOM_DW1          = 0x09,
        CUSTOM_DW2          = 0x0a,
        CUSTOM_DW3          = 0x0b,
    };
    std::vector<uint8_t> generate_pattern(data_pattern_t pattern, uint64_t addr, size_t length,
                                          uint32_t loop_count, uint32_t custom_dw0, uint32_t custom_dw1, uint32_t custom_dw2, uint32_t custom_dw3);
    std::vector<uint32_t> pack_data_to_u32(uint64_t addr, const std::vector<uint8_t>& data);
} // namespace post_si_pcietc
