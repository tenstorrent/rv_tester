#include "patch_utils.hpp"
#include "cvm/logger.hpp"
#include <fstream>
#include <sstream>

// Define static members
std::map<std::string, PatchEntry> PatchUtils::patches;
std::unordered_map<uint64_t, uint32_t> PatchUtils::hex_address_data;
std::unordered_map<uint64_t, uint64_t> PatchUtils::patch_ram;

// Utility function to swap bytes (big-endian to little-endian conversion)
static uint32_t swap_bytes(uint32_t value) {
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x000000FF) << 24);
}

void PatchUtils::read_hex_and_assembly(const std::string& hex_file_path) {
    cvm::log(cvm::MEDIUM, "[pwrmgmt] Processing hex file and corresponding assembly: {}\n", hex_file_path);
    
    // 1. Parse hex file directly for programming data
    parse_hex_file_for_direct_programming(hex_file_path);
    
    // 2. Find corresponding assembly file in the same directory
    size_t pos = hex_file_path.find_last_of("/\\");
    std::string directory = hex_file_path.substr(0, pos);
    std::string filename = hex_file_path.substr(pos + 1);
    
    // Replace .hex extension with .s extension
    size_t ext_pos = filename.find_last_of('.');
    if (ext_pos != std::string::npos) {
        std::string basename = filename.substr(0, ext_pos);
        std::string assembly_file = directory + "/" + basename + "_copy.s";
        
        cvm::log(cvm::HIGH, "[pwrmgmt] Looking for corresponding assembly file: {}\n", assembly_file);
        
        // 3. Check if assembly file exists and parse it for metadata
        std::ifstream check_assembly(assembly_file);
        if (check_assembly.good()) {
            check_assembly.close();
            cvm::log(cvm::HIGH, "[pwrmgmt] Found assembly file: {}\n", assembly_file);
            parse_assembly_for_patch_metadata(assembly_file);
        } else {
            cvm::log(cvm::ERROR, "[pwrmgmt] Assembly file not found: {}. Only hex data will be available (no patch metadata)\n", assembly_file);
        }
    } else {
        cvm::log(cvm::ERROR, "[pwrmgmt] Invalid hex file path - no extension found: {}\n", hex_file_path);
    }
}


void PatchUtils::parse_hex_file_for_direct_programming(const std::string& hex_file) {
    cvm::log(cvm::HIGH, "[pwrmgmt] Parsing hex file: {}\n", hex_file);
    
    std::ifstream file(hex_file);
    if (!file.is_open()) {
        cvm::log(cvm::ERROR, "Error: Could not open hex file: {}\n", hex_file);
        return;
    }
    
    // Clear previous data
    hex_address_data.clear();
    
    std::string line;
    uint64_t current_address = 0; // Use offset 0, base address applied when programming
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Handle simple hex format (16 hex chars per line, like patch_ucode.hex)
        if (line.length() == 16) {
            // Each line represents 8 bytes (64 bits)
            // Split into two 32-bit words and process each
            std::string first_word = line.substr(0, 8);   // First 32 bits
            std::string second_word = line.substr(8, 8);  // Second 32 bits
            
            try {
                // Process first 32-bit word with byte swapping
                uint32_t word1_raw = static_cast<uint32_t>(std::stoul(first_word, nullptr, 16));
                uint32_t word1_data = swap_bytes(word1_raw);
                if (word1_data != 0) {  // Skip zero values
                    hex_address_data[current_address] = word1_data;
                    cvm::log(cvm::HIGH, "[pwrmgmt] Loaded: 0x{:08x} @ offset 0x{:08x} (swapped from 0x{:08x})\n", word1_data, current_address, word1_raw);
                }
                
                // Process second 32-bit word with byte swapping
                uint32_t word2_raw = static_cast<uint32_t>(std::stoul(second_word, nullptr, 16));
                uint32_t word2_data = swap_bytes(word2_raw);
                if (word2_data != 0) {  // Skip zero values
                    hex_address_data[current_address + 4] = word2_data;
                    cvm::log(cvm::HIGH, "[pwrmgmt] Loaded: 0x{:08x} @ offset 0x{:08x} (swapped from 0x{:08x})\n", word2_data, current_address + 4, word2_raw);
                }
                
                current_address += 8; // Advance to next 64-bit line
            } catch (const std::exception& e) {
                cvm::log(cvm::ERROR, "[pwrmgmt] Error parsing hex line: {}\n", line);
                current_address += 8; // Still advance address even on parse error
                continue;
            }
        }
    }
    
    file.close();
    
    cvm::log(cvm::HIGH, "[pwrmgmt] Total non-zero hex entries loaded: {}\n", hex_address_data.size());
}

void PatchUtils::parse_assembly_for_patch_metadata(const std::string& assembly_file) {
    cvm::log(cvm::HIGH, "[pwrmgmt] ========== Parsing Assembly for Patch Metadata ==========\n");
    cvm::log(cvm::HIGH, "[pwrmgmt] Assembly file path: {}\n", assembly_file);
    
    std::ifstream file(assembly_file);
    if (!file.is_open()) {
        cvm::log(cvm::ERROR, "Error: Could not open assembly file: {}\n", assembly_file);
        return;
    }
    
    // Clear previous patch data
    patches.clear();
    
    // Maps to store extracted .equ values
    std::unordered_map<std::string, uint32_t> patch_instructions;
    std::unordered_map<std::string, uint32_t> patch_masks;
    std::unordered_map<std::string, uint32_t> enable_masks;
    
    std::string line;
    int line_count = 0;
    int equ_lines_found = 0;
    
    // Parse .equ definitions
    while (std::getline(file, line)) {
        line_count++;
        
        // Trim whitespace
        std::string trimmed_line = line;
        trimmed_line.erase(0, trimmed_line.find_first_not_of(" \t"));
        trimmed_line.erase(trimmed_line.find_last_not_of(" \t") + 1);
        
        // Skip empty lines and comments
        if (trimmed_line.empty() || trimmed_line[0] == '#') continue;
        
        // Parse .equ definitions
        if (trimmed_line.find(".equ") == 0) {
            equ_lines_found++;
            cvm::log(cvm::HIGH, "[pwrmgmt] Found .equ line {}: {}\n", line_count, trimmed_line);
            // Parse .equ SYMBOL, VALUE format
            size_t equ_end = trimmed_line.find(".equ") + 4;
            std::string line_after_equ = trimmed_line.substr(equ_end);
            
            // Find comma position in the entire line after .equ
            size_t comma_pos = line_after_equ.find(',');
            if (comma_pos != std::string::npos) {
                // Extract symbol (before comma)
                std::string symbol = line_after_equ.substr(0, comma_pos);
                symbol.erase(0, symbol.find_first_not_of(" \t"));
                symbol.erase(symbol.find_last_not_of(" \t") + 1);
                
                // Extract value (after comma)
                std::string value_str = line_after_equ.substr(comma_pos + 1);
                value_str.erase(0, value_str.find_first_not_of(" \t"));
                value_str.erase(value_str.find_last_not_of(" \t") + 1);
                
                //cvm::log(cvm::HIGH, "[pwrmgmt] Parsing symbol: '{}' with value_str: '{}'\n", symbol, value_str);
                
                try {
                    uint32_t value = static_cast<uint32_t>(std::stoull(value_str, nullptr, 16));
                    //cvm::log(cvm::HIGH, "[pwrmgmt] Successfully parsed value: 0x{:x}\n", value);
                    
                    // Extract instruction name and categorize
                    if (symbol.find("_PATCH_INSTRUCTION") != std::string::npos) {
                        std::string patch_tag = symbol.substr(0, symbol.find("_PATCH_INSTRUCTION"));
                        patch_instructions[patch_tag] = value;
                        cvm::log(cvm::HIGH, "[pwrmgmt] Found patch instruction: {} = 0x{:x}\n", patch_tag, value);
                    } else if (symbol.find("_PATCH_MASK") != std::string::npos) {
                        std::string patch_tag = symbol.substr(0, symbol.find("_PATCH_MASK"));
                        patch_masks[patch_tag] = value;
                        cvm::log(cvm::HIGH, "[pwrmgmt] Found patch mask: {} = 0x{:x}\n", patch_tag, value);
                    } else if (symbol.find("_ENABLE_MASK") != std::string::npos) {
                        std::string patch_tag = symbol.substr(0, symbol.find("_ENABLE_MASK"));
                        enable_masks[patch_tag] = value;
                        cvm::log(cvm::HIGH, "[pwrmgmt] Found enable mask: {} = 0x{:x}\n", patch_tag, value);
                    } else {
                        cvm::log(cvm::HIGH, "[pwrmgmt] Skipping .equ symbol (not patch-related): {}\n", symbol);
                    }
                } catch (const std::exception& e) {
                    cvm::log(cvm::HIGH, "[pwrmgmt] Failed to parse value '{}' for symbol '{}': {}\n", value_str, symbol, e.what());
                } catch (...) {
                    cvm::log(cvm::HIGH, "[pwrmgmt] Failed to parse value '{}' for symbol '{}': unknown error\n", value_str, symbol);
                }
            }
        }
    }
    file.close();
    
    cvm::log(cvm::HIGH, "[pwrmgmt] Assembly file parsing complete: {} lines processed, {} .equ lines found\n", line_count, equ_lines_found);
    cvm::log(cvm::HIGH, "[pwrmgmt] Found {} patch instructions, {} patch masks, {} enable masks\n", 
             patch_instructions.size(), patch_masks.size(), enable_masks.size());
    
    // Create PatchEntry structures from extracted .equ values
    cvm::log(cvm::HIGH, "[pwrmgmt] Creating patch entries from .equ definitions\n");
    for (const auto& instr_pair : patch_instructions) {
        const std::string& patch_tag = instr_pair.first;
        
        PatchEntry entry;
        entry.patchTag = patch_tag;
        entry.patchInstruction = instr_pair.second;
        entry.patchMask = patch_masks[patch_tag];
        entry.enableMask = enable_masks[patch_tag];
        
        patches[patch_tag] = entry;
        
        cvm::log(cvm::MEDIUM, "[pwrmgmt] Created patch entry: '{}' with instruction=0x{:08x}, mask=0x{:08x}, enable=0x{:04x}\n", 
                 patch_tag, entry.patchInstruction, entry.patchMask, entry.enableMask);
    }
    
    // Output patch metadata (tag, instruction, mask only)
    cvm::log(cvm::HIGH, "[pwrmgmt] ========== Patch Metadata ==========\n");
    for (const auto& pair : patches) {
        cvm::log(cvm::HIGH, "Patch Tag: {}\n", pair.second.patchTag);
        cvm::log(cvm::HIGH, "Patch Instruction: 0x{:x}\n", pair.second.patchInstruction);
        cvm::log(cvm::HIGH, "Patch Mask: 0x{:x}\n", pair.second.patchMask);
        cvm::log(cvm::HIGH, "***********************\n");
    }
    
    cvm::log(cvm::HIGH, "[pwrmgmt] Found {} patch metadata entries\n", patches.size());
}

std::vector<uint64_t> PatchUtils::concatenate_uint32_to_uint64(const std::vector<uint32_t>& input) {
    std::vector<uint64_t> result;
    int size = input.size();
    // Loop through input array and concatenate pairs
    for (int i = 0; i < size; i += 2) {
        uint32_t low = input[i];
        uint32_t high = (i + 1 < size) ? input[i + 1] : 0;  // Use 0 if no pair available
        uint64_t concatenated = static_cast<uint64_t>(high) << 32 | low;
        result.push_back(concatenated);
    }
    return result;
}

void PatchUtils::populate_patch_ram(uint64_t addr, const std::vector<uint64_t>& data) {
    int size = data.size();
    for(int i = 0; i < size; i++) {
        uint64_t addr_n = addr + i * 8;
        patch_ram[addr_n] = data[i];
        //cvm::log(cvm::NONE, "[pwrmgmt]  populate_patch_ram : addr 0x{:x} , data 0x{:x} \n", addr_n, data[i] );
    }
}

