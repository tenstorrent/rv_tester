#pragma once

#include <cstdint>
#include <string>
#include "post_si_pcietc_ce.hpp"

// Forward declaration
class post_si_pcietc_ce;

// Base register class interface
class ce_reg_base_t {
public:
    ce_reg_base_t(post_si_pcietc_ce* parent = nullptr, const std::string& name = "")
        : parent_ce_(parent), name_(name) {}
    virtual ~ce_reg_base_t() = default;
    virtual uint32_t read() const = 0;
    virtual void write(uint32_t value) {
        // Default empty implementation for read-only registers
        // Override in derived classes that need write functionality
        (void)value; // Suppress unused parameter warning
    }
    const std::string& get_name() const { return name_; }

protected:
    post_si_pcietc_ce* parent_ce_;
    const std::string name_;
};

// CE Configuration Register (0x000)
class ce_cfg_reg_t : public ce_reg_base_t {

public:
    ce_cfg_reg_t(post_si_pcietc_ce* parent = nullptr, uint32_t default_value = 0)
        : ce_reg_base_t(parent, "CE_CFG") {
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
            uint32_t pattern    : 4;    // [3:0]   - Data pattern selection
            uint32_t reserved_15_4 : 12; // [15:4]  - Reserved
            uint32_t r1_mode    : 1;    // [16]    - Region 1 mode
            uint32_t r2_mode    : 1;    // [17]    - Region 2 mode
            uint32_t r3_mode    : 1;    // [18]    - Region 3 mode
            uint32_t reserved_31_19 : 13; // [31:19] - Reserved
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// CE Control Register (0x004)
class ce_ctl_reg_t : public ce_reg_base_t {
    friend class post_si_pcietc_ce;

public:
    ce_ctl_reg_t(post_si_pcietc_ce* parent = nullptr, uint32_t default_value = 0)
        : ce_reg_base_t(parent, "CE_CTL") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    void write(uint32_t value) override;

    union {
        struct {
            uint32_t reserved_15_0 : 16; // [15:0]  - Reserved
            uint32_t reset         : 1;  // [16]    - Reset
            uint32_t reserved_31_17 : 15; // [31:17] - Reserved
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// CE Status Register (0x008)
class ce_sts_reg_t : public ce_reg_base_t {
    friend class post_si_pcietc_ce;

public:
    ce_sts_reg_t(post_si_pcietc_ce* parent = nullptr, uint32_t default_value = 0)
        : ce_reg_base_t(parent, "CE_STS") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t failure_detected : 1;  // [0]     - Failure detected
            uint32_t multiple_fail    : 1;  // [1]     - Multiple failures
            uint32_t reserved_31_2    : 30; // [31:2]  - Reserved
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// CE Failure Address Low Register (0x014)
class ce_fail_addr_lo_reg_t : public ce_reg_base_t {
    friend class post_si_pcietc_ce;

public:
    ce_fail_addr_lo_reg_t(post_si_pcietc_ce* parent = nullptr, uint32_t default_value = 0)
        : ce_reg_base_t(parent, "CE_FAIL_ADDR_LO") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t addr_lo : 32;  // [31:0] - Lower 32-bits of failure address
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// CE Failure Address High Register (0x018)
class ce_fail_addr_hi_reg_t : public ce_reg_base_t {
    friend class post_si_pcietc_ce;

public:
    ce_fail_addr_hi_reg_t(post_si_pcietc_ce* parent = nullptr, uint32_t default_value = 0)
        : ce_reg_base_t(parent, "CE_FAIL_ADDR_HI") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t addr_hi : 32;  // [31:0] - Upper 32-bits of failure address
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// CE Failure Expected Data Register (0x01C)
class ce_fail_exp_reg_t : public ce_reg_base_t {
    friend class post_si_pcietc_ce;

public:
    ce_fail_exp_reg_t(post_si_pcietc_ce* parent = nullptr, uint32_t default_value = 0)
        : ce_reg_base_t(parent, "CE_FAIL_EXP") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t data : 32;  // [31:0] - Expected data value
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// CE Failure Actual Data Register (0x020)
class ce_fail_act_reg_t : public ce_reg_base_t {
    friend class post_si_pcietc_ce;

public:
    ce_fail_act_reg_t(post_si_pcietc_ce* parent = nullptr, uint32_t default_value = 0)
        : ce_reg_base_t(parent, "CE_FAIL_ACT") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t data : 32;  // [31:0] - Actual data value
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// CE Region Delay Configuration Register (0x040, 0x044, 0x048, 0x04C)
class ce_region_dly_reg_t : public ce_reg_base_t {
    friend class post_si_pcietc_ce;

public:
    ce_region_dly_reg_t(post_si_pcietc_ce* parent = nullptr, uint32_t default_value = 0, const std::string& name = "CE_REGION_DLY")
        : ce_reg_base_t(parent, name) {
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
            uint32_t delay       : 8;   // [7:0]   - Delay value
            uint32_t delay_gran  : 2;   // [9:8]   - Delay granularity
            uint32_t reserved_31_10 : 22; // [31:10] - Reserved
        } fields;
        uint32_t raw_value;
    } reg_data;
};

// CE Custom Data Register (0x0C0, 0x0C4, 0x0C8, 0x0CC)
class ce_cdata_reg_t : public ce_reg_base_t {
    friend class post_si_pcietc_ce;

public:
    ce_cdata_reg_t(post_si_pcietc_ce* parent = nullptr, uint32_t default_value = 0, const std::string& name = "CE_CDATA")
        : ce_reg_base_t(parent, name) {
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
            uint32_t data : 32;  // [31:0] - Custom data value
        } fields;
        uint32_t raw_value;
    } reg_data;
};
