#include "Vtb.h"
#include "verilated.h"
#include <stdio.h>

int main(int argc, char** argv, char** /*env*/) {

  VerilatedContext context;
  context.threads(1);
  context.commandArgs(argc, argv);
  Vtb top(&context);

  printf("starting\n");

  top.vclk = 0;
  while (!Verilated::gotFinish()) {
    top.eval();
    top.vclk = !top.vclk;
    context.timeInc(5);
  }

  printf("exiting after %" PRId64 " cycles\n", context.time() / 10);
  exit(0);
}
