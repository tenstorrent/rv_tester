// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <cstdio>
#include <cstdlib>

#include "Vrv_tester_fifo_tb_top.h"
#include "verilated.h"

int main(int argc, char** argv, char** /*env*/) {
  VerilatedContext context;
  context.threads(1);
  context.commandArgs(argc, argv);
  Vrv_tester_fifo_tb_top top(&context);

  top.vclk = 0;
  top.vrst_ni = 0;

  int cycle = 0;
  auto half_cycle = [&]() {
    top.eval();
    top.vclk = !top.vclk;
    cycle += top.vclk;
    context.timeInc(5);
  };

  for (int i = 0; i < 10; i++)
    half_cycle();
  top.vrst_ni = 1;

  const int timeout = 20000;
  while (!context.gotFinish() && !top.test_done && cycle < timeout)
    half_cycle();
  top.eval();

  int rc = 0;
  if (context.gotFinish()) {
    printf("rv_tester_fifo_tb: simulation stopped early (assertion or $error)\n");
    rc = 1;
  } else if (!top.test_done) {
    printf("rv_tester_fifo_tb: timeout after %d cycles\n", cycle);
    rc = 1;
  } else if (!top.test_passed) {
    printf("rv_tester_fifo_tb: TEST FAILED after %d cycles\n", cycle);
    rc = 1;
  } else {
    printf("rv_tester_fifo_tb: TEST PASSED after %d cycles\n", cycle);
  }

  top.final();
  return rc;
}
