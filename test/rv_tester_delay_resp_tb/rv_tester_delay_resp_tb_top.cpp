#include <cinttypes>
#include "Vrv_tester_delay_resp_tb_top.h"
#include "svdpi.h"
#include "verilated.h"
#include <stdio.h>
#include <iostream>
#include <cassert>
#ifdef VERILATOR_FST
#include "verilated_fst_c.h"
#else
#include "verilated_vcd_c.h"
#endif

int main(int argc, char** argv, char** /*env*/) {

  VerilatedContext context;
  context.threads(1); // Use single thread to avoid race conditions
  context.commandArgs(argc, argv);
  Vrv_tester_delay_resp_tb_top top(&context);

  printf("Starting synthesizable testbench\n");

#ifdef VERILATOR_FST
  std::unique_ptr<VerilatedFstC> tfp;
#else
  std::unique_ptr<VerilatedVcdC> tfp;
#endif

  // Start waveform dumping from the beginning (before reset)
  Verilated::traceEverOn(true);
#ifdef VERILATOR_FST
  tfp = std::make_unique<VerilatedFstC>();
#else
  tfp = std::make_unique<VerilatedVcdC>();
#endif

  int cycle = 0;
  top.vclk = 0;
  top.vrst_ni = 0; // Start with reset active (active low)

  // Initialize waveform dumping before reset
  if (tfp) {
#ifdef VERILATOR_WAVES
    top.trace(tfp.get(), 99); // Trace 99 levels of hierarchy
#endif

#ifdef VERILATOR_FST
    tfp->open("dump.fst");
#else
    tfp->open("dump.vcd");
#endif
  }

  // Reset for a few cycles - ensure proper clock edges during reset
  for (int i = 0; i < 10; i++) {
    top.eval();
    top.vclk = !top.vclk;
    cycle += top.vclk % 2; // Only increment on positive edge
    context.timeInc(5);
    printf("RESETTING CYCLE %d\n", cycle);
    if (tfp) {
      tfp->dump(context.time());
    }
  }

  // Release reset - ensure we do this on a clock edge
  top.vrst_ni = 1;
  printf("Reset released, starting test\n");
  printf("Initial state: test_done=%d\n", (int)top.test_done);

  // Run until test completion or timeout
  int timeout = 10000; // 10k cycles timeout
  while (!Verilated::gotFinish() && cycle < timeout) {
    top.eval();
    top.vclk = !top.vclk;
    cycle += top.vclk % 2; // Only increment on positive edge
    context.timeInc(5);

    if (tfp) {
      tfp->dump(context.time());
    }

    // Check for test completion
    if (top.test_done) {
      printf("Test completed after %d cycles\n", cycle);
      if (top.test_passed) {
        printf("TEST PASSED\n");
      } else {
        printf("TEST FAILED\n");
      }
      break;
    }

    // Print periodic status
    if (cycle % 100 == 0) {
      printf("Cycle %d: test_done=%d, test_passed=%d\n",
             cycle, (int)top.test_done, (int)top.test_passed);
    }
  }

  if (cycle >= timeout) {
    printf("Test timeout after %d cycles\n", cycle);
    exit(1);
  }

  printf("Exiting after %" PRId64 " cycles\n", context.time() / 10);

  if (tfp) {
    tfp->flush();
    tfp->close();
  }

  top.final();

  // Return 0 for pass, 1 for fail
  exit(0);
}