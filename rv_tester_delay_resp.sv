//`include "axi_llc/typedef.svh"

module rv_tester_delay_resp #(
    parameter int unsigned AxiIdWidth           = 32'd8,
    parameter type slv_resp_t                   = logic,
    parameter type mst_resp_t                   = logic,
    parameter type r_chan_t                     = logic,  // R channel type parameter
    parameter type slv_ar_chan_t                = logic,
    parameter int unsigned MaxInFlight = 32'd16,
    parameter int unsigned MaxBeatsPerBurst = 32'd16
) (
    input   logic                       clk_i,
    input   logic                       rst_ni,
    input   int unsigned                delay_cycles,     // Runtime configurable delay
    
    // Slave Port Inputs (From Core) -- AR Channel
    input   slv_ar_chan_t               slv_req_ar_i,
    input   logic                       slv_req_ar_valid_i,  
    
    // Slave Port Inputs (From Core) -- R Channel
    input   logic                       slv_req_r_ready_i,
    // Slave Port Outputs (To Core)
    output  slv_resp_t                  slv_resp_o,

    // Master Port Outputs (To Xbar) -- AR Channel
    output  logic [AxiIdWidth-1:0]      mst_req_ar_id_o,

    // Master Port Inputs (From Xbar) -- R Channel
    input   mst_resp_t                  mst_resp_i
    );

    // Local parameters
    localparam int unsigned FIFO_IDX_WIDTH = $clog2(MaxInFlight);
    
    // // Build-time parameter validation - this will cause compilation error if invalid
    // localparam int unsigned REQUIRED_BITS = $clog2(MaxInFlight);
    // localparam int unsigned INVALID_CONFIG = (REQUIRED_BITS > AxiIdWidth) ? (1 / 0) : 0;
    
    // // Static assertion to check parameter values at elaboration time
    // `ifndef SYNTHESIS
    //     initial begin
    //         $display("rv_tester_delay_resp: AxiIdWidth=%0d, MaxInFlight=%0d, FIFO_IDX_WIDTH=%0d", 
    //                  AxiIdWidth, MaxInFlight, FIFO_IDX_WIDTH);
    //         assert(FIFO_IDX_WIDTH <= AxiIdWidth) else
    //             $fatal(1, "FIFO_IDX_WIDTH (%0d) exceeds AxiIdWidth (%0d)", FIFO_IDX_WIDTH, AxiIdWidth);
    //     end
    // `endif
    
    // // Build-time parameter validation
    // generate
    //     if ($clog2(MaxInFlight) > AxiIdWidth) begin : gen_param_error
    //         // This will cause a build error if the condition is true
    //         initial begin
    //             $fatal("rv_tester_delay_resp: FIFO_IDX_WIDTH (%0d) exceeds AxiIdWidth (%0d)", 
    //                    FIFO_IDX_WIDTH, AxiIdWidth);
    //         end
    //     end
    // endgenerate
    
    // Type definitions
    typedef logic [FIFO_IDX_WIDTH-1:0] index_t;
    typedef logic [7:0] beat_count_t; // Beat counter for bursts

    typedef struct packed {
        logic [AxiIdWidth-1:0] orig_req_id;
        int unsigned pop_time;
    } fifo_entry_t;

    // R channel data with validity bit
    typedef struct packed {
        logic r_valid;
        r_chan_t r;
    } r_ram_entry_t;

    
    index_t push_idx, pop_idx, wr_idx;
    logic push_en, pop_en, wr_en;
    logic fifo_full, fifo_empty;
    logic[$clog2(MaxInFlight+1)-1:0] write_ptr, read_ptr;
    int unsigned global_timer;
    logic ar_req_check;

    beat_count_t output_beat_idx, next_output_beat_idx;
    beat_count_t input_beat_counters[MaxInFlight-1:0]; // Beat counter per FIFO entry for incoming responses

    // Internal signals
    fifo_entry_t push_entry, pop_entry;

    // R channel RAM signals
    logic r_ram_wr_en;
    logic[$clog2(MaxInFlight * MaxBeatsPerBurst)-1:0] r_ram_wr_addr, r_ram_rd_addr;
    r_ram_entry_t r_ram_wr_data, r_ram_rd_data;

    // FIFO instantiation
    rv_tester_fifo #(
        .D(MaxInFlight),
        .T(fifo_entry_t)
    ) rv_fifo (
        .clk(clk_i),
        .reset_n(rst_ni),
        .push(push_en),
        .d(push_entry),
        .pop(pop_en),
        .q(pop_entry),
        .size(),
        .full(fifo_full),
        .empty(fifo_empty),
        .push_ptr(write_ptr),
        .pop_ptr(read_ptr),
        .push_idx(push_idx),
        .pop_idx(pop_idx)
    );

    // R channel data RAM instantiation
    rv_tester_ram #(
        .SIZE(MaxInFlight * MaxBeatsPerBurst),
        .DATA_TYPE(r_ram_entry_t),
        .NUM_WRITE_PORTS(1),
        .NUM_READ_PORTS(1)
    ) r_ram (
        .clk(clk_i),
        .wr_en('{r_ram_wr_en}),
        .wr_addr('{r_ram_wr_addr}),
        .wr_data('{r_ram_wr_data}),
        .rd_addr('{r_ram_rd_addr}),
        .rd_data('{r_ram_rd_data})
    );

    // ID tagging for master requests - use index directly as ID
    assign mst_req_ar_id_o = AxiIdWidth'(push_idx);
    
    // Sequential logic
    always_ff @(posedge clk_i) begin: new_ar_req_logic
        if (!rst_ni) begin
            global_timer <= '0;
            push_en <= '0;
            push_entry <= '{default: '0};
        end
        else begin
            global_timer <= global_timer + 1;
            push_en <= '0;
            // Handle new AR requests from core
            if (ar_req_check && !fifo_full) begin
                push_entry <= '{
                    orig_req_id: slv_req_ar_i.id,
                    pop_time: global_timer + delay_cycles,
                    default: '0
                };
                push_en <= 1'b1;
            end
        end
    end

    always_ff @(posedge clk_i) begin: beat_logic
        if (!rst_ni) begin
            output_beat_idx <= '0;
            input_beat_counters <= '{default: '0};
        end
        else begin
            output_beat_idx <= next_output_beat_idx;

            // Reset input beat counter when new AR request is processed
            if (ar_req_check && !fifo_full) begin
                input_beat_counters[push_idx] <= '0;
            end

            // Increment input beat counter when receiving responses
            if (mst_resp_i.r_valid) begin
                input_beat_counters[wr_idx] <= beat_count_t'(int'(input_beat_counters[wr_idx]) + 1);
            end
        end
    end
    // Combinational logic
    always_comb begin
        // Pass-through connections for non-read channels
        slv_resp_o.aw_ready = mst_resp_i.aw_ready;
        slv_resp_o.ar_ready = mst_resp_i.ar_ready;
        slv_resp_o.w_ready = mst_resp_i.w_ready;
        slv_resp_o.b_valid = mst_resp_i.b_valid;
        slv_resp_o.b.id = mst_resp_i.b.id[AxiIdWidth-1:0];
        slv_resp_o.b.resp = mst_resp_i.b.resp;
        slv_resp_o.b.user = mst_resp_i.b.user;
        
        // Default read channel values
        slv_resp_o.r = '0;
        slv_resp_o.r_valid = '0;

        // Internal signal defaults
        pop_en = '0;
        r_ram_wr_en = '0;
        r_ram_wr_addr = '0;
        r_ram_wr_data = '0;
        r_ram_rd_addr = '0;

        // Signal assignments
        // Extract FIFO index from response ID
        wr_idx = index_t'(mst_resp_i.r.id[FIFO_IDX_WIDTH-1:0]);  // Modulo operation
        ar_req_check = slv_req_ar_valid_i && mst_resp_i.ar_ready;
        next_output_beat_idx = output_beat_idx;

        // Calculate RAM addresses using FIFO indices
        // For reading: use pop_idx from FIFO and current beat index
        r_ram_rd_addr = ($clog2(MaxInFlight * MaxBeatsPerBurst))'((pop_idx * MaxBeatsPerBurst) + output_beat_idx);

        // Handle delayed read responses
        if (!fifo_empty && global_timer >= pop_entry.pop_time && r_ram_rd_data.r_valid && slv_req_r_ready_i) begin
            slv_resp_o.r_valid = 1'b1;
            slv_resp_o.r.data = r_ram_rd_data.r.data;
            slv_resp_o.r.id = pop_entry.orig_req_id;
            slv_resp_o.r.resp = r_ram_rd_data.r.resp;
            slv_resp_o.r.last = r_ram_rd_data.r.last;
            slv_resp_o.r.user = r_ram_rd_data.r.user;

            if (r_ram_rd_data.r.last) begin
                next_output_beat_idx = '0;
                pop_en = 1'b1;
            end else begin
                next_output_beat_idx = beat_count_t'(int'(output_beat_idx) + 1);
            end
        end

        // Store incoming read responses
        // Calculate RAM address for this beat using FIFO index and input beat counter
        r_ram_wr_addr = ($clog2(MaxInFlight * MaxBeatsPerBurst))'((wr_idx * MaxBeatsPerBurst) + input_beat_counters[wr_idx]);
        r_ram_wr_data.r_valid = mst_resp_i.r_valid;
        r_ram_wr_data.r = mst_resp_i.r;
        r_ram_wr_en = mst_resp_i.r_valid;
    end
`ifdef ASSERTION_ENABLE
    // Beat counter assertions
    beat_counter_overflow: assert property (@(posedge clk_i) disable iff (!rst_ni)
        mst_resp_i.r_valid |-> input_beat_counters[wr_idx] < beat_count_t'(MaxBeatsPerBurst))
        else $error("Beat counter overflow for FIFO index %0d: %0d", wr_idx, input_beat_counters[wr_idx]);

    // Assert that MaxInFlight fits within AxiIdWidth
    id_width_check: assert property (@(posedge clk_i) disable iff (!rst_ni)
        $clog2(MaxInFlight) <= AxiIdWidth)
        else $error("MaxInFlight (%0d) requires %0d bits which exceeds AxiIdWidth (%0d)",
                   MaxInFlight, $clog2(MaxInFlight), AxiIdWidth);

    // Assert that response ID width is sufficient for FIFO index
    resp_id_width_check: assert property (@(posedge clk_i) disable iff (!rst_ni)
        FIFO_IDX_WIDTH <= AxiIdWidth)
        else $error("FIFO index width (%0d) exceeds response ID width (%0d)",
                   FIFO_IDX_WIDTH, AxiIdWidth);
`endif
endmodule