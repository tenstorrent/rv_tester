module rv_tester_clkgen #(
    parameter int CLOCK_FREQ_MHZ = 2000
) (
    output logic clk
);

    int CLOCK_PERIOD_PS = 1000000 / CLOCK_FREQ_MHZ;

    `ifdef IXCOM_COMPILE
        IXCclkgen #(CLOCK_PERIOD_PS/2) uclk (clk);
    `else
        initial begin
            clk = '0;
            forever  #(CLOCK_PERIOD_PS*1ps/2) clk = ~clk;
        end
    `endif

endmodule
