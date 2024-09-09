#include <stdio.h>
#include <numeric>

#include "Vtop.h"
#include "verilated.h"
#ifdef VERILATOR_FST
    #include "verilated_fst_c.h"
#else
    #include "verilated_vcd_c.h"
#endif
#include "cvm/registry.hpp"


int main(int argc, char** argv) {

    VerilatedContext context;
    context.threads(1);
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
    uint32_t time_increment = 0;
    for (unsigned i=0; i<nclks; i++) {
        top.clk_ext[i] = 0;
        half_period_ps.push_back(1000000/(2*freq_mhz[i]));
        toggles.push_back(0);
        time_increment = std::gcd(time_increment, half_period_ps[i]);
    }

    int core_cycle = 0;

    #ifdef VERILATOR_FST 
        std::unique_ptr<VerilatedFstC> tfp;
    #else
        std::unique_ptr<VerilatedVcdC> tfp;
    #endif

    const char* vcd_cycle_on_parg = vl_mc_scan_plusargs("vcd_cycle_on=");
    int vcd_cycle_on = 0;
    bool waves_started = false;
    if (vcd_cycle_on_parg) {
        Verilated::traceEverOn(true);
        #ifdef VERILATOR_FST 
            tfp = std::make_unique<VerilatedFstC>();
        #else
            tfp = std::make_unique<VerilatedVcdC>();
        #endif
        vcd_cycle_on = atoi(vcd_cycle_on_parg);
    }

    while (!Verilated::gotFinish()) {
        top.eval();

        // Clock generation
        bool toggled = false;
        do {
            context.timeInc(time_increment);
            for (unsigned i=0; i<nclks; i++) {
                if ((context.time() % half_period_ps[i]) == 0) {
                    top.clk_ext[i] = !top.clk_ext[i];
                    toggles[i]++;
                    toggled = true;
                }
            }
        } while (!toggled);
        core_cycle = toggles[core_clk_idx]/2;

        // FST dump
        if (tfp && (core_cycle >= vcd_cycle_on)) {
            if (!waves_started) {
            #ifdef VERILATOR_WAVES
                top.trace(tfp.get(), 99);  // Trace 99 levels of hierarchy (or see below)
            #endif

            #ifdef VERILATOR_FST 
                tfp->open("dump.fst");
            #else
                tfp->open("dump.vcd");
            #endif
            
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
