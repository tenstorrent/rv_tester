module rv_tester_clkgen #(
    parameter int CLOCK_PERIOD_PS = 500
) (
    output logic clk
);

    `ifdef IXCOM_COMPILE
        IXCclkgen #(CLOCK_PERIOD_PS/2) uclk (clk);
    `else
        initial begin
            clk = '0;
            forever  #(CLOCK_PERIOD_PS*1ps/2) clk = ~clk;
        end
    `endif

endmodule
