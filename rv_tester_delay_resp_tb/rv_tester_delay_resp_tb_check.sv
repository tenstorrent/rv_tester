module rv_tester_delay_resp_tb_check #(
    parameter int unsigned TIMEOUT = 500,  // max cycles to wait after AR for R response
    parameter int CW = 32
)(
    input logic          clk,
    input logic          rst_ni,
    input logic [CW-1:0] delay_cycles,       // configured delay for current test
    input logic          ar_valid,           // slave AR valid
    input logic          ar_ready,           // master AR ready
    input logic          r_valid,            // slave R response valid (from DUT output)
    input logic          r_ready,            // slave R ready
    input logic          r_last,             // R last beat
    input logic [31:0]   test_count          // for display messages
);

    logic [31:0] cycle_count;
    logic [31:0] ar_accept_cycle;
    logic        waiting;
    logic [31:0] wait_counter;

    always_ff @(posedge clk) begin
        if (!rst_ni) begin
            cycle_count     <= '0;
            ar_accept_cycle <= '0;
            waiting         <= 1'b0;
            wait_counter    <= '0;
        end else begin
            cycle_count <= cycle_count + 1;

            // Latch AR accept cycle only on the first AR of a group
            if (ar_valid && ar_ready && !waiting) begin
                ar_accept_cycle <= cycle_count;
                waiting         <= 1'b1;
                wait_counter    <= '0;
            end

            // Increment wait counter while waiting
            if (waiting) begin
                wait_counter <= wait_counter + 1;
            end

            // Check on R response completion
            if (r_valid && r_ready && r_last && waiting) begin
                if (delay_cycles > 0 && (cycle_count - ar_accept_cycle) < delay_cycles) begin
                    $display("Error: Test %0d - response arrived too early: actual_delay=%0d, expected>=%0d",
                             test_count, cycle_count - ar_accept_cycle, delay_cycles);
                    $finish(1);
                end
                waiting      <= 1'b0;
                wait_counter <= '0;
            end

            // Hang detection
            if (waiting && wait_counter > TIMEOUT) begin
                $display("Error: Test %0d - HANG DETECTED: no response after %0d cycles (timeout=%0d)",
                         test_count, wait_counter, TIMEOUT);
                $finish(1);
            end
        end
    end

endmodule
