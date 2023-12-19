#include <string>
#include <cstdint>

namespace whisper {
    void initialize();
    std::string disassemble(const uint32_t opcode);
};
