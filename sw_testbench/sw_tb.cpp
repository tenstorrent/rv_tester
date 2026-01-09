#include <cstdio>
#include <limits>
#include <numeric>
#include <string>

#include "Vtop.h"
#include "verilated.h"

#ifdef VERILATOR_FST
#   include "verilated_fst_c.h"
using VltTraceFile = VerilatedFstC;
#else
#   include "verilated_vcd_c.h"
using VltTraceFile = VerilatedVcdC;
#endif

#include "cvm/registry.hpp"

int main(int argc, char** argv) {

    VerilatedContext context;
    context.threads(1);
    context.commandArgs(argc, argv);
    Vtop top(&context);

    printf("[main] starting\n");

    // Clock configuration
    const auto loc = cvm::topology::get_from_type("CLKI", 0);
    const size_t nclks = static_cast<size_t>(cvm::topology::attr(loc, "NCLKS").second);
    const auto core_clk_idx = cvm::topology::attr(loc, "CORE_CLK_IDX").second;
    const auto freq_mhz = cvm::topology::list_attr(loc, "CLOCK_FREQ_MHZ").second;
    std::vector<uint64_t> half_period_ps(nclks, 0);
    std::vector<uint64_t> toggles(nclks, 0);
    uint64_t time_increment = 0;
    for (size_t i = 0; i < nclks; ++i) {
        half_period_ps[i] = 1000000ULL/(2*freq_mhz[i]);
        time_increment = std::gcd(time_increment, half_period_ps[i]);
    }

    // Waveform tracing configuration
    std::unique_ptr<VltTraceFile> tfp;

    // Pick up +vcd_cyle_on, enable tracing if present
    uint64_t vcd_cycle_on = 0;
    if (const char* const plusargp = vl_mc_scan_plusargs("vcd_cycle_on=")) {
        vcd_cycle_on = std::stoull(plusargp);
#ifdef VERILATOR_WAVES
        Verilated::traceEverOn(true);
        tfp = std::make_unique<VltTraceFile>();
#endif
    }

    // Pick up +vcd_cyle_off
    uint64_t vcd_cycle_off = std::numeric_limits<uint64_t>::max();
    if (const char* const plusargp = vl_mc_scan_plusargs("vcd_cycle_off=")) {
        vcd_cycle_off = std::stoull(plusargp);
    }

    // Number of core cycles evaluated
    uint64_t core_cycles = 0;

    // All clocks start as zero on first evaluation
    for (unsigned i=0; i<nclks; i++) {
        top.clk_ext[i] = 0;
    }

    // Initial model evaluation at time 0
    top.eval();

    // Verilator misfeature: partition instances are only created and added
    // to the context after the first evaluation, so must regiser the trace
    // callbacks after the first 'eval', but before the first 'dump'.
    if (tfp) context.trace(tfp.get(), 99, 0);

    while (true) {
        // Number of core clock cycles so far
        core_cycles = toggles[core_clk_idx]/2;

        // Dump waveforms - This must always be immediately after 'eval',
        // before time is incremented otherwise the dump will be performed as
        // if dumped at the new time point
        if (tfp) {
            if (vcd_cycle_on <= core_cycles && core_cycles < vcd_cycle_off) {
                if (!tfp->isOpen()) {
#ifdef VERILATOR_FST
                    tfp->open("dump.fst");
#else
                    tfp->open("dump.vcd");
#endif
                }
                tfp->dump(context.time());
            }
        }

        // Check if reached end of simulation
        if (Verilated::gotFinish()) break;

        // Advance time and clocks
        bool toggled = false;
        do {
            context.timeInc(time_increment);
            for (size_t i = 0; i < nclks; ++i) {
                if ((context.time() % half_period_ps[i]) == 0) {
                    top.clk_ext[i] = !top.clk_ext[i];
                    ++toggles[i];
                    toggled = true;
                }
            }
        } while (!toggled);

        // Evaluate model
        top.eval();
    }

    // Close the waveform dump, if any
    if (tfp && tfp->isOpen()) {
        tfp->flush();
        tfp->close();
    }

    printf("[main] exiting after %lu core cycles\n", core_cycles);
    top.final();
    exit(0);
}

extern "C" void assert_on_dpi() {
    Verilated::assertOn(true);
}

extern "C" void assert_off_dpi() {
    Verilated::assertOn(false);
}
