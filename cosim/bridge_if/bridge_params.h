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

    typedef enum : size_t {
        U = 0,
        HS = 1,
        M = 3,
        D = 4,
        VU = 8,
        VS = 9,
        VM = 11
    } priv;

    const std::unordered_map<priv, std::string_view> to_string = {
        {U, "U"},
        {HS, "HS"},
        {M, "M"},
        {D, "D"},
        {VU, "VU"},
        {VS, "VS"},
        {VM, "M"},
    };

}
