#include <string>
#include <vector>
#include <cstdint>

namespace cosim_util {
    std::string exec(const char* cmd);
    void rtrim(std::string &s);
    std::string get_nth_word(const std::string& s, int n);
    std::vector<uint32_t> opcode_move_value_to_register(uint64_t value, uint32_t rd, uint32_t temp_gpr2, uint32_t temp_gpr3);
    std::vector<std::string> split_string(const std::string& input, const std::string& delimiter);
    std::string hex_string_to_binary_string(const std::string& hex);
    bool has_substring(const std::vector<std::string>& vec, const std::string& substr);
    bool is_csr_opcode(const uint32_t& opcode, uint32_t&csr_addr);
};
