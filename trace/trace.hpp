#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"

namespace {

  constexpr uint32_t core_crCsrDataPort        = 0x4200'4000;
  constexpr uint32_t core_crCsrCommandPort     = 0x4200'4008;
  constexpr uint32_t core_fuse_offset          = 0x0001'0000;
  constexpr uint32_t fe_dbg_mux_sel            = 0x7C0;

  constexpr uint32_t tr_funnel_control         = 0x4208'1000;
  constexpr uint32_t tr_funnel_disinput        = 0x4208'1008;
  constexpr uint32_t tr_ram_control            = 0x4208'0000;
  constexpr uint32_t tr_ram_start_low          = 0x4208'0010;
  constexpr uint32_t tr_ram_limit_low          = 0x4208'0018;
  constexpr uint32_t tr_ram_wp_low             = 0x4208'0020;
  constexpr uint32_t tr_ram_rp_low             = 0x4208'0028;
  constexpr uint32_t tr_ram_data               = 0x4208'0040;
  constexpr uint32_t tr_te_control             = 0x4200'1000;
  constexpr uint32_t tr_dst_ram_control        = 0x4208'2000;
  constexpr uint32_t tr_dst_ram_start_low      = 0x4208'2010;
  constexpr uint32_t tr_dst_ram_limit_low      = 0x4208'2018;
  constexpr uint32_t tr_dst_ram_wp_low         = 0x4208'2020;
  constexpr uint32_t tr_dst_ram_rp_low         = 0x4208'2028;
  constexpr uint32_t tr_dst_ram_data           = 0x4208'2040;
  constexpr uint32_t tr_dst_control            = 0x4200'2000;
  constexpr uint32_t tr_dst_impl               = 0x4200'2004;
  constexpr uint32_t tr_dst_inst_feature       = 0x4200'2008;
  constexpr uint32_t cdbg_cla_ctrl_status      = 0x4200'2190;
  constexpr uint32_t cdbg_dbg_mux_sel_cfg      = 0x4200'2198;
  constexpr uint32_t cdbg_node0_eap0_cfg       = 0x4200'2120;
  constexpr uint32_t cdbg_node0_eap1_cfg       = 0x4200'2128;
  constexpr uint32_t cdbg_node1_eap0_cfg       = 0x4200'2130;
  constexpr uint32_t cdbg_node1_eap1_cfg       = 0x4200'2138;
  constexpr uint32_t cdbg_cla_counter0         = 0x4200'2100;
  constexpr uint32_t cdbg_cla_counter1         = 0x4200'2108;
  constexpr uint32_t cdbg_cla_counter2         = 0x4200'2110;
  constexpr uint32_t cdbg_cla_counter3         = 0x4200'2118;

  constexpr uint32_t tr_dst_control_empty_idx  = 3;
  constexpr uint32_t tr_ram_active_idx         = 0;
  constexpr uint32_t tr_ram_enable_idx         = 1;
  constexpr uint32_t tr_ram_empty_idx          = 3;
  constexpr uint32_t tr_ram_mem_mode_idx       = 4;
  constexpr uint32_t tr_ram_wrap_mode_idx      = 8;
  constexpr uint32_t tr_funnel_active_idx      = 0;
  constexpr uint32_t tr_te_control_empty_idx   = 3;
  constexpr uint32_t tr_te_control_insttr_mask = 0xFFFF'FFFB;
  constexpr uint32_t tr_te_control_enable_mask = 0xFFFF'FFF9;
  constexpr uint32_t tr_te_control_active_mask = 0xFFFF'FFF8;
  constexpr uint32_t tr_ram_enable_mask        = 0xFFFF'FFFD;
  constexpr uint32_t tr_ram_active_mask        = 0xFFFF'FFFC;
  constexpr uint32_t tr_ram_mem_mode_mask      = 0xFFFF'FFEF;
  constexpr uint32_t tr_ram_wrap_mode_mask     = 0xFFFF'FEFF;
  constexpr uint32_t tr_end_indicator_val      = 0xC001'C0DE;


  typedef enum : size_t { SZ_4B = 4, SZ_8B = 8 } sz_t;
  typedef enum : bool { BLOCK = true, NO_BLOCK = false } block_t;
  typedef enum : size_t { ENABLE, DISABLE } trace_ram_status_t;

}

class trace {

  public:

    trace(cvm::topology::loc_t, unsigned) {}
    ~trace() {}

};
