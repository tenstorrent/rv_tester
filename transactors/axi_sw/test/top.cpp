#include "Vtb.h"
#include "verilated.h"
#include <stdio.h>

static double vtime = 0;

double sc_time_stamp() {
    return vtime;
}

int main(int argc, char** argv, char** env) {

    Vtb top;
    Verilated::commandArgs(argc, argv);

    printf("starting\n");

    top.vclk = 0;
    while (!Verilated::gotFinish()) {
        top.eval();
        top.vclk = !top.vclk;
        vtime += 5;
    }

    printf("exiting after %f cycles\n", vtime/10);
    exit(0);
}
