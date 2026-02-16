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
        logic [63:0] pop_time;
    } fifo_entry_t;

    // R channel data with validity bit
    typedef struct packed {
        r_chan_t r;
    } r_ram_entry_t;

    // R Ram size 
    localparam int unsigned R_RAM_SIZE = MaxInFlight * MaxBeatsPerBurst;
    // R Ram address type
    typedef logic[$clog2(R_RAM_SIZE)-1:0] r_ram_addr_t;
    
    index_t push_idx, pop_idx, wr_idx, eff_pop_idx, next_pop_idx;
    logic push_en, pop_en, wr_en;
    logic fifo_full, fifo_empty;
    logic[$clog2(MaxInFlight+1)-1:0] write_ptr, read_ptr;
    logic [63:0] global_timer;
    logic ar_req_check;

    beat_count_t output_beat_idx, next_output_beat_idx, eff_output_beat_idx;
    beat_count_t input_beat_counters[MaxInFlight-1:0]; // Beat counter per FIFO entry for incoming responses

    // Internal signals
    fifo_entry_t push_entry, pop_entry;

    // R channel RAM signals
    logic r_ram_wr_en;
    r_ram_addr_t r_ram_wr_addr, r_ram_rd_addr;
    r_ram_entry_t r_ram_wr_data, r_ram_rd_data;

    // R channel RAM signals
    logic r_ram_wr_en_up [1];
    r_ram_addr_t r_ram_wr_addr_up [1], r_ram_rd_addr_up [1];
    r_ram_entry_t r_ram_wr_data_up [1], r_ram_rd_data_up [1];

    // R channel valid RAM signals
    // R_RAM_SIZE RAM of type logic. With R_RAM_SIZE write and read ports. 
    logic r_valid_ram_wr_en [R_RAM_SIZE];
    r_ram_addr_t r_valid_ram_wr_addr [R_RAM_SIZE];
    logic r_valid_ram_wr_data[R_RAM_SIZE];

    r_ram_addr_t r_valid_ram_rd_addr;    // Read address for the valid RAM
    r_ram_addr_t r_valid_ram_rd_addr_up [1];    // Read address for the valid RAM
    
    logic r_valid_ram_rd_data; 
    logic r_valid_ram_rd_data_up [1]; 

    // Signal to send the response back to the core [i.e when the timer is done and the beat to send is valid]
    logic send_r_resp_out;
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
        .pop_idx(pop_idx),
        .next_pop_idx(next_pop_idx)
    );

    // R channel data RAM instantiation
    rv_tester_ram #(
        .SIZE(R_RAM_SIZE),
        .DATA_TYPE(r_ram_entry_t),
        .NUM_WRITE_PORTS(1),
        .NUM_READ_PORTS(1)
    ) r_ram (
        .clk(clk_i),
        .wr_en(r_ram_wr_en_up),
        .wr_addr(r_ram_wr_addr_up),
        .wr_data(r_ram_wr_data_up),
        .rd_addr(r_ram_rd_addr_up),
        .rd_data(r_ram_rd_data_up)
    );

    assign r_ram_wr_en_up[0] = r_ram_wr_en;
    assign r_ram_wr_addr_up[0] = r_ram_wr_addr;
    assign r_ram_wr_data_up[0] = r_ram_wr_data;
    
    assign r_ram_rd_addr_up[0] = r_ram_rd_addr;
    assign r_ram_rd_data = r_ram_rd_data_up[0];

    // R channel valid RAM instantiation
    rv_tester_ram #(
        .SIZE(R_RAM_SIZE),
        .DATA_TYPE(logic),
        .NUM_WRITE_PORTS(R_RAM_SIZE),
        .NUM_READ_PORTS(1),
        .WRITE_ALL(1)
    ) r_valid_ram (
        .clk(clk_i),
        .wr_en(r_valid_ram_wr_en),
        .wr_addr(r_valid_ram_wr_addr),
        .wr_data(r_valid_ram_wr_data),
        .rd_addr(r_valid_ram_rd_addr_up),
        .rd_data(r_valid_ram_rd_data_up)
    );

    assign r_valid_ram_rd_addr_up[0] = r_valid_ram_rd_addr ;
    assign r_valid_ram_rd_data = r_valid_ram_rd_data_up[0];

    
    // Push entry to FIFO 
    assign push_en = ar_req_check && !fifo_full;
    assign push_entry = '{
                    orig_req_id: slv_req_ar_i.id,
                    pop_time: global_timer + 64'(delay_cycles),
                    default: '0
                };

    // Sequential logic
    always_ff @(posedge clk_i) begin: new_ar_req_logic
        if (!rst_ni) begin
            global_timer <= '0;
        end
        else begin
            global_timer <= global_timer + 1; 
        end
    end

    // Ram valid logic 
    always_ff @(posedge clk_i) begin: ram_valid_logic
        // At reset, all are invalid 
        if (!rst_ni) begin
            for(int i = 0; i < R_RAM_SIZE; i++) begin 
                r_valid_ram_wr_data [i]  <= '0;
                r_valid_ram_wr_en   [i]  <= '1;
                r_valid_ram_wr_addr [i]  <= r_ram_addr_t'(i);
            end
        end
        else begin 
            // Write disabled by default. 
            for(int i = 0; i < R_RAM_SIZE; i++) begin 
                r_valid_ram_wr_en [i]  <= '0;
            end
            // When pusing entry, invalidate the ram for all the beat in that transaction
            if (ar_req_check && !fifo_full) begin
                for(int i = 0; i < MaxBeatsPerBurst; i++) begin 
                    r_valid_ram_wr_data[i + push_idx * MaxBeatsPerBurst] <= '0;
                    r_valid_ram_wr_en  [i + push_idx * MaxBeatsPerBurst] <= '1;
                    r_valid_ram_wr_addr[i + push_idx * MaxBeatsPerBurst] <= r_ram_addr_t'(i + push_idx * MaxBeatsPerBurst);
                end
            end

            // When a valid resp is received, validate the corresponding beat in the ram. 
            if (mst_resp_i.r_valid) begin
                r_valid_ram_wr_data [r_ram_wr_addr]  <= '1;
                r_valid_ram_wr_en   [r_ram_wr_addr]  <= '1;
                r_valid_ram_wr_addr [r_ram_wr_addr]  <= r_ram_addr_t'(r_ram_wr_addr);
            end

            // Once the last beat is send, invalidate the ram.
            if (send_r_resp_out && r_ram_rd_data.r.last) begin
                for(int i = 0; i < MaxBeatsPerBurst; i++) begin 
                    r_valid_ram_wr_data[i + pop_idx * MaxBeatsPerBurst] <= '0;
                    r_valid_ram_wr_en  [i + pop_idx * MaxBeatsPerBurst] <= '1;
                    r_valid_ram_wr_addr[i + pop_idx * MaxBeatsPerBurst] <= r_ram_addr_t'(i + pop_idx * MaxBeatsPerBurst);
                end
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
                input_beat_counters[wr_idx] <= input_beat_counters[wr_idx] + beat_count_t'(1);
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

        send_r_resp_out = '0; 

        eff_pop_idx = '0; 
        eff_output_beat_idx = '0;

        // Signal assignments
        // Extract FIFO index from response ID
        wr_idx = index_t'(mst_resp_i.r.id[FIFO_IDX_WIDTH-1:0]);  // Modulo operation
        ar_req_check = slv_req_ar_valid_i && mst_resp_i.ar_ready;
        next_output_beat_idx = output_beat_idx;

        // Bypass logic when delay_cycles is 0
        if (delay_cycles == 0) begin
            // Direct pass-through for read channel [slv_resp_o.r_valid is being assigned above]
            slv_resp_o.r_valid = mst_resp_i.r_valid; 
            slv_resp_o.r.data = mst_resp_i.r.data;
            slv_resp_o.r.id = mst_resp_i.r.id[AxiIdWidth-1:0];
            slv_resp_o.r.resp = mst_resp_i.r.resp;
            slv_resp_o.r.last = mst_resp_i.r.last;
            slv_resp_o.r.user = mst_resp_i.r.user;
            // Use original request ID instead of modified ID
            mst_req_ar_id_o = slv_req_ar_i.id;

            r_valid_ram_rd_addr = '0;
        end else begin
            // Normal delay logic
            // Calculate RAM addresses using FIFO indices
            // For reading: use pop_idx from FIFO and current beat index
            
            send_r_resp_out = !fifo_empty && global_timer >= pop_entry.pop_time && r_valid_ram_rd_data && slv_req_r_ready_i;
            
            // Handle delayed read responses
            if (send_r_resp_out) begin
                slv_resp_o.r.data = r_ram_rd_data.r.data;
                slv_resp_o.r_valid = r_valid_ram_rd_data;
                slv_resp_o.r.id = pop_entry.orig_req_id;
                slv_resp_o.r.resp = r_ram_rd_data.r.resp;
                slv_resp_o.r.last = r_ram_rd_data.r.last;
                slv_resp_o.r.user = r_ram_rd_data.r.user;


                if (r_ram_rd_data.r.last) begin
                    next_output_beat_idx = '0;
                    pop_en = 1'b1;
                end else begin
                    next_output_beat_idx = output_beat_idx + beat_count_t'(1);
                end                
            end

            // if r_valid_ram_rd is valid and it's the last beat, use pop_idx + 1; else use pop_idx
            eff_pop_idx = send_r_resp_out &&  r_valid_ram_rd_data && r_ram_rd_data.r.last ? (next_pop_idx) : (pop_idx);
            // if r_valid_ram_rd_data is valid and it's not the last beat, use output_ + 1; else use output_beat_idx
            eff_output_beat_idx = send_r_resp_out && r_valid_ram_rd_data ? (next_output_beat_idx) : (output_beat_idx);
            
            /* verilator lint_off WIDTHEXPAND */
            r_ram_rd_addr = ($clog2(R_RAM_SIZE))'((eff_pop_idx * MaxBeatsPerBurst) + eff_output_beat_idx);
            /* verilator lint_off WIDTHEXPAND */
            r_valid_ram_rd_addr = r_ram_rd_addr;  

            // Store incoming read responses
            // Calculate RAM address for this beat using FIFO index and input beat counter
            /* verilator lint_off WIDTHEXPAND */
            r_ram_wr_addr = ($clog2(R_RAM_SIZE))'((wr_idx * MaxBeatsPerBurst) + input_beat_counters[wr_idx]);
            /* verilator lint_off WIDTHEXPAND */
            //r_ram_wr_data.r_valid = mst_resp_i.r_valid;
            r_ram_wr_data.r = mst_resp_i.r;
            r_ram_wr_en = mst_resp_i.r_valid;

            // Use FIFO index as ID for master requests
            mst_req_ar_id_o = AxiIdWidth'(push_idx);
        end
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
