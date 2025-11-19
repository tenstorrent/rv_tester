#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <mem_manager.h>
#include "cvm/logger.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/messenger.hpp"
#include "transactor.h"
#include "axi_sw_mst.h"
#include "rv_tester_transactions.hpp"
#include "post_si_pcietc_defs.hpp"
#include "post_si_pcietc_ee_regs.hpp"
#include "post_si_pcietc_ee_instr.hpp"


DECLARE_bool(post_si_pcietc_xtor_en);
DECLARE_double(post_si_pcietc_delay_multiplier);
DECLARE_int32(post_si_pcietc_tick_interval);
DECLARE_string(post_si_pcietc_ee0_file);
DECLARE_string(post_si_pcietc_ee1_file);
DECLARE_string(post_si_pcietc_ee2_file);
DECLARE_string(post_si_pcietc_ee3_file);

// EE class implementation
class post_si_pcietc_ee {
    // Allow register classes to access private members
    friend class ee_cfg_reg_t;
    friend class ee_ctl_reg_t;
    friend class ee_sts_reg_t;
    friend class ee_err_sts_reg_t;
    friend class ee_fail_addr_lo_reg_t;
    friend class ee_fail_addr_hi_reg_t;
    friend class ee_fail_exp_reg_t;
    friend class ee_fail_act_reg_t;
    friend class ee_fail_instr_reg_t;
    friend class ee_dbg0_reg_t;
    friend class ee_dbg1_reg_t;
    friend class ee_cdata_reg_t;

    // AXI transaction constants
    static constexpr size_t MAX_BEAT_SIZE = 64; // 2^6

    // Execution Engine register offsets
    enum class ee_regs_t : uint32_t {
        EE_CFG          = 0x000,    // Execute ee_cfg
        EE_CTL          = 0x004,    // Execute ee_ctl
        EE_STS          = 0x008,    // Execute ee_sts
        EE_ERR_STS      = 0x010,    // Execute ee_err_sts
        EE_FAIL_ADDR_LO = 0x014,    // Execute ee_fail_addr_lo
        EE_FAIL_ADDR_HI = 0x018,    // Execute ee_fail_addr_hi
        EE_FAIL_EXP     = 0x01C,    // Execute ee_fail_exp_data
        EE_FAIL_ACT     = 0x020,    // Execute ee_fail_act_data
        EE_FAIL_INSTR   = 0x024,    // Execute ee_fail_instr
        EE_DBG0         = 0x040,    // Execute ee_dbg0
        EE_DBG1         = 0x044,    // Execute ee_dbg1
        EE_LOOP_CNT     = 0x048,    // Execute ee_loop_cnt
        EE_CDATA0       = 0x0C0,    // Execute ee_cdata0
        EE_CDATA1       = 0x0C4,    // Execute ee_cdata1
        EE_CDATA2       = 0x0C8,    // Execute ee_cdata2
        EE_CDATA3       = 0x0CC     // Execute ee_cdata3
    };

public:
    // Constructor
    post_si_pcietc_ee(int index);
    // Destructor
    ~post_si_pcietc_ee();
    // Handle read function - takes offset and returns data
    bool handle_read(post_si_pcietc_helper_rpc_data_t &data);
    // Handle write function - takes offset and data
    bool handle_write(const post_si_pcietc_helper_rpc_data_t &data);

private:
    // Register objects
    ee_cfg_reg_t        reg_ee_cfg_;        // 0x000 - Configuration register
    ee_ctl_reg_t        reg_ee_ctl_;        // 0x004 - Control register
    ee_sts_reg_t        reg_ee_sts_;        // 0x008 - Status register
    ee_err_sts_reg_t    reg_ee_err_sts_;    // 0x010 - Error status register
    ee_fail_addr_lo_reg_t reg_ee_fail_addr_lo_; // 0x014 - Failure address low
    ee_fail_addr_hi_reg_t reg_ee_fail_addr_hi_; // 0x018 - Failure address high
    ee_fail_exp_reg_t   reg_ee_fail_exp_;   // 0x01C - Failure expected data
    ee_fail_act_reg_t   reg_ee_fail_act_;   // 0x020 - Failure actual data
    ee_fail_instr_reg_t reg_ee_fail_instr_; // 0x024 - Failure instruction
    ee_dbg0_reg_t       reg_ee_dbg0_;       // 0x040 - Debug register 0
    ee_dbg1_reg_t       reg_ee_dbg1_;       // 0x044 - Debug register 1
    ee_loop_cnt_reg_t   reg_ee_loop_cnt_;   // 0x048 - Loop count register
    ee_cdata_reg_t      reg_ee_cdata0_;     // 0x0C0 - Custom data 0
    ee_cdata_reg_t      reg_ee_cdata1_;     // 0x0C4 - Custom data 1
    ee_cdata_reg_t      reg_ee_cdata2_;     // 0x0C8 - Custom data 2
    ee_cdata_reg_t      reg_ee_cdata3_;     // 0x0CC - Custom data 3

    // Register map for offset-based access
    std::unordered_map<uint32_t, ee_reg_base_t*> register_map_;

    // Helper method to initialize register map
    void init_register_map();
    // Register read function - takes offset and returns data
    void reg_read(uint32_t offset, uint32_t &data);
    // Register write function - takes offset and data
    void reg_write(uint32_t offset, uint32_t data);
    // Memory read function - reads data from memory address
    std::vector<uint8_t> memory_read(uint32_t offset, size_t length);
    // Memory write function - writes data to memory address
    void memory_write(uint32_t offset, size_t length, const std::vector<uint8_t>& data);

    // Execution engine states enum
    enum class ee_state_t {
        EE_IDLE,
        EE_START_REQ,
        EE_RUNNING,
        EE_STOP_REQ,
        EE_STOPPED,
        EE_ERROR
    };
    // Enum for execution engine modes
    enum class ee_mode_t {
        NORMAL,      // Normal execution mode
        INF_REPEAT   // Infinite repeat mode
    };

    // AXI read tracking structure
    struct axi_read_entry_t {
        uint64_t rd_addr;     // Read address
        uint32_t rd_size;     // Read size
        unsigned int instr_idx;   // Instruction index

        // Default constructor for std::map compatibility
        axi_read_entry_t() : rd_addr(0), rd_size(0), instr_idx(0) {}

        axi_read_entry_t(uint64_t addr, unsigned int idx)
            : rd_addr(addr), rd_size(0), instr_idx(idx) {}
        axi_read_entry_t(uint64_t addr, uint32_t size, unsigned int idx)
            : rd_addr(addr), rd_size(size), instr_idx(idx) {}
    };

    using overlay_mst_t = axi_sw_mst<
        rv_tester_transactions::axi_sw_mst::b<>,
        rv_tester_transactions::axi_sw_mst::r<>,
        rv_tester_transactions::axi_sw_mst::ar_q_ptr<>,
        rv_tester_transactions::axi_sw_mst::aw_q_ptr<>,
        rv_tester_transactions::axi_sw_mst::w_q_ptr<>
    >;

    cvm::topology::loc_t tick_loc_;
    cvm::topology::loc_t axi_mst_loc_l;
    cvm::messenger::pool<axi::r_t>::channel_info r_channel;
    cvm::messenger::pool<axi::b_t>::channel_info wresp_channel;

    volatile uint64_t cnt_tick = 0;
    cvm::messenger::task<void> tick(int ticks = 1);

    volatile ee_state_t engine_state_ = ee_state_t::EE_IDLE;
    volatile bool engine_start_req_ = false;
    volatile bool engine_stop_req_ = false;
    unsigned int tgt_loop_count_ = 0;
    unsigned int loop_start_idx_ = 0;
    unsigned int instr_idx_ = 0;

    std::map<unsigned, uint64_t> pending_wr_id_;
    std::map<unsigned, uint64_t> axi_read_tracker_;  // Map indexed by AXI ID
    cvm::messenger::task<void> wr_ack();
    cvm::messenger::task<void> rd_cpl_task();

    int ee_index_;
    mem_manager ib_mem_;

    // Execution engine thread
    cvm::messenger::task<void> start_engine();
    // Get the current instruction at instr_idx
    std::unique_ptr<ee_instruction_base_t> get_instruction(uint32_t instr_idx);

    // Load instructions from file
    void load_instructions(const std::string& file_path);

    // Helper methods for engine state management
    cvm::messenger::task<void> engine_process_instruction(bool &internal_instruction);
    cvm::messenger::task<bool> engine_get_instruction(unsigned int instr_idx, std::unique_ptr<ee_instruction_base_t>& instr);
    cvm::messenger::task<bool> engine_delay_instruction(std::unique_ptr<ee_instruction_base_t>& instr);

    cvm::messenger::task<void> get_axi_req_token(bool write_req);
    cvm::messenger::task<void> release_axi_req_token(bool write_req);
    cvm::messenger::task<void> handle_memory_read_instruction(std::unique_ptr<ee_instruction_base_t>& instr);
    cvm::messenger::task<void> handle_memory_write_instruction(std::unique_ptr<ee_instruction_base_t>& instr);
    cvm::messenger::task<void> handle_msi_instruction(std::unique_ptr<ee_instruction_base_t>& instr);
    cvm::messenger::task<void> handle_loop_control_instruction(std::unique_ptr<ee_instruction_base_t>& instr);
    cvm::messenger::task<void> handle_fence_instruction(std::unique_ptr<ee_instruction_base_t>& instr);
    cvm::messenger::task<void> handle_end_instruction(std::unique_ptr<ee_instruction_base_t>& instr);
    void engine_log_error(ee_instr_error_t error);
    void engine_log_failure(unsigned int idx, uint64_t addr, uint32_t expected, uint32_t actual);
};
