// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

//`include "axi_llc/typedef.svh"

module rv_tester_delay_resp #(
                              parameter int unsigned AxiIdWidth           = 32'd8,
                              parameter type slv_resp_t                   = logic,
                              parameter type mst_resp_t                   = logic,
                              parameter type r_chan_t                     = logic,  // R channel type parameter
                              parameter type b_chan_t                     = logic,  // B channel type parameter
                              parameter type slv_ar_chan_t                = logic,
                              parameter type slv_aw_chan_t                = logic,  // AW channel type parameter
                              parameter int CW                            = 16,
                              parameter int unsigned MaxInFlight = 32'd16,
                              parameter int unsigned MaxBeatsPerBurst = 32'd16
                              ) (
                                 input   logic                       clk_i,
                                 input   logic                       rst_ni,
                                 input   logic[CW-1:0]               delay_cycles,     // Runtime configurable delay for R channel
                                 input   logic[CW-1:0]               delay_cycles_w,   // Runtime configurable delay for B channel

                                 // Slave Port Inputs (From Core) -- AR Channel
                                 input   slv_ar_chan_t               slv_req_ar_i,
                                 input   logic                       slv_req_ar_valid_i,

                                 // Slave Port Inputs (From Core) -- R Channel
                                 input   logic                       slv_req_r_ready_i,

                                 // Slave Port Inputs (From Core) -- AW Channel
                                 input   slv_aw_chan_t               slv_req_aw_i,
                                 input   logic                       slv_req_aw_valid_i,

                                 // Slave Port Inputs (From Core) -- B Channel
                                 input   logic                       slv_req_b_ready_i,

                                 // Slave Port Outputs (To Core)
                                 output  slv_resp_t                  slv_resp_o,

                                 // Master Port Outputs (To Xbar) -- AR Channel
                                 output  logic [AxiIdWidth-1:0]      mst_req_ar_id_o,

                                 // Master Port Outputs (To Xbar) -- AW Channel
                                 output  logic [AxiIdWidth-1:0]      mst_req_aw_id_o,

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
    logic [CW        -1:0] push_time;
  } fifo_entry_t;

  // R channel data with validity bit
  typedef struct packed {
    r_chan_t r;
  } r_ram_entry_t;

  // B channel data with validity bit
  typedef struct packed {
    b_chan_t b;
  } b_ram_entry_t;

  logic [CW-1:0] global_timer;

  // R Ram size
  localparam int unsigned R_RAM_SIZE = MaxInFlight * MaxBeatsPerBurst;
  // R Ram address type
  typedef logic[$clog2(R_RAM_SIZE)-1:0] r_ram_addr_t;

  // B Ram address type (one entry per FIFO slot)
  typedef logic[FIFO_IDX_WIDTH-1:0] b_ram_addr_t;

  // Write path signals (AW/B channels)
  index_t push_idx, pop_idx, wr_idx, eff_pop_idx, next_pop_idx;
  logic push;

  index_t push_idx_w, pop_idx_w, wr_idx_w, next_pop_idx_w;
  logic push_w;

  beat_count_t output_beat_idx, next_output_beat_idx, eff_output_beat_idx;
  beat_count_t input_beat_counters[MaxInFlight-1:0]; // Beat counter per FIFO entry for incoming responses

  // R channel RAM signals
  logic r_ram_wr_en;
  r_ram_addr_t r_ram_wr_addr, r_ram_rd_addr;
  r_ram_entry_t r_ram_wr_data, r_ram_rd_data;

  // R channel valid RAM read signals
  r_ram_addr_t r_valid_ram_rd_addr;
  logic r_valid_ram_rd_data;

  // Signal to send the response back to the core [i.e when the timer is done and the beat to send is valid]
  logic send_r_resp_out;

  // R channel delay FIFO outputs
  logic [AxiIdWidth-1:0] r_resp_id_fifo;
  logic [AxiIdWidth-1:0] ar_req_id_fifo;

  // B channel RAM signals
  logic b_ram_wr_en;
  b_ram_addr_t b_ram_wr_addr, b_ram_rd_addr;
  b_ram_entry_t b_ram_wr_data, b_ram_rd_data;

  // B channel valid RAM read signals
  index_t b_valid_ram_rd_addr;
  logic b_valid_ram_rd_data;

  // Signal to send the B response back to the core
  logic send_b_resp_out;

  // B channel delay FIFO outputs
  logic [AxiIdWidth-1:0] b_resp_id_fifo;
  logic [AxiIdWidth-1:0] aw_req_id_fifo;

  // R channel delay FIFO (FIFO + valid RAM + data RAM)
  rv_tester_delay_resp_fifo #(
                              .AxiIdWidth(AxiIdWidth),
                              .MaxInFlight(MaxInFlight),
                              .FIFO_IDX_WIDTH(FIFO_IDX_WIDTH),
                              .ValidRamSize(R_RAM_SIZE),
                              .ValidRamAddrWidth($clog2(R_RAM_SIZE)),
                              .MaxBeatsPerBurst(MaxBeatsPerBurst),
                              .DataRamSize(R_RAM_SIZE),
                              .DataRamAddrWidth($clog2(R_RAM_SIZE)),
                              .data_ram_entry_t(r_ram_entry_t),
                              .CW(CW)
                              ) r_delay_fifo (
                                              .clk_i(clk_i),
                                              .rst_ni(rst_ni),
                                              .delay_cycles(delay_cycles),
                                              .global_timer(global_timer),

                                              // Request channel (AR)
                                              .req_valid_i(slv_req_ar_valid_i),
                                              .req_ready_i(mst_resp_i.ar_ready),
                                              .req_id_i(slv_req_ar_i.id),
                                              .req_id_o(ar_req_id_fifo),

                                              // Response channel (R)
                                              .resp_valid_i(mst_resp_i.r_valid),
                                              .resp_ready_i(slv_req_r_ready_i),
                                              .resp_id_o(r_resp_id_fifo),

                                              // FIFO control and indices
                                              .push_idx_o(push_idx),
                                              .pop_idx_o(pop_idx),
                                              .next_pop_idx_o(next_pop_idx),

                                              // Control signals
                                              .push_o(push),
                                              .send_resp_o(send_r_resp_out),

                                              // Valid RAM interface
                                              .valid_ram_rd_addr_i(r_ram_rd_addr),
                                              .valid_ram_rd_data_o(r_valid_ram_rd_data),

                                              .resp_wr_addr_i(r_ram_wr_addr),
                                              .last_beat_sent_i(send_r_resp_out && r_ram_rd_data.r.last),

                                              // Data RAM interface
                                              .data_ram_wr_addr_i(r_ram_wr_addr),
                                              .data_ram_wr_data_i(r_ram_wr_data),
                                              .data_ram_wr_en_i(r_ram_wr_en),
                                              .data_ram_rd_addr_i(r_ram_rd_addr),
                                              .data_ram_rd_data_o(r_ram_rd_data)
                                              );

  // B channel delay FIFO (FIFO + valid RAM + data RAM)
  rv_tester_delay_resp_fifo #(
                              .AxiIdWidth(AxiIdWidth),
                              .MaxInFlight(MaxInFlight),
                              .FIFO_IDX_WIDTH(FIFO_IDX_WIDTH),
                              .ValidRamSize(MaxInFlight),
                              .ValidRamAddrWidth(FIFO_IDX_WIDTH),
                              .MaxBeatsPerBurst(1),
                              .DataRamSize(MaxInFlight),
                              .DataRamAddrWidth(FIFO_IDX_WIDTH),
                              .data_ram_entry_t(b_ram_entry_t),
                              .CW(CW)
                              ) b_delay_fifo (
                                              .clk_i(clk_i),
                                              .rst_ni(rst_ni),
                                              .delay_cycles(delay_cycles_w),
                                              .global_timer(global_timer),

                                              // Request channel (AW)
                                              .req_valid_i(slv_req_aw_valid_i),
                                              .req_ready_i(mst_resp_i.aw_ready),
                                              .req_id_i(slv_req_aw_i.id),
                                              .req_id_o(aw_req_id_fifo),

                                              // Response channel (B)
                                              .resp_valid_i(mst_resp_i.b_valid),
                                              .resp_ready_i(slv_req_b_ready_i),
                                              .resp_id_o(b_resp_id_fifo),

                                              // FIFO control and indices
                                              .push_idx_o(push_idx_w),
                                              .pop_idx_o(pop_idx_w),
                                              .next_pop_idx_o(next_pop_idx_w),

                                              // Control signals
                                              .push_o(push_w),
                                              .send_resp_o(send_b_resp_out),

                                              // Valid RAM interface
                                              .valid_ram_rd_addr_i(b_ram_rd_addr),
                                              .valid_ram_rd_data_o(b_valid_ram_rd_data),

                                              .resp_wr_addr_i(b_ram_wr_addr),
                                              .last_beat_sent_i(send_b_resp_out),

                                              // Data RAM interface
                                              .data_ram_wr_addr_i(b_ram_wr_addr),
                                              .data_ram_wr_data_i(b_ram_wr_data),
                                              .data_ram_wr_en_i(b_ram_wr_en),
                                              .data_ram_rd_addr_i(b_ram_rd_addr),
                                              .data_ram_rd_data_o(b_ram_rd_data)
                                              );

  assign wr_idx = mst_resp_i.r.id[FIFO_IDX_WIDTH-1:0];
  assign wr_idx_w = mst_resp_i.b.id[FIFO_IDX_WIDTH-1:0];

  // Sequential logic
  always_ff @(posedge clk_i) begin: new_ar_req_logic
    if (!rst_ni) begin
      global_timer <= '0;
    end
    else begin
      global_timer <= global_timer + 1;
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
      if (push) begin
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
    // Pass-through connections for non-delayed channels
    slv_resp_o.aw_ready = mst_resp_i.aw_ready;
    slv_resp_o.ar_ready = mst_resp_i.ar_ready;
    slv_resp_o.w_ready = mst_resp_i.w_ready;

    // Default B channel values
    slv_resp_o.b = '0;
    slv_resp_o.b_valid = '0;

    // Default read channel values
    slv_resp_o.r = '0;
    slv_resp_o.r_valid = '0;

    // Internal signal defaults (read path)
    r_ram_wr_en = '0;
    r_ram_wr_addr = '0;
    r_ram_wr_data = '0;
    r_ram_rd_addr = '0;
    eff_pop_idx = '0;
    eff_output_beat_idx = '0;

    // Internal signal defaults (write path)
    b_ram_wr_en = '0;
    b_ram_wr_addr = '0;
    b_ram_wr_data = '0;
    b_ram_rd_addr = '0;

    next_output_beat_idx = output_beat_idx;

    // ===== Read channel (R) delay logic =====
    // Bypass logic when delay_cycles is 0
    if (delay_cycles == 0) begin
      // Direct pass-through for read channel
      slv_resp_o.r_valid = mst_resp_i.r_valid;
      slv_resp_o.r.data = mst_resp_i.r.data;
      slv_resp_o.r.id = mst_resp_i.r.id[AxiIdWidth-1:0];
      slv_resp_o.r.resp = mst_resp_i.r.resp;
      slv_resp_o.r.last = mst_resp_i.r.last;
      slv_resp_o.r.user = mst_resp_i.r.user;
      mst_req_ar_id_o = slv_req_ar_i.id;
    end else begin
      // Normal delay logic for R channel (using outputs from r_delay_fifo)

      // Handle delayed read responses
      if (send_r_resp_out) begin
        slv_resp_o.r.data = r_ram_rd_data.r.data;
        slv_resp_o.r_valid = r_valid_ram_rd_data;
        slv_resp_o.r.id = r_resp_id_fifo;
        slv_resp_o.r.resp = r_ram_rd_data.r.resp;
        slv_resp_o.r.last = r_ram_rd_data.r.last;
        slv_resp_o.r.user = r_ram_rd_data.r.user;

        if (r_ram_rd_data.r.last) begin
          next_output_beat_idx = '0;
        end else begin
          next_output_beat_idx = output_beat_idx + beat_count_t'(1);
        end
      end

      // if r_valid_ram_rd is valid and it's the last beat, use pop_idx + 1; else use pop_idx
      eff_pop_idx = send_r_resp_out && r_valid_ram_rd_data && r_ram_rd_data.r.last ? (next_pop_idx) : (pop_idx);
      // if r_valid_ram_rd_data is valid and it's not the last beat, use output_ + 1; else use output_beat_idx
      eff_output_beat_idx = send_r_resp_out && r_valid_ram_rd_data ? (next_output_beat_idx) : (output_beat_idx);

      /* verilator lint_off WIDTHEXPAND */
      r_ram_rd_addr = ($clog2(R_RAM_SIZE))'((eff_pop_idx * MaxBeatsPerBurst) + eff_output_beat_idx);
      /* verilator lint_off WIDTHEXPAND */

      // Store incoming read responses
      // Calculate RAM address for this beat using FIFO index and input beat counter
      /* verilator lint_off WIDTHEXPAND */
      r_ram_wr_addr = ($clog2(R_RAM_SIZE))'((wr_idx * MaxBeatsPerBurst) + input_beat_counters[wr_idx]);
      /* verilator lint_off WIDTHEXPAND */
      r_ram_wr_data.r = mst_resp_i.r;
      r_ram_wr_en = mst_resp_i.r_valid;

      mst_req_ar_id_o = ar_req_id_fifo;
    end

    // ===== Write channel (B) delay logic =====
    // Bypass logic when delay_cycles_w is 0
    if (delay_cycles_w == 0) begin
      // Direct pass-through for write response channel
      slv_resp_o.b_valid = mst_resp_i.b_valid;
      slv_resp_o.b.id = mst_resp_i.b.id[AxiIdWidth-1:0];
      slv_resp_o.b.resp = mst_resp_i.b.resp;
      slv_resp_o.b.user = mst_resp_i.b.user;
      mst_req_aw_id_o = slv_req_aw_i.id;
    end else begin
      // Normal delay logic for B channel (using outputs from b_delay_fifo)

      // Handle delayed write responses
      if (send_b_resp_out) begin
        slv_resp_o.b_valid = b_valid_ram_rd_data;
        slv_resp_o.b.id = b_resp_id_fifo;
        slv_resp_o.b.resp = b_ram_rd_data.b.resp;
        slv_resp_o.b.user = b_ram_rd_data.b.user;
      end

      // B valid RAM read address
      b_ram_rd_addr = send_b_resp_out && b_valid_ram_rd_data ? next_pop_idx_w : pop_idx_w;

      // Store incoming write responses
      b_ram_wr_addr = wr_idx_w;
      b_ram_wr_data.b = mst_resp_i.b;
      b_ram_wr_en = mst_resp_i.b_valid;

      mst_req_aw_id_o = aw_req_id_fifo;
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

// Encapsulated delay FIFO module with valid RAM
// Handles common logic for response delay (used by both R and B channels)
module rv_tester_delay_resp_fifo #(
                                   parameter int unsigned AxiIdWidth = 8,
                                   parameter int unsigned MaxInFlight = 16,
                                   parameter int unsigned FIFO_IDX_WIDTH = 4,       // Index width for FIFO addressing
                                   parameter int unsigned ValidRamSize = 16,        // MaxInFlight for B, MaxInFlight*MaxBeatsPerBurst for R
                                   parameter int unsigned ValidRamAddrWidth = 4,    // Address width for valid RAM
                                   parameter int unsigned MaxBeatsPerBurst = 1,     // 1 for B channel, MaxBeatsPerBurst for R channel
                                   parameter int unsigned DataRamSize = 16,         // Size of data RAM (MaxInFlight for B, R_RAM_SIZE for R)
                                   parameter int unsigned DataRamAddrWidth = 4,     // Address width for data RAM
                                   parameter type data_ram_entry_t = logic,         // Data RAM entry type (r_ram_entry_t or b_ram_entry_t)
                                   parameter int CW = 16
                                   ) (
                                      input   logic                           clk_i,
                                      input   logic                           rst_ni,
                                      input   logic[CW-1:0]                   delay_cycles,
                                      input   logic[CW-1:0]                   global_timer,

                                      // Request channel signals (from slave/core)
                                      input   logic                           req_valid_i,
                                      input   logic                           req_ready_i,      // From master (pass-through)
                                      input   logic [AxiIdWidth-1:0]          req_id_i,
                                      output  logic [AxiIdWidth-1:0]          req_id_o,         // Modified ID to master

                                      // Response channel signals (from master)
                                      input   logic                           resp_valid_i,
                                      input   logic                           resp_ready_i,     // From slave/core

                                      // Response channel signals (to slave - delayed)
                                      output  logic [AxiIdWidth-1:0]          resp_id_o,        // Restored original ID

                                      // FIFO control and indices
                                      output  logic [FIFO_IDX_WIDTH-1:0]      push_idx_o,
                                      output  logic [FIFO_IDX_WIDTH-1:0]      pop_idx_o,
                                      output  logic [FIFO_IDX_WIDTH-1:0]      next_pop_idx_o,

                                      // Control signals for external logic
                                      output  logic                           push_o,
                                      output  logic                           send_resp_o,          // Send response to slave

                                      // Valid RAM read interface (for checking if response can be sent)
                                      input   logic [ValidRamAddrWidth-1:0]   valid_ram_rd_addr_i,
                                      output  logic                           valid_ram_rd_data_o,

                                      input   logic [ValidRamAddrWidth-1:0]   resp_wr_addr_i,
                                      input   logic                           last_beat_sent_i,

                                      // Data RAM interface
                                      input   logic [DataRamAddrWidth-1:0]    data_ram_wr_addr_i,
                                      input   data_ram_entry_t                data_ram_wr_data_i,
                                      input   logic                           data_ram_wr_en_i,
                                      input   logic [DataRamAddrWidth-1:0]    data_ram_rd_addr_i,
                                      output  data_ram_entry_t                data_ram_rd_data_o
                                      );

  // Type definitions
  typedef logic [FIFO_IDX_WIDTH-1:0] index_t;
  typedef logic [ValidRamAddrWidth-1:0] valid_addr_t;
  typedef logic [DataRamAddrWidth-1:0] data_ram_addr_t;

  // FIFO entry type
  typedef struct packed {
    logic [AxiIdWidth-1:0] orig_req_id;
    logic [CW-1:0]         push_time;
  } fifo_entry_t;

  // Internal signals
  index_t push_idx, pop_idx, next_pop_idx;
  logic push_en, pop_en;
  logic fifo_full, fifo_empty;
  logic[$clog2(MaxInFlight+1)-1:0] write_ptr, read_ptr;

  fifo_entry_t push_entry, pop_entry;
  logic req_check;
  logic send_resp_internal;

  // Valid RAM signals
  logic valid_ram_wr_en [ValidRamSize];
  valid_addr_t valid_ram_wr_addr [ValidRamSize];
  logic valid_ram_wr_data [ValidRamSize];

  logic valid_ram_rd_data;
  logic valid_ram_rd_data_up [1];
  valid_addr_t valid_ram_rd_addr_up [1];

  // Data RAM signals
  logic data_ram_wr_en_up [1];
  data_ram_addr_t data_ram_wr_addr_up [1], data_ram_rd_addr_up [1];
  data_ram_entry_t data_ram_wr_data_up [1], data_ram_rd_data_up [1];

  // FIFO instantiation
  rv_tester_fifo #(
                   .D(MaxInFlight),
                   .T(fifo_entry_t)
                   ) fifo_inst (
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

  rv_tester_ram #(
                  .SIZE(ValidRamSize),
                  .DATA_TYPE(logic),
                  .NUM_WRITE_PORTS(ValidRamSize),
                  .NUM_READ_PORTS(1),
                  .WRITE_ALL(1)
                  ) valid_ram (
                               .clk(clk_i),
                               .wr_en(valid_ram_wr_en),
                               .wr_addr(valid_ram_wr_addr),
                               .wr_data(valid_ram_wr_data),
                               .rd_addr(valid_ram_rd_addr_up),
                               .rd_data(valid_ram_rd_data_up)
                               );

  assign valid_ram_rd_addr_up[0] = valid_ram_rd_addr_i;
  assign valid_ram_rd_data = valid_ram_rd_data_up[0];

  // Data RAM instantiation
  rv_tester_ram #(
                  .SIZE(DataRamSize),
                  .DATA_TYPE(data_ram_entry_t),
                  .NUM_WRITE_PORTS(1),
                  .NUM_READ_PORTS(1)
                  ) data_ram (
                              .clk(clk_i),
                              .wr_en(data_ram_wr_en_up),
                              .wr_addr(data_ram_wr_addr_up),
                              .wr_data(data_ram_wr_data_up),
                              .rd_addr(data_ram_rd_addr_up),
                              .rd_data(data_ram_rd_data_up)
                              );

  assign data_ram_wr_en_up[0] = data_ram_wr_en_i;
  assign data_ram_wr_addr_up[0] = data_ram_wr_addr_i;
  assign data_ram_wr_data_up[0] = data_ram_wr_data_i;
  assign data_ram_rd_addr_up[0] = data_ram_rd_addr_i;
  assign data_ram_rd_data_o = data_ram_rd_data_up[0];

  // Valid RAM control logic
  always_ff @(posedge clk_i) begin
    if (!rst_ni) begin
      // At reset, invalidate all entries
      for(int i = 0; i < ValidRamSize; i++) begin
        valid_ram_wr_data [i]  <= '0;
        valid_ram_wr_en   [i]  <= '1;
        valid_ram_wr_addr [i]  <= valid_addr_t'(i);
      end
    end
    else begin
      // Write disabled by default
      for(int i = 0; i < ValidRamSize; i++) begin
        valid_ram_wr_en [i]  <= '0;
      end

      // When pushing entry, invalidate the corresponding RAM entries
      if (push_en) begin
        for(int i = 0; i < MaxBeatsPerBurst; i++) begin
          valid_ram_wr_data[i + push_idx * MaxBeatsPerBurst] <= '0;
          valid_ram_wr_en  [i + push_idx * MaxBeatsPerBurst] <= '1;
          valid_ram_wr_addr[i + push_idx * MaxBeatsPerBurst] <= valid_addr_t'(i + push_idx * MaxBeatsPerBurst);
        end
      end

      // When a valid response is received, validate the corresponding entry
      if (resp_valid_i) begin
        valid_ram_wr_data [resp_wr_addr_i]  <= '1;
        valid_ram_wr_en   [resp_wr_addr_i]  <= '1;
        valid_ram_wr_addr [resp_wr_addr_i]  <= resp_wr_addr_i;
      end

      // When last beat is sent (R channel) or response sent (B channel), invalidate entries
      if (pop_en) begin
        for(int i = 0; i < MaxBeatsPerBurst; i++) begin
          valid_ram_wr_data[i + pop_idx * MaxBeatsPerBurst] <= '0;
          valid_ram_wr_en  [i + pop_idx * MaxBeatsPerBurst] <= '1;
          valid_ram_wr_addr[i + pop_idx * MaxBeatsPerBurst] <= valid_addr_t'(i + pop_idx * MaxBeatsPerBurst);
        end
      end
    end
  end

  // Combinational logic
  always_comb begin
    // Request handling
    req_check = req_valid_i && req_ready_i;

    // Push entry to FIFO when request is accepted
    push_entry = '{
                   orig_req_id: req_id_i,
                   push_time: global_timer,
                   default: '0
                   };

    push_en = '0;
    pop_en = '0;
    send_resp_internal = '0;

    if (delay_cycles != 0) begin
      // Delay condition: Check if sufficient time has passed and response is valid
      send_resp_internal = !fifo_empty &&
                           CW'(global_timer - pop_entry.push_time) >= CW'(delay_cycles) &&
                           valid_ram_rd_data &&
                           resp_ready_i;

      // FIFO control
      push_en = req_check && !fifo_full;
      pop_en = last_beat_sent_i;
    end

    // Output assignments for delayed response
    resp_id_o = pop_entry.orig_req_id;

    // Request ID modification (use FIFO index for tracking)
    req_id_o = AxiIdWidth'(push_idx);
  end

  // Output assignments
  assign push_idx_o = push_idx;
  assign pop_idx_o = pop_idx;
  assign next_pop_idx_o = next_pop_idx;
  assign push_o = push_en;
  assign send_resp_o = send_resp_internal;
  assign valid_ram_rd_data_o = valid_ram_rd_data;

endmodule
