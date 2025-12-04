#include <cstring>
#include <algorithm>
#include "cvm/logger.hpp"
#include "post_si_pcietc_ce.hpp"
#include <iomanip>
#include <sstream>
// Constructor
post_si_pcietc_ce::post_si_pcietc_ce()
    : reg_ce_cfg_(this), reg_ce_ctl_(this), reg_ce_sts_(this), reg_ce_fail_addr_lo_(this), reg_ce_fail_addr_hi_(this),
      reg_ce_fail_exp_(this), reg_ce_fail_act_(this),
      reg_ce_r0_dly_(this, 0, "CE_R0_DLY"), reg_ce_r1_dly_(this, 0, "CE_R1_DLY"),
      reg_ce_r2_dly_(this, 0, "CE_R2_DLY"), reg_ce_r3_dly_(this, 0, "CE_R3_DLY"),
      reg_ce_cdata0_(this, 0x1a2b3c4d, "CE_CDATA0"), reg_ce_cdata1_(this, 0xffffffff, "CE_CDATA1"),
      reg_ce_cdata2_(this, 0x5a5a5a5a, "CE_CDATA2"), reg_ce_cdata3_(this, 0xfeedf00d, "CE_CDATA3") {
    init_register_map();
}

// Destructor
post_si_pcietc_ce::~post_si_pcietc_ce() {
    // No special cleanup needed
}

// Register read function - takes offset and returns data
void post_si_pcietc_ce::reg_read(uint32_t offset, uint32_t &data) {
    data = 0;
    auto it = register_map_.find(offset);
    if (it != register_map_.end()) {
        cvm::log(cvm::HIGH, "[post_si_pcietc_ce] reg_read: offset={:#x} found in map: {}\n", offset, it->second->get_name());
        data = it->second->read();
    } else {
        cvm::log(cvm::HIGH, "[post_si_pcietc_ce] reg_read: offset={:#x} not found in register map\n", offset);
        for (const auto& reg_entry : register_map_) {
            cvm::log(cvm::DEBUG, "[post_si_pcietc_ce] register_map_: offset={:#x}, name={}\n", reg_entry.first, reg_entry.second->get_name());
        }
    }
    // If offset not found in map, data remains 0
}

// Register write function - takes offset and data
void post_si_pcietc_ce::reg_write(uint32_t offset, uint32_t data) {
    auto it = register_map_.find(offset);
    if (it != register_map_.end()) {
        cvm::log(cvm::HIGH, "[post_si_pcietc_ce] reg_write: offset={:#x} found in map: {}\n", offset, it->second->get_name());
        it->second->write(data);
    } else {
        cvm::log(cvm::HIGH, "[post_si_pcietc_ce] reg_write: offset={:#x} not found in register map\n", offset);
        for (const auto& reg_entry : register_map_) {
            cvm::log(cvm::DEBUG, "[post_si_pcietc_ce] register_map_: offset={:#x}, name={}\n", reg_entry.first, reg_entry.second->get_name());
        }
    }
    // If offset not found in map, silently ignore
}

// Memory read function - reads data from memory address
std::vector<uint8_t> post_si_pcietc_ce::memory_read(uint64_t addr, size_t length) {
    return generate_pattern(static_cast<post_si_pcietc::data_pattern_t>(reg_ce_cfg_.reg_data.fields.pattern), addr, length,
                            0, reg_ce_cdata0_.read(), reg_ce_cdata1_.read(), reg_ce_cdata2_.read(), reg_ce_cdata3_.read());
}

void post_si_pcietc_ce::log_failure(uint64_t addr, uint32_t expected, uint32_t actual) {
    cvm::log(cvm::NONE, "[post_si_pcietc_ce] Failure: Data mismatch: at address {:#x} Expected: {:#x}, Actual: {:#x}\n", addr, expected, actual);
    if (reg_ce_sts_.reg_data.fields.failure_detected == 1) {
        reg_ce_sts_.reg_data.fields.multiple_fail = 1;
    } else {
        reg_ce_sts_.reg_data.fields.failure_detected = 1;
        reg_ce_fail_addr_lo_.reg_data.fields.addr_lo = addr & 0xffffffff;
        reg_ce_fail_addr_hi_.reg_data.fields.addr_hi = (addr >> 32) & 0xffffffff;
        reg_ce_fail_exp_.reg_data.fields.data = expected;
        reg_ce_fail_act_.reg_data.fields.data = actual;
    }
}

// Memory write function - writes data to memory address
void post_si_pcietc_ce::memory_write(uint64_t addr, size_t length, const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return; // Nothing to write
    }

    std::vector<uint8_t> expected_data = generate_pattern(static_cast<post_si_pcietc::data_pattern_t>(reg_ce_cfg_.reg_data.fields.pattern), addr, length,
                                                          0, reg_ce_cdata0_.read(), reg_ce_cdata1_.read(), reg_ce_cdata2_.read(), reg_ce_cdata3_.read());
    std::vector<uint32_t> expected_data_u32 = post_si_pcietc::pack_data_to_u32(addr, expected_data);
    std::vector<uint32_t> extracted_data_u32 = post_si_pcietc::pack_data_to_u32(addr, data);

    for (size_t i = 0; i < expected_data_u32.size(); i++) {
        if (extracted_data_u32[i] != expected_data_u32[i]) {
            log_failure(addr + i * 4, expected_data_u32[i], extracted_data_u32[i]);
        }
    }
    return;
}

bool post_si_pcietc_ce::handle_read(post_si_pcietc_helper_rpc_data_t &data) {
    uint32_t offset = data.offset;
    size_t length = data.length;

    if (is_in_range(offset, length, post_si_pcietc::address_region_t::CE_REGISTERS, post_si_pcietc::size_constants::CE_REGISTER_REGION_SIZE)) {
        if (offset % 4 != 0 || length != 4) {
            cvm::log(cvm::ERROR, "[post_si_pcietc_ce] handle_read: Register: offset={:#x} length={:#x} not aligned or not 4 bytes\n", offset, length);
            return false;
        }
        uint32_t read_data = 0;
        uint32_t block_offset = static_cast<uint32_t>(offset - static_cast<uint32_t>(post_si_pcietc::address_region_t::CE_REGISTERS));
        cvm::log(cvm::HIGH, "[post_si_pcietc_ce] handle_read: Register: offset={:#x} block_offset={:#x}\n", offset, block_offset);
        reg_read(block_offset, read_data);
        cvm::log(cvm::HIGH, "[post_si_pcietc_ce] handle_read: Register: offset={:#x} block_offset={:#x} read_data={:#x}\n", offset, block_offset, read_data);
        SERIALIZE_INT(read_data, 4, data.data);
        return true;
    }
    if (is_in_range(offset, length, post_si_pcietc::address_region_t::CE_MEMORY, post_si_pcietc::size_constants::CE_MEMORY_SIZE)) {
        cvm::log(cvm::HIGH, "[post_si_pcietc_ce] handle_read: Memory: offset={:#x} addr={:#x} length={:#x}\n", offset, data.addr, data.length);
        data.data = memory_read(data.addr, data.length);
        return true;
    }
    cvm::log(cvm::DEBUG, "[post_si_pcietc_ce] handle_read: offset={:#x} length={:#x} not handled\n", offset, length);
    return false;
}

bool post_si_pcietc_ce::handle_write(const post_si_pcietc_helper_rpc_data_t &data) {
    uint32_t offset = data.offset;
    size_t length = data.length;

    if (is_in_range(offset, length, post_si_pcietc::address_region_t::CE_REGISTERS, post_si_pcietc::size_constants::CE_REGISTER_REGION_SIZE)) {
        if (offset % 4 != 0 || length != 4) {
            cvm::log(cvm::ERROR, "[post_si_pcietc_ce] handle_write: Register: offset={:#x} length={:#x} not aligned or not 4 bytes\n", offset, length);
            return false;
        }
        uint32_t write_data = 0;
        DESERIALIZE_INT(data.data, 4, write_data);
        uint32_t block_offset = static_cast<uint32_t>(offset - static_cast<uint32_t>(post_si_pcietc::address_region_t::CE_REGISTERS));
        cvm::log(cvm::HIGH, "[post_si_pcietc_ce] handle_write: Register: offset={:#x} block_offset={:#x} write_data={:#x}\n", offset, block_offset, write_data);
        reg_write(block_offset, write_data);
        return true;
    }
    if (is_in_range(offset, length, post_si_pcietc::address_region_t::CE_MEMORY, post_si_pcietc::size_constants::CE_MEMORY_SIZE)) {
        cvm::log(cvm::HIGH, "[post_si_pcietc_ce] handle_write: Memory: offset={:#x} addr={:#x} length={:#x}\n", offset, data.addr, data.length);
        memory_write(data.addr, data.length, data.data);
        return true;
    }
    cvm::log(cvm::DEBUG, "[post_si_pcietc_ce] handle_write: offset={:#x} length={:#x} not handled\n", offset, length);
    return false;
}

// Helper method to initialize the register map
void post_si_pcietc_ce::init_register_map() {
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_CFG)]          = &reg_ce_cfg_;
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_CTL)]          = &reg_ce_ctl_;
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_STS)]          = &reg_ce_sts_;
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_FAIL_ADDR_LO)] = &reg_ce_fail_addr_lo_;
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_FAIL_ADDR_HI)] = &reg_ce_fail_addr_hi_;
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_FAIL_EXP)]     = &reg_ce_fail_exp_;
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_FAIL_ACT)]     = &reg_ce_fail_act_;
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_R0_DLY)]       = &reg_ce_r0_dly_;
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_R1_DLY)]       = &reg_ce_r1_dly_;
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_R2_DLY)]       = &reg_ce_r2_dly_;
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_R3_DLY)]       = &reg_ce_r3_dly_;
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_CDATA0)]       = &reg_ce_cdata0_;
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_CDATA1)]       = &reg_ce_cdata1_;
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_CDATA2)]       = &reg_ce_cdata2_;
    register_map_[static_cast<uint32_t>(ce_regs_t::CE_CDATA3)]       = &reg_ce_cdata3_;
}

void ce_ctl_reg_t::write(uint32_t value) {
    reg_data.raw_value = value;

    if (reg_data.fields.reset) {
        // Clear failure registers when reset bit is set
        parent_ce_->reg_ce_sts_.reg_data.fields.failure_detected = 0;
        parent_ce_->reg_ce_sts_.reg_data.fields.multiple_fail = 0;
        parent_ce_->reg_ce_fail_addr_lo_.reg_data.fields.addr_lo = 0;
        parent_ce_->reg_ce_fail_addr_hi_.reg_data.fields.addr_hi = 0;
        parent_ce_->reg_ce_fail_exp_.reg_data.fields.data = 0;
        parent_ce_->reg_ce_fail_act_.reg_data.fields.data = 0;

        reg_data.fields.reset = 0;  // Write-only bit, auto-clear
    }
}