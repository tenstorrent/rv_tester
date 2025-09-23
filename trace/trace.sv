module trace
import rv_tester_params::*;
#(
  parameter int NUM = -1,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_TRACE_OUTPUT_PARAMS
)
(
  input logic tb_clk,
  input logic tb_reset,
  input logic clk,
  input logic reset,
  input logic [NHARTS-1:0] core_no_fetch,
  input logic [NHARTS-1:0][31:0]cycles_since_retire,
  input logic [31:0]max_stall_detect_cycle,
  output logic warm_reset_release_hang,
  output logic terminate_ntrace_test,
  output logic terminate_dst_trace_seq,
  `RV_TESTER_TRANSACTIONS_TRACE_OUTPUT_PORTS
);

  import "DPI-C" context function void trace_set_scope(int unsigned location);
  import "DPI-C" function bit trace_stop_on_wrap_en_func();
  import "DPI-C" function bit dst_trace_seq_en_func();


  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.TRACE.ID, NUM);
  bit trace_stop_on_wrap_en = '0;
  bit dst_trace_seq_en = '0;

  always @(posedge tb_clk) begin
    if (tb_reset) begin
      /* verilator lint_off BLKSEQ */
      trace_stop_on_wrap_en = trace_stop_on_wrap_en_func();
      dst_trace_seq_en = dst_trace_seq_en_func();
      warm_reset_release_hang = '0;
      terminate_ntrace_test = '0;
      terminate_dst_trace_seq = '0;
      /* verilator lint_on BLKSEQ */
      if (location != cvm_topology::nil) begin
        trace_set_scope(location);
      end
    end
  end

  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------

  int unsigned tb_clocks = 0;
  always @(posedge tb_clk) begin
    tb_clocks <= tb_clocks + 1;
  end

  int unsigned dut_clocks = 0;
  logic [NHARTS-1:0] core_no_fetch_d1;
  logic detect_core_hang, detect_core_hang_d1, detect_core_hang_valid;
  always @(posedge clk) begin
    detect_core_hang_d1 <= detect_core_hang;
    core_no_fetch_d1 <= core_no_fetch;
    dut_clocks <= dut_clocks + 1;
  end

  assign detect_core_hang_valid = detect_core_hang & ~detect_core_hang_d1;
  always_comb begin
    detect_core_hang = '0;
    for (int i = 0; i < NHARTS; i++) begin
      detect_core_hang |= (cycles_since_retire[i] > max_stall_detect_cycle);
    end
  end

  // m_core_no_fetch
  assign m_core_no_fetchs[0].valid = (~|core_no_fetch & |core_no_fetch_d1) & (location != cvm_topology::nil);
  assign m_core_no_fetchs[0].data.location = location;
  assign m_core_no_fetchs[0].data.val = core_no_fetch;

  // m_tick
  assign m_ticks[0].valid = (trace_stop_on_wrap_en || dst_trace_seq_en) & ~|core_no_fetch & (location != cvm_topology::nil);
  assign m_ticks[0].data.location = location;
  assign m_ticks[0].data.cycle = tb_clocks;

  // m_detect_core_hang
  assign m_detect_core_hangs[0].valid = detect_core_hang_valid & ~|core_no_fetch & (location != cvm_topology::nil);
  assign m_detect_core_hangs[0].data.location = location;

  // -------------------------
  // C++->SV Callbacks
  // -------------------------
  export "DPI-C" function terminate_ntrace_test_func;
  export "DPI-C" function terminate_dst_trace_seq_func;
  export "DPI-C" function warm_reset_release_corehang_func;

  function void terminate_ntrace_test_func(bit terminate_test);
      /* verilator lint_off BLKSEQ */
      terminate_ntrace_test = terminate_test;
      /* verilator lint_on BLKSEQ */
  endfunction

  function void terminate_dst_trace_seq_func(bit terminate_test);
      /* verilator lint_off BLKSEQ */
      terminate_dst_trace_seq = terminate_test;
      /* verilator lint_on BLKSEQ */
  endfunction

  function void warm_reset_release_corehang_func(bit warm_reset_req);
      /* verilator lint_off BLKSEQ */
      warm_reset_release_hang = warm_reset_req;
      /* verilator lint_on BLKSEQ */
  endfunction

endmodule
