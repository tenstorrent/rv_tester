module rv_tester_cdc_pulse (
    input  logic        clk_a,            // Clock domain A
    input  logic        clk_b,            // Clock domain B
    input  logic        pulse_a,          // Pulse in domain A
    output logic        pulse_b,          // Pulse in domain B
    output logic        pulse_pending_or_asserted_a   // Pulse pending in domain A
);

    bit pulse_b_sync1 = 0, pulse_b_sync2 = 0, pulse_b_sync3 = 0;
    bit pulse_pending_a = 0;

    // Register to hold pulse_a until acknowledged
    always @(posedge clk_a) begin
        if (pulse_a) begin
            pulse_pending_a <= 1'b1;
        end else if (pulse_b_sync2 & ~pulse_b_sync3) begin
            pulse_pending_a <= 1'b0;
        end
    end

    assign pulse_pending_or_asserted_a = pulse_pending_a || pulse_b_sync3;

    // Synchronizer flip-flops for pulse crossing from A to B
    bit sync1 = 0, sync2 = 0, sync3 = 0;
    always @(posedge clk_b) begin
        sync1 <= pulse_pending_a;
        sync2 <= sync1;
        sync3 <= sync2;
    end

    // Generate pulse in domain B
    assign pulse_b = sync2 & ~sync3;

    // Synchronizer flip-flops for pulse_b crossing from B to A
    always @(posedge clk_a) begin
        pulse_b_sync1 <= pulse_b;
        pulse_b_sync2 <= pulse_b_sync1;
        pulse_b_sync3 <= pulse_b_sync2;
    end

endmodule

module rv_tester_sync3 (
    input  logic clk,
    input  logic d  ,
    output logic q
);

  logic sync1, sync2, sync3;

  always_ff @(posedge clk) begin
    sync1 <= d;
    sync2 <= sync1;
    q     <= sync2;
  end

endmodule

