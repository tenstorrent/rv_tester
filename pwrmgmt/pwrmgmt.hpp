#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"

namespace {
  constexpr uint32_t pll_ip_ver          = 0x210'3000;
  constexpr uint32_t pll_control         = 0x210'3004;
  constexpr uint32_t pll_status          = 0x210'3008;
  constexpr uint32_t pll_interrupts      = 0x210'300C;

  constexpr uint32_t cold_powerup_done   = 4;

  constexpr uint32_t rst_ctl_ip_ver      = 0x210'2000;
  constexpr uint32_t rst_ctl_cold        = 0x210'2004;
  constexpr uint32_t rst_ctl_warm        = 0x210'2008;
  constexpr uint32_t rst_ctl_nofetch     = 0x210'200C;

  constexpr uint32_t cpl_cl_cold_reset_n = 0;
  constexpr uint32_t cpl_cl_warm_reset_n = 0;
  constexpr uint32_t cpl_cl_no_fetch     = 0;

  constexpr uint32_t fuse_core_mmr       = 0x4200'FFF8;
  constexpr uint32_t fuse_trace_mmr      = 0x4208'FFF8;
  constexpr uint32_t fuse_aclint_mmr     = 0x4218'FFF8;
  constexpr uint32_t fuse_dm_mmr         = 0x4219'FFF8;
  constexpr uint32_t fuse_sc_mmr         = 0x421A'7FD8;
  constexpr uint32_t fuse_sw_mmr         = 0x421B'FFF8;
  constexpr uint32_t fuse_hart_offset    = 0x0001'0000;

  typedef enum : bool { COLD = true, WARM = false } rst_t;
  typedef enum : size_t { SZ_4B = 4, SZ_8B = 8 } sz_t;
}

class pwrmgmt {

  public:

    pwrmgmt(cvm::topology::loc_t, unsigned) {}
    ~pwrmgmt() {}

};
