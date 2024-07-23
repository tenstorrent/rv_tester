#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"

namespace {
  constexpr uint32_t pll_ip_ver             = 0x210'3000;
  constexpr uint32_t pll_control            = 0x210'3004;
  constexpr uint32_t pll_status             = 0x210'3008;
  constexpr uint32_t pll_interrupts         = 0x210'300C;
  constexpr uint32_t pll_parameters0        = 0x210'3010;
  constexpr uint32_t pll_parameters1        = 0x210'3014;

  constexpr uint32_t cold_powerup_idx       = 4;
  constexpr uint32_t dfs_done_idx           = 0;
  constexpr uint32_t dfs_req_idx            = 0;
  constexpr uint32_t scalar_div_idx         = 16;
  constexpr uint32_t main_divider_div_idx   = 6;
  constexpr uint32_t pre_divider_div_idx    = 0;

  constexpr uint32_t rst_ctl_ip_ver         = 0x210'2000;
  constexpr uint32_t rst_ctl_cold           = 0x210'2004;
  constexpr uint32_t rst_ctl_warm           = 0x210'2008;
  constexpr uint32_t rst_ctl_nofetch        = 0x210'200C;

  constexpr uint32_t cpl_cl_cold_reset_n    = 0;
  constexpr uint32_t cpl_cl_warm_reset_n    = 0;
  constexpr uint32_t cpl_cl_no_fetch        = 0;

  constexpr uint32_t core_fuse_mmr          = 0x4200'FFF8;
  constexpr uint32_t trace_fuse_mmr         = 0x4208'FFF8;
  constexpr uint32_t aclint_fuse_mmr        = 0x4218'FFF8;
  constexpr uint32_t dm_fuse_mmr            = 0x4219'FFF8;
  constexpr uint32_t sc_fuse_mmr            = 0x421A'7FD8;
  constexpr uint32_t sw_fuse_mmr            = 0x421B'FFF8;
  constexpr uint32_t core_fuse_offset       = 0x0001'0000;

  constexpr uint32_t core_fuse_idx          = 16;
  constexpr uint32_t trace_fuse_idx         = 8;
  constexpr uint32_t dm_fuse_idx            = 9;
  constexpr uint32_t sc_fuse_idx            = 0;
  constexpr uint32_t lock_idx               = 15;

  constexpr uint32_t core_pversion_mmr          = 0x4200'5000;
  constexpr uint32_t core_pcontrol_mmr          = 0x4200'5040;
  constexpr uint32_t core_preg0_mmr             = 0x4200'5080;
  constexpr uint32_t core_preg1_mmr             = 0x4200'5088;
  constexpr uint32_t core_preg2_mmr             = 0x4200'5090;
  constexpr uint32_t core_preg3_mmr             = 0x4200'5098;
  constexpr uint32_t core_ptvec_csr             = 0x4200'3DA8;

  constexpr uint32_t smc_local_base             = 0x0210'0000;
  constexpr uint32_t cpl_sram_base              = smc_local_base +0x40000;
  constexpr uint32_t cpl_patch_ram_base         = cpl_sram_base +0x0c000;
  constexpr uint32_t cpl_patch_ram_ptrig_0     =  cpl_patch_ram_base + 0x0400;
  constexpr uint32_t cpl_patch_ram_ptrig_1     =  cpl_patch_ram_base + 0x0440;
  constexpr uint32_t cpl_patch_ram_ptrig_2     =  cpl_patch_ram_base + 0x0480;
  constexpr uint32_t cpl_patch_ram_ptrig_3     =  cpl_patch_ram_base + 0x04c0;
  constexpr uint32_t cpl_patch_ram_pbody_0     =  cpl_patch_ram_base + 0x0500;
  constexpr uint32_t cpl_patch_ram_pbody_1     =  cpl_patch_ram_base + 0x0900;
  constexpr uint32_t cpl_patch_ram_pbody_2     =  cpl_patch_ram_base + 0x0d00;
  constexpr uint32_t cpl_patch_ram_pbody_3     =  cpl_patch_ram_base + 0x01100;
  constexpr uint32_t cpl_patch_ram_pdata       =  cpl_patch_ram_base + 0x1600;


  typedef enum : bool { COLD = true, WARM = false } rst_t;
  typedef enum : size_t { SZ_4B = 4, SZ_8B = 8 } sz_t;
}

class pwrmgmt {

  public:

    pwrmgmt(cvm::topology::loc_t, unsigned) {}
    ~pwrmgmt() {}

};
