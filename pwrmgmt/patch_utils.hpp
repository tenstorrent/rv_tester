#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstdint>

// Patch entry structure
struct PatchEntry {
    std::string patchTag;
    uint32_t patchInstruction;
    uint32_t patchMask;
    uint32_t enableMask;
};


class PatchUtils {
public:
    // Patch data structures
    static std::map<std::string, PatchEntry> patches;
    static std::unordered_map<uint64_t, uint32_t> hex_address_data;
    static std::unordered_map<uint64_t, uint64_t> patch_ram;

    // Non-task patch functions
    static void read_hex_and_assembly(const std::string& hex_file_path);
    static void parse_hex_file_for_direct_programming(const std::string& hex_file);
    static void parse_assembly_for_patch_metadata(const std::string& assembly_file);
    static std::vector<uint64_t> concatenate_uint32_to_uint64(const std::vector<uint32_t>& input);
    static void populate_patch_ram(uint64_t addr, const std::vector<uint64_t>& data);

};
