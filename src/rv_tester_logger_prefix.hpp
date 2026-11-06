// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "cvm/logger.hpp"

namespace rv_tester {

// cvm::ERROR lines start with error_prefix instead of the time stamp: triage
// scripts match "<error_prefix><message>" on a single line.
inline std::string_view format_logger_prefix(std::string& out, cvm::verbosity_level v, std::string_view error_prefix, std::uint64_t timestamp, std::uint64_t clock) {
  if (v == cvm::ERROR) {
    out = error_prefix;
  } else {
    out = (timestamp) ? "[" + std::to_string(timestamp) + "ps]" : "";
    out += (clock) ? "[" + std::to_string(clock) + "] " : "";
  }
  return out;
}

} // namespace rv_tester
