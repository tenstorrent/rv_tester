#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/logger.hpp"
#include "memmap.h"
#include <iostream>

DEFINE_int32(quiesce_timeout, 500, "cycles to wait after eot condition before calling $finish");
DEFINE_bool(terminate_call_finish, true, "Call $finish on sim termination");

extern "C" void rv_tester_terminate();

extern "C" {
    void rv_tester_parse_flags() {
        cvm::plusargs::parse();
    }

    void rv_tester_parse_memmap() {
        memmap::parse();
    }

    void rv_tester_reset_registry() {
        cvm::registry::reset();
    }

    void rv_tester_flush_callbacks() {
        cvm::registry::callbacks.flush();
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
