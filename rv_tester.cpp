#include <string_view>

#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/logger.hpp"
#include "memmap.h"
#include "rv_tester_transactions.hpp"
#include "rv_tester/rv_tester_structs.h"
static bool validate_ge0(const char* flagname, const int value) {
    if (value < 0) {
        cvm::log(cvm::NONE, "Invalid value for +{}={}, must be >= 0\n", flagname, value);
        return false;
    }
    return true;
}

DEFINE_int32(quiesce_timeout, 500, "cycles to wait after eot condition before calling $finish");
DEFINE_int32(flush_timeout, 25000, "cycles to wait after flush is initiated before calling $finish");
DEFINE_bool(terminate_call_finish, true, "Call $finish on sim termination");
DEFINE_bool(bypass_mem, true, "Bypass xbar+cache switch");
DEFINE_bool(bypass_cache, false, "Bypass cache switch");
DEFINE_int32(num_reruns, 0, "Rerun the same test this many times, to test test chaining for emulation. The test is run for a total of N+1 times.");
DEFINE_bool(trace_en, false, "Set this while running trace test");
DEFINE_int32(trace_timeout, 2000, "trace test end timeout after to host end call");
DEFINE_validator(num_reruns, &validate_ge0);
DEFINE_string(gen_clocks_verbosity, "DEBUG", "verbosity at which to generate clocks with cvm::logger prints");

extern "C" void rv_tester_terminate();
extern "C" void rv_tester_set_address_map(std::uint32_t i, std::uint64_t start_addr, std::uint64_t end_addr, std::uint32_t device);

class logger_instrument {

    public:
        logger_instrument(cvm::topology::loc_t loc, unsigned) : loc(loc) {};

        void configure() {
            clock = 0;

            cvm::set_logger_prefix([]() -> std::string_view {
                prefix = (clock)? "[" + std::to_string(clock) + "] " : "";
                return prefix;
            });

            cvm::registry::messenger.connect<rv_tester_transactions::logger::cycle<>>(loc, [] (const auto& c) { clock = c.clock; });
        }

        void check() {
            // set front to true so that this skips ahead of all other messages
            // mainly to skip over a bunch of cosim transactions that could be queued up on zebu
            // processing those transactions could take a long time, especially in cases which cause whisper to also print, eg tohost writes
            cvm::registry::messenger.signal<rv_tester::terminate_called>(loc, rv_tester::terminate_called{}, true /* front */);
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

    int rv_tester_parse_flags() {
        cvm::plusargs::parse();
        return 0;
    }

    void rv_tester_parse_memmap(std::uint32_t no_addr_rules) {

        memmap::parse();

        memmap::memmap_t m;
        memmap::get(m);

        if (m.size() > no_addr_rules) {
            cvm::log(cvm::ERROR, "Test specifying more address rules ({}) than in sv ({})", m.size(), no_addr_rules);
            return;
        }

        std::uint32_t i = 0;
        for (const auto& it : m) {
            const auto& e = it.second;
            rv_tester_set_address_map(i, e.base, e.end, e.type != "memory");
            i++;
        }
        for(; i < no_addr_rules; i++) {
            rv_tester_set_address_map(i, 1, 1, 1);
        }
    }

    void rv_tester_build_registry() {
        cvm::registry::build();
        cvm::registry::configure();
    }

    uint8_t rv_tester_shutdown_registry() {
        return cvm::registry::shutdown();
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
