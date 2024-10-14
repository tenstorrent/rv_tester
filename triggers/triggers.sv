module triggers
import rv_tester_params::*;
#(
  parameter int NUM = -1,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_TRIGGERS_OUTPUT_PARAMS
)
(
  input logic tb_clk,
  input logic tb_reset,
  input logic clk,
  input logic reset,
  input event_trigger_intf_t event_trigger_vec,
  `RV_TESTER_TRANSACTIONS_TRIGGERS_OUTPUT_PORTS
);

  import "DPI-C" context function void triggers_set_scope(int unsigned location);

  logic [TRIGGER_COUNT-1:0] event_trigger;
  genvar i;
  generate
    for (i = 0; i < TRIGGER_COUNT; i = i + 1) begin
      assign event_trigger[i] = event_trigger_vec[i].valid;
    end
  endgenerate

  parameter int unsigned location = cvm_topology_gen::get_location (topology.TOP.PLATFORM.SYSMOD.ID, NUM);
  
  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------
  logic reset_d1;
  always @(posedge tb_clk) begin
    reset_d1 <= reset;
    if (~reset & reset_d1) begin
      if (location != cvm_topology::nil) begin
        triggers_set_scope(location);
      end
    end
  end

  bit [TRIGGER_COUNT-1:0] prev_event_trigger = 0;
  bit interrupt_trigger_in_progress = 0;
  int unsigned captured_clocks = 0;
  int unsigned patch_trigger_start_clocks = 0;
  int unsigned uarch_trigger_start_clocks = 0;
  bit patch_event_based_interrupt = 0;
  bit uarch_event_based_interrupt = 0;
  int unsigned clocks = 0;
  always @(posedge event_trigger[PATCH]) begin
      /* verilator lint_off BLKSEQ */
    if (reset) begin
      patch_event_based_interrupt = 1'b0;
    end
    else begin 
          patch_event_based_interrupt = 1'b1;
          patch_trigger_start_clocks = clocks;
    end
      /* verilator lint_on BLKSEQ */
  end
  always @(posedge event_trigger[UARCH_INTR]) begin
      /* verilator lint_off BLKSEQ */
    if (reset) begin
      uarch_event_based_interrupt = 1'b0;
    end
    else begin 
          uarch_event_based_interrupt = 1'b1;
          uarch_trigger_start_clocks = clocks;
    end
      /* verilator lint_on BLKSEQ */
end

  always @(posedge clk) begin
    clocks <= clocks + 1;
    if(patch_trigger_start_clocks + 50 == clocks)
      /* verilator lint_off BLKSEQ */
          patch_event_based_interrupt = 1'b0;
      /* verilator lint_on BLKSEQ */
  end

  always @(posedge clk) begin
    clocks <= clocks + 1;
    if(uarch_trigger_start_clocks == clocks)
      /* verilator lint_off BLKSEQ */
          uarch_event_based_interrupt = 1'b0;
      /* verilator lint_on BLKSEQ */
  end

  // m_event_trigger_ticks
  assign m_event_trigger_ticks[0].valid = (uarch_event_based_interrupt | patch_event_based_interrupt) & (location != cvm_topology::nil);
  assign m_event_trigger_ticks[0].data.location = location;
  /* verilator lint_off WIDTHEXPAND */
  assign m_event_trigger_ticks[0].data.event_trigger = event_trigger;
  /* verilator lint_on WIDTHEXPAND */




endmodule
