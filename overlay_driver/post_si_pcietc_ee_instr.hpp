#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <memory>
#include <format>

// Forward declaration
class post_si_pcietc_ee;

// Instruction type enumerations based on bit representation
enum class ee_instr_type_t : uint32_t {
    MEMORY_READ  = 0x1,  // Memory Read
    MEMORY_WRITE = 0x2,  // Memory Write
    MSI          = 0x10,  // MSI
    FENCE        = 0x80,  // Fence
    LOOP_CONTROL = 0x81,  // Loop Control
    END          = 0xFF  // End
};

enum class ee_delay_gran_t : uint32_t {
    NANOSECOND   = 0x0,  // ns
    MICROSECOND  = 0x1,  // us
    MILLISECOND  = 0x2,  // ms
    UNDEFINED    = 0x3   // undefined
};

enum class ee_instr_error_t : uint32_t {
    NO_ERROR     = 0x0,  // No error
    INSTRUCTION_ERROR = 0x1,  // Instruction error
    DELAY_ERROR = 0x2,  // Delay error
    ADDRESS_UNALIGNED_ERROR = 0x3,  // Address unaligned error
    INVALID_SIZE_ERROR = 0x4,  // Invalid size error
    DATA_SIZE_MISMATCH_ERROR = 0x5,  // Data size mismatch error
};

enum class ee_fence_type_t : uint32_t {
    NOP = 0x0,  // NOP
    ENGINE_IDLE  = 0x1,  // ENGINE_IDLE
};

// Base instruction class with common DWORD 0 fields
class ee_instruction_base_t {
public:
    // Constructor takes all 4 DWORDs
    ee_instruction_base_t(uint32_t dword0, uint32_t dword1, uint32_t dword2, uint32_t dword3) {
        base_data_.dword0.raw_value = dword0;
        base_data_.dword1 = dword1;
        base_data_.dword2 = dword2;
        base_data_.dword3 = dword3;
    }

    virtual ~ee_instruction_base_t() = default;

    // Base class data structure - stores all 4 DWORDs with DWORD0 bitfields
    struct base_instruction_data_t {
        union {
            struct {
                uint32_t instruction : 8;   // [7:0]   - Instruction type
                uint32_t reserved_21_8 : 14; // [21:8]  - Reserved
                uint32_t delay_gran : 2;    // [23:22] - Delay granularity (0x0=ns, 0x1=us, 0x2=ms, 0x3=undefined)
                uint32_t delay_val : 8;     // [31:24] - Delay value (0x00-0xFF representing 0 delay to 10)
            } fields;
            uint32_t raw_value;
        } dword0;

        uint32_t dword1;  // Raw storage for derived classes to interpret
        uint32_t dword2;  // Raw storage for derived classes to interpret
        uint32_t dword3;  // Raw storage for derived classes to interpret
    };

    // Access to raw DWORDs
    uint32_t get_dword(int index) const {
        switch(index) {
            case 0: return base_data_.dword0.raw_value;
            case 1: return base_data_.dword1;
            case 2: return base_data_.dword2;
            case 3: return base_data_.dword3;
            default: return 0;
        }
    }

    const std::array<uint32_t, 4> get_dwords() const {
        return {base_data_.dword0.raw_value, base_data_.dword1, base_data_.dword2, base_data_.dword3};
    }

    // Common DWORD 0 field accessors (read-only)
    ee_instr_type_t get_instruction_type() const {
        return static_cast<ee_instr_type_t>(base_data_.dword0.fields.instruction);
    }

    ee_delay_gran_t get_delay_granularity() const {
        return static_cast<ee_delay_gran_t>(base_data_.dword0.fields.delay_gran);
    }

    uint8_t get_delay_value() const {
        return static_cast<uint8_t>(base_data_.dword0.fields.delay_val);
    }

    // Virtual methods for derived classes
    virtual std::string to_string() const {
        return fmt::format("Base Instruction: Type={} DelayGran={} DelayVal={}", static_cast<uint32_t>(get_instruction_type()), static_cast<uint32_t>(get_delay_granularity()), static_cast<uint32_t>(get_delay_value()));
    }

protected:
    base_instruction_data_t base_data_;
};

// Memory Read Instruction Class
class ee_memory_rw_instruction_t : public ee_instruction_base_t {
public:
    ee_memory_rw_instruction_t(uint32_t dword0, uint32_t dword1, uint32_t dword2, uint32_t dword3)
        : ee_instruction_base_t(dword0, dword1, dword2, dword3) {
          memory_rw_data_.dword0.raw_value = dword0;
          memory_rw_data_.dword1.raw_value = dword1;
          memory_rw_data_.dword2.raw_value = dword2;
          memory_rw_data_.dword3.raw_value = dword3;
        }

    // Memory Read specific data structure with bitfields
    struct memory_rw_data_t {
        union {
            struct {
                uint32_t instruction : 8;   // [7:0]   - Instruction type
                uint32_t reserved_21_8 : 14; // [21:8]  - Reserved
                uint32_t delay_gran : 2;    // [23:22] - Delay granularity
                uint32_t delay_val : 8;     // [31:24] - Delay value
            } fields;
            uint32_t raw_value;
        } dword0;

        union {
            struct {
                uint32_t size : 12;         // [11:0]  - Size (0x00001=1Byte, 0x00000=4KB pecial case)
                uint32_t reserved_19_12 : 8; // [19:12] - Reserved
                uint32_t data_pattern : 4;  // [23:20] - Data Pattern (0xC-0xF unused)
                uint32_t no_verify : 1;     // [24]    - No Verify (1 = Do not Verify, 0 = Verify)
                uint32_t reserved_31_25 : 7; // [31:25] - Reserved
            } fields;
            uint32_t raw_value;
        } dword1;

        union {
            uint32_t raw_value;
            struct {
                uint32_t address_low;   // DWORD 2 - Lower 32 bits of address
            } fields;
        } dword2;
        union {
            uint32_t raw_value;
            struct {
                uint32_t address_high;  // DWORD 3 - Upper 32 bits of address
            } fields;
        } dword3;
    };

    bool is_size_valid() const {
        uint32_t size = get_size();
        return (size == 1) || (size == 2) || (size == 4) || (size == 8) ||
                        (size == 16) || (size == 32) || (size == 64) || (size % 64 == 0);
    }

    bool is_address_valid() const {
        uint64_t addr = get_address();
        uint32_t size = get_size();

        bool alignment_size = size <= 64 ? size : 64;
        // Check if address is aligned to size
        bool aligned = (addr % alignment_size == 0);

        // Ensure the request does not cross a 4KB page boundary
        // A request crosses a page boundary if (addr & ~0xFFF) != ((addr + size - 1) & ~0xFFF)
        // bool page_crossing = ((addr & ~0xFFFULL) != ((addr + size - 1) & ~0xFFFULL));
        return aligned;
        // return aligned && !page_crossing;
    }

    // Memory Read specific accessors (read-only)
    uint32_t get_size() const {
        // The size field is 20 bits wide. If the value is 0, it is a special case meaning 1MB.
        // Otherwise, it is the actual size in bytes.
        uint32_t size = memory_rw_data_.dword1.fields.size;
        return (size == 0) ? 0x100000 : size;
    }

    post_si_pcietc::data_pattern_t get_data_pattern() const {
        return static_cast<post_si_pcietc::data_pattern_t>(memory_rw_data_.dword1.fields.data_pattern);
    }

    bool get_no_verify() const {
        return memory_rw_data_.dword1.fields.no_verify == 1;
    }

    uint64_t get_address() const {
        return (static_cast<uint64_t>(memory_rw_data_.dword3.fields.address_high) << 32) | memory_rw_data_.dword2.fields.address_low;
    }

    std::string to_string() const override {
        if (get_instruction_type() == ee_instr_type_t::MEMORY_READ) {
            return fmt::format("Memory Read: Addr={:#x} Size={:#x}, NoVerify={}, Delay={} DelayGran={}", get_address(), get_size(), get_no_verify(), get_delay_value(), static_cast<uint32_t>(get_delay_granularity()));
        } else {
            return fmt::format("Memory Write: Addr={:#x} Size={:#x}, Delay={} DelayGran={}", get_address(), get_size(), get_delay_value(), static_cast<uint32_t>(get_delay_granularity()));
        }
    }

private:
    memory_rw_data_t memory_rw_data_;
};

// MSI Instruction Class
class ee_msi_instruction_t : public ee_instruction_base_t {
public:
    ee_msi_instruction_t(uint32_t dword0, uint32_t dword1, uint32_t dword2, uint32_t dword3)
        : ee_instruction_base_t(dword0, dword1, dword2, dword3) {
          msi_data_.dword0.raw_value = dword0;
          msi_data_.dword1.raw_value = dword1;
          msi_data_.dword2.raw_value = dword2;
          msi_data_.dword3.raw_value = dword3;
        }

    // MSI specific data structure with bitfields
    struct msi_data_t {
        union {
            struct {
                uint32_t instruction : 8;   // [7:0]   - Instruction type
                uint32_t reserved_21_8 : 14; // [21:8]  - Reserved
                uint32_t delay_gran : 2;    // [23:22] - Delay granularity
                uint32_t delay_val : 8;     // [31:24] - Delay value
            } fields;
            uint32_t raw_value;
        } dword0;

        union {
            uint32_t raw_value;
            struct {
                uint32_t data; // DWORD 1 - MSI Data value
            } fields;
        } dword1;
        union {
            uint32_t raw_value;
            struct {
                uint32_t address_low; // DWORD 2 - Lower 32 bits of MSI address
            } fields;
        } dword2;
        union {
            uint32_t raw_value;
            struct {
                uint32_t address_high; // DWORD 3 - Upper 32 bits of MSI address
            } fields;
        } dword3;
    };

    bool is_address_valid() const {
        uint64_t addr = get_address();
        return addr % 4 == 0;
    }

    // MSI specific accessors (read-only)
    uint32_t get_data() const {
        return msi_data_.dword1.fields.data;
    }

    uint64_t get_address() const {
        return (static_cast<uint64_t>(msi_data_.dword3.fields.address_high) << 32) | msi_data_.dword2.fields.address_low;
    }

    std::string to_string() const override {
        return fmt::format("MSI: Addr={:#x} Data={:#x} Delay={} DelayGran={}", get_address(), get_data(), get_delay_value(), static_cast<uint32_t>(get_delay_granularity()));
    }

private:
    msi_data_t msi_data_;
};

// Fence Instruction Class
class ee_fence_instruction_t : public ee_instruction_base_t {
public:
    ee_fence_instruction_t(uint32_t dword0, uint32_t dword1, uint32_t dword2, uint32_t dword3)
        : ee_instruction_base_t(dword0, dword1, dword2, dword3) {
          fence_data_.dword0.raw_value = dword0;
          fence_data_.dword1.raw_value = dword1;
          fence_data_.dword2.raw_value = dword2;
          fence_data_.dword3.raw_value = dword3;
        }

    // Fence specific data structure with bitfields
    struct fence_data_t {
        union {
            struct {
                uint32_t instruction : 8;   // [7:0]   - Instruction type
                uint32_t reserved_21_8 : 14; // [21:8]  - Reserved
                uint32_t delay_gran : 2;    // [23:22] - Delay granularity
                uint32_t delay_val : 8;     // [31:24] - Delay value
            } fields;
            uint32_t raw_value;
        } dword0;

        union {
            uint32_t raw_value;
            struct {
                uint32_t fence_type : 4;  // [3:0]  - Fence type (0x0-0xF)
                uint32_t reserved_31_4 : 28; // [31:4] - Reserved
            } fields;
        } dword1;
        union {
            uint32_t raw_value;
            struct {
                uint32_t reserved_dw2;   // DWORD 2 - Reserved
            } fields;
        } dword2;
        union {
            uint32_t raw_value;
            struct {
                uint32_t reserved_dw3;   // DWORD 3 - Reserved
            } fields;
        } dword3;
    };

    // Fence specific accessors (read-only)
    uint32_t get_fence_type() const {
        return fence_data_.dword1.fields.fence_type;
    }

    std::string to_string() const override {
        return fmt::format("FENCE: Type={:#x} Delay={} DelayGran={}", get_fence_type(), get_delay_value(), static_cast<uint32_t>(get_delay_granularity()));
    }

private:
    fence_data_t fence_data_;
};

// Loop Control Instruction Class
class ee_loop_control_instruction_t : public ee_instruction_base_t {
public:
    ee_loop_control_instruction_t(uint32_t dword0, uint32_t dword1, uint32_t dword2, uint32_t dword3)
        : ee_instruction_base_t(dword0, dword1, dword2, dword3) {
          loop_data_.dword0.raw_value = dword0;
          loop_data_.dword1.raw_value = dword1;
          loop_data_.dword2.raw_value = dword2;
          loop_data_.dword3.raw_value = dword3;
        }

    // Loop Control specific data structure with bitfields
    struct loop_data_t {
        union {
            struct {
                uint32_t instruction : 8;   // [7:0]   - Instruction type
                uint32_t loop_end : 1;  // [8]     - Loop end bit (1 = End)
                uint32_t reserved_31_9 : 23; // [31:9] - Reserved
            } fields;
            uint32_t raw_value;
        } dword0;

        union {
            uint32_t raw_value;
            struct {
                uint32_t loop_count; // DWORD 1 - Loop count value
            } fields;
        } dword1;
        union {
            uint32_t raw_value;
            struct {
                uint32_t reserved_dw2;   // DWORD 2 - Reserved
            } fields;
        } dword2;
        union {
            uint32_t raw_value;
            struct {
                uint32_t reserved_dw3;   // DWORD 3 - Reserved
            } fields;
        } dword3;
    };

    // Loop Control specific accessors (read-only)
    bool get_loop_end() const {
        return loop_data_.dword0.fields.loop_end;
    }

    uint32_t get_loop_count() const {
        return loop_data_.dword1.fields.loop_count;
    }

    std::string to_string() const override {
        return fmt::format("LOOP_CONTROL: Start={} End={} Count={} Delay={} DelayGran={}",
                          !get_loop_end(), get_loop_end(), get_loop_count(), get_delay_value(),
                          static_cast<uint32_t>(get_delay_granularity()));
    }

private:
    loop_data_t loop_data_;
};

// END Instruction Class
class ee_end_instruction_t : public ee_instruction_base_t {
public:
    ee_end_instruction_t(uint32_t dword0, uint32_t dword1, uint32_t dword2, uint32_t dword3)
        : ee_instruction_base_t(dword0, dword1, dword2, dword3) {
          end_data_.dword0.raw_value = dword0;
          end_data_.dword1.raw_value = dword1;
          end_data_.dword2.raw_value = dword2;
          end_data_.dword3.raw_value = dword3;
        }

    // END specific data structure
    struct end_data_t {
        union {
            struct {
                uint32_t instruction : 8;   // [7:0]   - Instruction type
                uint32_t reserved_21_8 : 14; // [21:8]  - Reserved
                uint32_t delay_gran : 2;    // [23:22] - Delay granularity
                uint32_t delay_val : 8;     // [31:24] - Delay value
            } fields;
            uint32_t raw_value;
        } dword0;

        union {
            uint32_t raw_value;
            struct {
                uint32_t reserved_dw1;     // DWORD 1 - Reserved
            } fields;
        } dword1;
        union {
            uint32_t raw_value;
            struct {
                uint32_t reserved_dw2;   // DWORD 2 - Reserved
            } fields;
        } dword2;
        union {
            uint32_t raw_value;
            struct {
                uint32_t reserved_dw3;   // DWORD 3 - Reserved
            } fields;
        } dword3;
    };

    std::string to_string() const override {
        return "END";
    }

private:
    end_data_t end_data_;
};

namespace post_si_pcietc_ee_instr {

    // Helper function to create appropriate instruction from raw DWORDs
    inline std::unique_ptr<ee_instruction_base_t> create_instruction_from_dwords(
        const std::array<uint32_t, 4>& dwords) {

        // Extract instruction type from DWORD 0
        uint32_t instr_type_bits = dwords[0] & 0xFF;
        ee_instr_type_t instr_type = static_cast<ee_instr_type_t>(instr_type_bits);

        switch(instr_type) {
            case ee_instr_type_t::MEMORY_READ:
                return std::make_unique<ee_memory_rw_instruction_t>(dwords[0], dwords[1], dwords[2], dwords[3]);

            case ee_instr_type_t::MEMORY_WRITE:
                return std::make_unique<ee_memory_rw_instruction_t>(dwords[0], dwords[1], dwords[2], dwords[3]);

            case ee_instr_type_t::MSI:
                return std::make_unique<ee_msi_instruction_t>(dwords[0], dwords[1], dwords[2], dwords[3]);

            case ee_instr_type_t::LOOP_CONTROL:
                return std::make_unique<ee_loop_control_instruction_t>(dwords[0], dwords[1], dwords[2], dwords[3]);

            case ee_instr_type_t::FENCE:
                return std::make_unique<ee_fence_instruction_t>(dwords[0], dwords[1], dwords[2], dwords[3]);

            case ee_instr_type_t::END:
                return std::make_unique<ee_end_instruction_t>(dwords[0], dwords[1], dwords[2], dwords[3]);

            default:
                // Default to memory read for unknown types
                cvm::log(cvm::NONE, "[post_si_pcietc_ee_instr] create_instruction_from_dwords: Unknown instruction type={} from dwords={:x} {:x} {:x} {:x}\n", static_cast<uint32_t>(instr_type), dwords[0], dwords[1], dwords[2], dwords[3]);
                return nullptr;
        }
    }

}
