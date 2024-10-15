#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"

namespace {
  typedef enum : uint8_t { ASSERT = 1, DEASSERT = 0 } signal_t;
}

class jtag_driver {

  public:

    jtag_driver(cvm::topology::loc_t, unsigned) {}
    ~jtag_driver() {}

};
