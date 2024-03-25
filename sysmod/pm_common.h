#pragma once

#include <string>
#include <vector>
#include <functional>
#include <fstream>
#include <sstream>
#include "cvm/registry.hpp"
#include "cvm/messenger.hpp"
#include "cvm/topology.hpp"
#include "cvm/logger.hpp"


class pm_common {
  public:
    struct pm_common_tx_t {
          uint32_t addr;
          uint32_t data;
    };
    
    struct pll_write_tx_t {
          uint32_t addr;
          uint32_t data;
          uint32_t sideband;
    };

    struct pm_nw_write_tx_t {
          uint32_t addr;
          uint32_t data;
          uint32_t sideband;
    };
    pm_common() { };
    virtual ~pm_common() { };

};
