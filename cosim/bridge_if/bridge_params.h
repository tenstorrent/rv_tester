#pragma once
#include <array>
#include <vector>
#include <unordered_map>
#include "common/device_address_map/device_address_map.h"

namespace {

    constexpr int max_intr = 64;
    constexpr int xlen = 64;
    constexpr int vlen = 256;
    constexpr int va_hi = 56;
    // MMR range from device_address_map (base to last device AXISW 0x1B + 64K)
    inline uint64_t mmr_lo_addr() { uint32_t cluster_id = 0; return generate_cr_device_addr(cluster_id); }
    inline uint64_t mmr_hi_addr() { uint32_t cluster_id = 0; return generate_axisw_device_addr(cluster_id) + 0xFFFF; }
    constexpr uint64_t smc_lo_addr = 0xc000'0000;
    constexpr uint64_t smc_hi_addr = 0xc214'ffff;
    constexpr uint64_t patch_ram_lo = 0x4214C000;
    constexpr uint64_t patch_ram_hi = 0x4214DFFF;
    constexpr uint16_t hgatp_valid_modes[] = {0, 8, 9, 10};
    constexpr uint16_t pmm_legal_values[] = {0, 2};
    constexpr uint64_t pmm_hstatus_mask_lo = 48;
    constexpr uint64_t pmm_cfgs_mask_lo = 32;
    constexpr uint64_t pmm_mask_size = 2;
    constexpr uint64_t time_csr = 0xC01;
    constexpr uint64_t c_dtvec_csr_addr = 0x7DA;
    // ACLINT M-time / M-timecmp (generate_acl_device_addr() + 0 / + 0x8000)
    inline uint64_t mtime_mmr() { uint32_t cluster_id = 0; return generate_acl_device_addr(cluster_id) + 0; }
    inline uint64_t mtimecmp_mmr() { uint32_t cluster_id = 0; return generate_acl_device_addr(cluster_id) + 0x8000; }

    constexpr uint32_t opcode_nop    = 0x13;
    constexpr uint32_t opcode_ret    = 0x8067;
    constexpr uint32_t opcode_ebreak = 0x00100073;
    struct csr_reg {
      uint32_t addr       = 0;
      std::string name    = "";
      bool metric         = false;
      bool nonzero_reset  = false;
      unsigned shadow_csr = 0;
      bool allowlist_custom_csr = false; // perform core arch checks for these allowlisted custom CSRs
    };

    struct mmr_reg {
      std::string name = "";
      uint64_t address = 0;
    };

    struct mmr_entry {
      std::string name;
      uint32_t addr_offset;
    };

#define CSRS                                                                   \
      CSR(FFLAGS,           0x001, "fflags"        ,true)                      \
      CSR(FRM,              0x002, "frm")                                      \
      CSR(FCSR,             0x003, "fcsr")                                     \
      CSR(SEED,             0x015, "seed")                                     \
      CSR(SSTATUS,          0x100, "sstatus"       ,true, true, MSTATUS)       \
      CSR(SIE,              0x104, "sie"           ,true)                      \
      CSR(STVEC,            0x105, "stvec"         ,true)                      \
      CSR(SCOUNTEREN,       0x106, "scounteren")                               \
      CSR(SENVCFG,          0x10A, "senvcfg")                                  \
      CSR(SSCRATCH,         0x140, "sscratch")                                 \
      CSR(SEPC,             0x141, "sepc"          ,true)                      \
      CSR(SCAUSE,           0x142, "scause"        ,true)                      \
      CSR(STVAL,            0x143, "stval"         ,true)                      \
      CSR(SIP,              0x144, "sip"           ,true)                      \
      CSR(SATP,             0x180, "satp")                                     \
      CSR(SRMCFG,           0x181, "srmcfg")                                   \
      CSR(SCONTEXT,         0x5A8, "scontext")                                 \
      CSR(HSTATUS,          0x600, "hstatus"       ,false, true)               \
      CSR(HEDELEG,          0x602, "hedeleg")                                  \
      CSR(HIDELEG,          0x603, "hideleg")                                  \
      CSR(HIE,              0x604, "hie"           ,true)                      \
      CSR(HCOUNTEREN,       0x606, "hcounteren")                               \
      CSR(HGEIE,            0x607, "hgeie")                                    \
      CSR(HTVAL,            0x643, "htval")                                    \
      CSR(HIP,              0x644, "hip"           ,true)                      \
      CSR(HVIP,             0x645, "hvip"          ,true)                      \
      CSR(HTINST,           0x64A, "htinst")                                   \
      CSR(HGEIP,            0xE12, "hgeip")                                    \
      CSR(HENVCFG,          0x60A, "henvcfg")                                  \
      CSR(HENVCFGH,         0x61A, "henvcfgh")                                 \
      CSR(HGATP,            0x680, "hgatp")                                    \
      CSR(HCONTEXT,         0x6A8, "hcontext")                                 \
      CSR(HTIMEDELTA,       0x605, "htimedelta")                               \
      CSR(HTIMEDELTAH,      0x615, "htimedeltah")                              \
      CSR(HVIEN,            0x608, "hvien")                                    \
      CSR(HVICTL,           0x609, "hvictl")                                   \
      CSR(HVIPRIO1,         0x646, "hviprio1")                                 \
      CSR(HVIPRIO2,         0x647, "hviprio2")                                 \
      CSR(VSSTATUS,         0x200, "vsstatus"      ,false, true)               \
      CSR(VSIE,             0x204, "vsie"          ,true)                      \
      CSR(VSTVEC,           0x205, "vstvec")                                   \
      CSR(VSSCRATCH,        0x240, "vsscratch")                                \
      CSR(VSEPC,            0x241, "vsepc")                                    \
      CSR(VSCAUSE,          0x242, "vscause")                                  \
      CSR(VSTVAL,           0x243, "vstval")                                   \
      CSR(VSIP,             0x244, "vsip"          ,true)                      \
      CSR(VSATP,            0x280, "vsatp")                                    \
      CSR(MVENDORID,        0xF11, "mvendorid")                                \
      CSR(MARCHID,          0xF12, "marchid")                                  \
      CSR(MIMPID,           0xF13, "mimpid")                                   \
      CSR(MHARTID,          0xF14, "mhartid")                                  \
      CSR(MCONFIGPTR,       0xF15, "mconfigptr")                               \
      CSR(MSTATUS,          0x300, "mstatus"      , true, true, SSTATUS)       \
      CSR(MISA,             0x301, "misa"         , true, true)                \
      CSR(MEDELEG,          0x302, "medeleg"      , true, true)                \
      CSR(MIDELEG,          0x303, "mideleg"      , true, true)                \
      CSR(MIE,              0x304, "mie"          , true, true)                \
      CSR(MTVEC,            0x305, "mtvec"        , true)                      \
      CSR(MCOUNTEREN,       0x306, "mcounteren")                               \
      CSR(MSTATUSH,         0x310, "mstatush")                                 \
      CSR(MSCRATCH,         0x340, "mscratch")                                 \
      CSR(MEPC,             0x341, "mepc"         , true)                      \
      CSR(MCAUSE,           0x342, "mcause"       , true)                      \
      CSR(MTVAL,            0x343, "mtval"        , true)                      \
      CSR(MIP,              0x344, "mip"          , true)                      \
      CSR(VSTART,           0x008, "vstart")                                   \
      CSR(VXSAT,            0x009, "vxsat")                                    \
      CSR(VXRM,             0x00A, "vxrm")                                     \
      CSR(VCSR,             0x00F, "vcsr")                                     \
      CSR(VL,               0xc20, "vl")                                       \
      CSR(VTYPE,            0xc21, "vtype")                                    \
      CSR(VLENB,            0xC22, "vlenb")                                    \
      CSR(MTINST,           0x34A, "mtinst")                                   \
      CSR(MTVAL2,           0x34B, "mtval2")                                   \
      CSR(MENVCFG,          0x30A, "menvcfg"      , true)                      \
      CSR(MENVCFGH,         0x31A, "menvcfgh")                                 \
      CSR(MSECCFG,          0x747, "mseccfg")                                  \
      CSR(MSECCFGH,         0x757, "mseccfgh")                                 \
      CSR(MNSCRATCH,        0x740, "mnscratch")                                \
      CSR(MNEPC,            0x741, "mnepc")                                    \
      CSR(MNCAUSE,          0x742, "mncause")                                  \
      CSR(MNSTATUS,         0x744, "mnstatus")                                 \
      CSR(PMPCFG0,          0x3A0, "pmpcfg0")                                  \
      CSR(PMPCFG1,          0x3A1, "pmpcfg1")                                  \
      CSR(PMPCFG2,          0x3A2, "pmpcfg2")                                  \
      CSR(PMPCFG3,          0x3A3, "pmpcfg3")                                  \
      CSR(PMPCFG4,          0x3A4, "pmpcfg4")                                  \
      CSR(PMPCFG5,          0x3A5, "pmpcfg5")                                  \
      CSR(PMPCFG6,          0x3A6, "pmpcfg6")                                  \
      CSR(PMPCFG7,          0x3A7, "pmpcfg7")                                  \
      CSR(PMPCFG8,          0x3A8, "pmpcfg8")                                  \
      CSR(PMPCFG9,          0x3A9, "pmpcfg9")                                  \
      CSR(PMPCFG10,         0x3AA, "pmpcfg10")                                 \
      CSR(PMPCFG11,         0x3AB, "pmpcfg11")                                 \
      CSR(PMPCFG12,         0x3AC, "pmpcfg12")                                 \
      CSR(PMPCFG13,         0x3AD, "pmpcfg13")                                 \
      CSR(PMPCFG14,         0x3AE, "pmpcfg14")                                 \
      CSR(PMPCFG15,         0x3AF, "pmpcfg15")                                 \
      CSR(PMPADDR0,         0x3B0, "pmpaddr0")                                 \
      CSR(PMPADDR1,         0x3B1, "pmpaddr1")                                 \
      CSR(PMPADDR2,         0x3B2, "pmpaddr2")                                 \
      CSR(PMPADDR3,         0x3B3, "pmpaddr3")                                 \
      CSR(PMPADDR4,         0x3B4, "pmpaddr4")                                 \
      CSR(PMPADDR5,         0x3B5, "pmpaddr5")                                 \
      CSR(PMPADDR6,         0x3B6, "pmpaddr6")                                 \
      CSR(PMPADDR7,         0x3B7, "pmpaddr7")                                 \
      CSR(PMPADDR8,         0x3B8, "pmpaddr8")                                 \
      CSR(PMPADDR9,         0x3B9, "pmpaddr9")                                 \
      CSR(PMPADDR10,        0x3BA, "pmpaddr10")                                \
      CSR(PMPADDR11,        0x3BB, "pmpaddr11")                                \
      CSR(PMPADDR12,        0x3BC, "pmpaddr12")                                \
      CSR(PMPADDR13,        0x3BD, "pmpaddr13")                                \
      CSR(PMPADDR14,        0x3BE, "pmpaddr14")                                \
      CSR(PMPADDR15,        0x3BF, "pmpaddr15")                                \
      CSR(PMPADDR16,        0x3C0, "pmpaddr16")                                \
      CSR(PMPADDR17,        0x3C1, "pmpaddr17")                                \
      CSR(PMPADDR18,        0x3C2, "pmpaddr18")                                \
      CSR(PMPADDR19,        0x3C3, "pmpaddr19")                                \
      CSR(PMPADDR20,        0x3C4, "pmpaddr20")                                \
      CSR(PMPADDR21,        0x3C5, "pmpaddr21")                                \
      CSR(PMPADDR22,        0x3C6, "pmpaddr22")                                \
      CSR(PMPADDR23,        0x3C7, "pmpaddr23")                                \
      CSR(PMPADDR24,        0x3C8, "pmpaddr24")                                \
      CSR(PMPADDR25,        0x3C9, "pmpaddr25")                                \
      CSR(PMPADDR26,        0x3CA, "pmpaddr26")                                \
      CSR(PMPADDR27,        0x3CB, "pmpaddr27")                                \
      CSR(PMPADDR28,        0x3CC, "pmpaddr28")                                \
      CSR(PMPADDR29,        0x3CD, "pmpaddr29")                                \
      CSR(PMPADDR30,        0x3CE, "pmpaddr30")                                \
      CSR(PMPADDR31,        0x3CF, "pmpaddr31")                                \
      CSR(PMPADDR32,        0x3D0, "pmpaddr32")                                \
      CSR(PMPADDR33,        0x3D1, "pmpaddr33")                                \
      CSR(PMPADDR34,        0x3D2, "pmpaddr34")                                \
      CSR(PMPADDR35,        0x3D3, "pmpaddr35")                                \
      CSR(PMPADDR36,        0x3D4, "pmpaddr36")                                \
      CSR(PMPADDR37,        0x3D5, "pmpaddr37")                                \
      CSR(PMPADDR38,        0x3D6, "pmpaddr38")                                \
      CSR(PMPADDR39,        0x3D7, "pmpaddr39")                                \
      CSR(PMPADDR40,        0x3D8, "pmpaddr40")                                \
      CSR(PMPADDR41,        0x3D9, "pmpaddr41")                                \
      CSR(PMPADDR42,        0x3DA, "pmpaddr42")                                \
      CSR(PMPADDR43,        0x3DB, "pmpaddr43")                                \
      CSR(PMPADDR44,        0x3DC, "pmpaddr44")                                \
      CSR(PMPADDR45,        0x3DD, "pmpaddr45")                                \
      CSR(PMPADDR46,        0x3DE, "pmpaddr46")                                \
      CSR(PMPADDR47,        0x3DF, "pmpaddr47")                                \
      CSR(PMPADDR48,        0x3E0, "pmpaddr48")                                \
      CSR(PMPADDR49,        0x3E1, "pmpaddr49")                                \
      CSR(PMPADDR50,        0x3E2, "pmpaddr50")                                \
      CSR(PMPADDR51,        0x3E3, "pmpaddr51")                                \
      CSR(PMPADDR52,        0x3E4, "pmpaddr52")                                \
      CSR(PMPADDR53,        0x3E5, "pmpaddr53")                                \
      CSR(PMPADDR54,        0x3E6, "pmpaddr54")                                \
      CSR(PMPADDR55,        0x3E7, "pmpaddr55")                                \
      CSR(PMPADDR56,        0x3E8, "pmpaddr56")                                \
      CSR(PMPADDR57,        0x3E9, "pmpaddr57")                                \
      CSR(PMPADDR58,        0x3EA, "pmpaddr58")                                \
      CSR(PMPADDR59,        0x3EB, "pmpaddr59")                                \
      CSR(PMPADDR60,        0x3EC, "pmpaddr60")                                \
      CSR(PMPADDR61,        0x3ED, "pmpaddr61")                                \
      CSR(PMPADDR62,        0x3EE, "pmpaddr62")                                \
      CSR(PMPADDR63,        0x3EF, "pmpaddr63")                                \
      CSR(PMACFG0,          0x7E0, "pmacfg0")                                  \
      CSR(PMACFG1,          0x7E1, "pmacfg1")                                  \
      CSR(PMACFG2,          0x7E2, "pmacfg2")                                  \
      CSR(PMACFG3,          0x7E3, "pmacfg3")                                  \
      CSR(PMACFG4,          0x7E4, "pmacfg4")                                  \
      CSR(PMACFG5,          0x7E5, "pmacfg5")                                  \
      CSR(PMACFG6,          0x7E6, "pmacfg6")                                  \
      CSR(PMACFG7,          0x7E7, "pmacfg7")                                  \
      CSR(PMACFG8,          0x7E8, "pmacfg8")                                  \
      CSR(PMACFG9,          0x7E9, "pmacfg9")                                  \
      CSR(PMACFG10,         0x7EA, "pmacfg10")                                 \
      CSR(PMACFG11,         0x7EB, "pmacfg11")                                 \
      CSR(PMACFG12,         0x7EC, "pmacfg12")                                 \
      CSR(PMACFG13,         0x7ED, "pmacfg13")                                 \
      CSR(PMACFG14,         0x7EE, "pmacfg14")                                 \
      CSR(PMACFG15,         0x7EF, "pmacfg15")                                 \
      CSR(PMACFG16,         0x7F0, "pmacfg16")                                 \
      CSR(PMACFG17,         0x7F1, "pmacfg17")                                 \
      CSR(PMACFG18,         0x7F2, "pmacfg18")                                 \
      CSR(PMACFG19,         0x7F3, "pmacfg19")                                 \
      CSR(PMACFG20,         0x7F4, "pmacfg20")                                 \
      CSR(PMACFG21,         0x7F5, "pmacfg21")                                 \
      CSR(PMACFG22,         0x7F6, "pmacfg22")                                 \
      CSR(PMACFG23,         0x7F7, "pmacfg23")                                 \
      CSR(PMACFG24,         0x7F8, "pmacfg24")                                 \
      CSR(PMACFG25,         0x7F9, "pmacfg25")                                 \
      CSR(PMACFG26,         0x7FA, "pmacfg26")                                 \
      CSR(PMACFG27,         0x7FB, "pmacfg27")                                 \
      CSR(PMACFG28,         0x7FC, "pmacfg28")                                 \
      CSR(PMACFG29,         0x7FD, "pmacfg29")                                 \
      CSR(PMACFG30,         0x7FE, "pmacfg30")                                 \
      CSR(PMACFG31,         0x7FF, "pmacfg31")                                 \
      CSR(PMACFG32,         0xFE0, "pmacfg32")                                 \
      CSR(PMACFG33,         0xFE1, "pmacfg33")                                 \
      CSR(PMACFG34,         0xFE2, "pmacfg34")                                 \
      CSR(PMACFG35,         0xFE3, "pmacfg35")                                 \
      CSR(PMACFG36,         0xFE4, "pmacfg36")                                 \
      CSR(PMACFG37,         0xFE5, "pmacfg37")                                 \
      CSR(PMACFG38,         0xFE6, "pmacfg38")                                 \
      CSR(PMACFG39,         0xFE7, "pmacfg39")                                 \
      CSR(PMACFG40,         0xFE8, "pmacfg40")                                 \
      CSR(PMACFG41,         0xFE9, "pmacfg41")                                 \
      CSR(PMACFG42,         0xFEA, "pmacfg42")                                 \
      CSR(PMACFG43,         0xFEB, "pmacfg43")                                 \
      CSR(PMACFG44,         0xFEC, "pmacfg44")                                 \
      CSR(PMACFG45,         0xFED, "pmacfg45")                                 \
      CSR(PMACFG46,         0xFEE, "pmacfg46")                                 \
      CSR(PMACFG47,         0xFEF, "pmacfg47")                                 \
      CSR(PMACFG48,         0xFF0, "pmacfg48")                                 \
      CSR(PMACFG49,         0xFF1, "pmacfg49")                                 \
      CSR(PMACFG50,         0xFF2, "pmacfg50")                                 \
      CSR(PMACFG51,         0xFF3, "pmacfg51")                                 \
      CSR(PMACFG52,         0xFF4, "pmacfg52")                                 \
      CSR(PMACFG53,         0xFF5, "pmacfg53")                                 \
      CSR(PMACFG54,         0xFF6, "pmacfg54")                                 \
      CSR(PMACFG55,         0xFF7, "pmacfg55")                                 \
      CSR(PMACFG56,         0xFF8, "pmacfg56")                                 \
      CSR(PMACFG57,         0xFF9, "pmacfg57")                                 \
      CSR(PMACFG58,         0xFFA, "pmacfg58")                                 \
      CSR(PMACFG59,         0xFFB, "pmacfg59")                                 \
      CSR(PMACFG60,         0xFFC, "pmacfg60")                                 \
      CSR(PMACFG61,         0xFFD, "pmacfg61")                                 \
      CSR(PMACFG62,         0xFFE, "pmacfg62")                                 \
      CSR(PMACFG63,         0xFFF, "pmacfg63")                                 \
      CSR(MCYCLE,           0xB00, "mcycle")                                   \
      CSR(MINSTRET,         0xB02, "minstret")                                 \
      CSR(MHPMCOUNTER3,     0xB03, "mhpmcounter3")                             \
      CSR(MHPMCOUNTER4,     0xB04, "mhpmcounter4")                             \
      CSR(MHPMCOUNTER5,     0xB05, "mhpmcounter5")                             \
      CSR(MHPMCOUNTER6,     0xB06, "mhpmcounter6")                             \
      CSR(MHPMCOUNTER7,     0xB07, "mhpmcounter7")                             \
      CSR(MHPMCOUNTER8,     0xB08, "mhpmcounter8")                             \
      CSR(MHPMCOUNTER9,     0xB09, "mhpmcounter9")                             \
      CSR(MHPMCOUNTER10,    0xB0A, "mhpmcounter10")                            \
      CSR(MHPMCOUNTER11,    0xB0B, "mhpmcounter11")                            \
      CSR(MHPMCOUNTER12,    0xB0C, "mhpmcounter12")                            \
      CSR(MHPMCOUNTER13,    0xB0D, "mhpmcounter13")                            \
      CSR(MHPMCOUNTER14,    0xB0E, "mhpmcounter14")                            \
      CSR(MHPMCOUNTER15,    0xB0F, "mhpmcounter15")                            \
      CSR(MHPMCOUNTER16,    0xB10, "mhpmcounter16")                            \
      CSR(MHPMCOUNTER17,    0xB11, "mhpmcounter17")                            \
      CSR(MHPMCOUNTER18,    0xB12, "mhpmcounter18")                            \
      CSR(MHPMCOUNTER19,    0xB13, "mhpmcounter19")                            \
      CSR(MHPMCOUNTER20,    0xB14, "mhpmcounter20")                            \
      CSR(MHPMCOUNTER21,    0xB15, "mhpmcounter21")                            \
      CSR(MHPMCOUNTER22,    0xB16, "mhpmcounter22")                            \
      CSR(MHPMCOUNTER23,    0xB17, "mhpmcounter23")                            \
      CSR(MHPMCOUNTER24,    0xB18, "mhpmcounter24")                            \
      CSR(MHPMCOUNTER25,    0xB19, "mhpmcounter25")                            \
      CSR(MHPMCOUNTER26,    0xB1A, "mhpmcounter26")                            \
      CSR(MHPMCOUNTER27,    0xB1B, "mhpmcounter27")                            \
      CSR(MHPMCOUNTER28,    0xB1C, "mhpmcounter28")                            \
      CSR(MHPMCOUNTER29,    0xB1D, "mhpmcounter29")                            \
      CSR(MHPMCOUNTER30,    0xB1E, "mhpmcounter30")                            \
      CSR(MHPMCOUNTER31,    0xB1F, "mhpmcounter31")                            \
      CSR(CYCLE,            0xC00, "cycle")                                    \
      CSR(TIME,             0xC01, "time")                                     \
      CSR(INSTRET,          0xC02, "instret")                                  \
      CSR(HPMCOUNTER3,      0xC03, "hpmcounter3")                              \
      CSR(HPMCOUNTER4,      0xC04, "hpmcounter4")                              \
      CSR(HPMCOUNTER5,      0xC05, "hpmcounter5")                              \
      CSR(HPMCOUNTER6,      0xC06, "hpmcounter6")                              \
      CSR(HPMCOUNTER7,      0xC07, "hpmcounter7")                              \
      CSR(HPMCOUNTER8,      0xC08, "hpmcounter8")                              \
      CSR(HPMCOUNTER9,      0xC09, "hpmcounter9")                              \
      CSR(HPMCOUNTER10,     0xC0A, "hpmcounter10")                             \
      CSR(HPMCOUNTER11,     0xC0B, "hpmcounter11")                             \
      CSR(HPMCOUNTER12,     0xC0C, "hpmcounter12")                             \
      CSR(HPMCOUNTER13,     0xC0D, "hpmcounter13")                             \
      CSR(HPMCOUNTER14,     0xC0E, "hpmcounter14")                             \
      CSR(HPMCOUNTER15,     0xC0F, "hpmcounter15")                             \
      CSR(HPMCOUNTER16,     0xC10, "hpmcounter16")                             \
      CSR(HPMCOUNTER17,     0xC11, "hpmcounter17")                             \
      CSR(HPMCOUNTER18,     0xC12, "hpmcounter18")                             \
      CSR(HPMCOUNTER19,     0xC13, "hpmcounter19")                             \
      CSR(HPMCOUNTER20,     0xC14, "hpmcounter20")                             \
      CSR(HPMCOUNTER21,     0xC15, "hpmcounter21")                             \
      CSR(HPMCOUNTER22,     0xC16, "hpmcounter22")                             \
      CSR(HPMCOUNTER23,     0xC17, "hpmcounter23")                             \
      CSR(HPMCOUNTER24,     0xC18, "hpmcounter24")                             \
      CSR(HPMCOUNTER25,     0xC19, "hpmcounter25")                             \
      CSR(HPMCOUNTER26,     0xC1A, "hpmcounter26")                             \
      CSR(HPMCOUNTER27,     0xC1B, "hpmcounter27")                             \
      CSR(HPMCOUNTER28,     0xC1C, "hpmcounter28")                             \
      CSR(HPMCOUNTER29,     0xC1D, "hpmcounter29")                             \
      CSR(HPMCOUNTER30,     0xC1E, "hpmcounter30")                             \
      CSR(HPMCOUNTER31,     0xC1F, "hpmcounter31")                             \
      CSR(MCOUNTINHIBIT,    0x320, "mcountinhibit")                            \
      CSR(MHPMEVENT3,       0x323, "mhpmevent3")                               \
      CSR(MHPMEVENT4,       0x324, "mhpmevent4")                               \
      CSR(MHPMEVENT5,       0x325, "mhpmevent5")                               \
      CSR(MHPMEVENT6,       0x326, "mhpmevent6")                               \
      CSR(MHPMEVENT7,       0x327, "mhpmevent7")                               \
      CSR(MHPMEVENT8,       0x328, "mhpmevent8")                               \
      CSR(MHPMEVENT9,       0x329, "mhpmevent9")                               \
      CSR(MHPMEVENT10,      0x32A, "mhpmevent10")                              \
      CSR(MHPMEVENT11,      0x32B, "mhpmevent11")                              \
      CSR(MHPMEVENT12,      0x32C, "mhpmevent12")                              \
      CSR(MHPMEVENT13,      0x32D, "mhpmevent13")                              \
      CSR(MHPMEVENT14,      0x32E, "mhpmevent14")                              \
      CSR(MHPMEVENT15,      0x32F, "mhpmevent15")                              \
      CSR(MHPMEVENT16,      0x330, "mhpmevent16")                              \
      CSR(MHPMEVENT17,      0x331, "mhpmevent17")                              \
      CSR(MHPMEVENT18,      0x332, "mhpmevent18")                              \
      CSR(MHPMEVENT19,      0x333, "mhpmevent19")                              \
      CSR(MHPMEVENT20,      0x334, "mhpmevent20")                              \
      CSR(MHPMEVENT21,      0x335, "mhpmevent21")                              \
      CSR(MHPMEVENT22,      0x336, "mhpmevent22")                              \
      CSR(MHPMEVENT23,      0x337, "mhpmevent23")                              \
      CSR(MHPMEVENT24,      0x338, "mhpmevent24")                              \
      CSR(MHPMEVENT25,      0x339, "mhpmevent25")                              \
      CSR(MHPMEVENT26,      0x33A, "mhpmevent26")                              \
      CSR(MHPMEVENT27,      0x33B, "mhpmevent27")                              \
      CSR(MHPMEVENT28,      0x33C, "mhpmevent28")                              \
      CSR(MHPMEVENT29,      0x33D, "mhpmevent29")                              \
      CSR(MHPMEVENT30,      0x33E, "mhpmevent30")                              \
      CSR(MHPMEVENT31,      0x33F, "mhpmevent31")                              \
      CSR(SCOUNTOVF,        0xDA0, "scountovf")                                \
      CSR(TSELECT,          0x7A0, "tselect")                                  \
      CSR(TDATA1,           0x7A1, "tdata1")                                   \
      CSR(TDATA2,           0x7A2, "tdata2")                                   \
      CSR(TDATA3,           0x7A3, "tdata3")                                   \
      CSR(MCONTEXT,         0x7A8, "mcontext")                                 \
      CSR(TINFO,            0x7A4, "tinfo")                                    \
      CSR(DCSR,             0x7B0, "dcsr"          ,false, true)               \
      CSR(DPC,              0x7B1, "dpc")                                      \
      CSR(DSCRATCH0,        0x7B2, "dscratch0")                                \
      CSR(DSCRATCH1,        0x7B3, "dscratch1")                                \
      CSR(SISELECT,         0x150, "siselect")                                 \
      CSR(SIREG,            0x151, "sireg")                                    \
      CSR(VSIREG,           0x251, "vsireg")                                   \
      CSR(VSISELECT,        0x250, "vsiselect")                                \
      CSR(MIREG,            0x351, "mireg")                                    \
      CSR(STOPEI,           0x15c, "stopei", true)                             \
      CSR(VSTOPEI,          0x25c, "vstopei"       ,true)                      \
      CSR(STOPI,            0xDB0, "stopi")                                    \
      CSR(VSTOPI,           0xEB0, "vstopi"        ,true)                      \
      CSR(MISELECT,         0x350, "miselect")                                 \
      CSR(MTOPEI,           0x35C, "mtopei"      ,  true)                      \
      CSR(MTOPI,            0xFB0, "mtopi")                                    \
      CSR(MVIEN,            0x308, "mvien")                                    \
      CSR(MVIP,             0x309, "mvip")                                     \
      CSR(C_FECFG,          0xBC0, "c_fecfg",       true, true)                \
      CSR(C_FECFG1,         0xBC1, "c_fecfg1"    ,  true, true)                \
      CSR(C_FECFG2,         0xBC2, "c_fecfg2"    ,  true, true)                \
      CSR(C_FECFG3,         0xBC3, "c_fecfg3"    ,  true)                      \
      CSR(C_FECFG4,         0xBC4, "c_fecfg4"    ,  true)                      \
      CSR(C_MCCFG,          0xBC5, "c_mccfg"     ,  true, true)                \
      CSR(C_MCCFG1,         0xBCE, "c_mccfg1"    ,  true, true)                \
      CSR(C_MCTHRCFG0,      0xBC6, "c_mcthrcfg0" ,  true, true)                \
      CSR(C_MCTHRCFG1,      0xBC7, "c_mcthrcfg1" ,  true, true)                \
      CSR(C_WFITIMER,       0xBC8, "c_wfitimer"  ,  true, true)                \
      CSR(C_TRACECFG,       0xBC9, "c_tracecfg"  ,  true)                      \
      CSR(C_LSCFG0,         0xBD0, "c_lscfg0"    ,  true)                      \
      CSR(C_LSCFG1,         0xBD1, "c_lscfg1"    ,  true)                      \
      CSR(C_LSCFG2,         0xBD2, "c_lscfg2"    ,  true)                      \
      CSR(C_LSCFG3,         0xBD3, "c_lscfg3"    ,  true)                      \
      CSR(C_LSCFG4,         0xBD4, "c_lscfg4"    ,  true)                      \
      CSR(C_LSCFG5,         0xBD5, "c_lscfg5"    ,  true)                      \
      CSR(C_LSCFG6,         0xBD6, "c_lscfg6"    ,  true)                      \
      CSR(C_LSCFG7,         0xBD7, "c_lscfg7"    ,  true)                      \
      CSR(C_LSCFG8,         0xBD8, "c_lscfg8"    ,  true)                      \
      CSR(C_LSCFG9,         0xBD9, "c_lscfg9"    ,  true)                      \
      CSR(C_LSCFG10,        0xBDA, "c_lscfg10"   ,  true)                      \
      CSR(C_LSCFG11,        0xBDB, "c_lscfg11"   ,  true)                      \
      CSR(C_LSCFG12,        0xBDC, "c_lscfg12"   ,  true)                      \
      CSR(C_LSCFG13,        0xBDD, "c_lscfg13"   ,  true)                      \
      CSR(C_LSCFG14,        0xBDE, "c_lscfg14"   ,  true)                      \
      CSR(C_LSCFG15,        0xBDF, "c_lscfg15"   ,  true)                      \
      CSR(C_MSCFG1,         0xBE9, "c_mscfg1"    ,  true)                      \
      CSR(C_MSPPC,          0xBF0, "c_msppc"     ,  true)                      \
      CSR(C_ASYNCINTSTATUS, 0xBF2, "c_asyncintstatus")                         \
      CSR(MSTATEEN0,        0x30C, "mstateen0")                                \
      CSR(MSTATEEN1,        0x30D, "mstateen1")                                \
      CSR(MSTATEEN2,        0x30E, "mstateen2")                                \
      CSR(MSTATEEN3,        0x30F, "mstateen")                                 \
      CSR(MSTATEEN0H,       0x31C, "mstateen0h")                               \
      CSR(MSTATEEN1H,       0x31D, "mstateen1h")                               \
      CSR(MSTATEEN2H,       0x31E, "mstateen2h")                               \
      CSR(MSTATEEN3H,       0x31F, "mstateen3h")                               \
      CSR(SSTATEEN0,        0x10C, "sstateen0")                                \
      CSR(SSTATEEN1,        0x10D, "sstateen1")                                \
      CSR(SSTATEEN2,        0x10E, "sstateen2")                                \
      CSR(SSTATEEN3,        0x10F, "sstateen3")                                \
      CSR(SSTATEEN0H,       0x11C, "sstateen0h")                               \
      CSR(SSTATEEN1H,       0x11D, "sstateen1h")                               \
      CSR(SSTATEEN2H,       0x11E, "sstateen1h")                               \
      CSR(SSTATEEN3H,       0x11F, "sstateen1h")                               \
      CSR(HSTATEEN0,        0x60C, "hstateen0")                                \
      CSR(HSTATEEN1,        0x60D, "hstateen1")                                \
      CSR(HSTATEEN2,        0x60E, "hstateen2")                                \
      CSR(HSTATEEN3,        0x60F, "hstateen3")                                \
      CSR(HSTATEEN0H,       0x61C, "hstateen0h")                               \
      CSR(HSTATEEN1H,       0x61D, "hstateen1h")                               \
      CSR(HSTATEEN2H,       0x61E, "hstateen2h")                               \
      CSR(HSTATEEN3H,       0x61F, "hstateen3h")                               \
      CSR(STIMECMP,         0x14D, "stimecmp", false, true)                    \
      CSR(VSTIMECMP,        0x24D, "vstimecmp", false, true)                   \
      CSR(C_MATP,           0x7C7, "c_matp", true, true, 0, true)              \
      CSR(C_MISA_RVA23U64_M,           0xBCA, "c_misa_rva23u64_M", true, true, 0, true)              \
      CSR(C_MISA_RVA23U64_O,           0xBCB, "c_misa_rva23u64_O", true, true, 0, true)              \
      CSR(C_MISA_RVA23S64_M,           0xBCC, "c_misa_rva23s64_M", true, true, 0, true)              \
      CSR(C_MISA_RVA23S64_O,           0xBCD, "c_misa_rva23s64_O", true, true, 0, true)              \

    enum csr : unsigned {
#define CSR(name, value, name_str, ...) \
        name = value,
        CSRS
#undef CSR
    };

    const std::unordered_map<unsigned, csr_reg> csrs = {
#define CSR(name, value, name_str, ...) \
        {  name,  csr_reg{value, name_str, ##__VA_ARGS__}},
        CSRS
#undef CSR
    };
#undef CSRS

    // SC MMR entries: .address is offset within SC device; full address = get_mmr_full_address(i)
    const std::array<mmr_entry, 419> mmrs {{
        {"sc_ctrl",                       0x0000},
        {"sc_sp",                         0x0010},
        {"sc_cc_capabilities",            0x00C0},
        {"sc_cc_mon_ctl",                 0x00C8},
        {"sc_cc_mon_ctr_val",             0x00D0},
        {"sc_cc_alloc_ctl",               0x00D8},
        {"sc_cc_block_mask",              0x00E0},
        {"sc_cc_cunits",                  0x00E8},
        {"sc_dbg_cla_counter0_cfg",       0x0280},
        {"sc_dbg_cla_counter1_cfg",       0x0288},
        {"sc_dbg_cla_counter2_cfg",       0x0290},
        {"sc_dbg_cla_counter3_cfg",       0x0298},
        {"sc_dbg_node0_eap0",             0x02A0},
        {"sc_dbg_node0_eap1",             0x02A8},
        {"sc_dbg_node1_eap0",             0x02B0},
        {"sc_dbg_node1_eap1",             0x02B8},
        {"sc_dbg_node2_eap0",             0x02C0},
        {"sc_dbg_node2_eap1",             0x02C8},
        {"sc_dbg_node3_eap0",             0x02D0},
        {"sc_dbg_node3_eap1",             0x02D8},
        {"sc_dbg_signal_mask0",           0x02E0},
        {"sc_dbg_signal_match0",          0x02E8},
        {"sc_dbg_signal_mask1",           0x02F0},
        {"sc_dbg_signal_match1",          0x02F8},
        {"sc_dbg_signal_edge_detect_cfg", 0x0300},
        {"sc_dbg_transition_mask",        0x0318},
        {"sc_dbg_transition_from_value",  0x0320},
        {"sc_dbg_transition_to_value",    0x0328},
        {"sc_dbg_ones_count_mask",        0x0330},
        {"sc_dbg_ones_count_value",       0x0338},
        {"sc_dbg_any_change",             0x0340},
        {"sc_dbg_mux_control_A",          0x0388},
        {"sc_dbg_mux_control_B",          0x0390},
        {"sc_dbg_cla_counter0_cfg",       0x1280},
        {"sc_dbg_cla_counter1_cfg",       0x1288},
        {"sc_dbg_cla_counter2_cfg",       0x1290},
        {"sc_dbg_cla_counter3_cfg",       0x1298},
        {"sc_dbg_node0_eap0",             0x12A0},
        {"sc_dbg_node0_eap1",             0x12A8},
        {"sc_dbg_node1_eap0",             0x12B0},
        {"sc_dbg_node1_eap1",             0x12B8},
        {"sc_dbg_node2_eap0",             0x12C0},
        {"sc_dbg_node2_eap1",             0x12C8},
        {"sc_dbg_node3_eap0",             0x12D0},
        {"sc_dbg_node3_eap1",             0x12D8},
        {"sc_dbg_signal_mask0",           0x12E0},
        {"sc_dbg_signal_match0",          0x12E8},
        {"sc_dbg_signal_mask1",           0x12F0},
        {"sc_dbg_signal_match1",          0x12F8},
        {"sc_dbg_signal_edge_detect_cfg", 0x1300},
        {"sc_dbg_transition_mask",        0x1318},
        {"sc_dbg_transition_from_value",  0x1320},
        {"sc_dbg_transition_to_value",    0x1328},
        {"sc_dbg_ones_count_mask",        0x1330},
        {"sc_dbg_ones_count_value",       0x1338},
        {"sc_dbg_any_change",             0x1340},
        {"sc_dbg_mux_control_A",          0x1388},
        {"sc_dbg_mux_control_B",          0x1390},
        {"sc_dbg_cla_counter0_cfg",       0x2280},
        {"sc_dbg_cla_counter1_cfg",       0x2288},
        {"sc_dbg_cla_counter2_cfg",       0x2290},
        {"sc_dbg_cla_counter3_cfg",       0x2298},
        {"sc_dbg_node0_eap0",             0x22A0},
        {"sc_dbg_node0_eap1",             0x22A8},
        {"sc_dbg_node1_eap0",             0x22B0},
        {"sc_dbg_node1_eap1",             0x22B8},
        {"sc_dbg_node2_eap0",             0x22C0},
        {"sc_dbg_node2_eap1",             0x22C8},
        {"sc_dbg_node3_eap0",             0x22D0},
        {"sc_dbg_node3_eap1",             0x22D8},
        {"sc_dbg_signal_mask0",           0x22E0},
        {"sc_dbg_signal_match0",          0x22E8},
        {"sc_dbg_signal_mask1",           0x22F0},
        {"sc_dbg_signal_match1",          0x22F8},
        {"sc_dbg_signal_edge_detect_cfg", 0x2300},
        {"sc_dbg_transition_mask",        0x2318},
        {"sc_dbg_transition_from_value",  0x2320},
        {"sc_dbg_transition_to_value",    0x2328},
        {"sc_dbg_ones_count_mask",        0x2330},
        {"sc_dbg_ones_count_value",       0x2338},
        {"sc_dbg_any_change",             0x2340},
        {"sc_dbg_mux_control_A",          0x2388},
        {"sc_dbg_mux_control_B",          0x2390},
        {"sc_dbg_cla_counter0_cfg",       0x3280},
        {"sc_dbg_cla_counter1_cfg",       0x3288},
        {"sc_dbg_cla_counter2_cfg",       0x3290},
        {"sc_dbg_cla_counter3_cfg",       0x3298},
        {"sc_dbg_node0_eap0",             0x32A0},
        {"sc_dbg_node0_eap1",             0x32A8},
        {"sc_dbg_node1_eap0",             0x32B0},
        {"sc_dbg_node1_eap1",             0x32B8},
        {"sc_dbg_node2_eap0",             0x32C0},
        {"sc_dbg_node2_eap1",             0x32C8},
        {"sc_dbg_node3_eap0",             0x32D0},
        {"sc_dbg_node3_eap1",             0x32D8},
        {"sc_dbg_signal_mask0",           0x32E0},
        {"sc_dbg_signal_match0",          0x32E8},
        {"sc_dbg_signal_mask1",           0x32F0},
        {"sc_dbg_signal_match1",          0x32F8},
        {"sc_dbg_signal_edge_detect_cfg", 0x3300},
        {"sc_dbg_transition_mask",        0x3318},
        {"sc_dbg_transition_from_value",  0x3320},
        {"sc_dbg_transition_to_value",    0x3328},
        {"sc_dbg_ones_count_mask",        0x3330},
        {"sc_dbg_ones_count_value",       0x3338},
        {"sc_dbg_any_change",             0x3340},
        {"sc_dbg_mux_control_A",          0x3388},
        {"sc_dbg_mux_control_B",          0x3390},
        {"sc_dbg_cla_counter0_cfg",       0x4280},
        {"sc_dbg_cla_counter1_cfg",       0x4288},
        {"sc_dbg_cla_counter2_cfg",       0x4290},
        {"sc_dbg_cla_counter3_cfg",       0x4298},
        {"sc_dbg_node0_eap0",             0x42A0},
        {"sc_dbg_node0_eap1",             0x42A8},
        {"sc_dbg_node1_eap0",             0x42B0},
        {"sc_dbg_node1_eap1",             0x42B8},
        {"sc_dbg_node2_eap0",             0x42C0},
        {"sc_dbg_node2_eap1",             0x42C8},
        {"sc_dbg_node3_eap0",             0x42D0},
        {"sc_dbg_node3_eap1",             0x42D8},
        {"sc_dbg_signal_mask0",           0x42E0},
        {"sc_dbg_signal_match0",          0x42E8},
        {"sc_dbg_signal_mask1",           0x42F0},
        {"sc_dbg_signal_match1",          0x42F8},
        {"sc_dbg_signal_edge_detect_cfg", 0x4300},
        {"sc_dbg_transition_mask",        0x4318},
        {"sc_dbg_transition_from_value",  0x4320},
        {"sc_dbg_transition_to_value",    0x4328},
        {"sc_dbg_ones_count_mask",        0x4330},
        {"sc_dbg_ones_count_value",       0x4338},
        {"sc_dbg_any_change",             0x4340},
        {"sc_dbg_mux_control_A",          0x4388},
        {"sc_dbg_mux_control_B",          0x4390},
        {"sc_dbg_cla_counter0_cfg",       0x5280},
        {"sc_dbg_cla_counter1_cfg",       0x5288},
        {"sc_dbg_cla_counter2_cfg",       0x5290},
        {"sc_dbg_cla_counter3_cfg",       0x5298},
        {"sc_dbg_node0_eap0",             0x52A0},
        {"sc_dbg_node0_eap1",             0x52A8},
        {"sc_dbg_node1_eap0",             0x52B0},
        {"sc_dbg_node1_eap1",             0x52B8},
        {"sc_dbg_node2_eap0",             0x52C0},
        {"sc_dbg_node2_eap1",             0x52C8},
        {"sc_dbg_node3_eap0",             0x52D0},
        {"sc_dbg_node3_eap1",             0x52D8},
        {"sc_dbg_signal_mask0",           0x52E0},
        {"sc_dbg_signal_match0",          0x52E8},
        {"sc_dbg_signal_mask1",           0x52F0},
        {"sc_dbg_signal_match1",          0x52F8},
        {"sc_dbg_signal_edge_detect_cfg", 0x5300},
        {"sc_dbg_transition_mask",        0x5318},
        {"sc_dbg_transition_from_value",  0x5320},
        {"sc_dbg_transition_to_value",    0x5328},
        {"sc_dbg_ones_count_mask",        0x5330},
        {"sc_dbg_ones_count_value",       0x5338},
        {"sc_dbg_any_change",             0x5340},
        {"sc_dbg_mux_control_A",          0x5388},
        {"sc_dbg_mux_control_B",          0x5390},
        {"sc_dbg_cla_counter0_cfg",       0x6280},
        {"sc_dbg_cla_counter1_cfg",       0x6288},
        {"sc_dbg_cla_counter2_cfg",       0x6290},
        {"sc_dbg_cla_counter3_cfg",       0x6298},
        {"sc_dbg_node0_eap0",             0x62A0},
        {"sc_dbg_node0_eap1",             0x62A8},
        {"sc_dbg_node1_eap0",             0x62B0},
        {"sc_dbg_node1_eap1",             0x62B8},
        {"sc_dbg_node2_eap0",             0x62C0},
        {"sc_dbg_node2_eap1",             0x62C8},
        {"sc_dbg_node3_eap0",             0x62D0},
        {"sc_dbg_node3_eap1",             0x62D8},
        {"sc_dbg_signal_mask0",           0x62E0},
        {"sc_dbg_signal_match0",          0x62E8},
        {"sc_dbg_signal_mask1",           0x62F0},
        {"sc_dbg_signal_match1",          0x62F8},
        {"sc_dbg_signal_edge_detect_cfg", 0x6300},
        {"sc_dbg_transition_mask",        0x6318},
        {"sc_dbg_transition_from_value",  0x6320},
        {"sc_dbg_transition_to_value",    0x6328},
        {"sc_dbg_ones_count_mask",        0x6330},
        {"sc_dbg_ones_count_value",       0x6338},
        {"sc_dbg_any_change",             0x6340},
        {"sc_dbg_mux_control_A",          0x6388},
        {"sc_dbg_mux_control_B",          0x6390},
        {"sc_dbg_cla_counter0_cfg",       0x7280},
        {"sc_dbg_cla_counter1_cfg",       0x7288},
        {"sc_dbg_cla_counter2_cfg",       0x7290},
        {"sc_dbg_cla_counter3_cfg",       0x7298},
        {"sc_dbg_node0_eap0",             0x72A0},
        {"sc_dbg_node0_eap1",             0x72A8},
        {"sc_dbg_node1_eap0",             0x72B0},
        {"sc_dbg_node1_eap1",             0x72B8},
        {"sc_dbg_node2_eap0",             0x72C0},
        {"sc_dbg_node2_eap1",             0x72C8},
        {"sc_dbg_node3_eap0",             0x72D0},
        {"sc_dbg_node3_eap1",             0x72D8},
        {"sc_dbg_signal_mask0",           0x72E0},
        {"sc_dbg_signal_match0",          0x72E8},
        {"sc_dbg_signal_mask1",           0x72F0},
        {"sc_dbg_signal_match1",          0x72F8},
        {"sc_dbg_signal_edge_detect_cfg", 0x7300},
        {"sc_dbg_transition_mask",        0x7318},
        {"sc_dbg_transition_from_value",  0x7320},
        {"sc_dbg_transition_to_value",    0x7328},
        {"sc_dbg_ones_count_mask",        0x7330},
        {"sc_dbg_ones_count_value",       0x7338},
        {"sc_dbg_any_change",             0x7340},
        {"sc_dbg_mux_control_A",          0x7388},
        {"sc_dbg_mux_control_B",          0x7390},
        {"sc_dbg_cla_counter0_cfg",       0x8280},
        {"sc_dbg_cla_counter1_cfg",       0x8288},
        {"sc_dbg_cla_counter2_cfg",       0x8290},
        {"sc_dbg_cla_counter3_cfg",       0x8298},
        {"sc_dbg_node0_eap0",             0x82A0},
        {"sc_dbg_node0_eap1",             0x82A8},
        {"sc_dbg_node1_eap0",             0x82B0},
        {"sc_dbg_node1_eap1",             0x82B8},
        {"sc_dbg_node2_eap0",             0x82C0},
        {"sc_dbg_node2_eap1",             0x82C8},
        {"sc_dbg_node3_eap0",             0x82D0},
        {"sc_dbg_node3_eap1",             0x82D8},
        {"sc_dbg_signal_mask0",           0x82E0},
        {"sc_dbg_signal_match0",          0x82E8},
        {"sc_dbg_signal_mask1",           0x82F0},
        {"sc_dbg_signal_match1",          0x82F8},
        {"sc_dbg_signal_edge_detect_cfg", 0x8300},
        {"sc_dbg_transition_mask",        0x8318},
        {"sc_dbg_transition_from_value",  0x8320},
        {"sc_dbg_transition_to_value",    0x8328},
        {"sc_dbg_ones_count_mask",        0x8330},
        {"sc_dbg_ones_count_value",       0x8338},
        {"sc_dbg_any_change",             0x8340},
        {"sc_dbg_mux_control_A",          0x8388},
        {"sc_dbg_mux_control_B",          0x8390},
        {"sc_dbg_cla_counter0_cfg",       0x9280},
        {"sc_dbg_cla_counter1_cfg",       0x9288},
        {"sc_dbg_cla_counter2_cfg",       0x9290},
        {"sc_dbg_cla_counter3_cfg",       0x9298},
        {"sc_dbg_node0_eap0",             0x92A0},
        {"sc_dbg_node0_eap1",             0x92A8},
        {"sc_dbg_node1_eap0",             0x92B0},
        {"sc_dbg_node1_eap1",             0x92B8},
        {"sc_dbg_node2_eap0",             0x92C0},
        {"sc_dbg_node2_eap1",             0x92C8},
        {"sc_dbg_node3_eap0",             0x92D0},
        {"sc_dbg_node3_eap1",             0x92D8},
        {"sc_dbg_signal_mask0",           0x92E0},
        {"sc_dbg_signal_match0",          0x92E8},
        {"sc_dbg_signal_mask1",           0x92F0},
        {"sc_dbg_signal_match1",          0x92F8},
        {"sc_dbg_signal_edge_detect_cfg", 0x9300},
        {"sc_dbg_transition_mask",        0x9318},
        {"sc_dbg_transition_from_value",  0x9320},
        {"sc_dbg_transition_to_value",    0x9328},
        {"sc_dbg_ones_count_mask",        0x9330},
        {"sc_dbg_ones_count_value",       0x9338},
        {"sc_dbg_any_change",             0x9340},
        {"sc_dbg_mux_control_A",          0x9388},
        {"sc_dbg_mux_control_B",          0x9390},
        {"sc_dbg_cla_counter0_cfg",       0xA280},
        {"sc_dbg_cla_counter1_cfg",       0xA288},
        {"sc_dbg_cla_counter2_cfg",       0xA290},
        {"sc_dbg_cla_counter3_cfg",       0xA298},
        {"sc_dbg_node0_eap0",             0xA2A0},
        {"sc_dbg_node0_eap1",             0xA2A8},
        {"sc_dbg_node1_eap0",             0xA2B0},
        {"sc_dbg_node1_eap1",             0xA2B8},
        {"sc_dbg_node2_eap0",             0xA2C0},
        {"sc_dbg_node2_eap1",             0xA2C8},
        {"sc_dbg_node3_eap0",             0xA2D0},
        {"sc_dbg_node3_eap1",             0xA2D8},
        {"sc_dbg_signal_mask0",           0xA2E0},
        {"sc_dbg_signal_match0",          0xA2E8},
        {"sc_dbg_signal_mask1",           0xA2F0},
        {"sc_dbg_signal_match1",          0xA2F8},
        {"sc_dbg_signal_edge_detect_cfg", 0xA300},
        {"sc_dbg_transition_mask",        0xA318},
        {"sc_dbg_transition_from_value",  0xA320},
        {"sc_dbg_transition_to_value",    0xA328},
        {"sc_dbg_ones_count_mask",        0xA330},
        {"sc_dbg_ones_count_value",       0xA338},
        {"sc_dbg_any_change",             0xA340},
        {"sc_dbg_mux_control_A",          0xA388},
        {"sc_dbg_mux_control_B",          0xA390},
        {"sc_dbg_cla_counter0_cfg",       0xB280},
        {"sc_dbg_cla_counter1_cfg",       0xB288},
        {"sc_dbg_cla_counter2_cfg",       0xB290},
        {"sc_dbg_cla_counter3_cfg",       0xB298},
        {"sc_dbg_node0_eap0",             0xB2A0},
        {"sc_dbg_node0_eap1",             0xB2A8},
        {"sc_dbg_node1_eap0",             0xB2B0},
        {"sc_dbg_node1_eap1",             0xB2B8},
        {"sc_dbg_node2_eap0",             0xB2C0},
        {"sc_dbg_node2_eap1",             0xB2C8},
        {"sc_dbg_node3_eap0",             0xB2D0},
        {"sc_dbg_node3_eap1",             0xB2D8},
        {"sc_dbg_signal_mask0",           0xB2E0},
        {"sc_dbg_signal_match0",          0xB2E8},
        {"sc_dbg_signal_mask1",           0xB2F0},
        {"sc_dbg_signal_match1",          0xB2F8},
        {"sc_dbg_signal_edge_detect_cfg", 0xB300},
        {"sc_dbg_transition_mask",        0xB318},
        {"sc_dbg_transition_from_value",  0xB320},
        {"sc_dbg_transition_to_value",    0xB328},
        {"sc_dbg_ones_count_mask",        0xB330},
        {"sc_dbg_ones_count_value",       0xB338},
        {"sc_dbg_any_change",             0xB340},
        {"sc_dbg_mux_control_A",          0xB388},
        {"sc_dbg_mux_control_B",          0xB390},
        {"sc_dbg_cla_counter0_cfg",       0xC280},
        {"sc_dbg_cla_counter1_cfg",       0xC288},
        {"sc_dbg_cla_counter2_cfg",       0xC290},
        {"sc_dbg_cla_counter3_cfg",       0xC298},
        {"sc_dbg_node0_eap0",             0xC2A0},
        {"sc_dbg_node0_eap1",             0xC2A8},
        {"sc_dbg_node1_eap0",             0xC2B0},
        {"sc_dbg_node1_eap1",             0xC2B8},
        {"sc_dbg_node2_eap0",             0xC2C0},
        {"sc_dbg_node2_eap1",             0xC2C8},
        {"sc_dbg_node3_eap0",             0xC2D0},
        {"sc_dbg_node3_eap1",             0xC2D8},
        {"sc_dbg_signal_mask0",           0xC2E0},
        {"sc_dbg_signal_match0",          0xC2E8},
        {"sc_dbg_signal_mask1",           0xC2F0},
        {"sc_dbg_signal_match1",          0xC2F8},
        {"sc_dbg_signal_edge_detect_cfg", 0xC300},
        {"sc_dbg_transition_mask",        0xC318},
        {"sc_dbg_transition_from_value",  0xC320},
        {"sc_dbg_transition_to_value",    0xC328},
        {"sc_dbg_ones_count_mask",        0xC330},
        {"sc_dbg_ones_count_value",       0xC338},
        {"sc_dbg_any_change",             0xC340},
        {"sc_dbg_mux_control_A",          0xC388},
        {"sc_dbg_mux_control_B",          0xC390},
        {"sc_dbg_cla_counter0_cfg",       0xD280},
        {"sc_dbg_cla_counter1_cfg",       0xD288},
        {"sc_dbg_cla_counter2_cfg",       0xD290},
        {"sc_dbg_cla_counter3_cfg",       0xD298},
        {"sc_dbg_node0_eap0",             0xD2A0},
        {"sc_dbg_node0_eap1",             0xD2A8},
        {"sc_dbg_node1_eap0",             0xD2B0},
        {"sc_dbg_node1_eap1",             0xD2B8},
        {"sc_dbg_node2_eap0",             0xD2C0},
        {"sc_dbg_node2_eap1",             0xD2C8},
        {"sc_dbg_node3_eap0",             0xD2D0},
        {"sc_dbg_node3_eap1",             0xD2D8},
        {"sc_dbg_signal_mask0",           0xD2E0},
        {"sc_dbg_signal_match0",          0xD2E8},
        {"sc_dbg_signal_mask1",           0xD2F0},
        {"sc_dbg_signal_match1",          0xD2F8},
        {"sc_dbg_signal_edge_detect_cfg", 0xD300},
        {"sc_dbg_transition_mask",        0xD318},
        {"sc_dbg_transition_from_value",  0xD320},
        {"sc_dbg_transition_to_value",    0xD328},
        {"sc_dbg_ones_count_mask",        0xD330},
        {"sc_dbg_ones_count_value",       0xD338},
        {"sc_dbg_any_change",             0xD340},
        {"sc_dbg_mux_control_A",          0xD388},
        {"sc_dbg_mux_control_B",          0xD390},
        {"sc_dbg_cla_counter0_cfg",       0xE280},
        {"sc_dbg_cla_counter1_cfg",       0xE288},
        {"sc_dbg_cla_counter2_cfg",       0xE290},
        {"sc_dbg_cla_counter3_cfg",       0xE298},
        {"sc_dbg_node0_eap0",             0xE2A0},
        {"sc_dbg_node0_eap1",             0xE2A8},
        {"sc_dbg_node1_eap0",             0xE2B0},
        {"sc_dbg_node1_eap1",             0xE2B8},
        {"sc_dbg_node2_eap0",             0xE2C0},
        {"sc_dbg_node2_eap1",             0xE2C8},
        {"sc_dbg_node3_eap0",             0xE2D0},
        {"sc_dbg_node3_eap1",             0xE2D8},
        {"sc_dbg_signal_mask0",           0xE2E0},
        {"sc_dbg_signal_match0",          0xE2E8},
        {"sc_dbg_signal_mask1",           0xE2F0},
        {"sc_dbg_signal_match1",          0xE2F8},
        {"sc_dbg_signal_edge_detect_cfg", 0xE300},
        {"sc_dbg_transition_mask",        0xE318},
        {"sc_dbg_transition_from_value",  0xE320},
        {"sc_dbg_transition_to_value",    0xE328},
        {"sc_dbg_ones_count_mask",        0xE330},
        {"sc_dbg_ones_count_value",       0xE338},
        {"sc_dbg_any_change",             0xE340},
        {"sc_dbg_mux_control_A",          0xE388},
        {"sc_dbg_mux_control_B",          0xE390},
        {"sc_dbg_cla_counter0_cfg",       0xF280},
        {"sc_dbg_cla_counter1_cfg",       0xF288},
        {"sc_dbg_cla_counter2_cfg",       0xF290},
        {"sc_dbg_cla_counter3_cfg",       0xF298},
        {"sc_dbg_node0_eap0",             0xF2A0},
        {"sc_dbg_node0_eap1",             0xF2A8},
        {"sc_dbg_node1_eap0",             0xF2B0},
        {"sc_dbg_node1_eap1",             0xF2B8},
        {"sc_dbg_node2_eap0",             0xF2C0},
        {"sc_dbg_node2_eap1",             0xF2C8},
        {"sc_dbg_node3_eap0",             0xF2D0},
        {"sc_dbg_node3_eap1",             0xF2D8},
        {"sc_dbg_signal_mask0",           0xF2E0},
        {"sc_dbg_signal_match0",          0xF2E8},
        {"sc_dbg_signal_mask1",           0xF2F0},
        {"sc_dbg_signal_match1",          0xF2F8},
        {"sc_dbg_signal_edge_detect_cfg", 0xF300},
        {"sc_dbg_transition_mask",        0xF318},
        {"sc_dbg_transition_from_value",  0xF320},
        {"sc_dbg_transition_to_value",    0xF328},
        {"sc_dbg_ones_count_mask",        0xF330},
        {"sc_dbg_ones_count_value",       0xF338},
        {"sc_dbg_any_change",             0xF340},
        {"sc_dbg_mux_control_A",          0xF388},
        {"sc_dbg_mux_control_B",          0xF390},
        {"sc_chicken_bits",               0x0040},
        {"scb_cab_chicken",               0x0fc8},
        {"scb_acb_chicken",               0x0fd0},
        {"sc_pmu_select_0",               0x0140},
        {"sc_pmu_select_1",               0x0148},
        {"sc_pmu_select_2",               0x0150},
        {"sc_pmu_select_3",               0x0158},
        {"sc_pmu_select_4",               0x0160},
        {"sc_pmu_select_5",               0x0168},
        {"sc_pmu_select_6",               0x0170},
        {"sc_pmu_select_7",               0x0178}
    }};
    inline uint64_t get_mmr_full_address(uint32_t addr_offset) { uint32_t cluster_id = 0; return generate_sc_device_addr(cluster_id) + addr_offset; }

    enum patch_mode {
        NO_PATCH = 0,
        ENTER_PATCH,
        IN_PATCH,
        EXIT_PATCH
    };

    typedef enum : size_t {
        U  = 0,
        HS = 1,
        M  = 3,
        P  = 4,
        DE = 6,
        DP = 7,
        VU = 8,
        VS = 9,
        VM = 11
    } priv;

    const std::unordered_map<priv, std::string_view> priv_to_string = {
        {U , "U"} ,
        {HS, "HS"},
        {M , "M"} ,
        {P , "P"} ,
        {DE, "DE"},
        {DP, "DP"},
        {VU, "VU"},
        {VS, "VS"},
        {VM, "M"} ,
    };

    typedef enum : size_t {
        INSN_ADDR_MISALGN       = 0,
        INSN_ACCESS_FAULT       = 1,
        ILLEGAL_INSN            = 2,
        BREAKPOINT              = 3,
        LD_ADDR_MISALGN         = 4,
        LD_ACCESS_FAULT         = 5,
        ST_AMO_ADDR_MISALGN     = 6,
        ST_AMO_ACCESS_FAULT     = 7,
        ECALL_U                 = 8,
        ECALL_S                 = 9,
        ECALL_M                 = 11,
        INSN_PAGE_FAULT         = 12,
        LD_PAGE_FAULT           = 13,
        ST_AMO_PAGE_FAULT       = 15,
        HARDWARE_ERROR          = 19,
        INST_GUEST_PAGE_FAULT   = 20,
        LD_GUEST_PAGE_FAULT     = 21,
        VIRT_INST_FAULT         = 22,
        ST_AMO_GUEST_PAGE_FAULT = 23,
        CUSTOM_SINGLE_STEP      = 31,
        CUSTOM_VLZERO_EXCP      = 39,
        CUSTOM_VEC_CMODE        = 55
    } excp;

    const std::unordered_map<excp, std::string_view> excp_to_string = {
        {INSN_ADDR_MISALGN       , "INSN_ADDR_MISALGN"}    ,
        {INSN_ACCESS_FAULT       , "INSN_ACCESS_FAULT"}    ,
        {ILLEGAL_INSN            , "ILLEGAL_INSN"}         ,
        {BREAKPOINT              , "BREAKPOINT"}           ,
        {LD_ADDR_MISALGN         , "LD_ADDR_MISALGN"}      ,
        {LD_ACCESS_FAULT         , "LD_ACCESS_FAULT"}      ,
        {ST_AMO_ADDR_MISALGN     , "ST_AMO_ADDR_MISALGN"}  ,
        {ST_AMO_ACCESS_FAULT     , "ST_AMO_ACCESS_FAULT"}  ,
        {ECALL_U                 , "ECALL_U"}              ,
        {ECALL_S                 , "ECALL_S"}              ,
        {ECALL_M                 , "ECALL_M"}              ,
        {INSN_PAGE_FAULT         , "INSN_PAGE_FAULT"}      ,
        {LD_PAGE_FAULT           , "LD_PAGE_FAULT"}        ,
        {ST_AMO_PAGE_FAULT       , "ST_AMO_PAGE_FAULT"}    ,
        {HARDWARE_ERROR          , "HARDWARE_ERROR"}    ,
        {INST_GUEST_PAGE_FAULT   , "INST_GUEST_PAGE_FAULT"},
        {LD_GUEST_PAGE_FAULT     , "LD_GUEST_PAGE_FAULT"}  ,
        {VIRT_INST_FAULT         , "VIRT_INST_FAULT"}      ,
        {ST_AMO_GUEST_PAGE_FAULT , "ST_AMO_GUEST_PAGE_FAULT"}
    };

    typedef enum : size_t {
        DEBUG       = 0,
        SSI         = 1,
        VSSI        = 2,
        MSI         = 3,
        STI         = 5,
        VSTI        = 6,
        MTI         = 7,
        SEI         = 9,
        VSEI        = 10,
        MEI         = 11,
        SGEI        = 12,
        LCOFI       = 13,
        BUS_ERRI    = 23, // NOTE: This is now configurable - use dynamic value from interrupt_pend_t.buserr_bit
        C_HWAI      = 24,
        C_ENTROPY   = 25,
        LO_PRI_RASI = 35,
        HI_PRI_RASI = 43
    } intr;

    const std::unordered_map<intr, std::string_view> intr_to_string = {
        {DEBUG          , "DEBUG"},
        {SSI            , "SSI"}  ,
        {VSSI           , "VSSI"} ,
        {MSI            , "MSI"}  ,
        {STI            , "STI"}  ,
        {VSTI           , "VSTI"} ,
        {MTI            , "MTI"}  ,
        {SEI            , "SEI"}  ,
        {VSEI           , "VSEI"} ,
        {MEI            , "MEI"}  ,
        {SGEI           , "SGEI"} ,
        {LCOFI          , "LCOFI"},
        {C_ENTROPY      , "ENTROPY_SEED"},
        {BUS_ERRI       , "BUS_ERRI"},
        {C_HWAI         , "C_HWAI"} ,
        {LO_PRI_RASI    , "LO_PRI_RASI"},
        {HI_PRI_RASI    , "HI_PRI_RASI"}
    };

    typedef enum : size_t {
        EXT_NMI = 2,
        CLA_NMI = 3
    } nmi;

    const std::unordered_map<nmi, std::string_view> nmi_to_string = {
        {EXT_NMI, "EXT_NMI"},
        {CLA_NMI, "CLA_NMI"}
    };

    typedef enum : size_t {
        LR = 2,
        SC = 3,
        AMOSWAP = 1,
        AMOADD = 0,
        AMOXOR = 4,
        AMOAND = 12,
        AMOOR = 8,
        AMOMIN = 16,
        AMOMAX = 20,
        AMOMINU = 24,
        AMOMAXU = 28
    } amo_op;

    const std::unordered_map<amo_op, std::string_view> amo_op_to_string = {
        {LR, "LR"},
        {SC, "SC"},
        {AMOSWAP, "AMOSWAP"},
        {AMOADD , "AMOADD"} ,
        {AMOXOR , "AMOXOR"} ,
        {AMOAND , "AMOAND"} ,
        {AMOOR  , "AMOOR"}  ,
        {AMOMIN , "AMOMIN"} ,
        {AMOMAX , "AMOMAX"} ,
        {AMOMINU, "AMOMINU"},
        {AMOMAXU, "AMOMAXU"}
    };

    const std::unordered_map<uint64_t, uint32_t> renamed_csr = {
        {32, MEPC},
        {33, SEPC},
        {34, VSEPC},
        {35, MSCRATCH},
        {36, SSCRATCH},
        {37, VSSCRATCH}
    };

    typedef enum : size_t {
        EXCP = 0,
        INTR = 1,
        NMI = 2
    } trap;

    typedef enum : size_t {
        M_ITF = 0,
        S_ITF = 2,
    } itf;

    const std::unordered_map<itf, std::string_view> itf_to_string = {
        {M_ITF, "M"},
        {S_ITF, "S"},
    };

}
