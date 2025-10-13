#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include <map>

// Define MOVELLUS_PLL_MODEL by default unless explicitly overridden
#ifndef MOVELLUS_PLL_MODEL
#define MOVELLUS_PLL_MODEL
#endif

namespace {
  constexpr uint32_t pll_ip_ver             = 0x210'3000;
  constexpr uint32_t pll_control            = 0x210'3004;
  constexpr uint32_t pll_status             = 0x210'3008;
  constexpr uint32_t pll_interrupts         = 0x210'300C;
  constexpr uint32_t pll_parameters0        = 0x210'3014;  // Address 014 from Ascalon_cpl_regs.yml
  constexpr uint32_t pll_parameters1        = 0x210'3018;  // Address 018 from Ascalon_cpl_regs.yml

  constexpr uint32_t rst_ctl_nofetch_cfg_done_idx = 31;
  constexpr uint32_t rst_ctl_nofetch_clustercorego_idx = 27;
  constexpr uint32_t cold_powerup_idx       = 4;
  constexpr uint32_t dfs_done_idx           = 0;
  constexpr uint32_t dfs_req_idx            = 0;
  constexpr uint32_t wakeup_done_idx        = 2;
  constexpr uint32_t wakeup_req_idx         = 2;
  constexpr uint32_t scalar_div_idx         = 16;
  constexpr uint32_t main_divider_div_idx   = 6;
  constexpr uint32_t pre_divider_div_idx    = 0;

  constexpr uint32_t cpl_core_reset_csr     = 0x211'0020;
  constexpr uint32_t rst_ctl_ip_ver         = 0x210'2000;
  constexpr uint32_t rst_ctl_cold           = 0x210'2004;
  constexpr uint32_t rst_ctl_warm           = 0x210'2008;
  constexpr uint32_t rst_ctl_nofetch        = 0x210'200C;

  constexpr uint32_t cpl_cl_cold_reset_n    = 0;
  constexpr uint32_t cpl_cl_warm_reset_n    = 0;
  constexpr uint32_t cpl_cl_no_fetch        = 0;
  constexpr uint32_t cpl_force_ss_to_ref_clock_n = 1;

  constexpr uint32_t aclint_mtime_mmr       = 0x4218'0000;
  constexpr uint32_t core_fuse_mmr          = 0x4200'FFF8;
  constexpr uint32_t trace_fuse_mmr         = 0x4208'FFF8;
  constexpr uint32_t aclint_fuse_mmr        = 0x4218'FFF8;
  constexpr uint32_t dm_fuse_mmr            = 0x4219'FFF8;
  constexpr uint32_t sc_fuse_mmr            = 0x421A'7FD8;
  constexpr uint32_t sw_fuse_mmr            = 0x421B'FFF8;
  constexpr uint32_t core_fuse_offset       = 0x0001'0000;
  constexpr uint32_t core_physical_id_mmr   = 0x4200'2FF8;
  constexpr uint32_t sw_fuse_default_val    = 0x4200'2FF8;
  constexpr uint32_t dst_control_mmr        = 0x4200'2000;
  constexpr uint32_t trace_control_mmr      = 0x4200'1000;
  constexpr uint32_t core_cla_ctrl_status_mmr = 0x4200'2190;
  constexpr uint32_t cr_chicken_bits_mmr    = 0x4200'FFF0;

  constexpr uint32_t lock_idx               = 63;
  constexpr uint32_t dst_fuse_idx           = 52;
  constexpr uint32_t trace_fuse_idx         = 51;
  constexpr uint32_t cla_fuse_idx           = 50;
  constexpr uint32_t dm_fuse_idx            = 48;
  constexpr uint32_t core_fuse_idx          = 16;
  constexpr uint32_t io_cohr_fuse_idx       = 11;
  constexpr uint32_t exp_ctrl_fuse_idx      = 10;
  constexpr uint32_t sc_fuse_idx            = 0;

  constexpr uint32_t core_pversion_mmr          = 0x4200'5000;
  constexpr uint32_t core_pcontrol_mmr          = 0x4200'5040;
  constexpr uint32_t core_preg0_mmr             = 0x4200'5080;
  constexpr uint32_t core_preg1_mmr             = 0x4200'5088;
  constexpr uint32_t core_preg2_mmr             = 0x4200'5090;
  constexpr uint32_t core_preg3_mmr             = 0x4200'5098;
  constexpr uint32_t core_resetvector_mmr       = 0x4200'5300;
  
  constexpr uint32_t core_ptvec_csr             = 0x0000'07CA;
  constexpr uint32_t core_pwr_throttle_cfg_0    = 0x0000'0BC6;
  constexpr uint32_t core_pwr_throttle_cfg_1    = 0x0000'0BC7;
  
  constexpr uint32_t core_crCsrCommandPort      = 0x4200'4008;
  constexpr uint32_t core_crCsrDataPort         = 0x4200'4000;


  constexpr uint32_t smc_local_base                  =  0x0210'0000;
  constexpr uint32_t cpl_sram_base                   =  smc_local_base + 0x40000;
  constexpr uint32_t cpl_sram_fuse_cfg               =  smc_local_base + 0x49000;
  constexpr uint32_t cpl_sram_core_reset_vector_cfg  =  smc_local_base + 0x49008;
  constexpr uint32_t cpl_sram_cstate_limit_offset    =  smc_local_base + 0x481C0;
  constexpr uint32_t cpl_sram_limit             =  smc_local_base + 0x4FFFF;
  constexpr uint32_t cpl_sram_thub_config_base  =  cpl_sram_base + 0x81D0;
  constexpr uint32_t cpl_patch_ram_base         =  cpl_sram_base + 0x0c000;
  constexpr uint32_t cpl_patch_ram_ptrig_0      =  cpl_patch_ram_base + 0x0400;
  constexpr uint32_t cpl_patch_ram_ptrig_1      =  cpl_patch_ram_base + 0x0440;
  constexpr uint32_t cpl_patch_ram_ptrig_2      =  cpl_patch_ram_base + 0x0480;
  constexpr uint32_t cpl_patch_ram_ptrig_3      =  cpl_patch_ram_base + 0x04c0;
  constexpr uint32_t cpl_patch_ram_pbody_0      =  cpl_patch_ram_base + 0x0500;
  constexpr uint32_t cpl_patch_ram_pbody_1      =  cpl_patch_ram_base + 0x0900;
  constexpr uint32_t cpl_patch_ram_pbody_2      =  cpl_patch_ram_base + 0x0d00;
  constexpr uint32_t cpl_patch_ram_pbody_3      =  cpl_patch_ram_base + 0x1100;
  constexpr uint32_t cpl_patch_ram_pdata        =  cpl_patch_ram_base + 0x1500;
  constexpr uint32_t cpl_in_filter0_config      =  smc_local_base + 0x15000;
  constexpr uint32_t cpl_in_filter0_addr_l      =  smc_local_base + 0x15008;
  constexpr uint32_t cpl_in_filter0_addr_h      =  smc_local_base + 0x15010;
  constexpr uint32_t cpl_in_filter1_config      =  smc_local_base + 0x15020;
  constexpr uint32_t cpl_in_filter1_addr_l      =  smc_local_base + 0x15028;
  constexpr uint32_t cpl_in_filter1_addr_h      =  smc_local_base + 0x15030;
  constexpr uint32_t cpl_in_filter2_config      =  smc_local_base + 0x15040;
  constexpr uint32_t cpl_in_filter2_addr_l      =  smc_local_base + 0x15048;
  constexpr uint32_t cpl_in_filter2_addr_h      =  smc_local_base + 0x15050;
  constexpr uint32_t cpl_in_filter3_config      =  smc_local_base + 0x15060;
  constexpr uint32_t cpl_in_filter3_addr_l      =  smc_local_base + 0x15068;
  constexpr uint32_t cpl_in_filter3_addr_h      =  smc_local_base + 0x15070;
  constexpr uint32_t cpl_in_filter4_config      =  smc_local_base + 0x15080;
  constexpr uint32_t cpl_in_filter4_addr_l      =  smc_local_base + 0x15088;
  constexpr uint32_t cpl_in_filter4_addr_h      =  smc_local_base + 0x15090;
  constexpr uint32_t cpl_in_filter5_config      =  smc_local_base + 0x150A0;
  constexpr uint32_t cpl_in_filter5_addr_l      =  smc_local_base + 0x150A8;
  constexpr uint32_t cpl_in_filter5_addr_h      =  smc_local_base + 0x150B0;
  constexpr uint32_t cpl_in_filter6_config      =  smc_local_base + 0x150C0;
  constexpr uint32_t cpl_in_filter6_addr_l      =  smc_local_base + 0x150C8;
  constexpr uint32_t cpl_in_filter6_addr_h      =  smc_local_base + 0x150D0;
  constexpr uint32_t cpl_out_filter0_config     =  smc_local_base + 0x16000;
  constexpr uint32_t cpl_out_filter0_addr_l     =  smc_local_base + 0x16008;
  constexpr uint32_t cpl_out_filter0_addr_h     =  smc_local_base + 0x16010;
  
 

  constexpr uint32_t pm_mbox_base          =  0x0217'0000;
  constexpr uint32_t pm_mbox_reg           =  pm_mbox_base + 0x08;
  constexpr uint32_t pm_mbox_regdata       =  pm_mbox_base + 0x10;

  constexpr uint8_t thub_reg_base           =  0x0;
  constexpr uint8_t thub_control_reg        =  thub_reg_base + 0x4;
  constexpr uint8_t thub_threhold_param_reg =  thub_reg_base + 0x50;

  typedef enum : bool { COLD = true, WARM = false } rst_t;
  typedef enum : size_t { SZ_4B = 4, SZ_8B = 8 } sz_t;
  typedef enum : bool { WR = 1, RD = 0 } access_t;
  typedef enum : bool { BLOCK = true, NO_BLOCK = false } block_t;
  typedef enum : int { CPL_SRAM = 0, CORE_CSR = 1, MMR_PMNW = 2 } smc_dest_path_t;
  typedef struct { uint32_t addr; uint64_t data; sz_t sz; } smc_scratchpad_info_t;
  typedef enum : int { SMC, OVERLAY, INTF_COUNT } interface_t;
  const std::unordered_map<interface_t, std::string_view> port_to_string = {{OVERLAY, "OVERLAY"}, {SMC, "SMC"}};

  constexpr uint32_t dm_scratchpad      = 0x4219FFE8;
  constexpr uint32_t cr_scratchpad      = 0x42002400;
  constexpr uint32_t sw_scratchpad      = 0x421BFFF0;
  constexpr uint32_t ac_scratchpad      = 0x4218FFF0;
  constexpr uint32_t rc_scratchpad      = 0x210'2010;
  constexpr uint32_t cc_scratchpad      = 0x210'3024;
  constexpr uint32_t mb_scratchpad      = pm_mbox_base + 0xe8;
  constexpr uint32_t tr_scratchpad      = 0x4208'FFE8;
  
  constexpr uint64_t dm_scratchpad_rst  = 0xAFAFAFAFAFAFAFAF;
  constexpr uint64_t cr_scratchpad_rst  = 0xBFBFBFBFBFBFBFBF;
  constexpr uint64_t sw_scratchpad_rst  = 0xCFCFCFCFCFCFCFCF;
  constexpr uint64_t ac_scratchpad_rst  = 0xDFDFDFDFDFDFDFDF;
  constexpr uint32_t rc_scratchpad_rst  = 0xF4F4F4F4;
  constexpr uint32_t cc_scratchpad_rst  = 0xF5F5F5F5;
  constexpr uint64_t mb_scratchpad_rst  = 0x8F8F8F8F8F8F8F8F;
  constexpr uint64_t core_pwr_throttle_cfg_0_rst = 0xAFAFAFAFAFAFAFAF;
  constexpr uint64_t core_pwr_throttle_cfg_1_rst = 0xAFAFAFAFAFAFAFAF;

  std::vector<smc_scratchpad_info_t> smc_scratchpad_info = {
    {dm_scratchpad, dm_scratchpad_rst, SZ_8B},
    {cr_scratchpad, cr_scratchpad_rst, SZ_8B},
    {sw_scratchpad, sw_scratchpad_rst, SZ_8B},
    {ac_scratchpad, ac_scratchpad_rst, SZ_8B},
    {rc_scratchpad, rc_scratchpad_rst, SZ_4B},
    {cc_scratchpad, cc_scratchpad_rst, SZ_4B},
    {mb_scratchpad, mb_scratchpad_rst, SZ_8B}
  };

  std::vector<smc_scratchpad_info_t> smc_csr_info = {
    {core_pwr_throttle_cfg_0, core_pwr_throttle_cfg_0_rst, SZ_8B},
    {core_pwr_throttle_cfg_1, core_pwr_throttle_cfg_1_rst, SZ_8B}
  };


  std::map<uint32_t, uint64_t> fuse_data;

  std::map<uint32_t, uint64_t> patch_ram;
  uint64_t pcontrol_enable_mask;
  uint64_t pcontrol_data;


  // Structure to represent a patch entry
  struct PatchEntry {
      std::string patchTag;
      std::vector<uint32_t> ucodes;
      uint32_t patchInstruction;
      uint32_t patchMask;
      uint32_t enableMask;
  };

  //Map to hold the patch_routines, patch_mask, patch_opcode, enable_mask
  std::map<std::string, PatchEntry> patches; 

  std::vector<uint32_t> patch_trig_0 = {
    0x4214cfb7,        	//lui	t6,0x4214c
    0x500f8f9b,        	//addiw	t6,t6,1280 # 4214c500 <tohost-0x2deb3b00>
    0x000f8067,        	//jr	t6
  };
  std::vector<uint32_t> patch_trig_1 = {
    0x4214dfb7,        	//lui	t6,0x4214d
    0x900f8f9b,        	//addiw	t6,t6,-1792 # 4214c900 <tohost-0x2deb3700>
    0x000f8067,        	//jr	t6
  };
  std::vector<uint32_t> patch_trig_2 = {
    0x4214dfb7,         //lui	t6,0x4214d
    0xd00f8f9b,         //addiw	t6,t6,-768 # 4214cd00 <tohost-0x2deb3300>
    0x000f8067,         //jr	t6
  };
  std::vector<uint32_t> patch_trig_3 = {
    0x4214dfb7,         //lui	t6,0x4214d
    0x100f8f9b,         //addiw	t6,t6,256 # 4214d100 <tohost-0x2deb2f00>
    0x000f8067,         //jr	t6
  };

  // PLL Register Bit Definitions
  // PLL Status Register bits
  constexpr uint32_t PLL0_ACTIVE_BIT = 0;
  constexpr uint32_t PLL0_LOCKED_BIT = 1;  
  constexpr uint32_t PLL0_RESET_BIT = 2;
  constexpr uint32_t PLL1_ACTIVE_BIT = 16;
  constexpr uint32_t PLL1_LOCKED_BIT = 17;
  constexpr uint32_t PLL1_RESET_BIT = 18;
  
  // PLL Control Register bits
  constexpr uint32_t PLL_DFS_REQ_BIT = 0;
  constexpr uint32_t PLL_SHUTDOWN_REQ_BIT = 1;
  constexpr uint32_t PLL_WAKEUP_REQ_BIT = 2;
  constexpr uint32_t PLL_BYPASS_BIT = 3;
  constexpr uint32_t PLL_SCALAR_MODE_BIT = 4;
  constexpr uint32_t PLL_RESETB_BIT = 5;
  constexpr uint32_t PLL_RESETB_OVERRIDE_BIT = 6;
  constexpr uint32_t DIS_INACTIVE_PLL_SHUTDOWN_BIT = 7;
  
  // PLL Interrupt Register bits
  constexpr uint32_t DFS_DONE_BIT = 0;
  constexpr uint32_t SHUTDOWN_DONE_BIT = 1;
  constexpr uint32_t WAKEUP_DONE_BIT = 2;
  constexpr uint32_t PLL_LOCK_LOST_BIT = 3;
  constexpr uint32_t COLD_POWERUP_DONE_BIT = 4;

  
  // PLL Programming Utility Functions
  namespace pll_utils {
    
    struct pll_params_t {
      uint32_t fcw_int;
      uint32_t postdiv;
      uint32_t freq_ratio;
      uint32_t scalar_div;
      uint32_t main_divider_div;
      uint32_t pre_divider_div;
    };
    
    // Calculate PLL parameters for given frequency
    inline pll_params_t calculate_pll_params(uint32_t target_freq_mhz, uint32_t ref_freq_mhz = 100) {
      pll_params_t params = {};
      
#ifdef MOVELLUS_PLL_MODEL
      // According to MOVELLUS Equation:
      // Fout = Fcw_int * (1 / (2**postdiv)) * Ref
      // Supported frequencies: 1000, 1600, 2000, 2500, 2700 MHz
      switch(target_freq_mhz) {
        case 1000: params.fcw_int = 20; params.postdiv = 1; break;  // 1GHz
        case 1600: params.fcw_int = 32; params.postdiv = 1; break;  // 1.6GHz  
        case 2000: params.fcw_int = 40; params.postdiv = 1; break;  // 2.0GHz  
        case 2500: params.fcw_int = 50; params.postdiv = 1; break;  // 2.5GHz  
        case 2700: params.fcw_int = 54; params.postdiv = 1; break;  // 2.7GHz
        default:
          // For other frequencies, calculate dynamically
          // Solve: target_freq = fcw_int * ref_freq / (2**postdiv)
          params.postdiv = 1;
          params.fcw_int = target_freq_mhz / (ref_freq_mhz / (1 << params.postdiv));
          if (params.fcw_int > 63) {
            params.postdiv = 2;
            params.fcw_int = target_freq_mhz / (ref_freq_mhz / (1 << params.postdiv));
          }
          if (params.fcw_int > 63) {
            params.postdiv = 3;
            params.fcw_int = target_freq_mhz / (ref_freq_mhz / (1 << params.postdiv));
          }
          break;
      }
#else
      // Generic PLL model
      params.freq_ratio = 2400 / target_freq_mhz;
      params.scalar_div = 1;
      params.main_divider_div = 52;
      params.pre_divider_div = params.freq_ratio;
#endif
      return params;
    }
    
    // Configure PLL Parameters0 register with calculated values
    inline uint32_t configure_pll_parameters0(uint32_t current_value, const pll_params_t& params) {
#ifdef MOVELLUS_PLL_MODEL
      // Configure postdiv in bits [27:25]
      return (current_value & ~(0x7 << 25)) | (params.postdiv << 25);
#else
      // Configure Generic PLL fields
      uint32_t new_value = current_value;
      new_value = (new_value & ~(0x3F << 0)) | (params.pre_divider_div << 0);        // [5:0]
      new_value = (new_value & ~(0x3F << 6)) | (params.main_divider_div << 6);       // [11:6]
      new_value = (new_value & ~(0x1 << 16)) | (params.scalar_div << 16);            // [16:16]
      return new_value;
#endif
    }
    
    // Configure PLL Parameters1 register with calculated values
    inline uint32_t configure_pll_parameters1(uint32_t current_value, const pll_params_t& params) {
#ifdef MOVELLUS_PLL_MODEL
      // Configure fcw_int in bits [27:20]
      return (current_value & ~(0xFF << 20)) | (params.fcw_int << 20);
#else
      // Generic PLL doesn't use parameters1 register typically
      return current_value;
#endif
    }
  }
  
  // NTrace MMRs 
  constexpr uint32_t tr_ram_control            = 0x4208'0000;
  constexpr uint32_t tr_ram_start_low          = 0x4208'0010;
  constexpr uint32_t tr_ram_limit_low          = 0x4208'0018;
  constexpr uint32_t tr_ram_wp_low             = 0x4208'0020;
  constexpr uint32_t tr_ram_rp_low             = 0x4208'0028; 
  constexpr uint32_t tr_funnel_control         = 0x4208'1000;
  constexpr uint32_t tr_funnel_disinput        = 0x4208'1008;
  constexpr uint32_t cdbg_tr_frame_cfg         = 0x4200'21A8;
  constexpr uint32_t tr_te_control             = 0x4200'1000;

}

class pwrmgmt {

  public:

    pwrmgmt(cvm::topology::loc_t, unsigned) {}
    ~pwrmgmt() {}

};
