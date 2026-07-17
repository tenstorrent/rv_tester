// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

module rv_tester_tick_generator
  #(
    parameter string NAME = "noname",
    parameter string EN = {NAME, "_rand_en"},
    parameter string COUNT = {NAME, "_count"},
    parameter string INTERVAL = {NAME, "_interval"},
    parameter string WIDTH = {NAME, "_width"}
    ) (
       input logic clk,
       input logic reset,
       input logic inhibit,
       output logic tick,
       output logic last
       );

  // Internal variables
  bit en;
  int unsigned count;
  int unsigned interval;
  int unsigned width;
  int unsigned tick_count;   // Keeps track of how many ticks have been generated
  int unsigned nxt_clock;    // Holds the time for the next tick
  int unsigned clocks;

  logic nxt_tick;
  logic reset_d1;
  logic generating_tick;     // Signal to indicate if we are generating a tick

  always @(posedge clk) begin
    if (!inhibit)
      clocks <= clocks + 1;
  end

  always @(posedge clk) begin
    reset_d1 <= reset;
    if (reset) begin
      // Reset all internal variables
      nxt_tick <= 1'b0;
      tick_count <= 0;
      generating_tick <= 1'b0;
    end else if (reset_d1 & ~reset) begin
      // On reset release, initialize random values
      /* verilator lint_off BLKSEQ */
      en = cvm_plusargs::get_bool(EN) != 0;
      if (en) begin
        count = cvm_rand::get(COUNT);
        interval = cvm_rand::get(INTERVAL);
        width = cvm_rand::get(WIDTH);
        $display("[%s] rand: count=%0d", NAME, count);
        $display("[%s] rand: interval=%0d, width=%0d", NAME, interval, width);
        tick_count <= 0;
        nxt_clock <= clocks + interval;
      end
      /* verilator lint_on BLKSEQ */
    end else begin
      // Handle tick generation
      if (en && (tick_count < count)) begin
        if (clocks >= nxt_clock && clocks < nxt_clock + width) begin
          nxt_tick <= 1'b1;  // Generate tick
          generating_tick <= 1'b1;
        end else begin
          nxt_tick <= 1'b0;  // Turn off tick
        end

        if (clocks >= nxt_clock + width && generating_tick) begin
          // Re-randomize interval and width after each tick
          /* verilator lint_off BLKSEQ */
          interval = cvm_rand::get(INTERVAL);
          width = cvm_rand::get(WIDTH);
          /* verilator lint_on BLKSEQ */

          // Move to the next tick after the width is reached
          nxt_clock <= clocks + interval;
          tick_count <= tick_count + 1;
          generating_tick <= 1'b0;
        end
      end else begin
        nxt_tick <= 1'b0;  // No more ticks
      end
    end
  end

  assign tick = nxt_tick;
  assign last = !en || (tick_count > (count - 1));

endmodule
