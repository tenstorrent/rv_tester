#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/callbacks.hpp"
#include "memmap.h"
#include "rv_tester.hpp"

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
}
