// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

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

  pm_common() {};
  virtual ~pm_common() {};
};
