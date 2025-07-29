module rv_tester_clkgen #(
    parameter int CLOCK_FREQ_MHZ = 2000,
    parameter int FASTEST_FREQ_MHZ = 2000
) (
    input logic fastest_clk,
    output logic clk
);

    localparam int CLOCK_PERIOD_PS = 1000000 / CLOCK_FREQ_MHZ;

    `ifdef IXCOM_COMPILE
        IXCcake1x #(.FREQ(CLOCK_FREQ_MHZ),.BFREQ(FASTEST_FREQ_MHZ)) uclk (.clkBase(fastest_clk),.clkOut(clk));
    `else
    `ifdef ZEBU_CLOCK_DELAY_PORT
        clockDelayPort #(CLOCK_PERIOD_PS/2,CLOCK_PERIOD_PS/2,0) ClockPort(clk);
    `else
        initial begin
            clk = '0;
            forever  #(CLOCK_PERIOD_PS*1ps/2) clk = ~clk;
        end
    `endif
    `endif

endmodule
