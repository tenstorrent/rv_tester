#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"

namespace {
  constexpr uint32_t tr_funnel_control             = 0x4208'1000;
  constexpr uint32_t tr_ram_control                = 0x4208'0000;
  constexpr uint32_t tr_dst_ram_control            = 0x4208'2000;
  constexpr uint32_t tr_dst_ram_start_low          = 0x4208'2010;
  constexpr uint32_t tr_dst_ram_limit_low          = 0x4208'2018;
  constexpr uint32_t tr_dst_ram_wp_low            = 0x4208'2020;
  constexpr uint32_t tr_dst_ram_rp_low            = 0x4208'2028;
  constexpr uint32_t tr_dst_ram_data              = 0x4208'2040;
  constexpr uint32_t tr_dst_control               = 0x4200'2000;
  constexpr uint32_t cdbg_cla_ctrl_status         = 0x4200'2190;
  constexpr uint32_t cdbg_cla_counter0            = 0x4200'2100;
  constexpr uint32_t tr_funnel_disinput           = 0x4208'1008;
  constexpr uint32_t tr_ram_active_idx         = 0;
  constexpr uint32_t tr_ram_enable_idx         = 1;
  constexpr uint32_t tr_dst_control_empty_idx  = 3;

  typedef enum : size_t { SZ_4B = 4, SZ_8B = 8 } sz_t;
}

class trace {

  public:

    trace(cvm::topology::loc_t, unsigned) {}
    ~trace() {}

};
