
#include "rv_tester_crash_handler.hpp"

#include <csignal>
#include <exception>
#include <stdexcept>

#include "gtest/gtest.h"

namespace {

void trigger_terminate_with_exception() {
  try {
    throw std::runtime_error("boom");
  } catch (...) {
    std::terminate();
  }
}

TEST(RvTesterCrashHandlerDeathTest, SignalHandlerReportsNameAndNumber) {
  EXPECT_DEATH(
      {
        rv_tester_install_crash_handlers();
        ::raise(SIGSEGV);
      },
      "FATAL: caught SIGSEGV \\(signal 11\\)");
}

TEST(RvTesterCrashHandlerDeathTest, SigabrtReportsNameAndNumber) {
  EXPECT_DEATH(
      {
        rv_tester_install_crash_handlers();
        ::raise(SIGABRT);
      },
      "FATAL: caught SIGABRT \\(signal 6\\)");
}

TEST(RvTesterCrashHandlerDeathTest, SignalHandlerDumpsStackTrace) {
  EXPECT_DEATH(
      {
        rv_tester_install_crash_handlers();
        ::raise(SIGSEGV);
      },
      "rv_tester stack trace");
}

TEST(RvTesterCrashHandlerDeathTest, TerminateReportsUncaughtException) {
  EXPECT_DEATH(
      {
        rv_tester_install_crash_handlers();
        trigger_terminate_with_exception();
      },
      "std::terminate called - uncaught exception: boom");
}

TEST(RvTesterCrashHandlerDeathTest, TerminateDumpsStdStacktrace) {
  EXPECT_DEATH(
      {
        rv_tester_install_crash_handlers();
        trigger_terminate_with_exception();
      },
      "std::stacktrace");
}

TEST(RvTesterCrashHandlerTest, InstallIsIdempotent) {
  rv_tester_install_crash_handlers();
  rv_tester_install_crash_handlers();
  SUCCEED();
}

} // namespace
