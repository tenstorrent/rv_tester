// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

namespace rv_tester::clocks {

uint32_t num_profiles();
uint32_t nclks();
uint32_t freq_mhz(uint32_t profile, uint32_t clk);

} // namespace rv_tester::clocks
