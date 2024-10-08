#include <cstdlib>
#include <string_view>

#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/logger.hpp"
#include "cvm/random.hpp"
#include "memmap.h"
#include "rv_tester_transactions.hpp"
#include "rv_tester/rv_tester_structs.h"
#include "sysmod/sysmod_plusargs.h"
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
DEFINE_bool(overlay_mmr_en, false, "Set this while running overlay test");
DEFINE_bool(jtag_en, false, "Set this while running jtag test");
DEFINE_bool(smc_sweep_test ,false, "Set this while running small core sram sweep test");
DEFINE_int32(dmi_poll_timeout, 50000, "Debug poll timeout after to host end call");
DEFINE_int32(trace_timeout, 50000, "trace test end timeout after to host end call");
DEFINE_int32(freq_switch_ncycles, 20000, "Switch clk frequencies after freq_switch_ncycles");
DEFINE_int32(clk_profile, 0, "Clk profile to drive various clocks");
DEFINE_bool(dyn_clk_switch, false, "Enable dynamic clk switching");
DEFINE_validator(num_reruns, &validate_ge0);
DEFINE_string(gen_clocks_verbosity, "HIGH", "verbosity at which to generate clocks with cvm::logger prints");
DEFINE_int32(assertion_test_cycle, 0, "If non-zero, assert false on this cycle. Used for testing assertion infrastructure.");
DEFINE_int32(rand_dmi_driver_dly, 0, "Random delay cycles, to be used while driving DMI transactions");
DEFINE_int32(dm_single_step_count, 0, "No of times core to single step, to be used while driving DMI transactions");

extern "C" void rv_tester_terminate();
extern "C" void rv_tester_set_address_map(std::uint32_t i, std::uint64_t start_addr, std::uint64_t end_addr, std::uint32_t device);

static bool check_called;
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
            // we want this to be low prio and async so it goes behind existing rvfi transactions in the queue
            // because of QoS this could have been seen before all rvfi transactions up to this instruction were processed
            cvm::registry::messenger.signal_async<rv_tester::terminate_called>(loc, rv_tester::terminate_called{}, cvm::messenger::lowest_priority);
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

    void rv_tester_streaming_dpi_init() {
        char *env_var = std::getenv("ZEBU_OFFLINE_DPI");
        if (env_var != nullptr && std::string(env_var) == "1") {
            cvm::plusargs::parse();
            cvm::rand::seed(FLAGS_seed);
            cvm::log(cvm::NONE, "Initialize Offline DPI\n");
        }
    }

    int rv_tester_parse_flags() {
        cvm::log(cvm::NONE, "[plusargs] Parsing...\n");
        cvm::plusargs::parse();
        return 0;
    }

    void rv_tester_set_seed() {
        cvm::log(cvm::NONE, "[random] +seed={}\n", FLAGS_seed);
        cvm::rand::seed(FLAGS_seed);
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
        check_called = false;
        cvm::registry::build();
        cvm::registry::configure();
    }

    uint8_t rv_tester_shutdown_registry() {
        if (!check_called) {
            cvm::log(cvm::NONE, "[registry] check...\n");
            cvm::registry::check();
            check_called = true;
        }

        cvm::log(cvm::NONE, "[registry] shutdown...\n");
        return cvm::registry::shutdown();
    }

    uint8_t rv_tester_flush_callbacks() {
        cvm::registry::callbacks.flush();
        // force verilator to serialize
        return true;
    }

    void rv_tester_cvm_error() {
        if (!check_called) {
            cvm::registry::check();
        }
    }

    void rv_tester_cvm_error_handler() {
        logger_instrument::set_scope(svGetScope());
        cvm::set_logger_handler(cvm::ERROR, rv_tester_cvm_error);
    }
}

svScope logger_instrument::scope;
std::string logger_instrument::prefix;
uint64_t logger_instrument::clock;

REGISTRY_register(logger_instrument, TOP.PLATFORM, 0);
