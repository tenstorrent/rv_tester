#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/logger.hpp"
#include "memmap.h"
#include <iostream>

static bool validate_ge0(const char* flagname, const int value) {
    if (value < 0) {
        cvm::log(cvm::NONE, "Invalid value for +{}={}, must be >= 0\n", flagname, value);
        return false;
    }
    return true;
}

DEFINE_int32(quiesce_timeout, 500, "cycles to wait after eot condition before calling $finish");
DEFINE_bool(terminate_call_finish, true, "Call $finish on sim termination");
DEFINE_int32(rerun_test, 0, "Rerun the same test this many times, to test test chaining for emulation. The test is run for a total of N+1 times.");
DEFINE_validator(rerun_test, &validate_ge0);

extern "C" void rv_tester_terminate();

extern "C" {
    void rv_tester_parse_flags() {
        cvm::plusargs::parse();
    }

    void rv_tester_parse_memmap() {
        memmap::parse();
    }

    void rv_tester_build_registry() {
        cvm::registry::build();
    }

    void rv_tester_shutdown_registry() {
        cvm::registry::shutdown();
    }

    uint8_t rv_tester_flush_callbacks() {
        cvm::registry::callbacks.flush();
        // force verilator to serialize
        return true;
    }

    void rv_tester_cvm_error_handler() {
        svScope scope = svGetScope();
        cvm::set_logger_handler(cvm::ERROR, [scope]() {
            cvm::registry::callbacks.push(
                scope,
                []() {
                  return rv_tester_terminate();
                });
            });
    }
}
