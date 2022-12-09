#include "cvm/plusargs.hpp"

extern "C" {
    void rv_tester_parse_flags() {
        cvm::plusargs::parse();
    }
}
