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
    std::uint64_t perf_val;
    
    inline std::uint64_t extract_value(std::uint64_t csr_data) const {
        return (csr_data & bit_mask) >> range.second;
    }
};

struct csr_base {
    std::string name;
    std::uint64_t address;  // 12-bit hex address
    int size;
    csr_base* alias_of;  // Pointer to aliased CSR struct
    std::uint64_t reset_val;  // 64-bit field-based concatenated reset value
    std::uint64_t perf_val;
    bool cac_check;  // CAC check enable flag
    
    csr_base(const std::string& csr_name, std::uint64_t csr_address, int csr_size, bool csr_cac_check = true)
        : name(csr_name), address(csr_address), size(csr_size), alias_of(nullptr), reset_val(0), perf_val(0), cac_check(csr_cac_check) {}
    
    csr_base() = default;
};

struct time__csr : public csr_base {
    time__csr() : csr_base("time", 0xC01, 64, true) {
        reset_val = 0x0000000000000000ULL;
        perf_val = 0x0000000000000000ULL;
    }

    field COUNT = {
        "time.COUNT",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL,
        0x0000000000000000ULL
    };
};

struct utime_csr : public csr_base {
    utime_csr() : csr_base("utime", 0xD01, 64, true) {
        reset_val = 0x0000000000000000ULL;
        perf_val = 0x0000000000000000ULL;
    }

    field COUNT = {
        "utime.COUNT",
        64,
        {63, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xFFFFFFFFFFFFFFFFULL,
        0x0000000000000000ULL
    };
};

struct my_ctl_csr : public csr_base {
    my_ctl_csr() : csr_base("my_ctl", 0x7C0, 64, true) {
        reset_val = 0x0000000000000AB0ULL;
        perf_val = 0x00000000000005C0ULL;
    }

    field CFG_SEL = {
        "my_ctl.CFG-SEL",
        8,
        {11, 4},
        0x00000000000000ABULL,
        {0x00000000000000ABULL},
        "RW",
        "",
        0x0000000000000FF0ULL,
        0x000000000000005CULL
    };
    field MODE = {
        "my_ctl.MODE",
        2,
        {1, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL},
        "WARL",
        "",
        0x0000000000000003ULL,
        0x0000000000000000ULL
    };
};

struct c_dbg_ctl_csr : public csr_base {
    c_dbg_ctl_csr() : csr_base("c_dbg_ctl", 0xBC0, 64, true) {
        reset_val = 0x0000000000000027ULL;
        perf_val = 0x0000000000000001ULL;
    }

    field EN = {
        "c_dbg_ctl.EN",
        1,
        {0, 0},
        0x0000000000000001ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000001ULL,
        0x0000000000000000ULL
    };
    field _2ND_EN = {
        "c_dbg_ctl.2ND_EN",
        1,
        {5, 5},
        0x0000000000000001ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000020ULL,
        0x0000000000000000ULL
    };
    field SEL_MODE = {
        "c_dbg_ctl.SEL MODE",
        3,
        {3, 1},
        0x0000000000000003ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000000000EULL,
        0x0000000000000002ULL
    };
    field GHOST = {
        "c_dbg_ctl.GHOST",
        3,
        {3, 1},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x000000000000000EULL,
        0x0000000000000000ULL
    };
};

struct misa_csr : public csr_base {
    misa_csr() : csr_base("misa", 0x301, 64, false) {
        reset_val = 0x8000000000000141ULL;
        perf_val = 0x8000000000000141ULL;
    }

    field MXL = {
        "misa.MXL",
        4,
        {63, 60},
        0x0000000000000008ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0xF000000000000000ULL,
        0x0000000000000000ULL
    };
    field EXTENSIONS = {
        "misa.EXTENSIONS",
        26,
        {25, 0},
        0x0000000000000141ULL,
        {0x0000000000000141ULL, 0x0000000000001104ULL},
        "WARL",
        "",
        0x0000000003FFFFFFULL,
        0x0000000000000000ULL
    };
};

struct c_scratch_csr : public csr_base {
    c_scratch_csr() : csr_base("c_scratch", 0x5C0, 64, false) {
        reset_val = 0x0000000000000000ULL;
        perf_val = 0x0000000000000000ULL;
    }

    field DATA = {
        "c_scratch.DATA",
        32,
        {31, 0},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "RW",
        "",
        0x00000000FFFFFFFFULL,
        0x0000000000000000ULL
    };
    field TAG = {
        "c_scratch.TAG",
        4,
        {35, 32},
        0x000000000000001FULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000F00000000ULL,
        0x0000000000000000ULL
    };
};

struct addon_status_csr : public csr_base {
    addon_status_csr() : csr_base("addon_status", 0xFC0, 32, true) {
        reset_val = 0x0000000050000002ULL;
        perf_val = 0x0000000090000002ULL;
    }

    field FLAGS = {
        "addon_status.FLAGS",
        4,
        {31, 28},
        0x0000000000000005ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x00000000F0000000ULL,
        0x0000000000000009ULL
    };
    field CROSSING = {
        "addon_status.CROSSING",
        4,
        {35, 32},
        0x0000000000000000ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000000ULL,
        0x0000000000000000ULL
    };
    field IO_MODE = {
        "addon_status.IO.MODE",
        2,
        {1, 0},
        0x0000000000000002ULL,
        {0x0000000000000000ULL},
        "WARL",
        "",
        0x0000000000000003ULL,
        0x0000000000000000ULL
    };
};

// Global CSR instances
extern time__csr time_;
extern utime_csr utime;
extern my_ctl_csr my_ctl;
extern c_dbg_ctl_csr c_dbg_ctl;
extern misa_csr misa;
extern c_scratch_csr c_scratch;
extern addon_status_csr addon_status;

// CSR instance definitions
inline time__csr time_;
inline utime_csr utime;
inline my_ctl_csr my_ctl;
inline c_dbg_ctl_csr c_dbg_ctl;
inline misa_csr misa;
inline c_scratch_csr c_scratch;
inline addon_status_csr addon_status;

// Vector containing all CSR instances
extern std::vector<csr_base*> csr_map;

// Vector definition with all CSR instances
inline std::vector<csr_base*> csr_map = {
    &time_,
    &utime,
    &my_ctl,
    &c_dbg_ctl,
    &misa,
    &c_scratch,
    &addon_status
};

// Utility functions for CSR management
inline size_t get_csr_count() {
    return csr_map.size();
}

inline csr_base* find_csr_by_name(const std::string& name) {
    for (auto* csr : csr_map) {
        if (csr->name == name) {
            return csr;
        }
    }
    return nullptr;
}

inline csr_base* find_csr_by_address(std::uint64_t address) {
    for (auto* csr : csr_map) {
        if (csr->address == address) {
            return csr;
        }
    }
    return nullptr;
}

// Function to initialize alias pointers
// Call this after all CSR instances are created to set up CSR aliases
inline void initialize_csr_aliases() {
    utime.alias_of = &time_;
}

} // namespace CSR
