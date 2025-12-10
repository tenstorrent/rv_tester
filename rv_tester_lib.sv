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

module rv_tester_ram #(
    parameter int unsigned SIZE = 16,                    // RAM size (number of entries)
    parameter type         DATA_TYPE = logic[31:0],      // Data type/width
    parameter int unsigned NUM_WRITE_PORTS = 1,          // Number of write ports
    parameter int unsigned NUM_READ_PORTS = 1,           // Number of read ports
    parameter bit          WRITE_ALL = '0,               // NUM_WRITE_PORTS == SIZE optimization
    localparam type        addr_t = logic[$clog2(SIZE >= 2 ? SIZE : 2)-1:0]
)(
    input  logic                clk,

    // Write ports
    input  logic                 wr_en     [NUM_WRITE_PORTS],
    input  addr_t                wr_addr   [NUM_WRITE_PORTS],
    input  DATA_TYPE             wr_data   [NUM_WRITE_PORTS],

    // Read ports
    input  addr_t                rd_addr   [NUM_READ_PORTS],
    output DATA_TYPE             rd_data   [NUM_READ_PORTS]
);

    // Internal RAM storage
    DATA_TYPE ram[SIZE];

    if (WRITE_ALL && NUM_WRITE_PORTS != SIZE) $error("%0m: When WRITE_ALL NUM_WRITE_PORTS (%0d) must equal SIZE (%0d)", NUM_WRITE_PORTS, SIZE);

    // Write logic
    if (!WRITE_ALL) begin
        for (genvar j = 0; j < NUM_WRITE_PORTS; j++) begin
            always_ff @(posedge clk) begin
                if (wr_en[j]) begin
                    ram[wr_addr[j]] <= wr_data[j];
                end
            end
        end
    end else begin
        always_ff @(posedge clk) begin
            for (int j = 0; j < NUM_WRITE_PORTS; j++) begin
                if (wr_en[j]) begin
                    ram[j] <= wr_data[j];
                end
            end
        end
    end

    // Read logic (synchronous)
    for (genvar i = 0; i < NUM_READ_PORTS; i++) begin : gen_read
        always_ff @(posedge clk) begin
            rd_data[i] <= ram[rd_addr[i]];
        end
    end

endmodule

module rv_tester_fifo #(
    parameter  int unsigned D = 1,
    parameter  type         T = logic,
    localparam type         ptr_t = logic[$clog2(D+1)-1:0],
    localparam type         idx_t = logic[($clog2(D) > 0 ? $clog2(D)-1 : 0):0]
)(
    input  logic                clk,
    input  logic                reset_n,
    input  logic                push,
    input  T                    d,
    input  logic                pop,
    output T                    q,
    output ptr_t                size,
    output logic                full,
    output logic                empty,
    output ptr_t                push_ptr,
    output ptr_t                pop_ptr,
    output idx_t                push_idx,
    output idx_t                pop_idx, 
    output idx_t                next_pop_idx
);

    if ((D & (D-1)) != 0)
        $error("Depth %0d not a power of 2, modulo operator below is going to be more gates", D);

    ptr_t rptr, wptr, rptr_nxt, wptr_nxt;

    // RAM interface signals
    idx_t ram_rd_addr [1];
    T ram_rd_data [1];

    // Array connections for single-port RAM
    logic wr_en_array [1];
    idx_t wr_addr_array [1];
    T wr_data_array [1];
    idx_t rd_addr_array [1];

    // Bypass signals
    T q_bypass;
    logic bypass_enable;

    assign size  = wptr - rptr;
    assign full  = size == ptr_t'(D);
    assign empty = size == '0;
    assign push_ptr = wptr;
    assign pop_ptr  = rptr;
    assign push_idx = ptr_to_idx(wptr);
    assign pop_idx  = ptr_to_idx(rptr);
    assign next_pop_idx = ptr_to_idx(rptr + ptr_t'(1));
    localparam int M = D > 1 ? $clog2(D) : 1;

    always_comb begin
        rptr_nxt = rptr + ptr_t'(pop);
        wptr_nxt = wptr + ptr_t'(push);
    end

    function automatic idx_t ptr_to_idx(ptr_t p);
        return M'(p % ptr_t'(D));
    endfunction

    // RAM instance
    rv_tester_ram #(
        .SIZE(D),
        .DATA_TYPE(T),
        .NUM_WRITE_PORTS(1),
        .NUM_READ_PORTS(1)
    ) ram_inst (
        .clk(clk),
        .wr_en(wr_en_array),
        .wr_addr(wr_addr_array),
        .wr_data(wr_data_array),
        .rd_addr(rd_addr_array),
        .rd_data(ram_rd_data)
    );

    // RAM control logic
    always_comb begin
        // Array assignments
        wr_en_array[0] = push;
        wr_addr_array[0] = push_idx;
        wr_data_array[0] = d;
        rd_addr_array[0] = ptr_to_idx(rptr_nxt);
    end

    // Output mux
    assign q = bypass_enable ? q_bypass : ram_rd_data[0];

    always_ff @(posedge clk) begin

        bypass_enable <= '0;
        q_bypass <= '0;

        if (!reset_n) begin
            // Properly initialize all signals during reset
            rptr <= '0;
            wptr <= '0;
        end else begin
            // Normal operation
            rptr <= rptr_nxt;
            wptr <= wptr_nxt;

            // Bypass logic
            if (push && wptr == rptr) begin
                // Bypass: pushing to empty FIFO
                bypass_enable <= '1;
                q_bypass <= d;
            end
        end
    end

    push_when_full: assert property(@(posedge clk) disable iff(!reset_n) push -> !full)
        else $error("pushing when fifo is full");
    pop_when_empty: assert property(@(posedge clk) disable iff(!reset_n) pop  -> !empty)
        else $error("popping when fifo is empty");

endmodule
