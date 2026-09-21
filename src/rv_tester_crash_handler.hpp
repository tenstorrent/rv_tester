#pragma once

// ---------------------------------------------------------------------------
// Global crash / terminate handling.
//
// This DPI library is invoked from the SystemVerilog simulator, so a fatal
// fault (uncaught C++ exception, SIGSEGV, SIGABRT, ...) originating in any DPI
// code normally just tears the process down with no C++ context, forcing a
// gdb rerun to find the culprit. The handlers below dump a full stack trace to
// stderr first so the failing frame is visible directly in the sim log.
//
// The signal path MUST stay async-signal-safe (backtrace()/backtrace_symbols_fd
// only). The std::terminate path runs in normal context, so it uses the richer
// C++23 std::stacktrace (see rv_tester_dump_backtrace_normal).
//
// This header is header-only and installs its handlers via a namespace-scope
// static initializer, so it must be #included in exactly one translation unit
// (rv_tester.cpp). Including it in more than one TU is harmless but would just
// re-install the same handlers.
// ---------------------------------------------------------------------------

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <initializer_list>
#include <sstream>
#include <stacktrace>
#include <execinfo.h>
#include <unistd.h>

namespace {

// Async-signal-safe write of a NUL-terminated string to stderr.
void rv_tester_safe_write(const char* s) {
  if (s == nullptr)
    return;
  ssize_t n [[maybe_unused]] = ::write(STDERR_FILENO, s, std::strlen(s));
}

// Async-signal-safe write of a non-negative integer to stderr. Avoids
// snprintf(), which is not on the async-signal-safe list; only ::write() is.
void rv_tester_safe_write_int(int value) {
  char buf[16];
  unsigned v = (value < 0) ? 0u : static_cast<unsigned>(value);
  int i = static_cast<int>(sizeof(buf));
  if (v == 0) {
    buf[--i] = '0';
  } else {
    while (v > 0 && i > 0) {
      buf[--i] = static_cast<char>('0' + (v % 10));
      v /= 10;
    }
  }
  ssize_t n [[maybe_unused]] = ::write(STDERR_FILENO, buf + i, sizeof(buf) - i);
}

// Async-signal-safe backtrace dump. backtrace()/backtrace_symbols_fd() do not
// allocate, so they are safe to call from a signal handler.
void rv_tester_dump_backtrace_safe() {
  constexpr int kMaxFrames = 128;
  void* frames[kMaxFrames];
  const int n = ::backtrace(frames, kMaxFrames);
  rv_tester_safe_write("===== rv_tester stack trace (most recent call first) =====\n");
  // Skip frame 0 (this function) and frame 1 (the handler that called us).
  const int skip = (n > 2) ? 2 : 0;
  ::backtrace_symbols_fd(frames + skip, n - skip, STDERR_FILENO);
  rv_tester_safe_write("==========================================================\n");
}

// Normal-context dump used by the std::terminate handler. Uses C++23
// std::stacktrace (one call gives demangled frames + file:line when built with
// -g). NOT async-signal-safe (std::stacktrace allocates) — the signal path must
// keep using rv_tester_dump_backtrace_safe().
void rv_tester_dump_backtrace_normal() {
  std::ostringstream os;
  os << std::stacktrace::current();
  std::fprintf(stderr,
               "===== rv_tester stack trace (std::stacktrace, most recent call first) =====\n"
               "%s\n"
               "===========================================================================\n",
               os.str().c_str());
}

const char* rv_tester_signal_name(int signum) {
  switch (signum) {
  case SIGSEGV:
    return "SIGSEGV";
  case SIGABRT:
    return "SIGABRT";
  case SIGBUS:
    return "SIGBUS";
  case SIGFPE:
    return "SIGFPE";
  case SIGILL:
    return "SIGILL";
  default:
    return "signal";
  }
}

// Handlers that were installed before us (e.g. the simulator's own crash
// tracer, such as VCS's SigHandler). We chain to them after dumping our trace
// so their annotated stack dump and orderly shutdown still run.
struct sigaction rv_tester_prev_action[NSIG];

// True if `act` is a real handler worth chaining to (not the default action).
bool rv_tester_prev_actionable(const struct sigaction& act) {
  if (act.sa_flags & SA_SIGINFO)
    return act.sa_sigaction != nullptr;
  return act.sa_handler != SIG_DFL && act.sa_handler != SIG_IGN;
}

// Signal handler. Dumps our stack trace, then hands off to whatever handler was
// installed before us (typically the simulator's) so its trace and cleanup run.
// If nothing was there, falls back to the default action (core dump + status).
void rv_tester_signal_handler(int signum) {
  static volatile sig_atomic_t in_handler = 0;
  if (in_handler) {
    // We faulted while dumping; go straight to the default action to avoid a
    // handler loop.
    std::signal(signum, SIG_DFL);
    ::raise(signum);
    return;
  }
  in_handler = 1;

  rv_tester_safe_write("\n[rv_tester] FATAL: caught ");
  rv_tester_safe_write(rv_tester_signal_name(signum));
  rv_tester_safe_write(" (signal ");
  rv_tester_safe_write_int(signum);
  rv_tester_safe_write(")\n");
  rv_tester_dump_backtrace_safe();

  if (signum > 0 && signum < NSIG && rv_tester_prev_actionable(rv_tester_prev_action[signum])) {
    // Restore and re-raise so the previous (e.g. VCS) handler runs. The signal
    // is masked while we are in here; it is delivered to the restored handler
    // once we return.
    rv_tester_safe_write("[rv_tester] chaining to previously-installed handler...\n");
    ::sigaction(signum, &rv_tester_prev_action[signum], nullptr);
  } else {
    std::signal(signum, SIG_DFL);
  }
  ::raise(signum);
}

// std::terminate handler for uncaught C++ exceptions.
void rv_tester_terminate_handler() {
  std::fflush(stdout);
  std::fprintf(stderr, "\n[rv_tester] FATAL: std::terminate called");
  if (std::exception_ptr eptr = std::current_exception()) {
    try {
      std::rethrow_exception(eptr);
    } catch (const std::exception& e) {
      std::fprintf(stderr, " - uncaught exception: %s\n", e.what());
    } catch (...) {
      std::fprintf(stderr, " - uncaught exception of unknown type\n");
    }
  } else {
    std::fprintf(stderr, " (no active exception)\n");
  }
  rv_tester_dump_backtrace_normal();
  std::fflush(stderr);
  // Hand the SIGABRT from abort() to the previously-installed handler (the
  // simulator's tracer) if there is one, so its dump/cleanup runs; otherwise
  // use the default disposition. Either way we avoid re-entering our own signal
  // handler and dumping the trace twice.
  if (rv_tester_prev_actionable(rv_tester_prev_action[SIGABRT])) {
    ::sigaction(SIGABRT, &rv_tester_prev_action[SIGABRT], nullptr);
  } else {
    std::signal(SIGABRT, SIG_DFL);
  }
  std::abort();
}

// Installs our handlers. Safe to call more than once: it runs at library load
// (below) for early coverage, and should be called again from an early DPI
// entry point (rv_tester_init) so that we sit on top of any handlers the
// simulator installs during its own startup (e.g. VCS installs its SIGSEGV
// tracer in VCS_MAIN, after our load-time install). Whatever handler we
// displace is recorded so rv_tester_signal_handler can chain to it.
void rv_tester_install_crash_handlers() {
  std::set_terminate(&rv_tester_terminate_handler);

  struct sigaction sa;
  std::memset(&sa, 0, sizeof(sa));
  sa.sa_handler = &rv_tester_signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  for (const int sig : {SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL}) {
    struct sigaction old;
    std::memset(&old, 0, sizeof(old));
    ::sigaction(sig, &sa, &old);
    // Record the displaced handler to chain to — but never record ourselves,
    // so a second install keeps the simulator's handler rather than pointing
    // our chain back at our own handler.
    if (old.sa_handler != &rv_tester_signal_handler)
      rv_tester_prev_action[sig] = old;
  }
}

const bool rv_tester_crash_handlers_installed = (rv_tester_install_crash_handlers(), true);

} // namespace
