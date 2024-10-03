#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"

namespace {
  typedef enum : uint8_t { ASSERT = 1, DEASSERT = 0 } signal_t;
}

class snoop_gen {

  public:

    snoop_gen(cvm::topology::loc_t, unsigned) {}
    ~snoop_gen() {}

};
