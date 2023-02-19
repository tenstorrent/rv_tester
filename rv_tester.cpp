#include "cvm/plusargs.hpp"
#include "memmap.h"

extern "C" {
    void rv_tester_parse_flags() {
        cvm::plusargs::parse();
    }

    void rv_tester_parse_memmap() {
        memmap::parse();
    }
}
