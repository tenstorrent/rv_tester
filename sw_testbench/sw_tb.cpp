#include <stdio.h>

#include "Vtop.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include "cvm/registry.hpp"


int main(int argc, char** argv) {

    VerilatedContext context;
    context.commandArgs(argc, argv);
    Vtop top(&context);

    printf("[main] starting\n");

    // Clock generation
    auto loc = cvm::topology::get_from_type("CLKI", 0);
    auto nclks = cvm::topology::attr(loc, "NCLKS").second;
    auto core_clk_idx = cvm::topology::attr(loc, "CORE_CLK_IDX").second;
    auto freq_mhz = cvm::topology::list_attr(loc, "CLOCK_FREQ_MHZ").second;
    std::vector<uint32_t> half_period_ps;
    std::vector<uint32_t> toggles;
    for (unsigned i=0; i<nclks; i++) {
        top.clk_ext[i] = 0;
        half_period_ps.push_back(1000000/(2*freq_mhz[i]));
        toggles.push_back(0);
    }

    // VCD dump
    std::unique_ptr<VerilatedVcdC> tfp;
    const char* vcd_cycle_on_parg = vl_mc_scan_plusargs("vcd_cycle_on=");
    int core_cycle = 0;
    int vcd_cycle_on = 0;
    bool waves_started = false;
    if (vcd_cycle_on_parg) {
        Verilated::traceEverOn(true);
        tfp = std::make_unique<VerilatedVcdC>();
        vcd_cycle_on = atoi(vcd_cycle_on_parg);
    }

    while (!Verilated::gotFinish()) {
        top.eval();

        // Clock generation
        context.timeInc(1);
        for (unsigned i=0; i<nclks; i++) {
            if ((context.time() % half_period_ps[i]) == 0) {
                top.clk_ext[i] = !top.clk_ext[i];
                toggles[i]++;
            }
        }
        core_cycle = toggles[core_clk_idx]/2;

        // VCD dump
        if (tfp && (core_cycle >= vcd_cycle_on)) {
            if (!waves_started) {
#ifdef VERILATOR_WAVES
                top.trace(tfp.get(), 99);  // Trace 99 levels of hierarchy (or see below)
#endif
                tfp->open("dump.vcd");
                waves_started = true;
            }
            tfp->dump(context.time());
        }
    }

    if (tfp) {
        tfp->flush();
        tfp->close();
    }

    printf("[main] exiting after %d core cycles\n", core_cycle);
    top.final();
    exit(0);
}

extern "C" void assert_on_dpi() {
    Verilated::assertOn(true);
}

extern "C" void assert_off_dpi() {
    Verilated::assertOn(false);
}
