// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

module rv_tester_delay_resp_tb_check #(
    parameter int unsigned TIMEOUT = 500,  // max cycles to wait after AR/AW for R/B response
    parameter int CW = 32
)(
    input logic          clk,
    input logic          rst_ni,
    input logic [CW-1:0] delay_cycles,       // configured delay for read transactions
    input logic [CW-1:0] delay_cycles_w,     // configured delay for write transactions
    input logic          ar_valid,           // slave AR valid
    input logic          ar_ready,           // master AR ready
    input logic          r_valid,            // slave R response valid (from DUT output)
    input logic          r_ready,            // slave R ready
    input logic          r_last,             // R last beat
    input logic          aw_valid,           // slave AW valid
    input logic          aw_ready,           // master AW ready
    input logic          b_valid,            // slave B response valid (from DUT output)
    input logic          b_ready,            // slave B ready
    input logic [31:0]   test_count          // for display messages
);

    logic [31:0] cycle_count;

    // Read transaction tracking
    logic [31:0] ar_accept_cycle;
    logic        rd_waiting;
    logic [31:0] rd_wait_counter;

    // Write transaction tracking
    logic [31:0] aw_accept_cycle;
    logic        wr_waiting;
    logic [31:0] wr_wait_counter;

    always_ff @(posedge clk) begin
        if (!rst_ni) begin
            cycle_count     <= '0;
            ar_accept_cycle <= '0;
            rd_waiting      <= 1'b0;
            rd_wait_counter <= '0;
            aw_accept_cycle <= '0;
            wr_waiting      <= 1'b0;
            wr_wait_counter <= '0;
        end else begin
            cycle_count <= cycle_count + 1;

            // ========== READ TRANSACTION TRACKING ==========
            // Latch AR accept cycle only on the first AR of a group
            if (ar_valid && ar_ready && !rd_waiting) begin
                ar_accept_cycle <= cycle_count;
                rd_waiting      <= 1'b1;
                rd_wait_counter <= '0;
            end

            // Increment read wait counter while waiting
            if (rd_waiting) begin
                rd_wait_counter <= rd_wait_counter + 1;
            end

            // Check on R response completion
            if (r_valid && r_ready && r_last && rd_waiting) begin
                if (delay_cycles > 0 && (cycle_count - ar_accept_cycle) < delay_cycles) begin
                    $display("Error: Test %0d - READ response arrived too early: actual_delay=%0d, expected>=%0d",
                             test_count, cycle_count - ar_accept_cycle, delay_cycles);
                    $finish(1);
                end
                rd_waiting      <= 1'b0;
                rd_wait_counter <= '0;
            end

            // Read hang detection
            if (rd_waiting && rd_wait_counter > TIMEOUT) begin
                $display("Error: Test %0d - READ HANG DETECTED: no response after %0d cycles (timeout=%0d)",
                         test_count, rd_wait_counter, TIMEOUT);
                $finish(1);
            end

            // ========== WRITE TRANSACTION TRACKING ==========
            // Latch AW accept cycle only on the first AW of a group
            if (aw_valid && aw_ready && !wr_waiting) begin
                aw_accept_cycle <= cycle_count;
                wr_waiting      <= 1'b1;
                wr_wait_counter <= '0;
            end

            // Increment write wait counter while waiting
            if (wr_waiting) begin
                wr_wait_counter <= wr_wait_counter + 1;
            end

            // Check on B response completion
            if (b_valid && b_ready && wr_waiting) begin
                if (delay_cycles_w > 0 && (cycle_count - aw_accept_cycle) < delay_cycles_w) begin
                    $display("Error: Test %0d - WRITE response arrived too early: actual_delay=%0d, expected>=%0d",
                             test_count, cycle_count - aw_accept_cycle, delay_cycles_w);
                    $finish(1);
                end
                wr_waiting      <= 1'b0;
                wr_wait_counter <= '0;
            end

            // Write hang detection
            if (wr_waiting && wr_wait_counter > TIMEOUT) begin
                $display("Error: Test %0d - WRITE HANG DETECTED: no response after %0d cycles (timeout=%0d)",
                         test_count, wr_wait_counter, TIMEOUT);
                $finish(1);
            end
        end
    end

endmodule
