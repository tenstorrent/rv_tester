#include "cvm/plusargs.hpp"
#include "cvm/messenger.hpp"
#include "cvm/registry.hpp"
#include "memmap.h"

extern "C" {
    void rv_tester_parse_flags() {
        cvm::plusargs::parse();
    }

    void rv_tester_reset_messenger() {
        cvm::messenger_reset::reset();
    }

    void rv_tester_reset_registry() {
        cvm::registry::reset();
    }

    void rv_tester_parse_memmap() {
        memmap::parse();
    }
}
