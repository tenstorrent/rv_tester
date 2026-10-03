// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// Self-checking rv_tester_fifo test. Each checker drives one FIFO instance
// and compares q/size/empty/full against a reference model every cycle, in
// three phases:
//   1. hold one entry and pop + push it every cycle (the pushed entry becomes
//      the head immediately, so the RAM read alone would return stale data)
//   2. repeatedly fill to full and drain to empty (pointer wrap, full/empty)
//   3. random push/pop with occupancy bias changing every 256 cycles
module rv_tester_fifo_tb_checker #(
    parameter int unsigned D    = 16,
    parameter logic [31:0] SEED = 32'h1
) (
    input  logic clk,
    input  logic rst_ni,
    output logic done,
    output logic ok
);
  typedef logic [15:0] data_t;

  localparam int unsigned HOLD_CYCLES   = 64;
  localparam int unsigned FILL_CYCLES   = 8 * D + 16;
  localparam int unsigned RANDOM_CYCLES = 4096;
  localparam int unsigned END_CYCLE     = HOLD_CYCLES + FILL_CYCLES + RANDOM_CYCLES;
  localparam int unsigned MD            = (D < 2) ? 2 : D;
  localparam int unsigned AW            = $clog2(MD);
  localparam int unsigned MAX_REPORTS   = 10;

  typedef logic [AW-1:0] idx_t;

  // DUT
  logic  push, pop, full, empty;
  data_t d, q;
  logic [$clog2(D+1)-1:0] size;

  rv_tester_fifo #(
      .D(D),
      .T(data_t)
  ) dut (
      .clk    (clk),
      .reset_n(rst_ni),
      .push   (push),
      .d      (d),
      .pop    (pop),
      .q      (q),
      .size   (size),
      .full   (full),
      .empty  (empty)
  );

  // reference model
  data_t       mem_m [MD];
  idx_t        head_m;
  int unsigned cnt_m;

  // stimulus
  int unsigned cyc;
  logic [31:0] rnd;
  logic [3:0]  push_bias, pop_bias;
  logic        filling;
  logic        push_req, pop_req;

  always_comb begin
    push_req = 1'b0;
    pop_req  = 1'b0;
    if (cyc < HOLD_CYCLES) begin
      push_req = 1'b1;
      pop_req  = cnt_m != 0;
    end else if (cyc < HOLD_CYCLES + FILL_CYCLES) begin
      push_req = filling;
      pop_req  = !filling;
    end else if (cyc < END_CYCLE) begin
      push_req = rnd[3:0] < push_bias;
      pop_req  = rnd[7:4] < pop_bias;
    end
  end

  assign push = rst_ni && !done && push_req && !full;
  assign pop  = rst_ni && !done && pop_req && !empty;

  // coverage of the cases this test exists for
  int unsigned n_one_entry_pop_push, n_full, n_wrap;
  int unsigned n_reports;

  function automatic idx_t idx_add(idx_t i, int unsigned n);
    return idx_t'((int'(i) + int'(n)) % int'(D));
  endfunction

  always_ff @(posedge clk) begin
    if (!rst_ni) begin
      cyc                  <= 0;
      rnd                  <= SEED;
      push_bias            <= 4'd8;
      pop_bias             <= 4'd8;
      filling              <= 1'b1;
      d                    <= '0;
      head_m               <= '0;
      cnt_m                <= 0;
      done                 <= 1'b0;
      ok                   <= 1'b1;
      n_one_entry_pop_push <= 0;
      n_full               <= 0;
      n_wrap               <= 0;
      n_reports            <= 0;
    end else if (!done) begin
      automatic logic        bad;
      automatic int unsigned cnt_nxt;

      // the FIFO's outputs and the model both describe the state before this
      // edge's push/pop
      bad = 1'b0;
      if (int'(size) != int'(cnt_m)) bad = 1'b1;
      if (empty != (cnt_m == 0)) bad = 1'b1;
      if (full != (cnt_m == D)) bad = 1'b1;
      if (cnt_m != 0 && q != mem_m[head_m]) bad = 1'b1;
      if (bad) begin
        ok <= 1'b0;
        if (n_reports < MAX_REPORTS) begin
          $error("rv_tester_fifo D=%0d cycle %0d: q=%h size=%0d empty=%0b full=%0b, expected q=%h size=%0d",
                 D, cyc, q, size, empty, full, mem_m[head_m], cnt_m);
          n_reports <= n_reports + 1;
        end
      end

      if (push && pop && cnt_m == 1) n_one_entry_pop_push <= n_one_entry_pop_push + 1;
      if (full) n_full <= n_full + 1;
      if (pop && int'(head_m) == int'(D) - 1) n_wrap <= n_wrap + 1;

      if (push) begin
        mem_m[idx_add(head_m, cnt_m)] <= d;
        d <= d + 1'b1;
      end
      if (pop) head_m <= idx_add(head_m, 1);
      cnt_nxt = cnt_m + int'(push) - int'(pop);
      cnt_m <= cnt_nxt;

      if (cnt_nxt == D) filling <= 1'b0;
      else if (cnt_nxt == 0) filling <= 1'b1;

      rnd <= {rnd[30:0], rnd[31] ^ rnd[21] ^ rnd[1] ^ rnd[0]};
      if (cyc[7:0] == 8'hff) begin
        push_bias <= rnd[11:8];
        pop_bias  <= rnd[15:12];
      end

      cyc <= cyc + 1;
      if (cyc + 1 == END_CYCLE) begin
        done <= 1'b1;
        // a one-entry FIFO is full at one entry, so it can never push and pop
        // the only entry in the same cycle
        if (D > 1 && n_one_entry_pop_push == 0) begin
          $error("rv_tester_fifo D=%0d: one-entry pop+push case never exercised", D);
          ok <= 1'b0;
        end
        if (n_full == 0 || n_wrap == 0) begin
          $error("rv_tester_fifo D=%0d: full (%0d) or pointer wrap (%0d) never exercised",
                 D, n_full, n_wrap);
          ok <= 1'b0;
        end
        $display("rv_tester_fifo D=%0d: %0d one-entry pop+push, %0d full cycles, %0d wraps",
                 D, n_one_entry_pop_push, n_full, n_wrap);
      end
    end
  end
endmodule

module rv_tester_fifo_tb_top (
    input  logic vclk,
    input  logic vrst_ni,
    output logic test_done,
    output logic test_passed
);
  localparam int unsigned N = 3;

  logic [N-1:0] done, ok;

  rv_tester_fifo_tb_checker #(.D(1),  .SEED(32'h1234_5678)) d1  (.clk(vclk), .rst_ni(vrst_ni), .done(done[0]), .ok(ok[0]));
  rv_tester_fifo_tb_checker #(.D(2),  .SEED(32'h9e37_79b9)) d2  (.clk(vclk), .rst_ni(vrst_ni), .done(done[1]), .ok(ok[1]));
  rv_tester_fifo_tb_checker #(.D(16), .SEED(32'hdead_beef)) d16 (.clk(vclk), .rst_ni(vrst_ni), .done(done[2]), .ok(ok[2]));

  assign test_done   = &done;
  assign test_passed = &ok;
endmodule
