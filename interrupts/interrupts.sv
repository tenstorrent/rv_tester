module interrupts
import rv_tester_params::*;
#(
  parameter int NUM = -1,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_INTERRUPTS_OUTPUT_PARAMS
)
(
  input logic tb_clk,
  input logic tb_reset,
  input logic dut_clk,
  input logic dut_reset,
  input logic no_fetch,
  input event_trigger_intf_t event_trigger,
  output logic nmi,
  `RV_TESTER_TRANSACTIONS_INTERRUPTS_OUTPUT_PORTS
);

  import "DPI-C" context function void interrupts_set_scope(int unsigned location);
  import "DPI-C" function bit interrupts_get_nmi_en(string mode);

  int unsigned location = cvm_topology::nil;
  logic tb_reset_d1;
  string nmi_mode;
  bit nmi_en;
  int unsigned nmi_count;
  int unsigned nmi_interval;
  int unsigned nmi_width;
  always @(posedge tb_clk) begin
    tb_reset_d1 <= tb_reset;
    if (tb_reset) begin
      interrupts_init();
    end
    if (~tb_reset & tb_reset_d1) begin
      /* verilator lint_off BLKSEQ */
      location = cvm_topology::get_location(topology_pkg::mods.TOP.PLATFORM.INTERRUPTS.ID, NUM);
      if (location != cvm_topology::nil) begin
        interrupts_set_scope(location);
        nmi_mode = cvm_plusargs::get_string("nmi");
        nmi_en = interrupts_get_nmi_en(nmi_mode);
        nmi_count <= cvm_rand::get("nmi_count");
        nmi_interval <= cvm_rand::get("nmi_interval");
        nmi_width <= cvm_rand::get("nmi_width");
      end
      /* verilator lint_on BLKSEQ */
    end
  end

  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------

  int unsigned tb_clocks = 0;
  bit nmi_start = 0;
  bit nmi_end = 0;
  bit nmi_in_progress = 0;
  always @(posedge tb_clk) begin
    if (nmi_en && (nmi_count != 0) && ~|no_fetch) begin
      tb_clocks <= tb_clocks + 1;
      nmi_start <= '0;
      nmi_end <= '0;
      if (tb_clocks > nmi_interval) begin
        nmi_start <= '1;
        nmi_in_progress <= '1;
        tb_clocks <= 0;
      end
      if (nmi_in_progress && (tb_clocks > nmi_width)) begin
        tb_clocks <= 0;
        nmi_in_progress <= '0;
        nmi_end <= '1;
      end
      if (nmi_end) begin
        nmi_count <= nmi_count - 1;
        nmi_interval <= cvm_rand::get("nmi_interval");
        nmi_width <= cvm_rand::get("nmi_width");
      end
    end
  end
  
  bit [31:0] prev_event_trigger = 0;
  bit interrupt_trigger_in_progress = 0;
  int unsigned captured_clocks = 0;
  bit event_based_interrupt = 0;
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

  int unsigned dut_clocks = 0;
  always @(posedge dut_clk) begin
    dut_clocks <= dut_clocks + 1;
  end

  // m_nmi_tick
  assign m_nmi_ticks[0].valid = ~dut_reset & (nmi_start | nmi_end) & (location != cvm_topology::nil);
  assign m_nmi_ticks[0].data.location = location;
  assign m_nmi_ticks[0].data.cycle = (nmi_start | nmi_end) ? dut_clocks : '0;

   // m_nmi_tick
  //assign m_event_trigger_ticks[0].valid = event_based_interrupt & (location != cvm_topology::nil);
  assign m_event_trigger_ticks[0].valid = 1'b1 & (location != cvm_topology::nil);
  assign m_event_trigger_ticks[0].data.location = location;
  assign m_event_trigger_ticks[0].data.event_trigger = event_trigger;

  // -------------------------
  // C++->SV Callbacks
  // -------------------------

  export "DPI-C" function interrupts_init;
  export "DPI-C" function interrupts_nmi;
  export "DPI-C" function trigger_interrupts_init;
  export "DPI-C" function trigger_interrupts;

  function void interrupts_init();
      /* verilator lint_off BLKSEQ */
      nmi = 0;
      /* verilator lint_on BLKSEQ */
  endfunction

  function void interrupts_nmi(bit val);
      nmi = val;
  endfunction

  function void trigger_interrupts_init();
   /* verilator lint_off BLKSEQ */
    nmi = 0;
    /* verilator lint_on BLKSEQ */
  endfunction

  function void trigger_interrupts(int val);
     /* verilator lint_off BLKSEQ */
    nmi = 0;
    $display("\ntrigger based interrupt %d\n",val);
    /* verilator lint_on BLKSEQ */
  endfunction
endmodule
