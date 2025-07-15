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
        r_chan_t [MaxBeatsPerBurst-1:0] r;
        int unsigned pop_time;
        logic r_valid;
    } fifo_entry_t;

    typedef struct packed {
        beat_count_t total_beats;
        beat_count_t resp_beat_counter;
    } beat_tracker_t;
    
    index_t push_idx, wr_idx;
    logic push_en, pop_en, wr_en;
    logic fifo_full, fifo_empty;
    logic[$clog2(MaxInFlight+1)-1:0] write_ptr, read_ptr;
    int unsigned global_timer;
    logic ar_req_check;

    beat_tracker_t beat_tracker[MaxInFlight-1:0];
    beat_count_t output_beat_idx, next_output_beat_idx;

    // Internal signals
    fifo_entry_t push_entry, pop_entry;
    fifo_entry_t resp_data_mask, resp_data;

    // FIFO instantiation
    rv_tester_fifo #(
        .D(MaxInFlight),
        .T(fifo_entry_t),
        .ENABLE_RANDOM_ACCESS_WRITE(1)
    ) rv_fifo (
        .clk(clk_i),
        .reset_n(rst_ni),
        .push(push_en),
        .d(push_entry),
        .wr_en(wr_en),
        .wr_idx(wr_idx),
        .wr_data(resp_data),
        .wr_mask(resp_data_mask),
        .pop(pop_en),
        .q(pop_entry),
        .size(),
        .full(fifo_full),
        .empty(fifo_empty),
        .push_ptr(write_ptr),
        .pop_ptr(read_ptr)
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
                    r: '{default: '0},
                    pop_time: global_timer + delay_cycles,
                    r_valid: 1'b0,
                    default: '0
                };
                push_en <= 1'b1;
            end
        end
    end

    always_ff @(posedge clk_i) begin: beat_tracker_logic
        if (!rst_ni) begin
            beat_tracker <= '{default: '0};
            output_beat_idx <= '0;
        end
        else begin
            output_beat_idx <= next_output_beat_idx;
            if (ar_req_check && !fifo_full) begin
                beat_tracker[push_idx].total_beats <= beat_count_t'(int'(slv_req_ar_i.len) + 1);
                beat_tracker[push_idx].resp_beat_counter <= '0;
            end
            // Update beat counter when receiving responses
            if (mst_resp_i.r_valid) begin
                beat_tracker[wr_idx].resp_beat_counter <= beat_count_t'(int'(beat_tracker[wr_idx].resp_beat_counter) + 1);
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
        wr_en = '0;
        resp_data = '{default: '0};
        resp_data_mask = '{default: '0};

        // Signal assignments
        push_idx = write_ptr[$clog2(MaxInFlight)-1:0];
        // Extract FIFO index from response ID
        wr_idx = index_t'(mst_resp_i.r.id[FIFO_IDX_WIDTH-1:0]);  // Modulo operation
        ar_req_check = slv_req_ar_valid_i && mst_resp_i.ar_ready;
        next_output_beat_idx = output_beat_idx;

        // Handle delayed read responses
        if (!fifo_empty && global_timer >= pop_entry.pop_time && pop_entry.r_valid && slv_req_r_ready_i) begin
            slv_resp_o.r_valid = 1'b1;
            slv_resp_o.r.data = pop_entry.r[output_beat_idx].data;
            slv_resp_o.r.id = pop_entry.orig_req_id;
            slv_resp_o.r.resp = pop_entry.r[output_beat_idx].resp;
            slv_resp_o.r.last = pop_entry.r[output_beat_idx].last;
            slv_resp_o.r.user = pop_entry.r[output_beat_idx].user;
            
            if (pop_entry.r[output_beat_idx].last) begin
                next_output_beat_idx = '0;
                pop_en = 1'b1;
            end else begin
                next_output_beat_idx = beat_count_t'(int'(output_beat_idx) + 1);
            end
        end

        // Store incoming read responses
        if (mst_resp_i.r_valid) begin
            resp_data = '{
                orig_req_id: '0,
                r: '{default: '0},
                pop_time: '0,
                r_valid: '1,
                default: '0
            };
            // Set the specific array element after struct creation
            resp_data.r[beat_tracker[wr_idx].resp_beat_counter] = mst_resp_i.r;
            
            // Build mask to only update the specific r array element and r_valid
            resp_data_mask = '{
                orig_req_id: '0,
                r: '{default: '0},
                pop_time: '0,
                r_valid: '1,
                default: '0
            };
            // Enable mask for the specific array element being updated
            resp_data_mask.r[beat_tracker[wr_idx].resp_beat_counter] = '1;
            
            wr_en = 1'b1;
        end
    end
`ifdef ASSERTION_ENABLE
    // Beat counter assertions
    beat_counter_overflow: assert property (@(posedge clk_i) disable iff (!rst_ni)
        mst_resp_i.r_valid |-> beat_tracker[wr_idx].resp_beat_counter < beat_count_t'(MaxBeatsPerBurst))
        else $error("Beat counter overflow for FIFO index %0d: %0d", wr_idx, beat_tracker[wr_idx].resp_beat_counter);
    
    last_beat_counter_mismatch: assert property (@(posedge clk_i) disable iff (!rst_ni)
        mst_resp_i.r_valid && mst_resp_i.r.last |-> 
        beat_tracker[wr_idx].resp_beat_counter == (beat_tracker[wr_idx].total_beats - 1))
        else $error("Last beat received but beat counter mismatch. Expected: %0d, Got: %0d", 
                   beat_tracker[wr_idx].total_beats - 1, beat_tracker[wr_idx].resp_beat_counter);

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