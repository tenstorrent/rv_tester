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
  input logic clk,
  input logic reset,
  input logic core_no_fetch,
  output logic nmi,
  `RV_TESTER_TRANSACTIONS_INTERRUPTS_OUTPUT_PORTS
);

  import "DPI-C" context function void interrupts_set_scope(int unsigned location);
  import "DPI-C" function bit interrupts_get_nmi_en(string mode);

  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.INTERRUPTS.ID, NUM);
  string nmi_mode;
  bit nmi_en;
  int unsigned nmi_count;
  int unsigned nmi_interval;
  int unsigned nmi_width;
  always @(posedge tb_clk) begin
    if (tb_reset) begin
      interrupts_init();
      /* verilator lint_off BLKSEQ */
      if (location != cvm_topology::nil) begin
        interrupts_set_scope(location);
        nmi_mode = cvm_plusargs::get_string("nmi");
        nmi_en = interrupts_get_nmi_en(nmi_mode);
        nmi_count = cvm_rand::get("nmi_count");
        nmi_interval = cvm_rand::get("nmi_interval");
        nmi_width = cvm_rand::get("nmi_width");
        $display("[interrupts] rand: nmi_count = %0d nmi_interval = %0d nmi_width = %0d",
          nmi_count, nmi_interval, nmi_width);
      end
      /* verilator lint_on BLKSEQ */
    end
  end

  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------

  int unsigned tb_clocks = 0;
  logic nmi_in_progress = 0;
  logic nmi_in_progress_d1 = 0;
  logic nmi_start;
  logic nmi_end;

  assign nmi_start = nmi_in_progress & ~nmi_in_progress_d1;
  assign nmi_end = ~nmi_in_progress & nmi_in_progress_d1;

  always @(posedge tb_clk) begin
    /* verilator lint_off BLKSEQ */
    //if (nmi_start | nmi_end) begin
    //  tb_clocks = 0;
    //end
    if (nmi_en && (nmi_count != 0) && ~&core_no_fetch) begin
      tb_clocks = tb_clocks + 1;
    end
    /* verilator lint_on BLKSEQ */
  end

  logic [NHARTS-1:0] core_no_fetch_d1;
  int unsigned dut_clocks = 0;
  always @(posedge clk) begin
    core_no_fetch_d1 <= core_no_fetch;
    nmi_in_progress_d1 <= nmi_in_progress;
    dut_clocks <= dut_clocks + 1;
    if (nmi_end) begin
      /* verilator lint_off BLKSEQ */
      tb_clocks = 0;
      nmi_count = nmi_count - 1;
      nmi_interval = cvm_rand::get("nmi_interval");
      nmi_width = cvm_rand::get("nmi_width");
      $display("[interrupts] rand: nmi_count = %0d nmi_interval = %0d nmi_width = %0d",
        nmi_count, nmi_interval, nmi_width);
      /* verilator lint_on BLKSEQ */
    end
    else if (nmi_in_progress && (tb_clocks > nmi_width)) begin
      nmi_in_progress = '0;
    end
    else if (tb_clocks > nmi_interval) begin
      nmi_in_progress = '1;
      tb_clocks = 0;
    end
  end

  // m_core_no_fetch
  assign m_core_no_fetchs[0].valid = (~&core_no_fetch & &core_no_fetch_d1) & (location != cvm_topology::nil);
  assign m_core_no_fetchs[0].data.location = location;
  assign m_core_no_fetchs[0].data.val = core_no_fetch;

  // m_nmi_tick
  assign m_nmi_ticks[0].valid = ~reset & (nmi_start | nmi_end) & (location != cvm_topology::nil);
  assign m_nmi_ticks[0].data.location = location;
  assign m_nmi_ticks[0].data.cycle = (nmi_start | nmi_end) ? tb_clocks : '0;

  // -------------------------
  // C++->SV Callbacks
  // -------------------------

  export "DPI-C" function interrupts_init;
  export "DPI-C" function interrupts_nmi;

  function void interrupts_init();
      /* verilator lint_off BLKSEQ */
      nmi = 0;
      /* verilator lint_on BLKSEQ */
  endfunction

  function void interrupts_nmi(bit val);
      nmi = val;
  endfunction

endmodule
