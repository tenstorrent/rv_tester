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

module rv_tester_fifo #(
    parameter  int unsigned D = 1,
    parameter  type         T = logic,
    parameter  bit          ENABLE_RANDOM_ACCESS_WRITE = 0,  // Enable selective field updates with write mask
    localparam type         ptr_t = logic[$clog2(D+1)-1:0],
    localparam type         idx_t = logic[($clog2(D) > 0 ? $clog2(D)-1 : 0):0]
)(
    input  logic                clk,
    input  logic                reset_n,
    input  logic                push,
    input  T                    d,
    input  logic                wr_en, 
    input  idx_t                wr_idx,
    input  T                    wr_data,
    input  T                    wr_mask,
    input  logic                pop,
    output T                    q,
    output ptr_t                size,
    output logic                full,
    output logic                empty,
    output ptr_t                push_ptr,
    output ptr_t                pop_ptr                
);

    if ((D & (D-1)) != 0)
        $error("Depth %0d not a power of 2, modulo operator below is going to be more gates", D);

    ptr_t rptr, wptr, rptr_nxt, wptr_nxt;
    T rmw_data;
    T ram[D];

    assign size  = wptr - rptr;
    assign full  = size == ptr_t'(D);
    assign empty = size == '0;
    assign push_ptr = wptr;
    assign pop_ptr  = rptr;
    localparam int M = D > 1 ? $clog2(D) : 1;

    always_comb begin
        rptr_nxt = rptr + ptr_t'(pop);
        wptr_nxt = wptr + ptr_t'(push);
    end

    if (ENABLE_RANDOM_ACCESS_WRITE) begin
        assign rmw_data = (ram[wr_idx] & ~wr_mask) | (wr_data & wr_mask);
    end

    function automatic idx_t ptr_to_idx(ptr_t p);
        return M'(p % ptr_t'(D));
    endfunction

    always_ff @(posedge clk) begin
        if (!reset_n) begin
            // Properly initialize all signals during reset
            rptr <= '0;
            wptr <= '0;
        end else begin
            // Normal operation
            rptr <= rptr_nxt;
            wptr <= wptr_nxt;

            if (push) begin
                ram[ptr_to_idx(wptr)] <= d;
            end

            if (ENABLE_RANDOM_ACCESS_WRITE && wr_en) begin
                ram[wr_idx] <= rmw_data;
            end

            if (push && wptr == rptr) begin
                q <= d;
            end else if (ENABLE_RANDOM_ACCESS_WRITE && wr_en && wr_idx == ptr_to_idx(rptr_nxt)) begin
                q <= rmw_data;
            end else begin
                q <= ram[ptr_to_idx(rptr_nxt)];
            end
        end
    end

    push_when_full: assert property(@(posedge clk) disable iff(!reset_n) push -> !full)
        else $error("pushing when fifo is full");
    pop_when_empty: assert property(@(posedge clk) disable iff(!reset_n) pop  -> !empty)
        else $error("popping when fifo is empty");

endmodule
