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
  input event_trigger_intf_t event_trigger_vec [NHARTS-1:0],
  `RV_TESTER_TRANSACTIONS_TRIGGERS_OUTPUT_PORTS
);

  import "DPI-C" context function void triggers_set_scope(int unsigned location);

  logic [TRIGGER_COUNT-1:0] hart_specific_event_trigger;
  logic [NHARTS-1:0] event_trigger_vlds;
  logic low_power_seq_en;
  
  genvar i;
  generate
    for (i = 0; i < TRIGGER_COUNT; i = i + 1) begin
      assign hart_specific_event_trigger[i] = event_trigger_vec[NUM][i].valid;
    end
  endgenerate

  always_comb begin
    for (int hart_num = 0; hart_num < NHARTS; hart_num++) begin
      event_trigger_vlds[hart_num] = 0;
      for (int trigger_num = 0; trigger_num < TRIGGER_COUNT; trigger_num++) begin
        event_trigger_vlds[hart_num] |= event_trigger_vec[hart_num][trigger_num].valid;
      end
    end
  end
  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.TRIGGERS.ID, NUM);
  
  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------
  logic reset_d1;
  int unsigned tb_clocks = 0;
  always @(posedge tb_clk) begin
    tb_clocks <= tb_clocks + 1;
    reset_d1 <= reset;
    if (~reset & reset_d1) begin
      low_power_seq_en <= $test$plusargs("low_power_seq") ?  1'b1 : 1'b0;
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
  bit patch_event_based_interrupt_delayed = 0;
  bit uarch_event_based_interrupt_delayed = 0;
  int unsigned clocks = 0;
  always @(posedge hart_specific_event_trigger[PATCH]) begin
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
  always @(posedge hart_specific_event_trigger[UARCH_INTR]) begin
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
    patch_event_based_interrupt_delayed <= patch_event_based_interrupt;
    if(patch_trigger_start_clocks == clocks)
      /* verilator lint_off BLKSEQ */
          patch_event_based_interrupt = 1'b0;
      /* verilator lint_on BLKSEQ */
  end

  always @(posedge clk) begin
    clocks <= clocks + 1;
    uarch_event_based_interrupt_delayed <= uarch_event_based_interrupt;
    if(uarch_trigger_start_clocks == clocks)
      /* verilator lint_off BLKSEQ */
          uarch_event_based_interrupt = 1'b0;
      /* verilator lint_on BLKSEQ */
  end

  // m_event_trigger_ticks
  assign m_event_trigger_ticks[0].valid = (uarch_event_based_interrupt | patch_event_based_interrupt) & (location != cvm_topology::nil);
  assign m_event_trigger_ticks[0].data.location = location;
  assign m_event_trigger_ticks[0].data.per_core_evt_vector = {{(32 - NHARTS){1'b0}}, event_trigger_vlds};
  /* verilator lint_off WIDTHEXPAND */
  assign m_event_trigger_ticks[0].data.event_trigger = hart_specific_event_trigger;
  /* verilator lint_on WIDTHEXPAND */
  assign m_event_trigger_delayed_ticks[0].valid = (uarch_event_based_interrupt_delayed | patch_event_based_interrupt_delayed) & (location != cvm_topology::nil);
  assign m_event_trigger_delayed_ticks[0].data.location = location;


  // C-Sequence
  // m_tick
  assign m_ticks[0].valid = (low_power_seq_en) & (location != cvm_topology::nil);
  assign m_ticks[0].data.location = location;
  assign m_ticks[0].data.cycle = tb_clocks;

endmodule
