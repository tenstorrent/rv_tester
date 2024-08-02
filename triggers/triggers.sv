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
  input logic dut_clk,
  input logic dut_reset,
  input logic no_fetch,
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

  int unsigned location = cvm_topology::nil;
  
  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------

  bit [TRIGGER_COUNT-1:0] prev_event_trigger = 0;
  bit interrupt_trigger_in_progress = 0;
  int unsigned captured_clocks = 0;
  bit event_based_interrupt = 0;
  int unsigned dut_clocks = 0;
  always @(posedge tb_clk) begin
      /* verilator lint_off BLKSEQ */
    if (tb_reset) begin
      prev_event_trigger = event_trigger;
    end
    else begin 
        if (event_trigger != prev_event_trigger) begin
            event_based_interrupt = 1'b1;
            captured_clocks = dut_clocks;
        end
        if(dut_clocks < captured_clocks + 50 ) begin
            event_based_interrupt = 1'b0;
        end
    end
    prev_event_trigger = event_trigger;
      /* verilator lint_on BLKSEQ */
end

  always @(posedge dut_clk) begin
    dut_clocks <= dut_clocks + 1;
  end

  // m_nmi_tick
  //assign m_event_trigger_ticks[0].valid = event_based_interrupt & (location != cvm_topology::nil);
  assign m_event_trigger_ticks[0].valid = 1'b1 & (location != cvm_topology::nil);
  assign m_event_trigger_ticks[0].data.location = location;
  /* verilator lint_off WIDTHEXPAND */
  assign m_event_trigger_ticks[0].data.event_trigger = event_trigger;
  /* verilator lint_on WIDTHEXPAND */

  // -------------------------
  // C++->SV Callbacks
  // -------------------------

  export "DPI-C" function trigger_triggers_init;
  export "DPI-C" function trigger_triggers;

  function void trigger_triggers_init();
   /* verilator lint_off BLKSEQ */
    /* verilator lint_on BLKSEQ */
  endfunction

  function void trigger_triggers(int val);
     /* verilator lint_off BLKSEQ */
    $display("\ntrigger based interrupt %d\n",val);
    /* verilator lint_on BLKSEQ */
  endfunction
endmodule
