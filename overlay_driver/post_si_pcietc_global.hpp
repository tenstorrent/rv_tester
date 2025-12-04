#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include "post_si_pcietc_defs.hpp"

// Forward declaration
class post_si_pcietc_global;

// Base register class interface for Global registers
class global_reg_base_t {
public:
    global_reg_base_t(post_si_pcietc_global* parent = nullptr, const std::string& name = "")
        : parent_global_(parent), name_(name) {}
    virtual ~global_reg_base_t() = default;
    virtual uint32_t read() const = 0;
    virtual void write(uint32_t value) {
        // Default empty implementation for read-only registers
        // Override in derived classes that need write functionality
        (void)value; // Suppress unused parameter warning
    }

    const std::string& get_name() const { return name_; }

protected:
    post_si_pcietc_global* parent_global_;
    const std::string name_;
};

// Global Version Register (0x000)
class global_version_reg_t : public global_reg_base_t {
    friend class post_si_pcietc_global;

public:
    global_version_reg_t(post_si_pcietc_global* parent = nullptr, uint32_t default_value = 0)
        : global_reg_base_t(parent, "GLOBAL_VERSION") {
        reg_data.raw_value = default_value;
    }

    uint32_t read() const override {
        return reg_data.raw_value;
    }

    union {
        struct {
            uint32_t minor      : 8;    // [7:0]   - Minor Version
            uint32_t major      : 8;    // [15:8]  - Major Version
            uint32_t reserved_31_16 : 16; // [31:16] - Reserved
        } fields;
        uint32_t raw_value;
    } reg_data;
};

class post_si_pcietc_global {
    // Allow register classes to access private members
    friend class global_version_reg_t;

    // Global register offsets
    enum class global_regs_t : uint32_t {
        GLOBAL_VERSION  = 0x000     // Version register
    };

public:
    // Constructor
    post_si_pcietc_global();
    // Destructor
    ~post_si_pcietc_global();

    bool handle_read(post_si_pcietc_helper_rpc_data_t &data);
    bool handle_write(const post_si_pcietc_helper_rpc_data_t &data);

private:
    // Register objects
    global_version_reg_t reg_version_;   // 0x000 - Version register

    // Register map for offset-based access
    std::unordered_map<uint32_t, global_reg_base_t*> register_map_;

    // Helper method to initialize register map
    void init_register_map();
    // Register read function - takes offset and returns data
    void reg_read(uint32_t offset, uint32_t &data);
    // Register write function - takes offset and data
    void reg_write(uint32_t offset, uint32_t data);
};
