module rv_tester_clkgen #(
    parameter int CLOCK_PERIOD_PS = 500
) (
    output logic clk
);

    initial begin
        `ifdef IXCOM_COMPILE
            $ixc_ctrl("map_delays");
        `endif
        clk = '0;
        forever  #(CLOCK_PERIOD_PS*1ps/2) clk = ~clk;
    end

endmodule
