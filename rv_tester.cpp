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

extern "C" void rv_tester_terminate();

extern "C" {

    void rv_tester_set_scope(cvm::topology::loc_t loc) {
        svScope scope = svGetScope();
        cvm::registry::messenger.signal<svScope>(loc, scope);
    }

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
        cvm::set_logger_handler(cvm::ERROR, cvm::registry::check);
    }
}

class logger_instrument {

    public:
        logger_instrument(cvm::topology::loc_t loc, unsigned) : loc_(loc) {};

        void configure() {
            cvm::set_logger_prefix([this]() -> std::string_view {
                this->prefix = (this->clock_)? "[" + std::to_string(this->clock_) + "]" : "";
                return prefix;
            });

            cvm::registry::messenger.connect<rv_tester_transactions::logger::cycle>(loc_, [this] (const auto& c) { this->clock_ = c.clock; });
            cvm::registry::messenger.connect<svScope>(loc_, [this] (svScope s) { this->scope_ = s; });
        }

        void check() {
            cvm::registry::callbacks.push(
                scope_,
                []() {
                    return rv_tester_terminate();
                });
        }

    private:

        svScope scope_;
        cvm::topology::loc_t loc_;
        std::string prefix;
        uint64_t clock_ = 0;
};

REGISTRY_register(logger_instrument, TOP.PLATFORM, 0);
