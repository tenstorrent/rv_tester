#pragma once
#include <array>
#include <unordered_map>

namespace {

    constexpr int max_intr = 16;
    constexpr int xlen = 64;
    constexpr int vlen = 256;
    constexpr int va_hi = 56;

    struct csr_entry {
        std::string name;
        uint64_t address;
    };

    std::array<csr_entry, 246> csrs {{
        {"fflags", 0x001},
        {"frm", 0x002},
        {"fcsr", 0x003},
        {"sstatus", 0x100},
        {"sie", 0x104},
        {"stvec", 0x105},
        {"scounteren", 0x106},
        {"senvcfg", 0x10A},
        {"sscratch", 0x140},
        {"sepc", 0x141},
        {"scause", 0x142},
        {"stval", 0x143},
        {"sip", 0x144},
        {"satp", 0x180},
        {"scontext", 0x5A8},
        {"hstatus", 0x600},
        {"hedeleg", 0x602},
        {"hideleg", 0x603},
        {"hie", 0x604},
        {"hcounteren", 0x606},
        {"hgeie", 0x607},
        {"htval", 0x643},
        {"hip", 0x644},
        {"hvip", 0x645},
        {"htinst", 0x64A},
        {"hgeip", 0xE12},
        {"henvcfg", 0x60A},
        {"henvcfgh", 0x61A},
        {"hgatp", 0x680},
        {"hcontext", 0x6A8},
        {"htimedelta", 0x605},
        {"htimedeltah", 0x615},
        {"vsstatus", 0x200},
        {"vsie", 0x204},
        {"vstvec", 0x205},
        {"vsscratch", 0x240},
        {"vsepc", 0x241},
        {"vscause", 0x242},
        {"vstval", 0x243},
        {"vsip", 0x244},
        {"vsatp", 0x280},
        {"mvendorid", 0xF11},
        {"marchid", 0xF12},
        {"mimpid", 0xF13},
        {"mhartid", 0xF14},
        {"mconfigptr", 0xF15},
        {"mstatus", 0x300},
        {"misa", 0x301},
        {"medeleg", 0x302},
        {"mideleg", 0x303},
        {"mie", 0x304},
        {"mtvec", 0x305},
        {"mcounteren", 0x306},
        {"mstatush", 0x310},
        {"mscratch", 0x340},
        {"mepc", 0x341},
        {"mcause", 0x342},
        {"mtval", 0x343},
        {"mip", 0x344}
    }};

    std::array<csr_entry, 4> nonzero_reset_csrs {{
        {"mstatus", 0x300},
        {"misa", 0x301},
        {"medeleg", 0x302},
        {"mideleg", 0x303}
    }};

    std::array<csr_entry, 18> metrics_csrs {{
        {"fcsr", 0x003},
        {"sstatus", 0x100},
        {"sie", 0x104},
        {"stvec", 0x105},
        {"sepc", 0x141},
        {"scause", 0x142},
        {"stval", 0x143},
        {"sip", 0x144},
        {"mstatus", 0x300},
        {"misa", 0x301},
        {"medeleg", 0x302},
        {"mideleg", 0x303},
        {"mie", 0x304},
        {"mtvec", 0x305},
        {"mepc", 0x341},
        {"mcause", 0x342},
        {"mtval", 0x343},
        {"mip", 0x344}
    }};

    typedef enum : size_t {
        U = 0,
        HS = 1,
        M = 3,
        D = 4,
        DE = 5,
        VU = 8,
        VS = 9,
        VM = 11
    } priv;

    const std::unordered_map<priv, std::string_view> priv_to_string = {
        {U, "U"},
        {HS, "HS"},
        {M, "M"},
        {D, "D"},
        {DE, "DE"},
        {VU, "VU"},
        {VS, "VS"},
        {VM, "M"},
    };

    typedef enum : size_t {
        INSN_ADDR_MISALGN = 0,
        INSN_ACCESS_FAULT = 1,
        ILLEGAL_INSN = 2,
        BREAKPOINT = 3,
        LD_ADDR_MISALGN = 4,
        LD_ACCESS_FAULT = 5,
        ST_AMO_ADDR_MISALGN = 6,
        ST_AMO_ACCESS_FAULT = 7,
        ECALL_U = 8,
        ECALL_S = 9,
        ECALL_M = 11,
        INSN_PAGE_FAULT = 12,
        LD_PAGE_FAULT = 13,
        ST_AMO_PAGE_FAULT = 15
    } excp;

    const std::unordered_map<excp, std::string_view> excp_to_string = {
        {INSN_ADDR_MISALGN, "INSN_ADDR_MISALGN"},
        {INSN_ACCESS_FAULT, "INSN_ACCESS_FAULT"},
        {ILLEGAL_INSN, "ILLEGAL_INSN"},
        {BREAKPOINT, "BREAKPOINT"},
        {LD_ADDR_MISALGN, "LD_ADDR_MISALGN"},
        {LD_ACCESS_FAULT, "LD_ACCESS_FAULT"},
        {ST_AMO_ADDR_MISALGN, "ST_AMO_ADDR_MISALGN"},
        {ST_AMO_ACCESS_FAULT, "ST_AMO_ACCESS_FAULT"},
        {ECALL_U, "ECALL_U"},
        {ECALL_S, "ECALL_S"},
        {ECALL_M, "ECALL_M"},
        {INSN_PAGE_FAULT, "INSN_PAGE_FAULT"},
        {LD_PAGE_FAULT, "LD_PAGE_FAULT"},
        {ST_AMO_PAGE_FAULT, "ST_AMO_PAGE_FAULT"}
    };

    typedef enum : size_t {
        SSI = 1,
        VSSI = 2,
        MSI = 3,
        STI = 5,
        VSTI = 6,
        MTI = 7,
        SEI = 9,
        VSEI = 10,
        MEI = 11,
        SGEI = 12
    } intr;

    const std::unordered_map<intr, std::string_view> intr_to_string = {
        {SSI, "SSI"},
        {VSSI, "VSSI"},
        {MSI, "MSI"},
        {STI, "STI"},
        {VSTI, "VSTI"},
        {MTI, "MTI"},
        {SEI, "SEI"},
        {VSEI, "VSEI"},
        {MEI, "MEI"},
        {SGEI, "SGEI"}
    };

}
