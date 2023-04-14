#include <stdio.h>

#include "Vtop.h"
#include "verilated.h"


static double vtime = 0;

double sc_time_stamp() {
    return vtime;
}

int main(int argc, char** argv, char** env) {

    Vtop top;
    Verilated::commandArgs(argc, argv);

    printf("starting\n");

    int toggles = 0;
    top.clk_ext = 0;
    while (!Verilated::gotFinish()) {
        vtime += 5;
        top.clk_ext = !top.clk_ext;
        toggles++;
        top.eval();
    }

    printf("exiting after %d cycles\n", toggles/2);
    top.final();
    exit(0);
}

