#include "cvm/logger.hpp"
#include "post_si_pcietc_defs.hpp"
#include "post_si_pcietc_global.hpp"

// Constructor
post_si_pcietc_global::post_si_pcietc_global()
    : reg_version_(this, 0xdeaddead) {  // Default: major=0, minor=0
    init_register_map();
}

// Destructor
post_si_pcietc_global::~post_si_pcietc_global() {
    // No special cleanup needed
}

// Register read function - takes offset and returns data
void post_si_pcietc_global::reg_read(uint32_t offset, uint32_t &data) {
    data = 0;

    auto it = register_map_.find(offset);
    if (it != register_map_.end()) {
        cvm::log(cvm::HIGH, "[post_si_pcietc_global] reg_read: found offset={:#x} in map: {}\n", offset, it->second->get_name());
        data = it->second->read();
    } else {
        cvm::log(cvm::HIGH, "[post_si_pcietc_global] reg_read: offset={:#x} not found in map: {}\n", offset, it->second->get_name());
        for (const auto& reg_entry : register_map_) {
            cvm::log(cvm::DEBUG, "[post_si_pcietc_global] register_map_: offset={:#x}, name={}\n", reg_entry.first, reg_entry.second->get_name());
        }
    }
    // If offset not found in map, data remains 0
}

// Register write function - takes offset and data
void post_si_pcietc_global::reg_write(uint32_t offset, uint32_t data) {
    auto it = register_map_.find(offset);
    if (it != register_map_.end()) {
        cvm::log(cvm::HIGH, "[post_si_pcietc_global] reg_write: offset={:#x} found in map: {}\n", offset, it->second->get_name());
        it->second->write(data);
    } else {
        cvm::log(cvm::HIGH, "[post_si_pcietc_global] reg_write: offset={:#x} not found in map: {}\n", offset, it->second->get_name());
        for (const auto& reg_entry : register_map_) {
            cvm::log(cvm::DEBUG, "[post_si_pcietc_global] register_map_: offset={:#x}, name={}\n", reg_entry.first, reg_entry.second->get_name());
        }
    }
    // If offset not found in map, silently ignore
}

bool post_si_pcietc_global::handle_read(post_si_pcietc_helper_rpc_data_t &data) {
    uint32_t offset = data.offset;
    size_t length = data.length;

    if (is_in_range(offset, length, post_si_pcietc::address_region_t::GLOBAL_REGISTERS, post_si_pcietc::size_constants::GLOBAL_REGION_SIZE)) {
        if (offset % 4 != 0 || length != 4) {
            cvm::log(cvm::ERROR, "[post_si_pcietc_global] handle_read: Register: offset={:#x} length={:#x} not aligned or not 4 bytes\n", offset, length);
            return false;
        }
        uint32_t read_data = 0;
        uint32_t block_offset = static_cast<uint32_t>(offset - static_cast<uint32_t>(post_si_pcietc::address_region_t::GLOBAL_REGISTERS));
        cvm::log(cvm::HIGH, "[post_si_pcietc_global] handle_read: Register: offset={:#x} block_offset={:#x}\n", offset, block_offset);
        reg_read(static_cast<uint32_t>(offset), read_data);
        cvm::log(cvm::HIGH, "[post_si_pcietc_global] handle_read: Register: offset={:#x} block_offset={:#x} read_data={:#x}\n", offset, block_offset, read_data);
        SERIALIZE_INT(read_data, 4, data.data);
        return true;
    }
    cvm::log(cvm::DEBUG, "[post_si_pcietc_global] handle_read: offset={:#x} length={:#x} not handled\n", offset, length);

    return false;
}

bool post_si_pcietc_global::handle_write(const post_si_pcietc_helper_rpc_data_t &data) {
    uint32_t offset = data.offset;
    size_t length = data.length;

    if (is_in_range(offset, length, post_si_pcietc::address_region_t::GLOBAL_REGISTERS, post_si_pcietc::size_constants::GLOBAL_REGION_SIZE)) {
        if (offset % 4 != 0 || length != 4) {
            cvm::log(cvm::ERROR, "[post_si_pcietc_global] handle_write: Register: offset={:#x} length={:#x} not aligned or not 4 bytes\n", offset, length);
            return false;
        }
        uint32_t write_data = 0;
        DESERIALIZE_INT(data.data, 4, write_data);
        uint32_t block_offset = static_cast<uint32_t>(offset - static_cast<uint32_t>(post_si_pcietc::address_region_t::GLOBAL_REGISTERS));
        cvm::log(cvm::HIGH, "[post_si_pcietc_global] handle_write: Register: offset={:#x} block_offset={:#x} write_data={:#x}\n", offset, block_offset, write_data);
        reg_write(static_cast<uint32_t>(offset), write_data);
        return true;
    }
    cvm::log(cvm::DEBUG, "[post_si_pcietc_global] handle_write: offset={:#x} length={:#x} not handled\n", offset, length);
    return false;
}

// Helper method to initialize the register map
void post_si_pcietc_global::init_register_map() {
    register_map_[static_cast<uint32_t>(global_regs_t::GLOBAL_VERSION)] = &reg_version_;
}

std::vector<uint8_t> post_si_pcietc::generate_pattern(post_si_pcietc::data_pattern_t pattern, uint64_t addr, size_t length,
                                                      uint32_t loop_count, uint32_t custom_dw0, uint32_t custom_dw1, uint32_t custom_dw2, uint32_t custom_dw3) {
    std::vector<uint8_t> result;

    if (length == 0) {
        return result;
    }

    uint64_t starting_addr = addr - (addr & 0x3);
    uint64_t end_addr = addr + length;
    // Round up end_addr to the next multiple of 4
    if (end_addr % 4 != 0) {
        end_addr = ((end_addr + 3) / 4) * 4;
    }
    uint32_t num_words = (end_addr - starting_addr) / 4;
    int offset = addr & 0x3;
    cvm::log(
        cvm::FULL,
        "[post_si_pcietc] pattern generator: starting_addr={:#x} end_addr={:#x} length={} num_words={} pattern={}\n",
        starting_addr, end_addr, length, num_words, static_cast<int>(pattern)
    );

    switch (static_cast<post_si_pcietc::data_pattern_t>(pattern)) {
        case post_si_pcietc::data_pattern_t::ZEROS:
            result.resize(num_words * 4, 0x00);
            break;

        case post_si_pcietc::data_pattern_t::ADDR_BASED: {
            result.resize(num_words * 4, 0x00);
            uint32_t value = starting_addr>>2;
            for (uint32_t i = 0; i < num_words; ++i) {
                result[i * 4 + 0] = static_cast<uint8_t>(value & 0xFF);
                result[i * 4 + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
                result[i * 4 + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
                result[i * 4 + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
                value += 1;
            }
            break;
        }

        case post_si_pcietc::data_pattern_t::INV_ADDR_BASED: {
            result.resize(num_words * 4, 0x00);
            uint32_t value = starting_addr>>2;
            for (uint32_t i = 0; i < num_words; ++i) {
                uint32_t inv_value = ~value;
                result[i * 4 + 0] = static_cast<uint8_t>(inv_value & 0xFF);
                result[i * 4 + 1] = static_cast<uint8_t>((inv_value >> 8) & 0xFF);
                result[i * 4 + 2] = static_cast<uint8_t>((inv_value >> 16) & 0xFF);
                result[i * 4 + 3] = static_cast<uint8_t>((inv_value >> 24) & 0xFF);
                value += 4;
            }
            break;
        }

        case post_si_pcietc::data_pattern_t::LOOP_COUNT: {
            uint32_t value = loop_count;
            std::vector<uint8_t> pattern = {
                static_cast<uint8_t>(value & 0xFF),
                static_cast<uint8_t>((value >> 8) & 0xFF),
                static_cast<uint8_t>((value >> 16) & 0xFF),
                static_cast<uint8_t>((value >> 24) & 0xFF)
            };
            for (uint32_t i = 0; i < num_words; ++i) {
                result.insert(result.end(), pattern.begin(), pattern.end());
            }
            break;
        }

        case post_si_pcietc::data_pattern_t::CUSTOM_DW0: {
            uint32_t value = custom_dw0;
            std::vector<uint8_t> pattern = {
                static_cast<uint8_t>(value & 0xFF),
                static_cast<uint8_t>((value >> 8) & 0xFF),
                static_cast<uint8_t>((value >> 16) & 0xFF),
                static_cast<uint8_t>((value >> 24) & 0xFF)
            };
            for (uint32_t i = 0; i < num_words; ++i) {
                result.insert(result.end(), pattern.begin(), pattern.end());
            }
            break;
        }

        case post_si_pcietc::data_pattern_t::CUSTOM_DW1: {
            uint32_t value = custom_dw1;
            std::vector<uint8_t> pattern = {
                static_cast<uint8_t>(value & 0xFF),
                static_cast<uint8_t>((value >> 8) & 0xFF),
                static_cast<uint8_t>((value >> 16) & 0xFF),
                static_cast<uint8_t>((value >> 24) & 0xFF)
            };
            for (uint32_t i = 0; i < num_words; ++i) {
                result.insert(result.end(), pattern.begin(), pattern.end());
            }
            break;
        }

        case post_si_pcietc::data_pattern_t::CUSTOM_DW2: {
            uint32_t value = custom_dw2;
            std::vector<uint8_t> pattern = {
                static_cast<uint8_t>(value & 0xFF),
                static_cast<uint8_t>((value >> 8) & 0xFF),
                static_cast<uint8_t>((value >> 16) & 0xFF),
                static_cast<uint8_t>((value >> 24) & 0xFF)
            };
            for (uint32_t i = 0; i < num_words; ++i) {
                result.insert(result.end(), pattern.begin(), pattern.end());
            }
            break;
        }

        case post_si_pcietc::data_pattern_t::CUSTOM_DW3: {
            uint32_t value = custom_dw3;
            std::vector<uint8_t> pattern = {
                static_cast<uint8_t>(value & 0xFF),
                static_cast<uint8_t>((value >> 8) & 0xFF),
                static_cast<uint8_t>((value >> 16) & 0xFF),
                static_cast<uint8_t>((value >> 24) & 0xFF)
            };
            for (uint32_t i = 0; i < num_words; ++i) {
                result.insert(result.end(), pattern.begin(), pattern.end());
            }
            break;
        }
    }

    return std::vector<uint8_t>(result.begin() + offset, result.begin() + offset + length);
}

// Pack data to uint32_t vector
std::vector<uint32_t> post_si_pcietc::pack_data_to_u32(uint64_t addr, const std::vector<uint8_t>& data) {
    // Calculate misalignment
    size_t misalign = addr % 4;
    size_t total_bytes = misalign + data.size();
    size_t num_dwords = (total_bytes + 3) / 4;

    std::vector<uint8_t> padded_data;

    // Pad with zeros for misalignment
    padded_data.resize(misalign, 0);
    // Copy the data
    padded_data.insert(padded_data.end(), data.begin(), data.end());
    // Pad at the end if needed
    padded_data.resize(num_dwords * 4, 0);

    std::vector<uint32_t> result;
    result.reserve(num_dwords);
    for (size_t i = 0; i < num_dwords; ++i) {
        uint32_t word = 0;
        word |= static_cast<uint32_t>(padded_data[i*4 + 0]) << 0;
        word |= static_cast<uint32_t>(padded_data[i*4 + 1]) << 8;
        word |= static_cast<uint32_t>(padded_data[i*4 + 2]) << 16;
        word |= static_cast<uint32_t>(padded_data[i*4 + 3]) << 24;
        result.push_back(word);
    }

    return result;
}
