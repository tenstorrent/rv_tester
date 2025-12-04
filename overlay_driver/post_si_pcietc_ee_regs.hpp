#pragma once

#include <cstdint>
#include <string>
#include "post_si_pcietc_ee.hpp"

// Forward declaration to avoid circular dependency
class post_si_pcietc_ee;

// Base register class interface for EE registers
class ee_reg_base_t {
public:
    ee_reg_base_t(post_si_pcietc_ee* parent = nullptr, const std::string& name = "")
        : parent_ee_(parent), name_(name) {}
    virtual ~ee_reg_base_t() = default;
    virtual uint32_t read() const = 0;
    virtual void write(uint32_t value) {
        // Default empty implementation for read-only registers
        // Override in derived classes that need write functionality
        (void)value; // Suppress unused parameter warning
    }

    const std::string& get_name() const { return name_; }

protected:
    post_si_pcietc_ee* parent_ee_;
    const std::string name_;
};

// EE Configuration Register (0x000)
class ee_cfg_reg_t : public ee_reg_base_t {

public:
    ee_cfg_reg_t(post_si_pcietc_ee* parent = nullptr, uint32_t default_value = 0)
        : ee_reg_base_t(parent, "EE_CFG") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    void write(uint32_t value) override {
        reg_data.raw_value = value;
    }

    union {
        struct {
            uint32_t mode       : 1;    // [0]     - 0x2-0x3
            uint32_t reserved_31_1 : 30; // [31:1]  - Reserved
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// EE Control Register (0x004)
class ee_ctl_reg_t : public ee_reg_base_t {
    friend class post_si_pcietc_ee;

public:
    ee_ctl_reg_t(post_si_pcietc_ee* parent = nullptr, uint32_t default_value = 0)
        : ee_reg_base_t(parent, "EE_CTL") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    void write(uint32_t value) override;

    union {
        struct {
            uint32_t start      : 1;    // [0]     - 0x1 to start
            uint32_t reserved_7_1 : 7; // [7:1]  - Reserved
            uint32_t stop       : 1;    // [8]    - 0x1 to stop
            uint32_t reserved_15_9 : 7; // [15:9] - Reserved
            uint32_t reset      : 1;    // [16]    - 0x1 to clear
            uint32_t reserved_31_17 : 15; // [31:17] - Reserved
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// EE Status Register (0x008)
class ee_sts_reg_t : public ee_reg_base_t {
    friend class post_si_pcietc_ee;

public:
    ee_sts_reg_t(post_si_pcietc_ee* parent = nullptr, uint32_t default_value = 0)
        : ee_reg_base_t(parent, "EE_STS") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t stopped     : 1;   // [0]     - 0x1 indicates stopped
            uint32_t error_detected   : 1;   // [1]     - 1 indicates error detected
            uint32_t failure_detected : 1;   // [2]     - 1 indicates failure detected
            uint32_t multiple_fail    : 1;   // [3]     - 1 indicates multiple failures
            uint32_t reserved_15_4 : 12; // [15:4]  - Reserved
            uint32_t instruction : 16;  // [31:16]  - Instruction causing failure
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// EE Error Status Register (0x010)
class ee_err_sts_reg_t : public ee_reg_base_t {
    friend class post_si_pcietc_ee;

public:
    ee_err_sts_reg_t(post_si_pcietc_ee* parent = nullptr, uint32_t default_value = 0)
        : ee_reg_base_t(parent, "EE_ERR_STS") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t reason      : 8;   // [7:0]   - Reason for error
            uint32_t reserved_31_8 : 24; // [31:8]  - Reserved
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// EE Failure Address Low Register (0x014)
class ee_fail_addr_lo_reg_t : public ee_reg_base_t {
    friend class post_si_pcietc_ee;

public:
    ee_fail_addr_lo_reg_t(post_si_pcietc_ee* parent = nullptr, uint32_t default_value = 0)
        : ee_reg_base_t(parent, "EE_FAIL_ADDR_LO") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t addr_lo : 32;  // [31:0] - Lower 32 bits of failure address
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// EE Failure Address High Register (0x018)
class ee_fail_addr_hi_reg_t : public ee_reg_base_t {
    friend class post_si_pcietc_ee;

public:
    ee_fail_addr_hi_reg_t(post_si_pcietc_ee* parent = nullptr, uint32_t default_value = 0)
        : ee_reg_base_t(parent, "EE_FAIL_ADDR_HI") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t addr_hi : 32;  // [31:0] - Upper 32 bits of failure address
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// EE Failure Expected Data Register (0x01C)
class ee_fail_exp_reg_t : public ee_reg_base_t {
    friend class post_si_pcietc_ee;

public:
    ee_fail_exp_reg_t(post_si_pcietc_ee* parent = nullptr, uint32_t default_value = 0)
        : ee_reg_base_t(parent, "EE_FAIL_EXP") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t data : 32;  // [31:0] - Expected data
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// EE Failure Actual Data Register (0x020)
class ee_fail_act_reg_t : public ee_reg_base_t {
    friend class post_si_pcietc_ee;

public:
    ee_fail_act_reg_t(post_si_pcietc_ee* parent = nullptr, uint32_t default_value = 0)
        : ee_reg_base_t(parent, "EE_FAIL_ACT") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t data : 32;  // [31:0] - Actual data
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// EE Failure Instruction Register (0x024)
class ee_fail_instr_reg_t : public ee_reg_base_t {
    friend class post_si_pcietc_ee;

public:
    ee_fail_instr_reg_t(post_si_pcietc_ee* parent = nullptr, uint32_t default_value = 0)
        : ee_reg_base_t(parent, "EE_FAIL_INSTR") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t index : 16;  // [15:0] - Instruction causing failure
            uint32_t reserved_31_16 : 16; // [31:16] - Reserved
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// EE Debug Register 0 (0x040)
class ee_dbg0_reg_t : public ee_reg_base_t {
    friend class post_si_pcietc_ee;

public:
    ee_dbg0_reg_t(post_si_pcietc_ee* parent = nullptr, uint32_t default_value = 0)
        : ee_reg_base_t(parent, "EE_DBG0") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t loopcnt : 32;  // [31:0]  - Current loopcount
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// EE Debug Register 1 (0x044)
class ee_dbg1_reg_t : public ee_reg_base_t {
    friend class post_si_pcietc_ee;

public:
    ee_dbg1_reg_t(post_si_pcietc_ee* parent = nullptr, uint32_t default_value = 0)
        : ee_reg_base_t(parent, "EE_DBG1") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t fence_info  : 4;   // [3:0]   - Fence pointer input
            uint32_t reserved_31_4 : 28; // [31:4]  - Reserved
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// EE Debug Register 1 (0x044)
class ee_loop_cnt_reg_t : public ee_reg_base_t {
    friend class post_si_pcietc_ee;

public:
    ee_loop_cnt_reg_t(post_si_pcietc_ee* parent = nullptr, uint32_t default_value = 0)
        : ee_reg_base_t(parent, "EE_LOOP_CNT") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t loop_cnt  : 32;   // [31:0]   - Loop count
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// EE Custom Data Register (0x0C0, 0x0C4, 0x0C8, 0x0CC)
class ee_cdata_reg_t : public ee_reg_base_t {
    friend class post_si_pcietc_ee;

public:
    ee_cdata_reg_t(post_si_pcietc_ee* parent = nullptr, uint32_t default_value = 0, const std::string& name = "EE_CDATA")
        : ee_reg_base_t(parent, name) {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    void write(uint32_t value) override {
        reg_data.raw_value = value;
    }

    union {
        struct {
            uint32_t data : 32;  // [31:0] - Custom data
        } fields;
        uint32_t raw_value;
    } reg_data;
};
