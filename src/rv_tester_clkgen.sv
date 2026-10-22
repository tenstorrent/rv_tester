// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

module rv_tester_clkgen #(
                          parameter int CLOCK_FREQ_MHZ = 2000,
                          parameter int FASTEST_FREQ_MHZ = 2000
                          ) (
                             input logic fastest_clk,
                             output logic clk
                             );

  localparam int CLOCK_PERIOD_PS = CLOCK_FREQ_MHZ == 0 ? 0 : 1000000 / CLOCK_FREQ_MHZ;

`ifdef IXCOM_COMPILE
 `ifdef PALLADIUM_CAKE1X
  IXCcake1x #(.FREQ(CLOCK_FREQ_MHZ),.BFREQ(FASTEST_FREQ_MHZ)) uclk (.clkBase(fastest_clk),.clkOut(clk));
 `else
  IXCclkgen #(CLOCK_PERIOD_PS/2) uclk (clk);
 `endif
`else
 `ifdef ZEBU_CLOCK_DELAY_PORT
  clockDelayPort #(CLOCK_PERIOD_PS/2,CLOCK_PERIOD_PS/2,0) ClockPort(clk);
 `else
     logic c = '0;
     initial begin
       if (CLOCK_FREQ_MHZ == 0) begin
         $fatal(1, "%m: CLOCK_FREQ_MHZ is 0; pass the clock frequency table to rv_tester through CLK_PROFILE_FREQ_MHZ");
       end
       forever #(CLOCK_PERIOD_PS*1ps/2) c = ~c;
     end
     assign clk = c;
 `endif
`endif

endmodule
