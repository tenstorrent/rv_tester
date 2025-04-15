#include "util.h"

#include <cstdio>
#include <iostream>
#include <sstream>          // stringstream
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <algorithm>


// https://stackoverflow.com/a/478960
std::string cosim_util::exec(const char* cmd)
 {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// https://stackoverflow.com/a/217605
// trim from end (in place)
void cosim_util::rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
                }).base(), s.end());
}

std::string cosim_util::get_nth_word(const std::string& s, int n) {
    std::istringstream iss(s);
    std::string word;
    for (int i = 0; i < n; i++) {
        if (!(iss >> word))
            return "";
    }
    // Remove trailing comma if present
    if (!word.empty() && word.back() == ',') {
        word.pop_back();
    }
    return word;
}

std::vector<uint32_t> cosim_util::opcode_move_value_to_register(uint64_t value, uint32_t rd, uint32_t temp_gpr2, uint32_t temp_gpr3) {
    // generates a list of opcodes to move an immediate value to register
    // temp_gpr2, temp_gpr3 are needed for intermediate values, user should ensure to store earlier values of this gpr if its important
    std::vector<uint32_t> opcodes;
    int lui_opcode = 0x37, op_imm_opcode = 0x13, or_opcode = 0x33, addw_opcode = 0x1b;

    uint32_t lui_op = lui_opcode + (rd<<7) + (((value & 0xfffff000)>>12)<<12);
    opcodes.push_back(lui_op);
    if (value & 0x800) {
        lui_op = lui_opcode + (temp_gpr3<<7) + (0x1000);
        opcodes.push_back(lui_op);
        uint32_t addw_op = addw_opcode + (temp_gpr3<<7) + (temp_gpr3<<15) + (((value & 0xfff) - 0x1000)<<20);
        opcodes.push_back(addw_op);
        uint32_t or_op = or_opcode + (rd<<7/*rd*/) + (0b110<<12) + (temp_gpr3<<15) + (rd<<20);
        opcodes.push_back(or_op);
    } else {
        uint32_t ori_op = op_imm_opcode + (rd<<7) + (0b110<<12) + (rd<<15) + ((value & 0xfff)<<20);
        opcodes.push_back(ori_op);
    }

    if (value & 0x80000000) { // data gets sign extended, shl and shr to correct it
        uint32_t slli_op = op_imm_opcode + (rd<<7) + (0b001<<12) + (rd<<15) + (32<<20); // slli x4, x4, 32
        opcodes.push_back(slli_op);
        uint32_t srli_op = op_imm_opcode + (rd<<7) + (0b101<<12) + (rd<<15) + (32<<20); // srli x4, x4, 32
        opcodes.push_back(srli_op);
    }
    if (value > uint64_t(0xffffffff)) { // data is greater than 32-bits (another opcode, another temporary register)
        uint32_t lui_op = lui_opcode + (temp_gpr2<<7) + ((((value>>32) & 0xfffff000) >> 12)<<12);
        opcodes.push_back(lui_op);

        if ((value>>32) & 0x800) {
          lui_op = lui_opcode + (temp_gpr3<<7) + (0x1000);
          opcodes.push_back(lui_op);
          uint32_t addw_op = addw_opcode + (temp_gpr3<<7) + (temp_gpr3<<15) + ((((value>>32) & 0xfff) - 0x1000)<<20);
          opcodes.push_back(addw_op);
          uint32_t or_op = or_opcode + (temp_gpr2<<7) + (0b110<<12) + (temp_gpr3<<15) + (temp_gpr2<<20);
          opcodes.push_back(or_op);
        } else {
          uint32_t ori_op = op_imm_opcode + (temp_gpr2<<7) + (0b110<<12) /*funct3*/ + (temp_gpr2<<15) + (((value>>32) & 0xfff)<<20);
          opcodes.push_back(ori_op);
        }
        uint32_t slli_op = op_imm_opcode + (temp_gpr2<<7) + (0b001<<12) + (temp_gpr2<<15) + (32<<20);
        opcodes.push_back(slli_op);
        uint32_t or_op = or_opcode + (rd<<7) + (0b110<<12) + (temp_gpr2<<15) + (rd<<20);
        opcodes.push_back(or_op);
      }
    return opcodes;
}

std::vector<std::string> cosim_util::split_string (const std::string& input, const std::string& delimiter) {
  std::vector<std::string> tokens;
  size_t start = 0;
  size_t end = input.find(delimiter);
    while (end != std::string::npos) {
        tokens.push_back(input.substr(start, end - start)); // Extract the substring
        start = end + delimiter.length();  // Move past the delimiter
        end = input.find(delimiter, start);  // Find the next delimiter
    }
    tokens.push_back(input.substr(start));  // Add the last part of the string
    return tokens;
}

bool cosim_util::has_substring(const std::vector<std::string>& vec, const std::string& substring) {
    if (vec.size() == 0)
        return false;

    return std::any_of(vec.begin(), vec.end(), [&](const std::string& s) {
        return s.find(substring) != std::string::npos;
    });
}
