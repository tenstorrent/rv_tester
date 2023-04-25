#pragma once
#include <array>

namespace {

    constexpr int num_harts = 1;
    constexpr int xlen = 64;
    constexpr int vlen = 256;

    struct csr_entry {
        std::string name;
        uint64_t address;
    };
    
    std::array<csr_entry, 20> csrs {{
        {"sstatus", 0x100},
        {"sie", 0x104},
        {"stvec", 0x105},
        {"sscratch", 0x140},
        {"sepc", 0x141},
        {"scause", 0x142},
        {"stval", 0x143},
        {"sip", 0x144},
        {"satp", 0x180},
        {"mstatus", 0x300},
        {"misa", 0x301},
        {"medeleg", 0x302},
        {"mideleg", 0x303},
        {"mie", 0x304},
        {"mtvec", 0x305},
        {"mscratch", 0x340},
        {"mepc", 0x341},
        {"mcause", 0x342},
        {"mtval", 0x343},
        {"mip", 0x344}
    }};
}
