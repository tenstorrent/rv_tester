#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"

namespace {
  constexpr uint32_t tr_ram_control            = 0x4208'0000;
  constexpr uint32_t tr_ram_active_idx         = 0;
  constexpr uint32_t tr_ram_enable_idx         = 1;

  typedef enum : size_t { SZ_4B = 4, SZ_8B = 8 } sz_t;
}

class trace {

  public:

    trace(cvm::topology::loc_t, unsigned) {}
    ~trace() {}

};
