// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include <sstream> // stringstream
#include <regex>

DEFINE_string(assert_ignore, "", "Downgrade asserts matching any string from this list");

extern "C" {
void rv_tester_cvm_terminate(char* msg) {
  std::string msg_str(msg);

  // Ignore assert if it matches +assert_ignore
  if (FLAGS_assert_ignore != "") {
    std::stringstream ss(FLAGS_assert_ignore);
    std::regex pattern("error", std::regex_constants::icase);
    while (ss.good()) {
      std::string s;
      std::getline(ss, s, ',');

      if (msg_str.find(s) != std::string::npos) {
        cvm::log(cvm::NONE, "Ignoring assert due to +assert_ignore: {}\n", std::regex_replace(msg_str, pattern, ""));
        return;
      }
    }
  }

  // If not waived, cvm::ERROR
  cvm::log(cvm::ERROR, "\n{}\n", msg_str);
}
}
