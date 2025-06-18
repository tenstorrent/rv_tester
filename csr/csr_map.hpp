#pragma once
#include <string>
#include <utility>
#include <cstdint>
#include <vector>

namespace CSR {

struct field {
    std::string name;
    int width;
    std::pair<int, int> range;  // (msb, lsb)
    std::uint64_t reset_val;  // 64-bit hex reset value
    std::vector<std::uint64_t> legal_value;  // Array of 64-bit hex legal values
    std::string sw_type;
    std::string description;
    std::uint64_t bit_mask;  // Bit position as hex number (bits set to 1 for field positions)
    
    inline std::uint64_t extract_value(std::uint64_t csr_data) const {
        return (csr_data & bit_mask) >> range.second;
    }
};

struct csr_base {
    std::string name;
    std::uint16_t address;  // 12-bit hex address
    int size;
    csr_base* alias_of;  // Pointer to aliased CSR struct
    
    csr_base(const std::string& csr_name, std::uint16_t csr_address, int csr_size)
        : name(csr_name), address(csr_address), size(csr_size), alias_of(nullptr) {}
    
    csr_base() = default;
};

struct cycle_csr : public csr_base {
    cycle_csr() : csr_base("cycle", 0xC00, 64) {}

    field CYCLE = {
        "cycle.CYCLE",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct time__csr : public csr_base {
    time__csr() : csr_base("time", 0xC01, 64) {}

    field TIME_ = {
        "time.TIME",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct instret_csr : public csr_base {
    instret_csr() : csr_base("instret", 0xC02, 64) {}

    field INSTRET = {
        "instret.INSTRET",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter3_csr : public csr_base {
    hpmcounter3_csr() : csr_base("hpmcounter3", 0xC03, 64) {}

    field HPMCOUNTER3 = {
        "hpmcounter3.HPMCOUNTER3",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter4_csr : public csr_base {
    hpmcounter4_csr() : csr_base("hpmcounter4", 0xC04, 64) {}

    field HPMCOUNTER4 = {
        "hpmcounter4.HPMCOUNTER4",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter5_csr : public csr_base {
    hpmcounter5_csr() : csr_base("hpmcounter5", 0xC05, 64) {}

    field HPMCOUNTER5 = {
        "hpmcounter5.HPMCOUNTER5",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter6_csr : public csr_base {
    hpmcounter6_csr() : csr_base("hpmcounter6", 0xC06, 64) {}

    field HPMCOUNTER6 = {
        "hpmcounter6.HPMCOUNTER6",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter7_csr : public csr_base {
    hpmcounter7_csr() : csr_base("hpmcounter7", 0xC07, 64) {}

    field HPMCOUNTER7 = {
        "hpmcounter7.HPMCOUNTER7",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter8_csr : public csr_base {
    hpmcounter8_csr() : csr_base("hpmcounter8", 0xC08, 64) {}

    field HPMCOUNTER8 = {
        "hpmcounter8.HPMCOUNTER8",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter9_csr : public csr_base {
    hpmcounter9_csr() : csr_base("hpmcounter9", 0xC09, 64) {}

    field HPMCOUNTER9 = {
        "hpmcounter9.HPMCOUNTER9",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter10_csr : public csr_base {
    hpmcounter10_csr() : csr_base("hpmcounter10", 0xC0A, 64) {}

    field HPMCOUNTER10 = {
        "hpmcounter10.HPMCOUNTER10",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter11_csr : public csr_base {
    hpmcounter11_csr() : csr_base("hpmcounter11", 0xC0B, 64) {}

    field HPMCOUNTER11 = {
        "hpmcounter11.HPMCOUNTER11",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter12_csr : public csr_base {
    hpmcounter12_csr() : csr_base("hpmcounter12", 0xC0C, 64) {}

    field HPMCOUNTER12 = {
        "hpmcounter12.HPMCOUNTER12",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter13_csr : public csr_base {
    hpmcounter13_csr() : csr_base("hpmcounter13", 0xC0D, 64) {}

    field HPMCOUNTER13 = {
        "hpmcounter13.HPMCOUNTER13",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter14_csr : public csr_base {
    hpmcounter14_csr() : csr_base("hpmcounter14", 0xC0E, 64) {}

    field HPMCOUNTER14 = {
        "hpmcounter14.HPMCOUNTER14",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter15_csr : public csr_base {
    hpmcounter15_csr() : csr_base("hpmcounter15", 0xC0F, 64) {}

    field HPMCOUNTER15 = {
        "hpmcounter15.HPMCOUNTER15",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter16_csr : public csr_base {
    hpmcounter16_csr() : csr_base("hpmcounter16", 0xC10, 64) {}

    field HPMCOUNTER16 = {
        "hpmcounter16.HPMCOUNTER16",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter17_csr : public csr_base {
    hpmcounter17_csr() : csr_base("hpmcounter17", 0xC11, 64) {}

    field HPMCOUNTER17 = {
        "hpmcounter17.HPMCOUNTER17",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter18_csr : public csr_base {
    hpmcounter18_csr() : csr_base("hpmcounter18", 0xC12, 64) {}

    field HPMCOUNTER18 = {
        "hpmcounter18.HPMCOUNTER18",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter19_csr : public csr_base {
    hpmcounter19_csr() : csr_base("hpmcounter19", 0xC13, 64) {}

    field HPMCOUNTER19 = {
        "hpmcounter19.HPMCOUNTER19",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter20_csr : public csr_base {
    hpmcounter20_csr() : csr_base("hpmcounter20", 0xC14, 64) {}

    field HPMCOUNTER20 = {
        "hpmcounter20.HPMCOUNTER20",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter21_csr : public csr_base {
    hpmcounter21_csr() : csr_base("hpmcounter21", 0xC15, 64) {}

    field HPMCOUNTER21 = {
        "hpmcounter21.HPMCOUNTER21",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter22_csr : public csr_base {
    hpmcounter22_csr() : csr_base("hpmcounter22", 0xC16, 64) {}

    field HPMCOUNTER22 = {
        "hpmcounter22.HPMCOUNTER22",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter23_csr : public csr_base {
    hpmcounter23_csr() : csr_base("hpmcounter23", 0xC17, 64) {}

    field HPMCOUNTER23 = {
        "hpmcounter23.HPMCOUNTER23",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter24_csr : public csr_base {
    hpmcounter24_csr() : csr_base("hpmcounter24", 0xC18, 64) {}

    field HPMCOUNTER24 = {
        "hpmcounter24.HPMCOUNTER24",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter25_csr : public csr_base {
    hpmcounter25_csr() : csr_base("hpmcounter25", 0xC19, 64) {}

    field HPMCOUNTER25 = {
        "hpmcounter25.HPMCOUNTER25",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter26_csr : public csr_base {
    hpmcounter26_csr() : csr_base("hpmcounter26", 0xC1A, 64) {}

    field HPMCOUNTER26 = {
        "hpmcounter26.HPMCOUNTER26",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter27_csr : public csr_base {
    hpmcounter27_csr() : csr_base("hpmcounter27", 0xC1B, 64) {}

    field HPMCOUNTER27 = {
        "hpmcounter27.HPMCOUNTER27",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter28_csr : public csr_base {
    hpmcounter28_csr() : csr_base("hpmcounter28", 0xC1C, 64) {}

    field HPMCOUNTER28 = {
        "hpmcounter28.HPMCOUNTER28",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter29_csr : public csr_base {
    hpmcounter29_csr() : csr_base("hpmcounter29", 0xC1D, 64) {}

    field HPMCOUNTER29 = {
        "hpmcounter29.HPMCOUNTER29",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter30_csr : public csr_base {
    hpmcounter30_csr() : csr_base("hpmcounter30", 0xC1E, 64) {}

    field HPMCOUNTER30 = {
        "hpmcounter30.HPMCOUNTER30",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hpmcounter31_csr : public csr_base {
    hpmcounter31_csr() : csr_base("hpmcounter31", 0xC1F, 64) {}

    field HPMCOUNTER31 = {
        "hpmcounter31.HPMCOUNTER31",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct misa_csr : public csr_base {
    misa_csr() : csr_base("misa", 0x301, 64) {}

    field MXL = {
        "misa.MXL",
        2,
        {63, 62},
        0x0000000000000002ULL,
        {0x0000000000000002ULL},
        "WARL",
        "",
        0xC000000000000000ULL
    };
    field WLRL0 = {
        "misa.WLRL0",
        36,
        {61, 26},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x3FFFFFFFFC000000ULL
    };
    field Z = {
        "misa.Z",
        1,
        {25, 25},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000002000000ULL
    };
    field Y = {
        "misa.Y",
        1,
        {24, 24},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000001000000ULL
    };
    field X = {
        "misa.X",
        1,
        {23, 23},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000800000ULL
    };
    field W = {
        "misa.W",
        1,
        {22, 22},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000400000ULL
    };
    field V = {
        "misa.V",
        1,
        {21, 21},
        0x0000000000000001ULL,
        {},
        "WARL",
        "",
        0x0000000000200000ULL
    };
    field U = {
        "misa.U",
        1,
        {20, 20},
        0x0000000000000001ULL,
        {0x0000000000000001ULL},
        "WARL",
        "",
        0x0000000000100000ULL
    };
    field T = {
        "misa.T",
        1,
        {19, 19},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000080000ULL
    };
    field S = {
        "misa.S",
        1,
        {18, 18},
        0x0000000000000001ULL,
        {0x0000000000000001ULL},
        "WARL",
        "",
        0x0000000000040000ULL
    };
    field R = {
        "misa.R",
        1,
        {17, 17},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000020000ULL
    };
    field Q = {
        "misa.Q",
        1,
        {16, 16},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000010000ULL
    };
    field P = {
        "misa.P",
        1,
        {15, 15},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000008000ULL
    };
    field O = {
        "misa.O",
        1,
        {14, 14},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000004000ULL
    };
    field N = {
        "misa.N",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field M = {
        "misa.M",
        1,
        {12, 12},
        0x0000000000000001ULL,
        {},
        "WARL",
        "",
        0x0000000000001000ULL
    };
    field L = {
        "misa.L",
        1,
        {11, 11},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000800ULL
    };
    field K = {
        "misa.K",
        1,
        {10, 10},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000400ULL
    };
    field J = {
        "misa.J",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field I = {
        "misa.I",
        1,
        {8, 8},
        0x0000000000000001ULL,
        {0x0000000000000001ULL},
        "WARL",
        "",
        0x0000000000000100ULL
    };
    field H = {
        "misa.H",
        1,
        {7, 7},
        0x0000000000000001ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field G = {
        "misa.G",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field F = {
        "misa.F",
        1,
        {5, 5},
        0x0000000000000001ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field E = {
        "misa.E",
        1,
        {4, 4},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000010ULL
    };
    field D = {
        "misa.D",
        1,
        {3, 3},
        0x0000000000000001ULL,
        {},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field C = {
        "misa.C",
        1,
        {2, 2},
        0x0000000000000001ULL,
        {0x0000000000000001ULL},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field B = {
        "misa.B",
        1,
        {1, 1},
        0x0000000000000001ULL,
        {},
        "Read-Only",
        "",
        0x0000000000000002ULL
    };
    field A = {
        "misa.A",
        1,
        {0, 0},
        0x0000000000000001ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct mstatus_csr : public csr_base {
    mstatus_csr() : csr_base("mstatus", 0x300, 64) {}

    field SD = {
        "mstatus.SD",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field MSTATUS_WPRI_4 = {
        "mstatus.MSTATUS_WPRI_4",
        23,
        {62, 40},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x7FFFFF0000000000ULL
    };
    field MPV = {
        "mstatus.MPV",
        1,
        {39, 39},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000008000000000ULL
    };
    field GVA = {
        "mstatus.GVA",
        1,
        {38, 38},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000004000000000ULL
    };
    field MBE = {
        "mstatus.MBE",
        1,
        {37, 37},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000002000000000ULL
    };
    field SBE = {
        "mstatus.SBE",
        1,
        {36, 36},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000001000000000ULL
    };
    field SXL = {
        "mstatus.SXL",
        2,
        {35, 34},
        0x0000000000000002ULL,
        {0x0000000000000002ULL},
        "WARL",
        "",
        0x0000000C00000000ULL
    };
    field UXL = {
        "mstatus.UXL",
        2,
        {33, 32},
        0x0000000000000002ULL,
        {0x0000000000000002ULL},
        "WARL",
        "",
        0x0000000300000000ULL
    };
    field MSTATUS_WPRI_3 = {
        "mstatus.MSTATUS_WPRI_3",
        9,
        {31, 23},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x00000000FF800000ULL
    };
    field TSR = {
        "mstatus.TSR",
        1,
        {22, 22},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000400000ULL
    };
    field TW = {
        "mstatus.TW",
        1,
        {21, 21},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000200000ULL
    };
    field TVM = {
        "mstatus.TVM",
        1,
        {20, 20},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000100000ULL
    };
    field MXR = {
        "mstatus.MXR",
        1,
        {19, 19},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000080000ULL
    };
    field SUM = {
        "mstatus.SUM",
        1,
        {18, 18},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000040000ULL
    };
    field MPRV = {
        "mstatus.MPRV",
        1,
        {17, 17},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000020000ULL
    };
    field XS = {
        "mstatus.XS",
        2,
        {16, 15},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000018000ULL
    };
    field FS = {
        "mstatus.FS",
        2,
        {14, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000006000ULL
    };
    field MPP = {
        "mstatus.MPP",
        2,
        {12, 11},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000001800ULL
    };
    field VS = {
        "mstatus.VS",
        2,
        {10, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000600ULL
    };
    field SPP = {
        "mstatus.SPP",
        1,
        {8, 8},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000100ULL
    };
    field MPIE = {
        "mstatus.MPIE",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field UBE = {
        "mstatus.UBE",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field SPIE = {
        "mstatus.SPIE",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field MSTATUS_WPRI_2 = {
        "mstatus.MSTATUS_WPRI_2",
        1,
        {4, 4},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x0000000000000010ULL
    };
    field MIE = {
        "mstatus.MIE",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field MSTATUS_WPRI_1 = {
        "mstatus.MSTATUS_WPRI_1",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x0000000000000004ULL
    };
    field SIE = {
        "mstatus.SIE",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field MSTATUS_WPRI_0 = {
        "mstatus.MSTATUS_WPRI_0",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x0000000000000001ULL
    };
};

struct mtvec_csr : public csr_base {
    mtvec_csr() : csr_base("mtvec", 0x305, 64) {}

    field BASE = {
        "mtvec.BASE",
        62,
        {63, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFCULL
    };
    field MODE_1 = {
        "mtvec.MODE_1",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field MODE_0 = {
        "mtvec.MODE_0",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct medeleg_csr : public csr_base {
    medeleg_csr() : csr_base("medeleg", 0x302, 64) {}

    field RSVD_2 = {
        "medeleg.RSVD_2",
        40,
        {63, 24},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFF000000ULL
    };
    field MEDELEG_3 = {
        "medeleg.MEDELEG_3",
        4,
        {23, 20},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000F00000ULL
    };
    field RSVD_1 = {
        "medeleg.RSVD_1",
        4,
        {19, 16},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x00000000000F0000ULL
    };
    field MEDELEG_2 = {
        "medeleg.MEDELEG_2",
        1,
        {15, 15},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000008000ULL
    };
    field RSVD_0 = {
        "medeleg.RSVD_0",
        1,
        {14, 14},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000004000ULL
    };
    field MEDELEG_1 = {
        "medeleg.MEDELEG_1",
        2,
        {13, 12},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000003000ULL
    };
    field ECALL_FROM_M = {
        "medeleg.ECALL_FROM_M",
        1,
        {11, 11},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000800ULL
    };
    field MEDELEG_MASKED_0 = {
        "medeleg.MEDELEG_MASKED_0",
        1,
        {10, 10},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000400ULL
    };
    field MEDELEG_0 = {
        "medeleg.MEDELEG_0",
        10,
        {9, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000003FFULL
    };
};

struct mideleg_csr : public csr_base {
    mideleg_csr() : csr_base("mideleg", 0x303, 64) {}

    field LCOFIP = {
        "mideleg.LCOFIP",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field SGEIP = {
        "mideleg.SGEIP",
        1,
        {12, 12},
        0x0000000000000001ULL,
        {0x0000000000000001ULL},
        "WARL",
        "",
        0x0000000000001000ULL
    };
    field MEIP = {
        "mideleg.MEIP",
        1,
        {11, 11},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000800ULL
    };
    field VSEIP = {
        "mideleg.VSEIP",
        1,
        {10, 10},
        0x0000000000000001ULL,
        {0x0000000000000001ULL},
        "WARL",
        "",
        0x0000000000000400ULL
    };
    field SEIP = {
        "mideleg.SEIP",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field MTIP = {
        "mideleg.MTIP",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field VSTIP = {
        "mideleg.VSTIP",
        1,
        {6, 6},
        0x0000000000000001ULL,
        {0x0000000000000001ULL},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field STIP = {
        "mideleg.STIP",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field MSIP = {
        "mideleg.MSIP",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field VSSIP = {
        "mideleg.VSSIP",
        1,
        {2, 2},
        0x0000000000000001ULL,
        {0x0000000000000001ULL},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field SSIP = {
        "mideleg.SSIP",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
};

struct mip_csr : public csr_base {
    mip_csr() : csr_base("mip", 0x344, 64) {}

    field NONSTANDARDINTERRUPTS = {
        "mip.NONSTANDARDINTERRUPTS",
        48,
        {63, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFF0000ULL
    };
    field LCOFIP = {
        "mip.LCOFIP",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field SGEIP = {
        "mip.SGEIP",
        1,
        {12, 12},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000001000ULL
    };
    field MEIP = {
        "mip.MEIP",
        1,
        {11, 11},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000800ULL
    };
    field VSEIP = {
        "mip.VSEIP",
        1,
        {10, 10},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000400ULL
    };
    field SEIP = {
        "mip.SEIP",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field MTIP = {
        "mip.MTIP",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field VSTIP = {
        "mip.VSTIP",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field STIP = {
        "mip.STIP",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field MSIP = {
        "mip.MSIP",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field VSSIP = {
        "mip.VSSIP",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field SSIP = {
        "mip.SSIP",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
};

struct mie_csr : public csr_base {
    mie_csr() : csr_base("mie", 0x304, 64) {}

    field NONSTANDARDINTERRUPTS = {
        "mie.NONSTANDARDINTERRUPTS",
        48,
        {63, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFF0000ULL
    };
    field LCOFIE = {
        "mie.LCOFIE",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field SGEIE = {
        "mie.SGEIE",
        1,
        {12, 12},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000001000ULL
    };
    field MEIE = {
        "mie.MEIE",
        1,
        {11, 11},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000800ULL
    };
    field VSEIE = {
        "mie.VSEIE",
        1,
        {10, 10},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000400ULL
    };
    field SEIE = {
        "mie.SEIE",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field MTIE = {
        "mie.MTIE",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field VSTIE = {
        "mie.VSTIE",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field STIE = {
        "mie.STIE",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field MSIE = {
        "mie.MSIE",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field VSSIE = {
        "mie.VSSIE",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field SSIE = {
        "mie.SSIE",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
};

struct mscratch_csr : public csr_base {
    mscratch_csr() : csr_base("mscratch", 0x340, 64) {}

    field MSCRATCH = {
        "mscratch.MSCRATCH",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mepc_csr : public csr_base {
    mepc_csr() : csr_base("mepc", 0x341, 64) {}

    field ADDR = {
        "mepc.ADDR",
        63,
        {63, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFEULL
    };
};

struct mcause_csr : public csr_base {
    mcause_csr() : csr_base("mcause", 0x342, 64) {}

    field INTERRUPT = {
        "mcause.INTERRUPT",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field EXCEPTIONCODEWLRL = {
        "mcause.EXCEPTIONCODEWLRL",
        63,
        {62, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x7FFFFFFFFFFFFFFFULL
    };
};

struct mtval_csr : public csr_base {
    mtval_csr() : csr_base("mtval", 0x343, 64) {}

    field MTVAL = {
        "mtval.MTVAL",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mconfigptr_csr : public csr_base {
    mconfigptr_csr() : csr_base("mconfigptr", 0xF15, 64) {}

    field MCONFIGPTR = {
        "mconfigptr.MCONFIGPTR",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct menvcfg_csr : public csr_base {
    menvcfg_csr() : csr_base("menvcfg", 0x30A, 64) {}

    field STCE = {
        "menvcfg.STCE",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field PBMTE = {
        "menvcfg.PBMTE",
        1,
        {62, 62},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x4000000000000000ULL
    };
    field HADE = {
        "menvcfg.HADE",
        1,
        {61, 61},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x2000000000000000ULL
    };
    field WPRI_2 = {
        "menvcfg.WPRI_2",
        27,
        {60, 34},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x1FFFFFFC00000000ULL
    };
    field PMM = {
        "menvcfg.PMM",
        2,
        {33, 32},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000002ULL},
        "WARL",
        "",
        0x0000000300000000ULL
    };
    field WPRI_1 = {
        "menvcfg.WPRI_1",
        24,
        {31, 8},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x00000000FFFFFF00ULL
    };
    field CBZE = {
        "menvcfg.CBZE",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field CBCFE = {
        "menvcfg.CBCFE",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field CBIE = {
        "menvcfg.CBIE",
        2,
        {5, 4},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000030ULL
    };
    field WPRI_0 = {
        "menvcfg.WPRI_0",
        3,
        {3, 1},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x000000000000000EULL
    };
    field FIOM = {
        "menvcfg.FIOM",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct mseccfg_csr : public csr_base {
    mseccfg_csr() : csr_base("mseccfg", 0x747, 64) {}

    field WPRI_2 = {
        "mseccfg.WPRI_2",
        30,
        {63, 34},
        0x0000000000000000ULL,
        {},
        "WPRI",
        "",
        0xFFFFFFFC00000000ULL
    };
    field PMM = {
        "mseccfg.PMM",
        2,
        {33, 32},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000002ULL},
        "WARL",
        "",
        0x0000000300000000ULL
    };
    field WPRI_1 = {
        "mseccfg.WPRI_1",
        22,
        {31, 10},
        0x0000000000000000ULL,
        {},
        "WPRI",
        "",
        0x00000000FFFFFC00ULL
    };
    field SSEED = {
        "mseccfg.SSEED",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field USEED = {
        "mseccfg.USEED",
        1,
        {8, 8},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000100ULL
    };
    field WPRI_0 = {
        "mseccfg.WPRI_0",
        5,
        {7, 3},
        0x0000000000000000ULL,
        {},
        "WPRI",
        "",
        0x00000000000000F8ULL
    };
    field RLB = {
        "mseccfg.RLB",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field MMWP = {
        "mseccfg.MMWP",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field MML = {
        "mseccfg.MML",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct mcycle_csr : public csr_base {
    mcycle_csr() : csr_base("mcycle", 0xB00, 64) {}

    field CYCLE = {
        "mcycle.CYCLE",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct minstret_csr : public csr_base {
    minstret_csr() : csr_base("minstret", 0xB02, 64) {}

    field INSTRET = {
        "minstret.INSTRET",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter3_csr : public csr_base {
    mhpmcounter3_csr() : csr_base("mhpmcounter3", 0xB03, 64) {}

    field HPMCOUNTER3 = {
        "mhpmcounter3.HPMCOUNTER3",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter4_csr : public csr_base {
    mhpmcounter4_csr() : csr_base("mhpmcounter4", 0xB04, 64) {}

    field HPMCOUNTER4 = {
        "mhpmcounter4.HPMCOUNTER4",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter5_csr : public csr_base {
    mhpmcounter5_csr() : csr_base("mhpmcounter5", 0xB05, 64) {}

    field HPMCOUNTER5 = {
        "mhpmcounter5.HPMCOUNTER5",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter6_csr : public csr_base {
    mhpmcounter6_csr() : csr_base("mhpmcounter6", 0xB06, 64) {}

    field HPMCOUNTER6 = {
        "mhpmcounter6.HPMCOUNTER6",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter7_csr : public csr_base {
    mhpmcounter7_csr() : csr_base("mhpmcounter7", 0xB07, 64) {}

    field HPMCOUNTER7 = {
        "mhpmcounter7.HPMCOUNTER7",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter8_csr : public csr_base {
    mhpmcounter8_csr() : csr_base("mhpmcounter8", 0xB08, 64) {}

    field HPMCOUNTER8 = {
        "mhpmcounter8.HPMCOUNTER8",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter9_csr : public csr_base {
    mhpmcounter9_csr() : csr_base("mhpmcounter9", 0xB09, 64) {}

    field HPMCOUNTER9 = {
        "mhpmcounter9.HPMCOUNTER9",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter10_csr : public csr_base {
    mhpmcounter10_csr() : csr_base("mhpmcounter10", 0xB0A, 64) {}

    field HPMCOUNTER10 = {
        "mhpmcounter10.HPMCOUNTER10",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter11_csr : public csr_base {
    mhpmcounter11_csr() : csr_base("mhpmcounter11", 0xB0B, 64) {}

    field HPMCOUNTER11 = {
        "mhpmcounter11.HPMCOUNTER11",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter12_csr : public csr_base {
    mhpmcounter12_csr() : csr_base("mhpmcounter12", 0xB0C, 64) {}

    field HPMCOUNTER12 = {
        "mhpmcounter12.HPMCOUNTER12",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter13_csr : public csr_base {
    mhpmcounter13_csr() : csr_base("mhpmcounter13", 0xB0D, 64) {}

    field HPMCOUNTER13 = {
        "mhpmcounter13.HPMCOUNTER13",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter14_csr : public csr_base {
    mhpmcounter14_csr() : csr_base("mhpmcounter14", 0xB0E, 64) {}

    field HPMCOUNTER14 = {
        "mhpmcounter14.HPMCOUNTER14",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter15_csr : public csr_base {
    mhpmcounter15_csr() : csr_base("mhpmcounter15", 0xB0F, 64) {}

    field HPMCOUNTER15 = {
        "mhpmcounter15.HPMCOUNTER15",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter16_csr : public csr_base {
    mhpmcounter16_csr() : csr_base("mhpmcounter16", 0xB10, 64) {}

    field HPMCOUNTER16 = {
        "mhpmcounter16.HPMCOUNTER16",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter17_csr : public csr_base {
    mhpmcounter17_csr() : csr_base("mhpmcounter17", 0xB11, 64) {}

    field HPMCOUNTER17 = {
        "mhpmcounter17.HPMCOUNTER17",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter18_csr : public csr_base {
    mhpmcounter18_csr() : csr_base("mhpmcounter18", 0xB12, 64) {}

    field HPMCOUNTER18 = {
        "mhpmcounter18.HPMCOUNTER18",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter19_csr : public csr_base {
    mhpmcounter19_csr() : csr_base("mhpmcounter19", 0xB13, 64) {}

    field HPMCOUNTER19 = {
        "mhpmcounter19.HPMCOUNTER19",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter20_csr : public csr_base {
    mhpmcounter20_csr() : csr_base("mhpmcounter20", 0xB14, 64) {}

    field HPMCOUNTER20 = {
        "mhpmcounter20.HPMCOUNTER20",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter21_csr : public csr_base {
    mhpmcounter21_csr() : csr_base("mhpmcounter21", 0xB15, 64) {}

    field HPMCOUNTER21 = {
        "mhpmcounter21.HPMCOUNTER21",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter22_csr : public csr_base {
    mhpmcounter22_csr() : csr_base("mhpmcounter22", 0xB16, 64) {}

    field HPMCOUNTER22 = {
        "mhpmcounter22.HPMCOUNTER22",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter23_csr : public csr_base {
    mhpmcounter23_csr() : csr_base("mhpmcounter23", 0xB17, 64) {}

    field HPMCOUNTER23 = {
        "mhpmcounter23.HPMCOUNTER23",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter24_csr : public csr_base {
    mhpmcounter24_csr() : csr_base("mhpmcounter24", 0xB18, 64) {}

    field HPMCOUNTER24 = {
        "mhpmcounter24.HPMCOUNTER24",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter25_csr : public csr_base {
    mhpmcounter25_csr() : csr_base("mhpmcounter25", 0xB19, 64) {}

    field HPMCOUNTER25 = {
        "mhpmcounter25.HPMCOUNTER25",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter26_csr : public csr_base {
    mhpmcounter26_csr() : csr_base("mhpmcounter26", 0xB1A, 64) {}

    field HPMCOUNTER26 = {
        "mhpmcounter26.HPMCOUNTER26",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter27_csr : public csr_base {
    mhpmcounter27_csr() : csr_base("mhpmcounter27", 0xB1B, 64) {}

    field HPMCOUNTER27 = {
        "mhpmcounter27.HPMCOUNTER27",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter28_csr : public csr_base {
    mhpmcounter28_csr() : csr_base("mhpmcounter28", 0xB1C, 64) {}

    field HPMCOUNTER28 = {
        "mhpmcounter28.HPMCOUNTER28",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter29_csr : public csr_base {
    mhpmcounter29_csr() : csr_base("mhpmcounter29", 0xB1D, 64) {}

    field HPMCOUNTER29 = {
        "mhpmcounter29.HPMCOUNTER29",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter30_csr : public csr_base {
    mhpmcounter30_csr() : csr_base("mhpmcounter30", 0xB1E, 64) {}

    field HPMCOUNTER30 = {
        "mhpmcounter30.HPMCOUNTER30",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmcounter31_csr : public csr_base {
    mhpmcounter31_csr() : csr_base("mhpmcounter31", 0xB1F, 64) {}

    field HPMCOUNTER31 = {
        "mhpmcounter31.HPMCOUNTER31",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent3_csr : public csr_base {
    mhpmevent3_csr() : csr_base("mhpmevent3", 0x323, 64) {}

    field OF = {
        "mhpmevent3.OF",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field MINH = {
        "mhpmevent3.MINH",
        1,
        {62, 62},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x4000000000000000ULL
    };
    field SINH = {
        "mhpmevent3.SINH",
        1,
        {61, 61},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x2000000000000000ULL
    };
    field UINH = {
        "mhpmevent3.UINH",
        1,
        {60, 60},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x1000000000000000ULL
    };
    field VSINH = {
        "mhpmevent3.VSINH",
        1,
        {59, 59},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0800000000000000ULL
    };
    field VUINH = {
        "mhpmevent3.VUINH",
        1,
        {58, 58},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0400000000000000ULL
    };
    field RESERVED = {
        "mhpmevent3.RESERVED",
        2,
        {57, 56},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0300000000000000ULL
    };
    field MHPMEVENT3 = {
        "mhpmevent3.MHPMEVENT3",
        56,
        {55, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00FFFFFFFFFFFFFFULL
    };
};

struct mhpmevent4_csr : public csr_base {
    mhpmevent4_csr() : csr_base("mhpmevent4", 0x324, 64) {}

    field OF = {
        "mhpmevent4.OF",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field MINH = {
        "mhpmevent4.MINH",
        1,
        {62, 62},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x4000000000000000ULL
    };
    field SINH = {
        "mhpmevent4.SINH",
        1,
        {61, 61},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x2000000000000000ULL
    };
    field UINH = {
        "mhpmevent4.UINH",
        1,
        {60, 60},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x1000000000000000ULL
    };
    field VSINH = {
        "mhpmevent4.VSINH",
        1,
        {59, 59},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0800000000000000ULL
    };
    field VUINH = {
        "mhpmevent4.VUINH",
        1,
        {58, 58},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0400000000000000ULL
    };
    field RESERVED = {
        "mhpmevent4.RESERVED",
        2,
        {57, 56},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0300000000000000ULL
    };
    field MHPMEVENT4 = {
        "mhpmevent4.MHPMEVENT4",
        56,
        {55, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00FFFFFFFFFFFFFFULL
    };
};

struct mhpmevent5_csr : public csr_base {
    mhpmevent5_csr() : csr_base("mhpmevent5", 0x325, 64) {}

    field OF = {
        "mhpmevent5.OF",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field MINH = {
        "mhpmevent5.MINH",
        1,
        {62, 62},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x4000000000000000ULL
    };
    field SINH = {
        "mhpmevent5.SINH",
        1,
        {61, 61},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x2000000000000000ULL
    };
    field UINH = {
        "mhpmevent5.UINH",
        1,
        {60, 60},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x1000000000000000ULL
    };
    field VSINH = {
        "mhpmevent5.VSINH",
        1,
        {59, 59},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0800000000000000ULL
    };
    field VUINH = {
        "mhpmevent5.VUINH",
        1,
        {58, 58},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0400000000000000ULL
    };
    field RESERVED = {
        "mhpmevent5.RESERVED",
        2,
        {57, 56},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0300000000000000ULL
    };
    field MHPMEVENT5 = {
        "mhpmevent5.MHPMEVENT5",
        56,
        {55, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00FFFFFFFFFFFFFFULL
    };
};

struct mhpmevent6_csr : public csr_base {
    mhpmevent6_csr() : csr_base("mhpmevent6", 0x326, 64) {}

    field OF = {
        "mhpmevent6.OF",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field MINH = {
        "mhpmevent6.MINH",
        1,
        {62, 62},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x4000000000000000ULL
    };
    field SINH = {
        "mhpmevent6.SINH",
        1,
        {61, 61},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x2000000000000000ULL
    };
    field UINH = {
        "mhpmevent6.UINH",
        1,
        {60, 60},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x1000000000000000ULL
    };
    field VSINH = {
        "mhpmevent6.VSINH",
        1,
        {59, 59},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0800000000000000ULL
    };
    field VUINH = {
        "mhpmevent6.VUINH",
        1,
        {58, 58},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0400000000000000ULL
    };
    field RESERVED = {
        "mhpmevent6.RESERVED",
        2,
        {57, 56},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0300000000000000ULL
    };
    field MHPMEVENT6 = {
        "mhpmevent6.MHPMEVENT6",
        56,
        {55, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00FFFFFFFFFFFFFFULL
    };
};

struct mhpmevent7_csr : public csr_base {
    mhpmevent7_csr() : csr_base("mhpmevent7", 0x327, 64) {}

    field OF = {
        "mhpmevent7.OF",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field MINH = {
        "mhpmevent7.MINH",
        1,
        {62, 62},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x4000000000000000ULL
    };
    field SINH = {
        "mhpmevent7.SINH",
        1,
        {61, 61},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x2000000000000000ULL
    };
    field UINH = {
        "mhpmevent7.UINH",
        1,
        {60, 60},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x1000000000000000ULL
    };
    field VSINH = {
        "mhpmevent7.VSINH",
        1,
        {59, 59},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0800000000000000ULL
    };
    field VUINH = {
        "mhpmevent7.VUINH",
        1,
        {58, 58},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0400000000000000ULL
    };
    field RESERVED = {
        "mhpmevent7.RESERVED",
        2,
        {57, 56},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0300000000000000ULL
    };
    field MHPMEVENT7 = {
        "mhpmevent7.MHPMEVENT7",
        56,
        {55, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00FFFFFFFFFFFFFFULL
    };
};

struct mhpmevent8_csr : public csr_base {
    mhpmevent8_csr() : csr_base("mhpmevent8", 0x328, 64) {}

    field OF = {
        "mhpmevent8.OF",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field MINH = {
        "mhpmevent8.MINH",
        1,
        {62, 62},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x4000000000000000ULL
    };
    field SINH = {
        "mhpmevent8.SINH",
        1,
        {61, 61},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x2000000000000000ULL
    };
    field UINH = {
        "mhpmevent8.UINH",
        1,
        {60, 60},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x1000000000000000ULL
    };
    field VSINH = {
        "mhpmevent8.VSINH",
        1,
        {59, 59},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0800000000000000ULL
    };
    field VUINH = {
        "mhpmevent8.VUINH",
        1,
        {58, 58},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0400000000000000ULL
    };
    field RESERVED = {
        "mhpmevent8.RESERVED",
        2,
        {57, 56},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0300000000000000ULL
    };
    field MHPMEVENT8 = {
        "mhpmevent8.MHPMEVENT8",
        56,
        {55, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00FFFFFFFFFFFFFFULL
    };
};

struct mhpmevent9_csr : public csr_base {
    mhpmevent9_csr() : csr_base("mhpmevent9", 0x329, 64) {}

    field OF = {
        "mhpmevent9.OF",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field MINH = {
        "mhpmevent9.MINH",
        1,
        {62, 62},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x4000000000000000ULL
    };
    field SINH = {
        "mhpmevent9.SINH",
        1,
        {61, 61},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x2000000000000000ULL
    };
    field UINH = {
        "mhpmevent9.UINH",
        1,
        {60, 60},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x1000000000000000ULL
    };
    field VSINH = {
        "mhpmevent9.VSINH",
        1,
        {59, 59},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0800000000000000ULL
    };
    field VUINH = {
        "mhpmevent9.VUINH",
        1,
        {58, 58},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0400000000000000ULL
    };
    field RESERVED = {
        "mhpmevent9.RESERVED",
        2,
        {57, 56},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0300000000000000ULL
    };
    field MHPMEVENT9 = {
        "mhpmevent9.MHPMEVENT9",
        56,
        {55, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00FFFFFFFFFFFFFFULL
    };
};

struct mhpmevent10_csr : public csr_base {
    mhpmevent10_csr() : csr_base("mhpmevent10", 0x32A, 64) {}

    field OF = {
        "mhpmevent10.OF",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field MINH = {
        "mhpmevent10.MINH",
        1,
        {62, 62},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x4000000000000000ULL
    };
    field SINH = {
        "mhpmevent10.SINH",
        1,
        {61, 61},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x2000000000000000ULL
    };
    field UINH = {
        "mhpmevent10.UINH",
        1,
        {60, 60},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x1000000000000000ULL
    };
    field VSINH = {
        "mhpmevent10.VSINH",
        1,
        {59, 59},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0800000000000000ULL
    };
    field VUINH = {
        "mhpmevent10.VUINH",
        1,
        {58, 58},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0400000000000000ULL
    };
    field RESERVED = {
        "mhpmevent10.RESERVED",
        2,
        {57, 56},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0300000000000000ULL
    };
    field MHPMEVENT10 = {
        "mhpmevent10.MHPMEVENT10",
        56,
        {55, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00FFFFFFFFFFFFFFULL
    };
};

struct mhpmevent11_csr : public csr_base {
    mhpmevent11_csr() : csr_base("mhpmevent11", 0x32B, 64) {}

    field MHPMEVENT11 = {
        "mhpmevent11.MHPMEVENT11",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent12_csr : public csr_base {
    mhpmevent12_csr() : csr_base("mhpmevent12", 0x32C, 64) {}

    field MHPMEVENT12 = {
        "mhpmevent12.MHPMEVENT12",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent13_csr : public csr_base {
    mhpmevent13_csr() : csr_base("mhpmevent13", 0x32D, 64) {}

    field MHPMEVENT13 = {
        "mhpmevent13.MHPMEVENT13",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent14_csr : public csr_base {
    mhpmevent14_csr() : csr_base("mhpmevent14", 0x32E, 64) {}

    field MHPMEVENT14 = {
        "mhpmevent14.MHPMEVENT14",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent15_csr : public csr_base {
    mhpmevent15_csr() : csr_base("mhpmevent15", 0x32F, 64) {}

    field MHPMEVENT15 = {
        "mhpmevent15.MHPMEVENT15",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent16_csr : public csr_base {
    mhpmevent16_csr() : csr_base("mhpmevent16", 0x330, 64) {}

    field MHPMEVENT16 = {
        "mhpmevent16.MHPMEVENT16",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent17_csr : public csr_base {
    mhpmevent17_csr() : csr_base("mhpmevent17", 0x331, 64) {}

    field MHPMEVENT17 = {
        "mhpmevent17.MHPMEVENT17",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent18_csr : public csr_base {
    mhpmevent18_csr() : csr_base("mhpmevent18", 0x332, 64) {}

    field MHPMEVENT18 = {
        "mhpmevent18.MHPMEVENT18",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent19_csr : public csr_base {
    mhpmevent19_csr() : csr_base("mhpmevent19", 0x333, 64) {}

    field MHPMEVENT19 = {
        "mhpmevent19.MHPMEVENT19",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent20_csr : public csr_base {
    mhpmevent20_csr() : csr_base("mhpmevent20", 0x334, 64) {}

    field MHPMEVENT20 = {
        "mhpmevent20.MHPMEVENT20",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent21_csr : public csr_base {
    mhpmevent21_csr() : csr_base("mhpmevent21", 0x335, 64) {}

    field MHPMEVENT21 = {
        "mhpmevent21.MHPMEVENT21",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent22_csr : public csr_base {
    mhpmevent22_csr() : csr_base("mhpmevent22", 0x336, 64) {}

    field MHPMEVENT22 = {
        "mhpmevent22.MHPMEVENT22",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent23_csr : public csr_base {
    mhpmevent23_csr() : csr_base("mhpmevent23", 0x337, 64) {}

    field MHPMEVENT23 = {
        "mhpmevent23.MHPMEVENT23",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent24_csr : public csr_base {
    mhpmevent24_csr() : csr_base("mhpmevent24", 0x338, 64) {}

    field MHPMEVENT24 = {
        "mhpmevent24.MHPMEVENT24",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent25_csr : public csr_base {
    mhpmevent25_csr() : csr_base("mhpmevent25", 0x339, 64) {}

    field MHPMEVENT25 = {
        "mhpmevent25.MHPMEVENT25",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent26_csr : public csr_base {
    mhpmevent26_csr() : csr_base("mhpmevent26", 0x33A, 64) {}

    field MHPMEVENT26 = {
        "mhpmevent26.MHPMEVENT26",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent27_csr : public csr_base {
    mhpmevent27_csr() : csr_base("mhpmevent27", 0x33B, 64) {}

    field MHPMEVENT27 = {
        "mhpmevent27.MHPMEVENT27",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent28_csr : public csr_base {
    mhpmevent28_csr() : csr_base("mhpmevent28", 0x33C, 64) {}

    field MHPMEVENT28 = {
        "mhpmevent28.MHPMEVENT28",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent29_csr : public csr_base {
    mhpmevent29_csr() : csr_base("mhpmevent29", 0x33D, 64) {}

    field MHPMEVENT29 = {
        "mhpmevent29.MHPMEVENT29",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent30_csr : public csr_base {
    mhpmevent30_csr() : csr_base("mhpmevent30", 0x33E, 64) {}

    field MHPMEVENT30 = {
        "mhpmevent30.MHPMEVENT30",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mhpmevent31_csr : public csr_base {
    mhpmevent31_csr() : csr_base("mhpmevent31", 0x33F, 64) {}

    field MHPMEVENT31 = {
        "mhpmevent31.MHPMEVENT31",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mcounteren_csr : public csr_base {
    mcounteren_csr() : csr_base("mcounteren", 0x306, 32) {}

    field HPM31 = {
        "mcounteren.HPM31",
        1,
        {31, 31},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000080000000ULL
    };
    field HPM30 = {
        "mcounteren.HPM30",
        1,
        {30, 30},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000040000000ULL
    };
    field HPM29 = {
        "mcounteren.HPM29",
        1,
        {29, 29},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000020000000ULL
    };
    field HPM28 = {
        "mcounteren.HPM28",
        1,
        {28, 28},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000010000000ULL
    };
    field HPM27 = {
        "mcounteren.HPM27",
        1,
        {27, 27},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000008000000ULL
    };
    field HPM26 = {
        "mcounteren.HPM26",
        1,
        {26, 26},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000004000000ULL
    };
    field HPM25 = {
        "mcounteren.HPM25",
        1,
        {25, 25},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000002000000ULL
    };
    field HPM24 = {
        "mcounteren.HPM24",
        1,
        {24, 24},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000001000000ULL
    };
    field HPM23 = {
        "mcounteren.HPM23",
        1,
        {23, 23},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000800000ULL
    };
    field HPM22 = {
        "mcounteren.HPM22",
        1,
        {22, 22},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000400000ULL
    };
    field HPM21 = {
        "mcounteren.HPM21",
        1,
        {21, 21},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000200000ULL
    };
    field HPM20 = {
        "mcounteren.HPM20",
        1,
        {20, 20},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000100000ULL
    };
    field HPM19 = {
        "mcounteren.HPM19",
        1,
        {19, 19},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000080000ULL
    };
    field HPM18 = {
        "mcounteren.HPM18",
        1,
        {18, 18},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000040000ULL
    };
    field HPM17 = {
        "mcounteren.HPM17",
        1,
        {17, 17},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000020000ULL
    };
    field HPM16 = {
        "mcounteren.HPM16",
        1,
        {16, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000010000ULL
    };
    field HPM15 = {
        "mcounteren.HPM15",
        1,
        {15, 15},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000008000ULL
    };
    field HPM14 = {
        "mcounteren.HPM14",
        1,
        {14, 14},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000004000ULL
    };
    field HPM13 = {
        "mcounteren.HPM13",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field HPM12 = {
        "mcounteren.HPM12",
        1,
        {12, 12},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000001000ULL
    };
    field HPM11 = {
        "mcounteren.HPM11",
        1,
        {11, 11},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000800ULL
    };
    field HPM10 = {
        "mcounteren.HPM10",
        1,
        {10, 10},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000400ULL
    };
    field HPM9 = {
        "mcounteren.HPM9",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field HPM8 = {
        "mcounteren.HPM8",
        1,
        {8, 8},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000100ULL
    };
    field HPM7 = {
        "mcounteren.HPM7",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field HPM6 = {
        "mcounteren.HPM6",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field HPM5 = {
        "mcounteren.HPM5",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field HPM4 = {
        "mcounteren.HPM4",
        1,
        {4, 4},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000010ULL
    };
    field HPM3 = {
        "mcounteren.HPM3",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field IR = {
        "mcounteren.IR",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field TM = {
        "mcounteren.TM",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field CY = {
        "mcounteren.CY",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct mcountinhibit_csr : public csr_base {
    mcountinhibit_csr() : csr_base("mcountinhibit", 0x320, 32) {}

    field HPM31 = {
        "mcountinhibit.HPM31",
        1,
        {31, 31},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000080000000ULL
    };
    field HPM30 = {
        "mcountinhibit.HPM30",
        1,
        {30, 30},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000040000000ULL
    };
    field HPM29 = {
        "mcountinhibit.HPM29",
        1,
        {29, 29},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000020000000ULL
    };
    field HPM28 = {
        "mcountinhibit.HPM28",
        1,
        {28, 28},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000010000000ULL
    };
    field HPM27 = {
        "mcountinhibit.HPM27",
        1,
        {27, 27},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000008000000ULL
    };
    field HPM26 = {
        "mcountinhibit.HPM26",
        1,
        {26, 26},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000004000000ULL
    };
    field HPM25 = {
        "mcountinhibit.HPM25",
        1,
        {25, 25},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000002000000ULL
    };
    field HPM24 = {
        "mcountinhibit.HPM24",
        1,
        {24, 24},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000001000000ULL
    };
    field HPM23 = {
        "mcountinhibit.HPM23",
        1,
        {23, 23},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000800000ULL
    };
    field HPM22 = {
        "mcountinhibit.HPM22",
        1,
        {22, 22},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000400000ULL
    };
    field HPM21 = {
        "mcountinhibit.HPM21",
        1,
        {21, 21},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000200000ULL
    };
    field HPM20 = {
        "mcountinhibit.HPM20",
        1,
        {20, 20},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000100000ULL
    };
    field HPM19 = {
        "mcountinhibit.HPM19",
        1,
        {19, 19},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000080000ULL
    };
    field HPM18 = {
        "mcountinhibit.HPM18",
        1,
        {18, 18},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000040000ULL
    };
    field HPM17 = {
        "mcountinhibit.HPM17",
        1,
        {17, 17},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000020000ULL
    };
    field HPM16 = {
        "mcountinhibit.HPM16",
        1,
        {16, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000010000ULL
    };
    field HPM15 = {
        "mcountinhibit.HPM15",
        1,
        {15, 15},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000008000ULL
    };
    field HPM14 = {
        "mcountinhibit.HPM14",
        1,
        {14, 14},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000004000ULL
    };
    field HPM13 = {
        "mcountinhibit.HPM13",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field HPM12 = {
        "mcountinhibit.HPM12",
        1,
        {12, 12},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000001000ULL
    };
    field HPM11 = {
        "mcountinhibit.HPM11",
        1,
        {11, 11},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000800ULL
    };
    field HPM10 = {
        "mcountinhibit.HPM10",
        1,
        {10, 10},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000400ULL
    };
    field HPM9 = {
        "mcountinhibit.HPM9",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field HPM8 = {
        "mcountinhibit.HPM8",
        1,
        {8, 8},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000100ULL
    };
    field HPM7 = {
        "mcountinhibit.HPM7",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field HPM6 = {
        "mcountinhibit.HPM6",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field HPM5 = {
        "mcountinhibit.HPM5",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field HPM4 = {
        "mcountinhibit.HPM4",
        1,
        {4, 4},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000010ULL
    };
    field HPM3 = {
        "mcountinhibit.HPM3",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field IR = {
        "mcountinhibit.IR",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field HARD0 = {
        "mcountinhibit.HARD0",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field CY = {
        "mcountinhibit.CY",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct miselect_csr : public csr_base {
    miselect_csr() : csr_base("miselect", 0x350, 64) {}

    field RSVD_63_8 = {
        "miselect.RSVD_63_8",
        56,
        {63, 8},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFF00ULL
    };
    field INTERRUPTS = {
        "miselect.INTERRUPTS",
        8,
        {7, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000000FFULL
    };
};

struct mireg_csr : public csr_base {
    mireg_csr() : csr_base("mireg", 0x351, 64) {}

    field MIREG = {
        "mireg.MIREG",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mtopei_csr : public csr_base {
    mtopei_csr() : csr_base("mtopei", 0x35C, 64) {}

    field RSVD_63_27 = {
        "mtopei.RSVD_63_27",
        37,
        {63, 27},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFF8000000ULL
    };
    field IDENTITY = {
        "mtopei.IDENTITY",
        11,
        {26, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000007FF0000ULL
    };
    field RSVD_15_11 = {
        "mtopei.RSVD_15_11",
        5,
        {15, 11},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000000F800ULL
    };
    field PRIORITY = {
        "mtopei.PRIORITY",
        11,
        {10, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000007FFULL
    };
};

struct mtopi_csr : public csr_base {
    mtopi_csr() : csr_base("mtopi", 0xFB0, 64) {}

    field RSVD_63_28 = {
        "mtopi.RSVD_63_28",
        36,
        {63, 28},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFF0000000ULL
    };
    field IID = {
        "mtopi.IID",
        12,
        {27, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x000000000FFF0000ULL
    };
    field RSVD_15_8 = {
        "mtopi.RSVD_15_8",
        8,
        {15, 8},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000000FF00ULL
    };
    field IPRIO = {
        "mtopi.IPRIO",
        8,
        {7, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000000FFULL
    };
};

struct mvien_csr : public csr_base {
    mvien_csr() : csr_base("mvien", 0x308, 64) {}

    field LCOFIP = {
        "mvien.LCOFIP",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field HARD0_2 = {
        "mvien.HARD0_2",
        3,
        {12, 10},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000001C00ULL
    };
    field SEIP = {
        "mvien.SEIP",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field HARD0_1 = {
        "mvien.HARD0_1",
        7,
        {8, 2},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x00000000000001FCULL
    };
    field SSIP = {
        "mvien.SSIP",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field HARD0_0 = {
        "mvien.HARD0_0",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct mvip_csr : public csr_base {
    mvip_csr() : csr_base("mvip", 0x309, 64) {}

    field LCOFIP = {
        "mvip.LCOFIP",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field HARD0_3 = {
        "mvip.HARD0_3",
        3,
        {12, 10},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000001C00ULL
    };
    field SEIP = {
        "mvip.SEIP",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field HARD0_2 = {
        "mvip.HARD0_2",
        3,
        {8, 6},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x00000000000001C0ULL
    };
    field STIP = {
        "mvip.STIP",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field HARD0_1 = {
        "mvip.HARD0_1",
        3,
        {4, 2},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000000001CULL
    };
    field SSIP = {
        "mvip.SSIP",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field HARD0_0 = {
        "mvip.HARD0_0",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct sstatus_csr : public csr_base {
    sstatus_csr() : csr_base("sstatus", 0x100, 64) {}

    field SD = {
        "sstatus.SD",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field SSTATUS_WPRI_6 = {
        "sstatus.SSTATUS_WPRI_6",
        29,
        {62, 34},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x7FFFFFFC00000000ULL
    };
    field UXL = {
        "sstatus.UXL",
        2,
        {33, 32},
        0x0000000000000002ULL,
        {0x0000000000000002ULL},
        "WARL",
        "",
        0x0000000300000000ULL
    };
    field SSTATUS_WPRI_5 = {
        "sstatus.SSTATUS_WPRI_5",
        12,
        {31, 20},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x00000000FFF00000ULL
    };
    field MXR = {
        "sstatus.MXR",
        1,
        {19, 19},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000080000ULL
    };
    field SUM = {
        "sstatus.SUM",
        1,
        {18, 18},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000040000ULL
    };
    field SSTATUS_WPRI_4 = {
        "sstatus.SSTATUS_WPRI_4",
        1,
        {17, 17},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x0000000000020000ULL
    };
    field XS = {
        "sstatus.XS",
        2,
        {16, 15},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000018000ULL
    };
    field FS = {
        "sstatus.FS",
        2,
        {14, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000006000ULL
    };
    field SSTATUS_WPRI_3 = {
        "sstatus.SSTATUS_WPRI_3",
        2,
        {12, 11},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x0000000000001800ULL
    };
    field VS = {
        "sstatus.VS",
        2,
        {10, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000600ULL
    };
    field SPP = {
        "sstatus.SPP",
        1,
        {8, 8},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000100ULL
    };
    field SSTATUS_WPRI_2 = {
        "sstatus.SSTATUS_WPRI_2",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x0000000000000080ULL
    };
    field UBE = {
        "sstatus.UBE",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field SPIE = {
        "sstatus.SPIE",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field SSTATUS_WPRI_1 = {
        "sstatus.SSTATUS_WPRI_1",
        3,
        {4, 2},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x000000000000001CULL
    };
    field SIE = {
        "sstatus.SIE",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field SSTATUS_WPRI_0 = {
        "sstatus.SSTATUS_WPRI_0",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x0000000000000001ULL
    };
};

struct stvec_csr : public csr_base {
    stvec_csr() : csr_base("stvec", 0x105, 64) {}

    field BASESXLEN12WARL = {
        "stvec.BASESXLEN12WARL",
        62,
        {63, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFCULL
    };
    field MODE_1 = {
        "stvec.MODE_1",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field MODE_0 = {
        "stvec.MODE_0",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct sip_csr : public csr_base {
    sip_csr() : csr_base("sip", 0x144, 64) {}

    field LCOFIP = {
        "sip.LCOFIP",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field HARD0_3 = {
        "sip.HARD0_3",
        3,
        {12, 10},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000001C00ULL
    };
    field SEIP = {
        "sip.SEIP",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field HARD0_2 = {
        "sip.HARD0_2",
        2,
        {7, 6},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x00000000000000C0ULL
    };
    field STIP = {
        "sip.STIP",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field HARD0_1 = {
        "sip.HARD0_1",
        2,
        {3, 2},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000000000CULL
    };
    field SSIP = {
        "sip.SSIP",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
};

struct sie_csr : public csr_base {
    sie_csr() : csr_base("sie", 0x104, 64) {}

    field LCOFIE = {
        "sie.LCOFIE",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field HARD0_2 = {
        "sie.HARD0_2",
        3,
        {12, 10},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000001C00ULL
    };
    field SEIE = {
        "sie.SEIE",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field HARD0_1 = {
        "sie.HARD0_1",
        2,
        {7, 6},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x00000000000000C0ULL
    };
    field STIE = {
        "sie.STIE",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field HARD0_0 = {
        "sie.HARD0_0",
        2,
        {3, 2},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000000000CULL
    };
    field SSIE = {
        "sie.SSIE",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
};

struct scounteren_csr : public csr_base {
    scounteren_csr() : csr_base("scounteren", 0x106, 32) {}

    field HPM31 = {
        "scounteren.HPM31",
        1,
        {31, 31},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000080000000ULL
    };
    field HPM30 = {
        "scounteren.HPM30",
        1,
        {30, 30},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000040000000ULL
    };
    field HPM29 = {
        "scounteren.HPM29",
        1,
        {29, 29},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000020000000ULL
    };
    field HPM28 = {
        "scounteren.HPM28",
        1,
        {28, 28},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000010000000ULL
    };
    field HPM27 = {
        "scounteren.HPM27",
        1,
        {27, 27},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000008000000ULL
    };
    field HPM26 = {
        "scounteren.HPM26",
        1,
        {26, 26},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000004000000ULL
    };
    field HPM25 = {
        "scounteren.HPM25",
        1,
        {25, 25},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000002000000ULL
    };
    field HPM24 = {
        "scounteren.HPM24",
        1,
        {24, 24},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000001000000ULL
    };
    field HPM23 = {
        "scounteren.HPM23",
        1,
        {23, 23},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000800000ULL
    };
    field HPM22 = {
        "scounteren.HPM22",
        1,
        {22, 22},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000400000ULL
    };
    field HPM21 = {
        "scounteren.HPM21",
        1,
        {21, 21},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000200000ULL
    };
    field HPM20 = {
        "scounteren.HPM20",
        1,
        {20, 20},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000100000ULL
    };
    field HPM19 = {
        "scounteren.HPM19",
        1,
        {19, 19},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000080000ULL
    };
    field HPM18 = {
        "scounteren.HPM18",
        1,
        {18, 18},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000040000ULL
    };
    field HPM17 = {
        "scounteren.HPM17",
        1,
        {17, 17},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000020000ULL
    };
    field HPM16 = {
        "scounteren.HPM16",
        1,
        {16, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000010000ULL
    };
    field HPM15 = {
        "scounteren.HPM15",
        1,
        {15, 15},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000008000ULL
    };
    field HPM14 = {
        "scounteren.HPM14",
        1,
        {14, 14},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000004000ULL
    };
    field HPM13 = {
        "scounteren.HPM13",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field HPM12 = {
        "scounteren.HPM12",
        1,
        {12, 12},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000001000ULL
    };
    field HPM11 = {
        "scounteren.HPM11",
        1,
        {11, 11},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000800ULL
    };
    field HPM10 = {
        "scounteren.HPM10",
        1,
        {10, 10},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000400ULL
    };
    field HPM9 = {
        "scounteren.HPM9",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field HPM8 = {
        "scounteren.HPM8",
        1,
        {8, 8},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000100ULL
    };
    field HPM7 = {
        "scounteren.HPM7",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field HPM6 = {
        "scounteren.HPM6",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field HPM5 = {
        "scounteren.HPM5",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field HPM4 = {
        "scounteren.HPM4",
        1,
        {4, 4},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000010ULL
    };
    field HPM3 = {
        "scounteren.HPM3",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field IR = {
        "scounteren.IR",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field TM = {
        "scounteren.TM",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field CY = {
        "scounteren.CY",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct sscratch_csr : public csr_base {
    sscratch_csr() : csr_base("sscratch", 0x140, 64) {}

    field SSCRATCH = {
        "sscratch.SSCRATCH",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct sepc_csr : public csr_base {
    sepc_csr() : csr_base("sepc", 0x141, 64) {}

    field ADDR = {
        "sepc.ADDR",
        63,
        {63, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFEULL
    };
};

struct scause_csr : public csr_base {
    scause_csr() : csr_base("scause", 0x142, 64) {}

    field INTERRUPT = {
        "scause.INTERRUPT",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field EXCEPTIONCODEWLRL = {
        "scause.EXCEPTIONCODEWLRL",
        63,
        {62, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x7FFFFFFFFFFFFFFFULL
    };
};

struct stval_csr : public csr_base {
    stval_csr() : csr_base("stval", 0x143, 64) {}

    field STVAL = {
        "stval.STVAL",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct stimecmp_csr : public csr_base {
    stimecmp_csr() : csr_base("stimecmp", 0x14D, 64) {}

    field STIMECMP = {
        "stimecmp.STIMECMP",
        64,
        {63, 0},
        0x00000000FFFFFFFFULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct senvcfg_csr : public csr_base {
    senvcfg_csr() : csr_base("senvcfg", 0x10A, 64) {}

    field WPRI_2 = {
        "senvcfg.WPRI_2",
        30,
        {63, 34},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0xFFFFFFFC00000000ULL
    };
    field PMM = {
        "senvcfg.PMM",
        2,
        {33, 32},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000002ULL},
        "WARL",
        "",
        0x0000000300000000ULL
    };
    field WPRI_1 = {
        "senvcfg.WPRI_1",
        24,
        {31, 8},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x00000000FFFFFF00ULL
    };
    field CBZE = {
        "senvcfg.CBZE",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field CBCFE = {
        "senvcfg.CBCFE",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field CBIE = {
        "senvcfg.CBIE",
        2,
        {5, 4},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000030ULL
    };
    field WPRI_0 = {
        "senvcfg.WPRI_0",
        3,
        {3, 1},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x000000000000000EULL
    };
    field FIOM = {
        "senvcfg.FIOM",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct satp_csr : public csr_base {
    satp_csr() : csr_base("satp", 0x180, 64) {}

    field MODE = {
        "satp.MODE",
        4,
        {63, 60},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xF000000000000000ULL
    };
    field ASID = {
        "satp.ASID",
        16,
        {59, 44},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0FFFF00000000000ULL
    };
    field PPN = {
        "satp.PPN",
        44,
        {43, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000FFFFFFFFFFFULL
    };
};

struct srmcfg_csr : public csr_base {
    srmcfg_csr() : csr_base("srmcfg", 0x181, 64) {}

    field MCID = {
        "srmcfg.MCID",
        12,
        {27, 16},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000FFF0000ULL
    };
    field RCID = {
        "srmcfg.RCID",
        12,
        {11, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000FFFULL
    };
};

struct siselect_csr : public csr_base {
    siselect_csr() : csr_base("siselect", 0x150, 64) {}

    field RSVD_63_9 = {
        "siselect.RSVD_63_9",
        55,
        {63, 9},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFE00ULL
    };
    field INTERRUPTS = {
        "siselect.INTERRUPTS",
        9,
        {8, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct sireg_csr : public csr_base {
    sireg_csr() : csr_base("sireg", 0x151, 64) {}

    field SIREG = {
        "sireg.SIREG",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct stopei_csr : public csr_base {
    stopei_csr() : csr_base("stopei", 0x15C, 64) {}

    field RSVD_63_27 = {
        "stopei.RSVD_63_27",
        37,
        {63, 27},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFF8000000ULL
    };
    field IDENTITY = {
        "stopei.IDENTITY",
        11,
        {26, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000007FF0000ULL
    };
    field RSVD_15_11 = {
        "stopei.RSVD_15_11",
        5,
        {15, 11},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000000F800ULL
    };
    field PRIORITY = {
        "stopei.PRIORITY",
        11,
        {10, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000007FFULL
    };
};

struct stopi_csr : public csr_base {
    stopi_csr() : csr_base("stopi", 0xDB0, 64) {}

    field RSVD_63_28 = {
        "stopi.RSVD_63_28",
        36,
        {63, 28},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFF0000000ULL
    };
    field IID = {
        "stopi.IID",
        12,
        {27, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x000000000FFF0000ULL
    };
    field RSVD_15_8 = {
        "stopi.RSVD_15_8",
        8,
        {15, 8},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000000FF00ULL
    };
    field IPRIO = {
        "stopi.IPRIO",
        8,
        {7, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000000FFULL
    };
};

struct seed_csr : public csr_base {
    seed_csr() : csr_base("seed", 0x015, 32) {}

    field OPST = {
        "seed.OPST",
        2,
        {31, 30},
        0x0000000000000001ULL,
        {},
        "WARL",
        "",
        0x00000000C0000000ULL
    };
    field RSVD_29_24 = {
        "seed.RSVD_29_24",
        6,
        {29, 24},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x000000003F000000ULL
    };
    field CUSTOM = {
        "seed.CUSTOM",
        8,
        {23, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000FF0000ULL
    };
    field ENTROPY = {
        "seed.ENTROPY",
        16,
        {15, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x000000000000FFFFULL
    };
};

struct fflags_csr : public csr_base {
    fflags_csr() : csr_base("fflags", 0x001, 5) {}

    field NV = {
        "fflags.NV",
        1,
        {4, 4},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000010ULL
    };
    field DZ = {
        "fflags.DZ",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field OF = {
        "fflags.OF",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field UF = {
        "fflags.UF",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field NX = {
        "fflags.NX",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct frm_csr : public csr_base {
    frm_csr() : csr_base("frm", 0x002, 3) {}

    field FRM = {
        "frm.FRM",
        3,
        {2, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000007ULL
    };
};

struct fcsr_csr : public csr_base {
    fcsr_csr() : csr_base("fcsr", 0x003, 32) {}

    field RESERVED = {
        "fcsr.RESERVED",
        24,
        {31, 8},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x00000000FFFFFF00ULL
    };
    field FRM = {
        "fcsr.FRM",
        3,
        {7, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000000E0ULL
    };
    field NV = {
        "fcsr.NV",
        1,
        {4, 4},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000010ULL
    };
    field DZ = {
        "fcsr.DZ",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field OF = {
        "fcsr.OF",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field UF = {
        "fcsr.UF",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field NX = {
        "fcsr.NX",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct vstart_csr : public csr_base {
    vstart_csr() : csr_base("vstart", 0x008, 64) {}

    field VSTART = {
        "vstart.VSTART",
        8,
        {7, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000000FFULL
    };
};

struct vxsat_csr : public csr_base {
    vxsat_csr() : csr_base("vxsat", 0x009, 1) {}

    field VXSAT = {
        "vxsat.VXSAT",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct vxrm_csr : public csr_base {
    vxrm_csr() : csr_base("vxrm", 0x00A, 2) {}

    field VXRM = {
        "vxrm.VXRM",
        2,
        {1, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000003ULL
    };
};

struct vcsr_csr : public csr_base {
    vcsr_csr() : csr_base("vcsr", 0x00F, 3) {}

    field VXRM = {
        "vcsr.VXRM",
        2,
        {2, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000006ULL
    };
    field VXSAT = {
        "vcsr.VXSAT",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct vl_csr : public csr_base {
    vl_csr() : csr_base("vl", 0xC20, 64) {}

    field VL = {
        "vl.VL",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct vtype_csr : public csr_base {
    vtype_csr() : csr_base("vtype", 0xC21, 64) {}

    field VILL = {
        "vtype.VILL",
        1,
        {63, 63},
        0x0000000000000001ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field RESERVED = {
        "vtype.RESERVED",
        55,
        {62, 8},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x7FFFFFFFFFFFFF00ULL
    };
    field VMA = {
        "vtype.VMA",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field VTA = {
        "vtype.VTA",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field VSEW = {
        "vtype.VSEW",
        3,
        {5, 3},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000038ULL
    };
    field VLMUL = {
        "vtype.VLMUL",
        3,
        {2, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000007ULL
    };
};

struct vlenb_csr : public csr_base {
    vlenb_csr() : csr_base("vlenb", 0xC22, 64) {}

    field VLENB = {
        "vlenb.VLENB",
        64,
        {63, 0},
        0x0000000000000020ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct pmpcfg0_csr : public csr_base {
    pmpcfg0_csr() : csr_base("pmpcfg0", 0x3A0, 64) {}

    field PMP7CFG_LOCKED = {
        "pmpcfg0.PMP7CFG_LOCKED",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field PMP7CFG_RSVD = {
        "pmpcfg0.PMP7CFG_RSVD",
        2,
        {62, 61},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x6000000000000000ULL
    };
    field PMP7CFG_MODE = {
        "pmpcfg0.PMP7CFG_MODE",
        2,
        {60, 59},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x1800000000000000ULL
    };
    field PMP7CFG_RWX = {
        "pmpcfg0.PMP7CFG_RWX",
        3,
        {58, 56},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0700000000000000ULL
    };
    field PMP6CFG_LOCKED = {
        "pmpcfg0.PMP6CFG_LOCKED",
        1,
        {55, 55},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0080000000000000ULL
    };
    field PMP6CFG_RSVD = {
        "pmpcfg0.PMP6CFG_RSVD",
        2,
        {54, 53},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0060000000000000ULL
    };
    field PMP6CFG_MODE = {
        "pmpcfg0.PMP6CFG_MODE",
        2,
        {52, 51},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0018000000000000ULL
    };
    field PMP6CFG_RWX = {
        "pmpcfg0.PMP6CFG_RWX",
        3,
        {50, 48},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0007000000000000ULL
    };
    field PMP5CFG_LOCKED = {
        "pmpcfg0.PMP5CFG_LOCKED",
        1,
        {47, 47},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000800000000000ULL
    };
    field PMP5CFG_RSVD = {
        "pmpcfg0.PMP5CFG_RSVD",
        2,
        {46, 45},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000600000000000ULL
    };
    field PMP5CFG_MODE = {
        "pmpcfg0.PMP5CFG_MODE",
        2,
        {44, 43},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0000180000000000ULL
    };
    field PMP5CFG_RWX = {
        "pmpcfg0.PMP5CFG_RWX",
        3,
        {42, 40},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0000070000000000ULL
    };
    field PMP4CFG_LOCKED = {
        "pmpcfg0.PMP4CFG_LOCKED",
        1,
        {39, 39},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000008000000000ULL
    };
    field PMP4CFG_RSVD = {
        "pmpcfg0.PMP4CFG_RSVD",
        2,
        {38, 37},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000006000000000ULL
    };
    field PMP4CFG_MODE = {
        "pmpcfg0.PMP4CFG_MODE",
        2,
        {36, 35},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0000001800000000ULL
    };
    field PMP4CFG_RWX = {
        "pmpcfg0.PMP4CFG_RWX",
        3,
        {34, 32},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0000000700000000ULL
    };
    field PMP3CFG_LOCKED = {
        "pmpcfg0.PMP3CFG_LOCKED",
        1,
        {31, 31},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000080000000ULL
    };
    field PMP3CFG_RSVD = {
        "pmpcfg0.PMP3CFG_RSVD",
        2,
        {30, 29},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000060000000ULL
    };
    field PMP3CFG_MODE = {
        "pmpcfg0.PMP3CFG_MODE",
        2,
        {28, 27},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0000000018000000ULL
    };
    field PMP3CFG_RWX = {
        "pmpcfg0.PMP3CFG_RWX",
        3,
        {26, 24},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0000000007000000ULL
    };
    field PMP2CFG_LOCKED = {
        "pmpcfg0.PMP2CFG_LOCKED",
        1,
        {23, 23},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000800000ULL
    };
    field PMP2CFG_RSVD = {
        "pmpcfg0.PMP2CFG_RSVD",
        2,
        {22, 21},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000600000ULL
    };
    field PMP2CFG_MODE = {
        "pmpcfg0.PMP2CFG_MODE",
        2,
        {20, 19},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0000000000180000ULL
    };
    field PMP2CFG_RWX = {
        "pmpcfg0.PMP2CFG_RWX",
        3,
        {18, 16},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0000000000070000ULL
    };
    field PMP1CFG_LOCKED = {
        "pmpcfg0.PMP1CFG_LOCKED",
        1,
        {15, 15},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000008000ULL
    };
    field PMP1CFG_RSVD = {
        "pmpcfg0.PMP1CFG_RSVD",
        2,
        {14, 13},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000006000ULL
    };
    field PMP1CFG_MODE = {
        "pmpcfg0.PMP1CFG_MODE",
        2,
        {12, 11},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0000000000001800ULL
    };
    field PMP1CFG_RWX = {
        "pmpcfg0.PMP1CFG_RWX",
        3,
        {10, 8},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0000000000000700ULL
    };
    field PMP0CFG_LOCKED = {
        "pmpcfg0.PMP0CFG_LOCKED",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field PMP0CFG_RSVD = {
        "pmpcfg0.PMP0CFG_RSVD",
        2,
        {6, 5},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000060ULL
    };
    field PMP0CFG_MODE = {
        "pmpcfg0.PMP0CFG_MODE",
        2,
        {4, 3},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0000000000000018ULL
    };
    field PMP0CFG_RWX = {
        "pmpcfg0.PMP0CFG_RWX",
        3,
        {2, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0000000000000007ULL
    };
};

struct pmpcfg2_csr : public csr_base {
    pmpcfg2_csr() : csr_base("pmpcfg2", 0x3A2, 64) {}

    field PMP15CFG_LOCKED = {
        "pmpcfg2.PMP15CFG_LOCKED",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field PMP15CFG_RSVD = {
        "pmpcfg2.PMP15CFG_RSVD",
        2,
        {62, 61},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x6000000000000000ULL
    };
    field PMP15CFG_MODE = {
        "pmpcfg2.PMP15CFG_MODE",
        2,
        {60, 59},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x1800000000000000ULL
    };
    field PMP15CFG_RWX = {
        "pmpcfg2.PMP15CFG_RWX",
        3,
        {58, 56},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0700000000000000ULL
    };
    field PMP14CFG_LOCKED = {
        "pmpcfg2.PMP14CFG_LOCKED",
        1,
        {55, 55},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0080000000000000ULL
    };
    field PMP14CFG_RSVD = {
        "pmpcfg2.PMP14CFG_RSVD",
        2,
        {54, 53},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0060000000000000ULL
    };
    field PMP14CFG_MODE = {
        "pmpcfg2.PMP14CFG_MODE",
        2,
        {52, 51},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0018000000000000ULL
    };
    field PMP14CFG_RWX = {
        "pmpcfg2.PMP14CFG_RWX",
        3,
        {50, 48},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0007000000000000ULL
    };
    field PMP13CFG_LOCKED = {
        "pmpcfg2.PMP13CFG_LOCKED",
        1,
        {47, 47},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000800000000000ULL
    };
    field PMP13CFG_RSVD = {
        "pmpcfg2.PMP13CFG_RSVD",
        2,
        {46, 45},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000600000000000ULL
    };
    field PMP13CFG_MODE = {
        "pmpcfg2.PMP13CFG_MODE",
        2,
        {44, 43},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0000180000000000ULL
    };
    field PMP13CFG_RWX = {
        "pmpcfg2.PMP13CFG_RWX",
        3,
        {42, 40},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0000070000000000ULL
    };
    field PMP12CFG_LOCKED = {
        "pmpcfg2.PMP12CFG_LOCKED",
        1,
        {39, 39},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000008000000000ULL
    };
    field PMP12CFG_RSVD = {
        "pmpcfg2.PMP12CFG_RSVD",
        2,
        {38, 37},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000006000000000ULL
    };
    field PMP12CFG_MODE = {
        "pmpcfg2.PMP12CFG_MODE",
        2,
        {36, 35},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0000001800000000ULL
    };
    field PMP12CFG_RWX = {
        "pmpcfg2.PMP12CFG_RWX",
        3,
        {34, 32},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0000000700000000ULL
    };
    field PMP11CFG_LOCKED = {
        "pmpcfg2.PMP11CFG_LOCKED",
        1,
        {31, 31},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000080000000ULL
    };
    field PMP11CFG_RSVD = {
        "pmpcfg2.PMP11CFG_RSVD",
        2,
        {30, 29},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000060000000ULL
    };
    field PMP11CFG_MODE = {
        "pmpcfg2.PMP11CFG_MODE",
        2,
        {28, 27},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0000000018000000ULL
    };
    field PMP11CFG_RWX = {
        "pmpcfg2.PMP11CFG_RWX",
        3,
        {26, 24},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0000000007000000ULL
    };
    field PMP10CFG_LOCKED = {
        "pmpcfg2.PMP10CFG_LOCKED",
        1,
        {23, 23},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000800000ULL
    };
    field PMP10CFG_RSVD = {
        "pmpcfg2.PMP10CFG_RSVD",
        2,
        {22, 21},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000600000ULL
    };
    field PMP10CFG_MODE = {
        "pmpcfg2.PMP10CFG_MODE",
        2,
        {20, 19},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0000000000180000ULL
    };
    field PMP10CFG_RWX = {
        "pmpcfg2.PMP10CFG_RWX",
        3,
        {18, 16},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0000000000070000ULL
    };
    field PMP9CFG_LOCKED = {
        "pmpcfg2.PMP9CFG_LOCKED",
        1,
        {15, 15},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000008000ULL
    };
    field PMP9CFG_RSVD = {
        "pmpcfg2.PMP9CFG_RSVD",
        2,
        {14, 13},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000006000ULL
    };
    field PMP9CFG_MODE = {
        "pmpcfg2.PMP9CFG_MODE",
        2,
        {12, 11},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0000000000001800ULL
    };
    field PMP9CFG_RWX = {
        "pmpcfg2.PMP9CFG_RWX",
        3,
        {10, 8},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0000000000000700ULL
    };
    field PMP8CFG_LOCKED = {
        "pmpcfg2.PMP8CFG_LOCKED",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field PMP8CFG_RSVD = {
        "pmpcfg2.PMP8CFG_RSVD",
        2,
        {6, 5},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000060ULL
    };
    field PMP8CFG_MODE = {
        "pmpcfg2.PMP8CFG_MODE",
        2,
        {4, 3},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0000000000000018ULL
    };
    field PMP8CFG_RWX = {
        "pmpcfg2.PMP8CFG_RWX",
        3,
        {2, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000004ULL, 0x0000000000000005ULL, 0x0000000000000007ULL},
        "WARL",
        "",
        0x0000000000000007ULL
    };
};

struct pmpaddr0_csr : public csr_base {
    pmpaddr0_csr() : csr_base("pmpaddr0", 0x3B0, 64) {}

    field ADDRESS_HI = {
        "pmpaddr0.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr0.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr1_csr : public csr_base {
    pmpaddr1_csr() : csr_base("pmpaddr1", 0x3B1, 64) {}

    field ADDRESS_HI = {
        "pmpaddr1.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr1.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr2_csr : public csr_base {
    pmpaddr2_csr() : csr_base("pmpaddr2", 0x3B2, 64) {}

    field ADDRESS_HI = {
        "pmpaddr2.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr2.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr3_csr : public csr_base {
    pmpaddr3_csr() : csr_base("pmpaddr3", 0x3B3, 64) {}

    field ADDRESS_HI = {
        "pmpaddr3.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr3.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr4_csr : public csr_base {
    pmpaddr4_csr() : csr_base("pmpaddr4", 0x3B4, 64) {}

    field ADDRESS_HI = {
        "pmpaddr4.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr4.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr5_csr : public csr_base {
    pmpaddr5_csr() : csr_base("pmpaddr5", 0x3B5, 64) {}

    field ADDRESS_HI = {
        "pmpaddr5.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr5.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr6_csr : public csr_base {
    pmpaddr6_csr() : csr_base("pmpaddr6", 0x3B6, 64) {}

    field ADDRESS_HI = {
        "pmpaddr6.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr6.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr7_csr : public csr_base {
    pmpaddr7_csr() : csr_base("pmpaddr7", 0x3B7, 64) {}

    field ADDRESS_HI = {
        "pmpaddr7.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr7.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr8_csr : public csr_base {
    pmpaddr8_csr() : csr_base("pmpaddr8", 0x3B8, 64) {}

    field ADDRESS_HI = {
        "pmpaddr8.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr8.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr9_csr : public csr_base {
    pmpaddr9_csr() : csr_base("pmpaddr9", 0x3B9, 64) {}

    field ADDRESS_HI = {
        "pmpaddr9.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr9.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr10_csr : public csr_base {
    pmpaddr10_csr() : csr_base("pmpaddr10", 0x3BA, 64) {}

    field ADDRESS_HI = {
        "pmpaddr10.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr10.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr11_csr : public csr_base {
    pmpaddr11_csr() : csr_base("pmpaddr11", 0x3BB, 64) {}

    field ADDRESS_HI = {
        "pmpaddr11.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr11.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr12_csr : public csr_base {
    pmpaddr12_csr() : csr_base("pmpaddr12", 0x3BC, 64) {}

    field ADDRESS_HI = {
        "pmpaddr12.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr12.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr13_csr : public csr_base {
    pmpaddr13_csr() : csr_base("pmpaddr13", 0x3BD, 64) {}

    field ADDRESS_HI = {
        "pmpaddr13.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr13.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr14_csr : public csr_base {
    pmpaddr14_csr() : csr_base("pmpaddr14", 0x3BE, 64) {}

    field ADDRESS_HI = {
        "pmpaddr14.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr14.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct pmpaddr15_csr : public csr_base {
    pmpaddr15_csr() : csr_base("pmpaddr15", 0x3BF, 64) {}

    field ADDRESS_HI = {
        "pmpaddr15.ADDRESS_HI",
        45,
        {53, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x003FFFFFFFFFFE00ULL
    };
    field ADDRESS_LO = {
        "pmpaddr15.ADDRESS_LO",
        9,
        {8, 0},
        0x00000000000001FFULL,
        {0x00000000000001FFULL},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct tselect_csr : public csr_base {
    tselect_csr() : csr_base("tselect", 0x7A0, 64) {}

    field INDEX = {
        "tselect.INDEX",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct dcsr_csr : public csr_base {
    dcsr_csr() : csr_base("dcsr", 0x7B0, 64) {}

    field XDEBUGVER = {
        "dcsr.XDEBUGVER",
        4,
        {31, 28},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000F0000000ULL
    };
    field HARD0_2 = {
        "dcsr.HARD0_2",
        10,
        {27, 18},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000FFC0000ULL
    };
    field EBREAKVS = {
        "dcsr.EBREAKVS",
        1,
        {17, 17},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000020000ULL
    };
    field EBREAKVU = {
        "dcsr.EBREAKVU",
        1,
        {16, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000010000ULL
    };
    field EBREAKM = {
        "dcsr.EBREAKM",
        1,
        {15, 15},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000008000ULL
    };
    field HARD0_1 = {
        "dcsr.HARD0_1",
        1,
        {14, 14},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000004000ULL
    };
    field EBREAKS = {
        "dcsr.EBREAKS",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field EBREAKU = {
        "dcsr.EBREAKU",
        1,
        {12, 12},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000001000ULL
    };
    field STEPIE = {
        "dcsr.STEPIE",
        1,
        {11, 11},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000800ULL
    };
    field STOPCOUNT = {
        "dcsr.STOPCOUNT",
        1,
        {10, 10},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000400ULL
    };
    field STOPTIME = {
        "dcsr.STOPTIME",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field CAUSE = {
        "dcsr.CAUSE",
        3,
        {8, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000001C0ULL
    };
    field V = {
        "dcsr.V",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field MPRVEN = {
        "dcsr.MPRVEN",
        1,
        {4, 4},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000010ULL
    };
    field NMIP = {
        "dcsr.NMIP",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field STEP = {
        "dcsr.STEP",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field PRV = {
        "dcsr.PRV",
        2,
        {1, 0},
        0x0000000000000003ULL,
        {},
        "WARL",
        "",
        0x0000000000000003ULL
    };
};

struct dpc_csr : public csr_base {
    dpc_csr() : csr_base("dpc", 0x7B1, 64) {}

    field DPC = {
        "dpc.DPC",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct dscratch0_csr : public csr_base {
    dscratch0_csr() : csr_base("dscratch0", 0x7B2, 64) {}

    field DSCRATCH0 = {
        "dscratch0.DSCRATCH0",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct dscratch1_csr : public csr_base {
    dscratch1_csr() : csr_base("dscratch1", 0x7B3, 64) {}

    field DSCRATCH1 = {
        "dscratch1.DSCRATCH1",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hstatus_csr : public csr_base {
    hstatus_csr() : csr_base("hstatus", 0x600, 64) {}

    field WPRI_5 = {
        "hstatus.WPRI_5",
        14,
        {63, 50},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0xFFFC000000000000ULL
    };
    field HUPMM = {
        "hstatus.HUPMM",
        2,
        {49, 48},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000002ULL},
        "WARL",
        "",
        0x0003000000000000ULL
    };
    field WPRI_4 = {
        "hstatus.WPRI_4",
        14,
        {47, 34},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x0000FFFC00000000ULL
    };
    field VSXL = {
        "hstatus.VSXL",
        2,
        {33, 32},
        0x0000000000000002ULL,
        {0x0000000000000002ULL},
        "WARL",
        "",
        0x0000000300000000ULL
    };
    field WPRI_3 = {
        "hstatus.WPRI_3",
        9,
        {31, 23},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x00000000FF800000ULL
    };
    field VTSR = {
        "hstatus.VTSR",
        1,
        {22, 22},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000400000ULL
    };
    field VTW = {
        "hstatus.VTW",
        1,
        {21, 21},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000200000ULL
    };
    field VTVM = {
        "hstatus.VTVM",
        1,
        {20, 20},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000100000ULL
    };
    field WPRI_2 = {
        "hstatus.WPRI_2",
        2,
        {19, 18},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x00000000000C0000ULL
    };
    field VGEIN = {
        "hstatus.VGEIN",
        6,
        {17, 12},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x000000000003F000ULL
    };
    field WPRI_1 = {
        "hstatus.WPRI_1",
        2,
        {11, 10},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x0000000000000C00ULL
    };
    field HU = {
        "hstatus.HU",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field SPVP = {
        "hstatus.SPVP",
        1,
        {8, 8},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000100ULL
    };
    field SPV = {
        "hstatus.SPV",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field GVA = {
        "hstatus.GVA",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field VSBE = {
        "hstatus.VSBE",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field WPRI_0 = {
        "hstatus.WPRI_0",
        5,
        {4, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x000000000000001FULL
    };
};

struct hedeleg_csr : public csr_base {
    hedeleg_csr() : csr_base("hedeleg", 0x602, 64) {}

    field HARD0_3 = {
        "hedeleg.HARD0_3",
        40,
        {63, 24},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFF000000ULL
    };
    field HARD0_2 = {
        "hedeleg.HARD0_2",
        4,
        {23, 20},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000F00000ULL
    };
    field HEDELEG_3 = {
        "hedeleg.HEDELEG_3",
        4,
        {19, 16},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x00000000000F0000ULL
    };
    field HEDELEG_2 = {
        "hedeleg.HEDELEG_2",
        1,
        {15, 15},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000008000ULL
    };
    field HARD0_1 = {
        "hedeleg.HARD0_1",
        1,
        {14, 14},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000004000ULL
    };
    field HEDELEG_1 = {
        "hedeleg.HEDELEG_1",
        2,
        {13, 12},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000003000ULL
    };
    field HARD0_0 = {
        "hedeleg.HARD0_0",
        3,
        {11, 9},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000E00ULL
    };
    field HEDELEG_0 = {
        "hedeleg.HEDELEG_0",
        9,
        {8, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct hideleg_csr : public csr_base {
    hideleg_csr() : csr_base("hideleg", 0x603, 64) {}

    field LCOFIP = {
        "hideleg.LCOFIP",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field HARD0_5 = {
        "hideleg.HARD0_5",
        2,
        {12, 11},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000001800ULL
    };
    field VSEIP = {
        "hideleg.VSEIP",
        1,
        {10, 10},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000400ULL
    };
    field HARD0_4 = {
        "hideleg.HARD0_4",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field HARD0_3 = {
        "hideleg.HARD0_3",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field VSTIP = {
        "hideleg.VSTIP",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field HARD0_2 = {
        "hideleg.HARD0_2",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field HARD0_1 = {
        "hideleg.HARD0_1",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field VSSIP = {
        "hideleg.VSSIP",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field HARD0_0 = {
        "hideleg.HARD0_0",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000002ULL
    };
};

struct hvip_csr : public csr_base {
    hvip_csr() : csr_base("hvip", 0x645, 64) {}

    field LCOFIP_VIRT = {
        "hvip.LCOFIP_VIRT",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field HARD0_5 = {
        "hvip.HARD0_5",
        2,
        {12, 11},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000001800ULL
    };
    field VSEIP_VIRT = {
        "hvip.VSEIP_VIRT",
        1,
        {10, 10},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000400ULL
    };
    field HARD0_4 = {
        "hvip.HARD0_4",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field HARD0_3 = {
        "hvip.HARD0_3",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field VSTIP_VIRT = {
        "hvip.VSTIP_VIRT",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field HARD0_2 = {
        "hvip.HARD0_2",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field HARD0_1 = {
        "hvip.HARD0_1",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field VSSIP = {
        "hvip.VSSIP",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field HARD0_0 = {
        "hvip.HARD0_0",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000002ULL
    };
};

struct hviprio1_csr : public csr_base {
    hviprio1_csr() : csr_base("hviprio1", 0x646, 64) {}

    field HVIPRIO1 = {
        "hviprio1.HVIPRIO1",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hviprio2_csr : public csr_base {
    hviprio2_csr() : csr_base("hviprio2", 0x647, 64) {}

    field HVIPRIO2 = {
        "hviprio2.HVIPRIO2",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hip_csr : public csr_base {
    hip_csr() : csr_base("hip", 0x644, 64) {}

    field SGEIP = {
        "hip.SGEIP",
        1,
        {12, 12},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000001000ULL
    };
    field HARD0_5 = {
        "hip.HARD0_5",
        1,
        {11, 11},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000800ULL
    };
    field VSEIP = {
        "hip.VSEIP",
        1,
        {10, 10},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000400ULL
    };
    field HARD0_4 = {
        "hip.HARD0_4",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field HARD0_3 = {
        "hip.HARD0_3",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field VSTIP = {
        "hip.VSTIP",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field HARD0_2 = {
        "hip.HARD0_2",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field HARD0_1 = {
        "hip.HARD0_1",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field VSSIP = {
        "hip.VSSIP",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field HARD0_0 = {
        "hip.HARD0_0",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000002ULL
    };
};

struct hie_csr : public csr_base {
    hie_csr() : csr_base("hie", 0x604, 64) {}

    field SGEIE = {
        "hie.SGEIE",
        1,
        {12, 12},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000001000ULL
    };
    field HARD0_5 = {
        "hie.HARD0_5",
        1,
        {11, 11},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000800ULL
    };
    field VSEIE = {
        "hie.VSEIE",
        1,
        {10, 10},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000400ULL
    };
    field HARD0_4 = {
        "hie.HARD0_4",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field HARD0_3 = {
        "hie.HARD0_3",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field VSTIE = {
        "hie.VSTIE",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field HARD0_2 = {
        "hie.HARD0_2",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field HARD0_1 = {
        "hie.HARD0_1",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field VSSIE = {
        "hie.VSSIE",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field HARD0_0 = {
        "hie.HARD0_0",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000002ULL
    };
};

struct hgeip_csr : public csr_base {
    hgeip_csr() : csr_base("hgeip", 0xE12, 64) {}

    field GUESTEXTERNALINTERRUPTS = {
        "hgeip.GUESTEXTERNALINTERRUPTS",
        63,
        {63, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFEULL
    };
    field HARD0 = {
        "hgeip.HARD0",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct hgeie_csr : public csr_base {
    hgeie_csr() : csr_base("hgeie", 0x607, 64) {}

    field HARD0_1 = {
        "hgeie.HARD0_1",
        58,
        {63, 6},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFC0ULL
    };
    field GUESTEXTERNALINTERRUPTSWARL = {
        "hgeie.GUESTEXTERNALINTERRUPTSWARL",
        5,
        {5, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x000000000000003EULL
    };
    field HARD0_0 = {
        "hgeie.HARD0_0",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct henvcfg_csr : public csr_base {
    henvcfg_csr() : csr_base("henvcfg", 0x60A, 64) {}

    field VSTCE = {
        "henvcfg.VSTCE",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field PBMTE = {
        "henvcfg.PBMTE",
        1,
        {62, 62},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x4000000000000000ULL
    };
    field HADE = {
        "henvcfg.HADE",
        1,
        {61, 61},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x2000000000000000ULL
    };
    field WPRI_2 = {
        "henvcfg.WPRI_2",
        27,
        {60, 34},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x1FFFFFFC00000000ULL
    };
    field PMM = {
        "henvcfg.PMM",
        2,
        {33, 32},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000002ULL},
        "WARL",
        "",
        0x0000000300000000ULL
    };
    field WPRI_1 = {
        "henvcfg.WPRI_1",
        24,
        {31, 8},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x00000000FFFFFF00ULL
    };
    field CBZE = {
        "henvcfg.CBZE",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field CBCFE = {
        "henvcfg.CBCFE",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field CBIE = {
        "henvcfg.CBIE",
        2,
        {5, 4},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000030ULL
    };
    field WPRI_0 = {
        "henvcfg.WPRI_0",
        3,
        {3, 1},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x000000000000000EULL
    };
    field FIOM = {
        "henvcfg.FIOM",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct hcounteren_csr : public csr_base {
    hcounteren_csr() : csr_base("hcounteren", 0x606, 32) {}

    field HPM31 = {
        "hcounteren.HPM31",
        1,
        {31, 31},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000080000000ULL
    };
    field HPM30 = {
        "hcounteren.HPM30",
        1,
        {30, 30},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000040000000ULL
    };
    field HPM29 = {
        "hcounteren.HPM29",
        1,
        {29, 29},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000020000000ULL
    };
    field HPM28 = {
        "hcounteren.HPM28",
        1,
        {28, 28},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000010000000ULL
    };
    field HPM27 = {
        "hcounteren.HPM27",
        1,
        {27, 27},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000008000000ULL
    };
    field HPM26 = {
        "hcounteren.HPM26",
        1,
        {26, 26},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000004000000ULL
    };
    field HPM25 = {
        "hcounteren.HPM25",
        1,
        {25, 25},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000002000000ULL
    };
    field HPM24 = {
        "hcounteren.HPM24",
        1,
        {24, 24},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000001000000ULL
    };
    field HPM23 = {
        "hcounteren.HPM23",
        1,
        {23, 23},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000800000ULL
    };
    field HPM22 = {
        "hcounteren.HPM22",
        1,
        {22, 22},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000400000ULL
    };
    field HPM21 = {
        "hcounteren.HPM21",
        1,
        {21, 21},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000200000ULL
    };
    field HPM20 = {
        "hcounteren.HPM20",
        1,
        {20, 20},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000100000ULL
    };
    field HPM19 = {
        "hcounteren.HPM19",
        1,
        {19, 19},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000080000ULL
    };
    field HPM18 = {
        "hcounteren.HPM18",
        1,
        {18, 18},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000040000ULL
    };
    field HPM17 = {
        "hcounteren.HPM17",
        1,
        {17, 17},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000020000ULL
    };
    field HPM16 = {
        "hcounteren.HPM16",
        1,
        {16, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000010000ULL
    };
    field HPM15 = {
        "hcounteren.HPM15",
        1,
        {15, 15},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000008000ULL
    };
    field HPM14 = {
        "hcounteren.HPM14",
        1,
        {14, 14},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000004000ULL
    };
    field HPM13 = {
        "hcounteren.HPM13",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field HPM12 = {
        "hcounteren.HPM12",
        1,
        {12, 12},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000001000ULL
    };
    field HPM11 = {
        "hcounteren.HPM11",
        1,
        {11, 11},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000800ULL
    };
    field HPM10 = {
        "hcounteren.HPM10",
        1,
        {10, 10},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000400ULL
    };
    field HPM9 = {
        "hcounteren.HPM9",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field HPM8 = {
        "hcounteren.HPM8",
        1,
        {8, 8},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000100ULL
    };
    field HPM7 = {
        "hcounteren.HPM7",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field HPM6 = {
        "hcounteren.HPM6",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field HPM5 = {
        "hcounteren.HPM5",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field HPM4 = {
        "hcounteren.HPM4",
        1,
        {4, 4},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000010ULL
    };
    field HPM3 = {
        "hcounteren.HPM3",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000008ULL
    };
    field IR = {
        "hcounteren.IR",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field TM = {
        "hcounteren.TM",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field CY = {
        "hcounteren.CY",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct htimedelta_csr : public csr_base {
    htimedelta_csr() : csr_base("htimedelta", 0x605, 64) {}

    field HTIMEDELTA = {
        "htimedelta.HTIMEDELTA",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct htval_csr : public csr_base {
    htval_csr() : csr_base("htval", 0x643, 64) {}

    field HTVAL = {
        "htval.HTVAL",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct htinst_csr : public csr_base {
    htinst_csr() : csr_base("htinst", 0x64A, 64) {}

    field HTINST = {
        "htinst.HTINST",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct hgatp_csr : public csr_base {
    hgatp_csr() : csr_base("hgatp", 0x680, 64) {}

    field MODE = {
        "hgatp.MODE",
        4,
        {63, 60},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000008ULL, 0x0000000000000009ULL, 0x000000000000000AULL},
        "WARL",
        "",
        0xF000000000000000ULL
    };
    field WARL0 = {
        "hgatp.WARL0",
        2,
        {59, 58},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0C00000000000000ULL
    };
    field VMID = {
        "hgatp.VMID",
        14,
        {57, 44},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x03FFF00000000000ULL
    };
    field PPN = {
        "hgatp.PPN",
        44,
        {43, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000FFFFFFFFFFFULL
    };
};

struct hvien_csr : public csr_base {
    hvien_csr() : csr_base("hvien", 0x608, 64) {}

    field LCOFIP = {
        "hvien.LCOFIP",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field HARD0_0 = {
        "hvien.HARD0_0",
        13,
        {12, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000001FFFULL
    };
};

struct hvictl_csr : public csr_base {
    hvictl_csr() : csr_base("hvictl", 0x609, 64) {}

    field VTI = {
        "hvictl.VTI",
        1,
        {30, 30},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000040000000ULL
    };
    field IID = {
        "hvictl.IID",
        6,
        {21, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000003F0000ULL
    };
    field DPR = {
        "hvictl.DPR",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field IPRIOM = {
        "hvictl.IPRIOM",
        1,
        {8, 8},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000100ULL
    };
    field IPRIO = {
        "hvictl.IPRIO",
        8,
        {7, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000000FFULL
    };
};

struct vsstatus_csr : public csr_base {
    vsstatus_csr() : csr_base("vsstatus", 0x200, 64) {}

    field SD = {
        "vsstatus.SD",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field VSSTATUS_WPRI_6 = {
        "vsstatus.VSSTATUS_WPRI_6",
        29,
        {62, 34},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x7FFFFFFC00000000ULL
    };
    field UXL = {
        "vsstatus.UXL",
        2,
        {33, 32},
        0x0000000000000002ULL,
        {0x0000000000000002ULL},
        "WARL",
        "",
        0x0000000300000000ULL
    };
    field VSSTATUS_WPRI_5 = {
        "vsstatus.VSSTATUS_WPRI_5",
        12,
        {31, 20},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x00000000FFF00000ULL
    };
    field MXR = {
        "vsstatus.MXR",
        1,
        {19, 19},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000080000ULL
    };
    field SUM = {
        "vsstatus.SUM",
        1,
        {18, 18},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000040000ULL
    };
    field VSSTATUS_WPRI_4 = {
        "vsstatus.VSSTATUS_WPRI_4",
        1,
        {17, 17},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x0000000000020000ULL
    };
    field XS = {
        "vsstatus.XS",
        2,
        {16, 15},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000018000ULL
    };
    field FS = {
        "vsstatus.FS",
        2,
        {14, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000006000ULL
    };
    field VSSTATUS_WPRI_3 = {
        "vsstatus.VSSTATUS_WPRI_3",
        2,
        {12, 11},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x0000000000001800ULL
    };
    field VS = {
        "vsstatus.VS",
        2,
        {10, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000600ULL
    };
    field SPP = {
        "vsstatus.SPP",
        1,
        {8, 8},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000100ULL
    };
    field VSSTATUS_WPRI_2 = {
        "vsstatus.VSSTATUS_WPRI_2",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x0000000000000080ULL
    };
    field UBE = {
        "vsstatus.UBE",
        1,
        {6, 6},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000040ULL
    };
    field SPIE = {
        "vsstatus.SPIE",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field VSSTATUS_WPRI_1 = {
        "vsstatus.VSSTATUS_WPRI_1",
        3,
        {4, 2},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x000000000000001CULL
    };
    field SIE = {
        "vsstatus.SIE",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field VSSTATUS_WPRI_0 = {
        "vsstatus.VSSTATUS_WPRI_0",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x0000000000000001ULL
    };
};

struct vsip_csr : public csr_base {
    vsip_csr() : csr_base("vsip", 0x244, 16) {}

    field LCOFIP = {
        "vsip.LCOFIP",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field HARD0_3 = {
        "vsip.HARD0_3",
        3,
        {12, 10},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000001C00ULL
    };
    field VSEIP = {
        "vsip.VSEIP",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field HARD0_2 = {
        "vsip.HARD0_2",
        2,
        {7, 6},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x00000000000000C0ULL
    };
    field VSTIP = {
        "vsip.VSTIP",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field HARD0_1 = {
        "vsip.HARD0_1",
        2,
        {3, 2},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000000000CULL
    };
    field VSSIP = {
        "vsip.VSSIP",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
};

struct vsie_csr : public csr_base {
    vsie_csr() : csr_base("vsie", 0x204, 16) {}

    field LCOFIE = {
        "vsie.LCOFIE",
        1,
        {13, 13},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000002000ULL
    };
    field HARD0_2 = {
        "vsie.HARD0_2",
        3,
        {12, 10},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000001C00ULL
    };
    field VSEIE = {
        "vsie.VSEIE",
        1,
        {9, 9},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000200ULL
    };
    field HARD0_1 = {
        "vsie.HARD0_1",
        2,
        {7, 6},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x00000000000000C0ULL
    };
    field VSTIE = {
        "vsie.VSTIE",
        1,
        {5, 5},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000020ULL
    };
    field HARD0_0 = {
        "vsie.HARD0_0",
        2,
        {3, 2},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000000000CULL
    };
    field VSSIE = {
        "vsie.VSSIE",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
};

struct vstvec_csr : public csr_base {
    vstvec_csr() : csr_base("vstvec", 0x205, 64) {}

    field BASESXLEN12WARL = {
        "vstvec.BASESXLEN12WARL",
        62,
        {63, 2},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFCULL
    };
    field MODE_1 = {
        "vstvec.MODE_1",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field MODE_0 = {
        "vstvec.MODE_0",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct vsscratch_csr : public csr_base {
    vsscratch_csr() : csr_base("vsscratch", 0x240, 64) {}

    field SSCRATCH = {
        "vsscratch.SSCRATCH",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct vsepc_csr : public csr_base {
    vsepc_csr() : csr_base("vsepc", 0x241, 64) {}

    field ADDR = {
        "vsepc.ADDR",
        63,
        {63, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFEULL
    };
};

struct vscause_csr : public csr_base {
    vscause_csr() : csr_base("vscause", 0x242, 64) {}

    field INTERRUPT = {
        "vscause.INTERRUPT",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field EXCEPTIONCODEWLRL = {
        "vscause.EXCEPTIONCODEWLRL",
        63,
        {62, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x7FFFFFFFFFFFFFFFULL
    };
};

struct vstval_csr : public csr_base {
    vstval_csr() : csr_base("vstval", 0x243, 64) {}

    field VSTVAL = {
        "vstval.VSTVAL",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct vstimecmp_csr : public csr_base {
    vstimecmp_csr() : csr_base("vstimecmp", 0x24D, 64) {}

    field VSTIMECMP = {
        "vstimecmp.VSTIMECMP",
        64,
        {63, 0},
        0x00000000FFFFFFFFULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct vsatp_csr : public csr_base {
    vsatp_csr() : csr_base("vsatp", 0x280, 64) {}

    field MODE = {
        "vsatp.MODE",
        4,
        {63, 60},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xF000000000000000ULL
    };
    field ASID = {
        "vsatp.ASID",
        16,
        {59, 44},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0FFFF00000000000ULL
    };
    field PPN = {
        "vsatp.PPN",
        44,
        {43, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000FFFFFFFFFFFULL
    };
};

struct vsiselect_csr : public csr_base {
    vsiselect_csr() : csr_base("vsiselect", 0x250, 64) {}

    field RSVD_63_9 = {
        "vsiselect.RSVD_63_9",
        55,
        {63, 9},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFE00ULL
    };
    field INTERRUPTS = {
        "vsiselect.INTERRUPTS",
        9,
        {8, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000001FFULL
    };
};

struct vsireg_csr : public csr_base {
    vsireg_csr() : csr_base("vsireg", 0x251, 64) {}

    field SIREG = {
        "vsireg.SIREG",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct vstopei_csr : public csr_base {
    vstopei_csr() : csr_base("vstopei", 0x25C, 64) {}

    field RSVD_63_27 = {
        "vstopei.RSVD_63_27",
        37,
        {63, 27},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFF8000000ULL
    };
    field IDENTITY = {
        "vstopei.IDENTITY",
        11,
        {26, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000007FF0000ULL
    };
    field RSVD_15_11 = {
        "vstopei.RSVD_15_11",
        5,
        {15, 11},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000000F800ULL
    };
    field PRIORITY = {
        "vstopei.PRIORITY",
        11,
        {10, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000007FFULL
    };
};

struct vstopi_csr : public csr_base {
    vstopi_csr() : csr_base("vstopi", 0xEB0, 64) {}

    field RSVD_63_28 = {
        "vstopi.RSVD_63_28",
        36,
        {63, 28},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFF0000000ULL
    };
    field IID = {
        "vstopi.IID",
        12,
        {27, 16},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x000000000FFF0000ULL
    };
    field RSVD_15_8 = {
        "vstopi.RSVD_15_8",
        8,
        {15, 8},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000000FF00ULL
    };
    field IPRIO = {
        "vstopi.IPRIO",
        8,
        {7, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000000000FFULL
    };
};

struct mtinst_csr : public csr_base {
    mtinst_csr() : csr_base("mtinst", 0x34A, 64) {}

    field MTINST = {
        "mtinst.MTINST",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mtval2_csr : public csr_base {
    mtval2_csr() : csr_base("mtval2", 0x34B, 64) {}

    field MTVAL2 = {
        "mtval2.MTVAL2",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct scountovf_csr : public csr_base {
    scountovf_csr() : csr_base("scountovf", 0xDA0, 64) {}

    field SCOUNTOVF = {
        "scountovf.SCOUNTOVF",
        32,
        {31, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x00000000FFFFFFFFULL
    };
};

struct mnstatus_csr : public csr_base {
    mnstatus_csr() : csr_base("mnstatus", 0x744, 64) {}

    field MNPP = {
        "mnstatus.MNPP",
        2,
        {12, 11},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000001800ULL
    };
    field MNPV = {
        "mnstatus.MNPV",
        1,
        {7, 7},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000080ULL
    };
    field NMIE = {
        "mnstatus.NMIE",
        1,
        {3, 3},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000008ULL
    };
};

struct mnscratch_csr : public csr_base {
    mnscratch_csr() : csr_base("mnscratch", 0x740, 64) {}

    field MNSCRATCH = {
        "mnscratch.MNSCRATCH",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL
    };
};

struct mnepc_csr : public csr_base {
    mnepc_csr() : csr_base("mnepc", 0x741, 64) {}

    field ADDR = {
        "mnepc.ADDR",
        63,
        {63, 1},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFEULL
    };
};

struct mncause_csr : public csr_base {
    mncause_csr() : csr_base("mncause", 0x742, 64) {}

    field INTERRUPT = {
        "mncause.INTERRUPT",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field EXCEPTIONCODE = {
        "mncause.EXCEPTIONCODE",
        63,
        {62, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x7FFFFFFFFFFFFFFFULL
    };
};

struct mstateen0_csr : public csr_base {
    mstateen0_csr() : csr_base("mstateen0", 0x30C, 64) {}

    field SE0 = {
        "mstateen0.SE0",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field ENVCFG = {
        "mstateen0.ENVCFG",
        1,
        {62, 62},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x4000000000000000ULL
    };
    field WPRI_1 = {
        "mstateen0.WPRI_1",
        1,
        {61, 61},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x2000000000000000ULL
    };
    field CSRIND = {
        "mstateen0.CSRIND",
        1,
        {60, 60},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x1000000000000000ULL
    };
    field AIA = {
        "mstateen0.AIA",
        1,
        {59, 59},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0800000000000000ULL
    };
    field IMSIC = {
        "mstateen0.IMSIC",
        1,
        {58, 58},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0400000000000000ULL
    };
    field CONTEXT = {
        "mstateen0.CONTEXT",
        1,
        {57, 57},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0200000000000000ULL
    };
    field P1P13 = {
        "mstateen0.P1P13",
        1,
        {56, 56},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0100000000000000ULL
    };
    field SRMCFG = {
        "mstateen0.SRMCFG",
        1,
        {55, 55},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0080000000000000ULL
    };
    field WPRI_0 = {
        "mstateen0.WPRI_0",
        52,
        {54, 3},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x007FFFFFFFFFFFF8ULL
    };
    field JVT = {
        "mstateen0.JVT",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field FCSR = {
        "mstateen0.FCSR",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field C = {
        "mstateen0.C",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct mstateen1_csr : public csr_base {
    mstateen1_csr() : csr_base("mstateen1", 0x30D, 64) {}

    field SE1 = {
        "mstateen1.SE1",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field WPRI = {
        "mstateen1.WPRI",
        63,
        {62, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x7FFFFFFFFFFFFFFFULL
    };
};

struct mstateen2_csr : public csr_base {
    mstateen2_csr() : csr_base("mstateen2", 0x30E, 64) {}

    field SE2 = {
        "mstateen2.SE2",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field WPRI = {
        "mstateen2.WPRI",
        63,
        {62, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x7FFFFFFFFFFFFFFFULL
    };
};

struct mstateen3_csr : public csr_base {
    mstateen3_csr() : csr_base("mstateen3", 0x30F, 64) {}

    field SE3 = {
        "mstateen3.SE3",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field WPRI = {
        "mstateen3.WPRI",
        63,
        {62, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x7FFFFFFFFFFFFFFFULL
    };
};

struct hstateen0_csr : public csr_base {
    hstateen0_csr() : csr_base("hstateen0", 0x60C, 64) {}

    field SE0 = {
        "hstateen0.SE0",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field ENVCFG = {
        "hstateen0.ENVCFG",
        1,
        {62, 62},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x4000000000000000ULL
    };
    field WPRI_1 = {
        "hstateen0.WPRI_1",
        1,
        {61, 61},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x2000000000000000ULL
    };
    field CSRIND = {
        "hstateen0.CSRIND",
        1,
        {60, 60},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x1000000000000000ULL
    };
    field AIA = {
        "hstateen0.AIA",
        1,
        {59, 59},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0800000000000000ULL
    };
    field IMSIC = {
        "hstateen0.IMSIC",
        1,
        {58, 58},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0400000000000000ULL
    };
    field CONTEXT = {
        "hstateen0.CONTEXT",
        1,
        {57, 57},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0200000000000000ULL
    };
    field WPRI_0 = {
        "hstateen0.WPRI_0",
        54,
        {56, 3},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x01FFFFFFFFFFFFF8ULL
    };
    field JVT = {
        "hstateen0.JVT",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field FCSR = {
        "hstateen0.FCSR",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field C = {
        "hstateen0.C",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct hstateen1_csr : public csr_base {
    hstateen1_csr() : csr_base("hstateen1", 0x60D, 64) {}

    field SE1 = {
        "hstateen1.SE1",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field WPRI = {
        "hstateen1.WPRI",
        63,
        {62, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x7FFFFFFFFFFFFFFFULL
    };
};

struct hstateen2_csr : public csr_base {
    hstateen2_csr() : csr_base("hstateen2", 0x60E, 64) {}

    field SE2 = {
        "hstateen2.SE2",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field WPRI = {
        "hstateen2.WPRI",
        63,
        {62, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x7FFFFFFFFFFFFFFFULL
    };
};

struct hstateen3_csr : public csr_base {
    hstateen3_csr() : csr_base("hstateen3", 0x60F, 64) {}

    field SE3 = {
        "hstateen3.SE3",
        1,
        {63, 63},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x8000000000000000ULL
    };
    field WPRI = {
        "hstateen3.WPRI",
        63,
        {62, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x7FFFFFFFFFFFFFFFULL
    };
};

struct sstateen0_csr : public csr_base {
    sstateen0_csr() : csr_base("sstateen0", 0x10C, 32) {}

    field WPRI = {
        "sstateen0.WPRI",
        29,
        {31, 3},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x00000000FFFFFFF8ULL
    };
    field JVT = {
        "sstateen0.JVT",
        1,
        {2, 2},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000004ULL
    };
    field FCSR = {
        "sstateen0.FCSR",
        1,
        {1, 1},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000002ULL
    };
    field C = {
        "sstateen0.C",
        1,
        {0, 0},
        0x0000000000000000ULL,
        {},
        "WARL",
        "",
        0x0000000000000001ULL
    };
};

struct sstateen1_csr : public csr_base {
    sstateen1_csr() : csr_base("sstateen1", 0x10D, 32) {}

    field WPRI = {
        "sstateen1.WPRI",
        32,
        {31, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x00000000FFFFFFFFULL
    };
};

struct sstateen2_csr : public csr_base {
    sstateen2_csr() : csr_base("sstateen2", 0x10E, 32) {}

    field WPRI = {
        "sstateen2.WPRI",
        32,
        {31, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x00000000FFFFFFFFULL
    };
};

struct sstateen3_csr : public csr_base {
    sstateen3_csr() : csr_base("sstateen3", 0x10F, 32) {}

    field WPRI = {
        "sstateen3.WPRI",
        32,
        {31, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WPRI",
        "",
        0x00000000FFFFFFFFULL
    };
};

// Global CSR instances
extern cycle_csr cycle;
extern time__csr time_;
extern instret_csr instret;
extern hpmcounter3_csr hpmcounter3;
extern hpmcounter4_csr hpmcounter4;
extern hpmcounter5_csr hpmcounter5;
extern hpmcounter6_csr hpmcounter6;
extern hpmcounter7_csr hpmcounter7;
extern hpmcounter8_csr hpmcounter8;
extern hpmcounter9_csr hpmcounter9;
extern hpmcounter10_csr hpmcounter10;
extern hpmcounter11_csr hpmcounter11;
extern hpmcounter12_csr hpmcounter12;
extern hpmcounter13_csr hpmcounter13;
extern hpmcounter14_csr hpmcounter14;
extern hpmcounter15_csr hpmcounter15;
extern hpmcounter16_csr hpmcounter16;
extern hpmcounter17_csr hpmcounter17;
extern hpmcounter18_csr hpmcounter18;
extern hpmcounter19_csr hpmcounter19;
extern hpmcounter20_csr hpmcounter20;
extern hpmcounter21_csr hpmcounter21;
extern hpmcounter22_csr hpmcounter22;
extern hpmcounter23_csr hpmcounter23;
extern hpmcounter24_csr hpmcounter24;
extern hpmcounter25_csr hpmcounter25;
extern hpmcounter26_csr hpmcounter26;
extern hpmcounter27_csr hpmcounter27;
extern hpmcounter28_csr hpmcounter28;
extern hpmcounter29_csr hpmcounter29;
extern hpmcounter30_csr hpmcounter30;
extern hpmcounter31_csr hpmcounter31;
extern misa_csr misa;
extern mstatus_csr mstatus;
extern mtvec_csr mtvec;
extern medeleg_csr medeleg;
extern mideleg_csr mideleg;
extern mip_csr mip;
extern mie_csr mie;
extern mscratch_csr mscratch;
extern mepc_csr mepc;
extern mcause_csr mcause;
extern mtval_csr mtval;
extern mconfigptr_csr mconfigptr;
extern menvcfg_csr menvcfg;
extern mseccfg_csr mseccfg;
extern mcycle_csr mcycle;
extern minstret_csr minstret;
extern mhpmcounter3_csr mhpmcounter3;
extern mhpmcounter4_csr mhpmcounter4;
extern mhpmcounter5_csr mhpmcounter5;
extern mhpmcounter6_csr mhpmcounter6;
extern mhpmcounter7_csr mhpmcounter7;
extern mhpmcounter8_csr mhpmcounter8;
extern mhpmcounter9_csr mhpmcounter9;
extern mhpmcounter10_csr mhpmcounter10;
extern mhpmcounter11_csr mhpmcounter11;
extern mhpmcounter12_csr mhpmcounter12;
extern mhpmcounter13_csr mhpmcounter13;
extern mhpmcounter14_csr mhpmcounter14;
extern mhpmcounter15_csr mhpmcounter15;
extern mhpmcounter16_csr mhpmcounter16;
extern mhpmcounter17_csr mhpmcounter17;
extern mhpmcounter18_csr mhpmcounter18;
extern mhpmcounter19_csr mhpmcounter19;
extern mhpmcounter20_csr mhpmcounter20;
extern mhpmcounter21_csr mhpmcounter21;
extern mhpmcounter22_csr mhpmcounter22;
extern mhpmcounter23_csr mhpmcounter23;
extern mhpmcounter24_csr mhpmcounter24;
extern mhpmcounter25_csr mhpmcounter25;
extern mhpmcounter26_csr mhpmcounter26;
extern mhpmcounter27_csr mhpmcounter27;
extern mhpmcounter28_csr mhpmcounter28;
extern mhpmcounter29_csr mhpmcounter29;
extern mhpmcounter30_csr mhpmcounter30;
extern mhpmcounter31_csr mhpmcounter31;
extern mhpmevent3_csr mhpmevent3;
extern mhpmevent4_csr mhpmevent4;
extern mhpmevent5_csr mhpmevent5;
extern mhpmevent6_csr mhpmevent6;
extern mhpmevent7_csr mhpmevent7;
extern mhpmevent8_csr mhpmevent8;
extern mhpmevent9_csr mhpmevent9;
extern mhpmevent10_csr mhpmevent10;
extern mhpmevent11_csr mhpmevent11;
extern mhpmevent12_csr mhpmevent12;
extern mhpmevent13_csr mhpmevent13;
extern mhpmevent14_csr mhpmevent14;
extern mhpmevent15_csr mhpmevent15;
extern mhpmevent16_csr mhpmevent16;
extern mhpmevent17_csr mhpmevent17;
extern mhpmevent18_csr mhpmevent18;
extern mhpmevent19_csr mhpmevent19;
extern mhpmevent20_csr mhpmevent20;
extern mhpmevent21_csr mhpmevent21;
extern mhpmevent22_csr mhpmevent22;
extern mhpmevent23_csr mhpmevent23;
extern mhpmevent24_csr mhpmevent24;
extern mhpmevent25_csr mhpmevent25;
extern mhpmevent26_csr mhpmevent26;
extern mhpmevent27_csr mhpmevent27;
extern mhpmevent28_csr mhpmevent28;
extern mhpmevent29_csr mhpmevent29;
extern mhpmevent30_csr mhpmevent30;
extern mhpmevent31_csr mhpmevent31;
extern mcounteren_csr mcounteren;
extern mcountinhibit_csr mcountinhibit;
extern miselect_csr miselect;
extern mireg_csr mireg;
extern mtopei_csr mtopei;
extern mtopi_csr mtopi;
extern mvien_csr mvien;
extern mvip_csr mvip;
extern sstatus_csr sstatus;
extern stvec_csr stvec;
extern sip_csr sip;
extern sie_csr sie;
extern scounteren_csr scounteren;
extern sscratch_csr sscratch;
extern sepc_csr sepc;
extern scause_csr scause;
extern stval_csr stval;
extern stimecmp_csr stimecmp;
extern senvcfg_csr senvcfg;
extern satp_csr satp;
extern srmcfg_csr srmcfg;
extern siselect_csr siselect;
extern sireg_csr sireg;
extern stopei_csr stopei;
extern stopi_csr stopi;
extern seed_csr seed;
extern fflags_csr fflags;
extern frm_csr frm;
extern fcsr_csr fcsr;
extern vstart_csr vstart;
extern vxsat_csr vxsat;
extern vxrm_csr vxrm;
extern vcsr_csr vcsr;
extern vl_csr vl;
extern vtype_csr vtype;
extern vlenb_csr vlenb;
extern pmpcfg0_csr pmpcfg0;
extern pmpcfg2_csr pmpcfg2;
extern pmpaddr0_csr pmpaddr0;
extern pmpaddr1_csr pmpaddr1;
extern pmpaddr2_csr pmpaddr2;
extern pmpaddr3_csr pmpaddr3;
extern pmpaddr4_csr pmpaddr4;
extern pmpaddr5_csr pmpaddr5;
extern pmpaddr6_csr pmpaddr6;
extern pmpaddr7_csr pmpaddr7;
extern pmpaddr8_csr pmpaddr8;
extern pmpaddr9_csr pmpaddr9;
extern pmpaddr10_csr pmpaddr10;
extern pmpaddr11_csr pmpaddr11;
extern pmpaddr12_csr pmpaddr12;
extern pmpaddr13_csr pmpaddr13;
extern pmpaddr14_csr pmpaddr14;
extern pmpaddr15_csr pmpaddr15;
extern tselect_csr tselect;
extern dcsr_csr dcsr;
extern dpc_csr dpc;
extern dscratch0_csr dscratch0;
extern dscratch1_csr dscratch1;
extern hstatus_csr hstatus;
extern hedeleg_csr hedeleg;
extern hideleg_csr hideleg;
extern hvip_csr hvip;
extern hviprio1_csr hviprio1;
extern hviprio2_csr hviprio2;
extern hip_csr hip;
extern hie_csr hie;
extern hgeip_csr hgeip;
extern hgeie_csr hgeie;
extern henvcfg_csr henvcfg;
extern hcounteren_csr hcounteren;
extern htimedelta_csr htimedelta;
extern htval_csr htval;
extern htinst_csr htinst;
extern hgatp_csr hgatp;
extern hvien_csr hvien;
extern hvictl_csr hvictl;
extern vsstatus_csr vsstatus;
extern vsip_csr vsip;
extern vsie_csr vsie;
extern vstvec_csr vstvec;
extern vsscratch_csr vsscratch;
extern vsepc_csr vsepc;
extern vscause_csr vscause;
extern vstval_csr vstval;
extern vstimecmp_csr vstimecmp;
extern vsatp_csr vsatp;
extern vsiselect_csr vsiselect;
extern vsireg_csr vsireg;
extern vstopei_csr vstopei;
extern vstopi_csr vstopi;
extern mtinst_csr mtinst;
extern mtval2_csr mtval2;
extern scountovf_csr scountovf;
extern mnstatus_csr mnstatus;
extern mnscratch_csr mnscratch;
extern mnepc_csr mnepc;
extern mncause_csr mncause;
extern mstateen0_csr mstateen0;
extern mstateen1_csr mstateen1;
extern mstateen2_csr mstateen2;
extern mstateen3_csr mstateen3;
extern hstateen0_csr hstateen0;
extern hstateen1_csr hstateen1;
extern hstateen2_csr hstateen2;
extern hstateen3_csr hstateen3;
extern sstateen0_csr sstateen0;
extern sstateen1_csr sstateen1;
extern sstateen2_csr sstateen2;
extern sstateen3_csr sstateen3;

// CSR instance definitions (include in one .cpp file)
#ifdef CSR_IMPLEMENTATIONS
cycle_csr cycle;
time__csr time_;
instret_csr instret;
hpmcounter3_csr hpmcounter3;
hpmcounter4_csr hpmcounter4;
hpmcounter5_csr hpmcounter5;
hpmcounter6_csr hpmcounter6;
hpmcounter7_csr hpmcounter7;
hpmcounter8_csr hpmcounter8;
hpmcounter9_csr hpmcounter9;
hpmcounter10_csr hpmcounter10;
hpmcounter11_csr hpmcounter11;
hpmcounter12_csr hpmcounter12;
hpmcounter13_csr hpmcounter13;
hpmcounter14_csr hpmcounter14;
hpmcounter15_csr hpmcounter15;
hpmcounter16_csr hpmcounter16;
hpmcounter17_csr hpmcounter17;
hpmcounter18_csr hpmcounter18;
hpmcounter19_csr hpmcounter19;
hpmcounter20_csr hpmcounter20;
hpmcounter21_csr hpmcounter21;
hpmcounter22_csr hpmcounter22;
hpmcounter23_csr hpmcounter23;
hpmcounter24_csr hpmcounter24;
hpmcounter25_csr hpmcounter25;
hpmcounter26_csr hpmcounter26;
hpmcounter27_csr hpmcounter27;
hpmcounter28_csr hpmcounter28;
hpmcounter29_csr hpmcounter29;
hpmcounter30_csr hpmcounter30;
hpmcounter31_csr hpmcounter31;
misa_csr misa;
mstatus_csr mstatus;
mtvec_csr mtvec;
medeleg_csr medeleg;
mideleg_csr mideleg;
mip_csr mip;
mie_csr mie;
mscratch_csr mscratch;
mepc_csr mepc;
mcause_csr mcause;
mtval_csr mtval;
mconfigptr_csr mconfigptr;
menvcfg_csr menvcfg;
mseccfg_csr mseccfg;
mcycle_csr mcycle;
minstret_csr minstret;
mhpmcounter3_csr mhpmcounter3;
mhpmcounter4_csr mhpmcounter4;
mhpmcounter5_csr mhpmcounter5;
mhpmcounter6_csr mhpmcounter6;
mhpmcounter7_csr mhpmcounter7;
mhpmcounter8_csr mhpmcounter8;
mhpmcounter9_csr mhpmcounter9;
mhpmcounter10_csr mhpmcounter10;
mhpmcounter11_csr mhpmcounter11;
mhpmcounter12_csr mhpmcounter12;
mhpmcounter13_csr mhpmcounter13;
mhpmcounter14_csr mhpmcounter14;
mhpmcounter15_csr mhpmcounter15;
mhpmcounter16_csr mhpmcounter16;
mhpmcounter17_csr mhpmcounter17;
mhpmcounter18_csr mhpmcounter18;
mhpmcounter19_csr mhpmcounter19;
mhpmcounter20_csr mhpmcounter20;
mhpmcounter21_csr mhpmcounter21;
mhpmcounter22_csr mhpmcounter22;
mhpmcounter23_csr mhpmcounter23;
mhpmcounter24_csr mhpmcounter24;
mhpmcounter25_csr mhpmcounter25;
mhpmcounter26_csr mhpmcounter26;
mhpmcounter27_csr mhpmcounter27;
mhpmcounter28_csr mhpmcounter28;
mhpmcounter29_csr mhpmcounter29;
mhpmcounter30_csr mhpmcounter30;
mhpmcounter31_csr mhpmcounter31;
mhpmevent3_csr mhpmevent3;
mhpmevent4_csr mhpmevent4;
mhpmevent5_csr mhpmevent5;
mhpmevent6_csr mhpmevent6;
mhpmevent7_csr mhpmevent7;
mhpmevent8_csr mhpmevent8;
mhpmevent9_csr mhpmevent9;
mhpmevent10_csr mhpmevent10;
mhpmevent11_csr mhpmevent11;
mhpmevent12_csr mhpmevent12;
mhpmevent13_csr mhpmevent13;
mhpmevent14_csr mhpmevent14;
mhpmevent15_csr mhpmevent15;
mhpmevent16_csr mhpmevent16;
mhpmevent17_csr mhpmevent17;
mhpmevent18_csr mhpmevent18;
mhpmevent19_csr mhpmevent19;
mhpmevent20_csr mhpmevent20;
mhpmevent21_csr mhpmevent21;
mhpmevent22_csr mhpmevent22;
mhpmevent23_csr mhpmevent23;
mhpmevent24_csr mhpmevent24;
mhpmevent25_csr mhpmevent25;
mhpmevent26_csr mhpmevent26;
mhpmevent27_csr mhpmevent27;
mhpmevent28_csr mhpmevent28;
mhpmevent29_csr mhpmevent29;
mhpmevent30_csr mhpmevent30;
mhpmevent31_csr mhpmevent31;
mcounteren_csr mcounteren;
mcountinhibit_csr mcountinhibit;
miselect_csr miselect;
mireg_csr mireg;
mtopei_csr mtopei;
mtopi_csr mtopi;
mvien_csr mvien;
mvip_csr mvip;
sstatus_csr sstatus;
stvec_csr stvec;
sip_csr sip;
sie_csr sie;
scounteren_csr scounteren;
sscratch_csr sscratch;
sepc_csr sepc;
scause_csr scause;
stval_csr stval;
stimecmp_csr stimecmp;
senvcfg_csr senvcfg;
satp_csr satp;
srmcfg_csr srmcfg;
siselect_csr siselect;
sireg_csr sireg;
stopei_csr stopei;
stopi_csr stopi;
seed_csr seed;
fflags_csr fflags;
frm_csr frm;
fcsr_csr fcsr;
vstart_csr vstart;
vxsat_csr vxsat;
vxrm_csr vxrm;
vcsr_csr vcsr;
vl_csr vl;
vtype_csr vtype;
vlenb_csr vlenb;
pmpcfg0_csr pmpcfg0;
pmpcfg2_csr pmpcfg2;
pmpaddr0_csr pmpaddr0;
pmpaddr1_csr pmpaddr1;
pmpaddr2_csr pmpaddr2;
pmpaddr3_csr pmpaddr3;
pmpaddr4_csr pmpaddr4;
pmpaddr5_csr pmpaddr5;
pmpaddr6_csr pmpaddr6;
pmpaddr7_csr pmpaddr7;
pmpaddr8_csr pmpaddr8;
pmpaddr9_csr pmpaddr9;
pmpaddr10_csr pmpaddr10;
pmpaddr11_csr pmpaddr11;
pmpaddr12_csr pmpaddr12;
pmpaddr13_csr pmpaddr13;
pmpaddr14_csr pmpaddr14;
pmpaddr15_csr pmpaddr15;
tselect_csr tselect;
dcsr_csr dcsr;
dpc_csr dpc;
dscratch0_csr dscratch0;
dscratch1_csr dscratch1;
hstatus_csr hstatus;
hedeleg_csr hedeleg;
hideleg_csr hideleg;
hvip_csr hvip;
hviprio1_csr hviprio1;
hviprio2_csr hviprio2;
hip_csr hip;
hie_csr hie;
hgeip_csr hgeip;
hgeie_csr hgeie;
henvcfg_csr henvcfg;
hcounteren_csr hcounteren;
htimedelta_csr htimedelta;
htval_csr htval;
htinst_csr htinst;
hgatp_csr hgatp;
hvien_csr hvien;
hvictl_csr hvictl;
vsstatus_csr vsstatus;
vsip_csr vsip;
vsie_csr vsie;
vstvec_csr vstvec;
vsscratch_csr vsscratch;
vsepc_csr vsepc;
vscause_csr vscause;
vstval_csr vstval;
vstimecmp_csr vstimecmp;
vsatp_csr vsatp;
vsiselect_csr vsiselect;
vsireg_csr vsireg;
vstopei_csr vstopei;
vstopi_csr vstopi;
mtinst_csr mtinst;
mtval2_csr mtval2;
scountovf_csr scountovf;
mnstatus_csr mnstatus;
mnscratch_csr mnscratch;
mnepc_csr mnepc;
mncause_csr mncause;
mstateen0_csr mstateen0;
mstateen1_csr mstateen1;
mstateen2_csr mstateen2;
mstateen3_csr mstateen3;
hstateen0_csr hstateen0;
hstateen1_csr hstateen1;
hstateen2_csr hstateen2;
hstateen3_csr hstateen3;
sstateen0_csr sstateen0;
sstateen1_csr sstateen1;
sstateen2_csr sstateen2;
sstateen3_csr sstateen3;
#endif // CSR_IMPLEMENTATIONS

// Function to initialize alias pointers
// Call this after all CSR instances are created to set up CSR aliases
inline void initialize_csr_aliases() {
    cycle.alias_of = &mcycle;
    instret.alias_of = &minstret;
    hpmcounter3.alias_of = &mhpmcounter3;
    hpmcounter4.alias_of = &mhpmcounter4;
    hpmcounter5.alias_of = &mhpmcounter5;
    hpmcounter6.alias_of = &mhpmcounter6;
    hpmcounter7.alias_of = &mhpmcounter7;
    hpmcounter8.alias_of = &mhpmcounter8;
    hpmcounter9.alias_of = &mhpmcounter9;
    hpmcounter10.alias_of = &mhpmcounter10;
    hpmcounter11.alias_of = &mhpmcounter11;
    hpmcounter12.alias_of = &mhpmcounter12;
    hpmcounter13.alias_of = &mhpmcounter13;
    hpmcounter14.alias_of = &mhpmcounter14;
    hpmcounter15.alias_of = &mhpmcounter15;
    hpmcounter16.alias_of = &mhpmcounter16;
    hpmcounter17.alias_of = &mhpmcounter17;
    hpmcounter18.alias_of = &mhpmcounter18;
    hpmcounter19.alias_of = &mhpmcounter19;
    hpmcounter20.alias_of = &mhpmcounter20;
    hpmcounter21.alias_of = &mhpmcounter21;
    hpmcounter22.alias_of = &mhpmcounter22;
    hpmcounter23.alias_of = &mhpmcounter23;
    hpmcounter24.alias_of = &mhpmcounter24;
    hpmcounter25.alias_of = &mhpmcounter25;
    hpmcounter26.alias_of = &mhpmcounter26;
    hpmcounter27.alias_of = &mhpmcounter27;
    hpmcounter28.alias_of = &mhpmcounter28;
    hpmcounter29.alias_of = &mhpmcounter29;
    hpmcounter30.alias_of = &mhpmcounter30;
    hpmcounter31.alias_of = &mhpmcounter31;
    sstatus.alias_of = &mstatus;
    sip.alias_of = &mip;
    fflags.alias_of = &fcsr;
    frm.alias_of = &fcsr;
    vxsat.alias_of = &vcsr;
    vxrm.alias_of = &vcsr;
    hvip.alias_of = &mip;
    hip.alias_of = &mip;
    hie.alias_of = &mie;
    vsip.alias_of = &mip;
}

} // namespace CSR
