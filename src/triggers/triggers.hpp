// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"

namespace {
typedef enum : uint8_t { ASSERT = 1,
                         DEASSERT = 0 } signal_t;
}

class triggers {

public:
  triggers(cvm::topology::loc_t, unsigned) {}
  ~triggers() {}
};
