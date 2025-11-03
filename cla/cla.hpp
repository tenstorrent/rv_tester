#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "pcg_random.hpp"

namespace {

  constexpr uint32_t core_crCsrDataPort        = 0x4200'4000;
  constexpr uint32_t core_crCsrCommandPort     = 0x4200'4008;
  constexpr uint32_t core_fuse_offset          = 0x0001'0000;
  constexpr uint32_t fe_dbg_mux_sel            = 0x7C0;

  constexpr uint32_t cdbg_cla_dbg_eap_sts      = 0x4200'2188;
  constexpr uint32_t cdbg_cla_ctrl_status      = 0x4200'2190;
  constexpr uint32_t cdbg_dbg_mux_sel_cfg      = 0x4200'2198;
  constexpr uint32_t cdbg_node0_eap0_cfg       = 0x4200'2120;
  constexpr uint32_t cdbg_node0_eap1_cfg       = 0x4200'2128;
  constexpr uint32_t cdbg_node1_eap0_cfg       = 0x4200'2130;
  constexpr uint32_t cdbg_node1_eap1_cfg       = 0x4200'2138;
  constexpr uint32_t cdbg_node2_eap0_cfg       = 0x4200'2140;
  constexpr uint32_t cdbg_node2_eap1_cfg       = 0x4200'2148;
  constexpr uint32_t cdbg_node3_eap0_cfg       = 0x4200'2150;
  constexpr uint32_t cdbg_node3_eap1_cfg       = 0x4200'2158;
  constexpr uint32_t cdbg_cla_counter0         = 0x4200'2100;
  constexpr uint32_t cdbg_cla_counter1         = 0x4200'2108;
  constexpr uint32_t cdbg_cla_counter2         = 0x4200'2110;
  constexpr uint32_t cdbg_cla_counter3         = 0x4200'2118;

  constexpr uint32_t cpl_cla_offset           = 0x216'0000;
  constexpr uint32_t cpl_cla_counter0         = cpl_cla_offset + 0x3100;
  constexpr uint32_t cpl_cla_counter1         = cpl_cla_offset + 0x3108;
  constexpr uint32_t cpl_cla_counter2         = cpl_cla_offset + 0x3110;
  constexpr uint32_t cpl_cla_counter3         = cpl_cla_offset + 0x3118;
  constexpr uint32_t cpl_node0_eap0_cfg       = cpl_cla_offset + 0x3120;
  constexpr uint32_t cpl_node0_eap1_cfg       = cpl_cla_offset + 0x3128;
  constexpr uint32_t cpl_node1_eap0_cfg       = cpl_cla_offset + 0x3130;
  constexpr uint32_t cpl_node1_eap1_cfg       = cpl_cla_offset + 0x3138;
  constexpr uint32_t cpl_node2_eap0_cfg       = cpl_cla_offset + 0x3140;
  constexpr uint32_t cpl_node2_eap1_cfg       = cpl_cla_offset + 0x3148;
  constexpr uint32_t cpl_node3_eap0_cfg       = cpl_cla_offset + 0x3150;
  constexpr uint32_t cpl_node3_eap1_cfg       = cpl_cla_offset + 0x3158;
  constexpr uint32_t cpl_cla_dbg_eap_sts      = cpl_cla_offset + 0x3188;
  constexpr uint32_t cpl_cla_ctrl_status      = cpl_cla_offset + 0x3190;

  typedef enum : size_t { SZ_4B = 4, SZ_8B = 8 } sz_t;
  typedef enum : bool { BLOCK = true, NO_BLOCK = false } block_t;

}

class cla {

  public:

    cla(cvm::topology::loc_t, unsigned) {}
    ~cla() {}

};
