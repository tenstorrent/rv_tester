// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "util/clock_profiles.h"
#include <vector>
#include "cvm/logger.hpp"

namespace {

std::vector<std::vector<uint32_t>> freq_table;

}

namespace rv_tester::clocks {

uint32_t num_profiles() { return freq_table.empty() ? 0 : static_cast<uint32_t>(freq_table.size() - 1); }

uint32_t nclks() { return freq_table.empty() ? 0 : static_cast<uint32_t>(freq_table.front().size()); }

uint32_t freq_mhz(const uint32_t profile, const uint32_t clk) {
  if (profile >= freq_table.size() || clk >= freq_table[profile].size()) {
    cvm::log(cvm::ERROR, "Error: clock profile {} clock {} is outside the registered {}x{} frequency table\n", profile,
             clk, freq_table.size(), nclks());
    return 0;
  }
  return freq_table[profile][clk];
}

} // namespace rv_tester::clocks

extern "C" void rv_tester_set_clock_freq(const uint32_t profile, const uint32_t clk, const uint32_t freq_mhz) {
  if (freq_table.size() <= profile) {
    freq_table.resize(profile + 1);
  }
  if (freq_table[profile].size() <= clk) {
    freq_table[profile].resize(clk + 1);
  }
  freq_table[profile][clk] = freq_mhz;
}
