// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "rv_tester_logger_prefix.hpp"

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include "cvm/logger.hpp"
#include "gtest/gtest.h"

namespace {

std::string prefix;
std::string error_prefix = "Error: ";
std::uint64_t timestamp = 0;
std::uint64_t clock_count = 0;

class LoggerPrefixTest : public ::testing::Test {
protected:
  void SetUp() override {
    error_prefix = "Error: ";
    timestamp = 0;
    clock_count = 0;
    cvm::set_logger_prefix([](cvm::verbosity_level v) -> std::string_view {
      return rv_tester::format_logger_prefix(prefix, v, error_prefix, timestamp, clock_count);
    });
  }

  template <typename... Args>
  std::string log(cvm::verbosity_level v, std::string_view format, Args&&... args) {
    std::ostringstream out;
    cvm::logger::log(out, v, format, std::forward<Args>(args)...);
    return out.str();
  }
};

TEST_F(LoggerPrefixTest, ErrorGetsDefaultPrefixOnOneLine) {
  EXPECT_EQ(log(cvm::ERROR, "Test specifying more address rules ({}) than in sv ({})\n", 5, 4),
            "Error: Test specifying more address rules (5) than in sv (4)\n");
}

TEST_F(LoggerPrefixTest, ErrorUsesConfiguredPrefix) {
  error_prefix = "ERROR: ";
  EXPECT_EQ(log(cvm::ERROR, "[memdump] empty token in expression\n"), "ERROR: [memdump] empty token in expression\n");
}

TEST_F(LoggerPrefixTest, ErrorPrefixCanBeEmpty) {
  error_prefix = "";
  EXPECT_EQ(log(cvm::ERROR, "pc mismatch\n"), "pc mismatch\n");
}

TEST_F(LoggerPrefixTest, ErrorDoesNotCarryTimeStamp) {
  timestamp = 617000;
  clock_count = 1234;
  EXPECT_EQ(log(cvm::ERROR, "pc mismatch\n"), "Error: pc mismatch\n");
}

TEST_F(LoggerPrefixTest, NonErrorGetsTimeStamp) {
  timestamp = 617000;
  clock_count = 1234;
  EXPECT_EQ(log(cvm::NONE, "[registry] check...\n"), "[617000ps][1234] [registry] check...\n");
}

TEST_F(LoggerPrefixTest, NonErrorWithoutTimeStampIsUnprefixed) {
  EXPECT_EQ(log(cvm::NONE, "[registry] check...\n"), "[registry] check...\n");
}

} // namespace
