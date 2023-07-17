#include <string_view>

#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/logger.hpp"
#include "memmap.h"
#include "rv_tester_transactions.hpp"

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
DEFINE_string(gen_clocks_verbosity, "DEBUG", "verbosity at which to generate clocks with cvm::logger prints");

extern "C" void rv_tester_terminate();

class logger_instrument {

    public:
        logger_instrument(cvm::topology::loc_t loc, unsigned) : loc(loc) {};

        void configure() {
            clock = 0;

            cvm::set_logger_prefix([]() -> std::string_view {
                prefix = (clock)? "[" + std::to_string(clock) + "] " : "";
                return prefix;
            });

            cvm::registry::messenger.connect<rv_tester_transactions::logger::cycle>(loc, [] (const auto& c) { clock = c.clock; });
        }

        void check() {
            cvm::registry::callbacks.push(
                scope,
                []() {
                    return rv_tester_terminate();
                });
        }

        static void set_scope(svScope s) { scope = s; };

    private:

        static svScope scope;
        static std::string prefix;
        static uint64_t clock;
        cvm::topology::loc_t loc;
};

extern "C" {

    void rv_tester_parse_flags() {
        cvm::plusargs::parse();
    }

    void rv_tester_parse_memmap() {
        memmap::parse();
    }

    void rv_tester_build_registry() {
        cvm::registry::build();
        cvm::registry::configure();
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
        logger_instrument::set_scope(svGetScope());
        cvm::set_logger_handler(cvm::ERROR, cvm::registry::check);
    }
}

svScope logger_instrument::scope;
std::string logger_instrument::prefix;
uint64_t logger_instrument::clock;

REGISTRY_register(logger_instrument, TOP.PLATFORM, 0);
