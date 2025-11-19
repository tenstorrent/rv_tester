#pragma once

#include <cstdint>
#include <vector>
#include "post_si_pcietc_defs.hpp"
#include "post_si_pcietc_ce_regs.hpp"

// CE class implementation
class post_si_pcietc_ce {
    // Allow register classes to access private members
    friend class ce_cfg_reg_t;
    friend class ce_ctl_reg_t;
    friend class ce_sts_reg_t;
    friend class ce_fail_addr_lo_reg_t;
    friend class ce_fail_addr_hi_reg_t;
    friend class ce_fail_exp_reg_t;
    friend class ce_fail_act_reg_t;
    friend class ce_region_dly_reg_t;
    friend class ce_cdata_reg_t;

    // Completer Engine register offsets
    enum class ce_regs_t : uint32_t {
        CE_CFG          = 0x000,    // Completion Engine Configuration
        CE_CTL          = 0x004,    // Completer Engine Control
        CE_STS          = 0x008,    // Completer Engine Status
        CE_FAIL_ADDR_LO = 0x014,    // Completer Engine Failure Address Low
        CE_FAIL_ADDR_HI = 0x018,    // Completer Engine Failure Address High
        CE_FAIL_EXP     = 0x01C,    // Completer Engine Failure Expected Data
        CE_FAIL_ACT     = 0x020,    // Completer Engine Failure Actual Data
        CE_R0_DLY       = 0x040,    // Completer Engine Region0 Delay Configuration
        CE_R1_DLY       = 0x044,    // Completer Engine Region1 Delay Configuration
        CE_R2_DLY       = 0x048,    // Completer Engine Region2 Delay Configuration
        CE_R3_DLY       = 0x04C,    // Completer Engine Region3 Delay Configuration
        CE_CDATA0       = 0x0C0,    // Completer Engine Custom Data0
        CE_CDATA1       = 0x0C4,    // Completer Engine Custom Data1
        CE_CDATA2       = 0x0C8,    // Completer Engine Custom Data2
        CE_CDATA3       = 0x0CC     // Completer Engine Custom Data3
    };

public:
    // Constructor
    post_si_pcietc_ce();
    // Destructor
    ~post_si_pcietc_ce();

    bool handle_read(post_si_pcietc_helper_rpc_data_t &data);
    bool handle_write(const post_si_pcietc_helper_rpc_data_t &data);

private:
    // Register objects
    ce_cfg_reg_t        reg_ce_cfg_;        // 0x000 - Configuration register
    ce_ctl_reg_t        reg_ce_ctl_;        // 0x004 - Control register
    ce_sts_reg_t        reg_ce_sts_;        // 0x008 - Status register
    ce_fail_addr_lo_reg_t reg_ce_fail_addr_lo_; // 0x014 - Failure address low
    ce_fail_addr_hi_reg_t reg_ce_fail_addr_hi_; // 0x018 - Failure address high
    ce_fail_exp_reg_t   reg_ce_fail_exp_;   // 0x01C - Failure expected data
    ce_fail_act_reg_t   reg_ce_fail_act_;   // 0x020 - Failure actual data
    ce_region_dly_reg_t reg_ce_r0_dly_;     // 0x040 - Region 0 delay
    ce_region_dly_reg_t reg_ce_r1_dly_;     // 0x044 - Region 1 delay
    ce_region_dly_reg_t reg_ce_r2_dly_;     // 0x048 - Region 2 delay
    ce_region_dly_reg_t reg_ce_r3_dly_;     // 0x04C - Region 3 delay
    ce_cdata_reg_t      reg_ce_cdata0_;     // 0x0C0 - Custom data 0
    ce_cdata_reg_t      reg_ce_cdata1_;     // 0x0C4 - Custom data 1
    ce_cdata_reg_t      reg_ce_cdata2_;     // 0x0C8 - Custom data 2
    ce_cdata_reg_t      reg_ce_cdata3_;     // 0x0CC - Custom data 3

    // Register map for offset-based access
    std::unordered_map<uint32_t, ce_reg_base_t*> register_map_;

    // Helper method to initialize register map
    void init_register_map();
    // Register read function - takes offset and returns data
    void reg_read(uint32_t offset, uint32_t &data);
    // Register write function - takes offset and data
    void reg_write(uint32_t offset, uint32_t data);
    // Memory read function - reads data from memory address
    std::vector<uint8_t> memory_read(uint64_t addr, size_t length);
    // Memory write function - writes data to memory address
    void memory_write(uint64_t addr, size_t length, const std::vector<uint8_t>& data);
    void log_failure(uint64_t addr, uint32_t expected, uint32_t actual);
    // TODO:
    // Implement data checking for each pattern
    // Implement data generation for each pattern
    // Implement data verification for each pattern
    // Implement delay configurations
    // Implement sub range modes
};
