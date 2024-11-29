module interrupts
import rv_tester_params::*;
#(
  parameter int NUM = -1,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_INTERRUPTS_OUTPUT_PARAMS
)
(
  input logic clk,
  input logic sys_reset,
  input logic reset,
  input longint unsigned clocks,
  output logic nmi,
  `RV_TESTER_TRANSACTIONS_INTERRUPTS_OUTPUT_PORTS
);

  import "DPI-C" context function void interrupts_set_scope(int unsigned location);
  import "DPI-C" function bit interrupts_get_nmi_en(string mode);

  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.INTERRUPTS.ID, NUM);
  string nmi_mode;
  bit nmi_en;
  int unsigned nmi_count;
  int unsigned cur_nmi_count = 0;
  longint unsigned nmi_interval;
  longint unsigned nmi_width;
  always @(posedge clk) begin
    if (sys_reset) begin
      interrupts_init();
      /* verilator lint_off BLKSEQ */
      if (location != cvm_topology::nil) begin
        interrupts_set_scope(location);
        nmi_mode = cvm_plusargs::get_string("nmi");
        nmi_en = interrupts_get_nmi_en(nmi_mode);
        nmi_count = cvm_rand::get("nmi_count");
        /* verilator lint_off WIDTHEXPAND */
        nmi_interval = cvm_rand::get("nmi_start_interval");
        nmi_width = cvm_rand::get("nmi_width");
        /* verilator lint_on WIDTHEXPAND */
        if (nmi_en)
          $display("[interrupts] rand nmi[%0d]: interval=%0d, width=%0d",
            cur_nmi_count+1, nmi_interval, nmi_width);
      end
      /* verilator lint_on BLKSEQ */
    end
  end

  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------

  longint unsigned nxt_nmi_clock = -1;
  longint unsigned triggered_nmi_clock = -1;
  logic nmi_in_progress = 0;
  logic nmi_in_progress_d1 = 0;
  logic reset_d1;
  logic nmi_start;
  logic nmi_end;
  logic triggered_nmi_in_progress;
  logic triggered_nmi_in_progress_d1, triggered_nmi_in_progress_d2;
  logic triggered_nmi_end;

  assign nmi_start = nmi_in_progress & ~nmi_in_progress_d1;
  assign nmi_end = ~nmi_in_progress & nmi_in_progress_d1;
  assign triggered_nmi_end = ~triggered_nmi_in_progress_d1 & triggered_nmi_in_progress_d2;

  always @(posedge clk) begin
    reset_d1 <= reset;
    if (reset | ~nmi_en | (cur_nmi_count == nmi_count)) begin
      nmi_in_progress <= '0;
      nmi_in_progress_d1 <= '0;
      nxt_nmi_clock <= clocks + nmi_interval;
    end else begin
      nmi_in_progress_d1 <= nmi_in_progress;
      if (nxt_nmi_clock < 0)
        nxt_nmi_clock <= clocks + nmi_interval;
      if (cur_nmi_count < nmi_count) begin
        if (clocks >= nxt_nmi_clock && clocks < nxt_nmi_clock + nmi_width) begin
          nmi_in_progress <= '1;
        end else begin
          nmi_in_progress <= '0;
          if (clocks >= nxt_nmi_clock + nmi_width) begin
            cur_nmi_count <= cur_nmi_count + 1;
            if (cur_nmi_count + 1 < nmi_count) begin
              /* verilator lint_off BLKSEQ */
              /* verilator lint_off WIDTHEXPAND */
              nmi_interval = cvm_rand::get("nmi_interval");
              nmi_width = cvm_rand::get("nmi_width");
              /* verilator lint_on BLKSEQ */
              /* verilator lint_on WIDTHEXPAND */
              nxt_nmi_clock <= clocks + nmi_interval;
              $display("[interrupts] rand nmi[%0d]: interval=%0d, width=%0d",
                cur_nmi_count+2, nmi_interval, nmi_width);
            end
          end
        end
      end
    end
    /* verilator lint_off BLKSEQ */
    /* verilator lint_off WIDTHEXPAND */
    if (reset | ~nmi_en) begin
      triggered_nmi_in_progress = 0;
      triggered_nmi_in_progress_d1 <= '0;
      triggered_nmi_in_progress_d2 <= '0;
    end else begin
      triggered_nmi_in_progress_d1 <= triggered_nmi_in_progress;
      triggered_nmi_in_progress_d2 <= triggered_nmi_in_progress_d1;
      nmi_width = cvm_rand::get("nmi_width");
      if(triggered_nmi_in_progress & clocks >= triggered_nmi_clock + nmi_width) begin
        triggered_nmi_in_progress = 0;
      end
      triggered_nmi_in_progress_d1 <= triggered_nmi_in_progress;
    end
    /* verilator lint_on BLKSEQ */
    /* verilator lint_on WIDTHEXPAND */
  end

  // m_reset
  assign m_resets[0].valid = nmi_en & (~reset & reset_d1) & (location != cvm_topology::nil);
  assign m_resets[0].data.location = location;
  assign m_resets[0].data.cycle = clocks;

  // m_nmi_tick
  assign m_nmi_ticks[0].valid = ~reset & nmi_en & (nmi_start | nmi_end | triggered_nmi_end) & (location != cvm_topology::nil);
  assign m_nmi_ticks[0].data.location = location;
  assign m_nmi_ticks[0].data.cycle = (nmi_start | nmi_end) ? clocks : '0;

  // -------------------------
  // C++->SV Callbacks
  // -------------------------

  export "DPI-C" function interrupts_init;
  export "DPI-C" function interrupts_nmi;
  export "DPI-C" function interrupts_nmi_triggered;

  function void interrupts_init();
      /* verilator lint_off BLKSEQ */
      nmi = 0;
      /* verilator lint_on BLKSEQ */
  endfunction

  function void interrupts_nmi(bit val);
      nmi = val;
  endfunction

  function void interrupts_nmi_triggered(bit val);
      triggered_nmi_in_progress = 1;
      triggered_nmi_clock = clocks;
  endfunction

endmodule
