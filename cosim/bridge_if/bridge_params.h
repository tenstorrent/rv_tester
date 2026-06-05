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
    // struct csr_reg {
    //   uint32_t addr       = 0;
    //   std::string name    = "";
    //   bool metric         = false;
    //   bool nonzero_reset  = false;
    //   unsigned shadow_csr = 0;
    //   bool allowlist_custom_csr = false; // perform core arch checks for these allowlisted custom CSRs
    // };

    struct mmr_reg {
      std::string name = "";
      uint64_t address = 0;
    };

    struct mmr_entry {
      std::string name;
      uint32_t addr_offset;
    };

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
        CUSTOM_DBG_ENTRY        = 33,
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
        {ST_AMO_GUEST_PAGE_FAULT , "ST_AMO_GUEST_PAGE_FAULT"},
        {CUSTOM_DBG_ENTRY        , "CUSTOM_DBG_ENTRY"},
        {CUSTOM_SINGLE_STEP      , "CUSTOM_SINGLE_STEP"},
    };

    enum priv_mode : uint8_t {
        PRIV_USER           = 0,
        PRIV_SUPERVISOR     = 1,
        PRIV_MACHINE        = 3,
        PRIV_DEBUG_ROM      = 6,
        PRIV_DEBUG_PROGBUF  = 7,
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
