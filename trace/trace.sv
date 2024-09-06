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
  `RV_TESTER_TRANSACTIONS_TRACE_OUTPUT_PORTS
);

  import "DPI-C" context function void trace_set_scope(int unsigned location);

  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.TRACE.ID, NUM);
  always @(posedge tb_clk) begin
    if (tb_reset) begin
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

  logic [NHARTS-1:0] core_no_fetch_d1;
  int unsigned dut_clocks = 0;
  always @(posedge clk) begin
    core_no_fetch_d1 <= core_no_fetch;
    dut_clocks <= dut_clocks + 1;
  end

  // m_core_no_fetch
  assign m_core_no_fetchs[0].valid = (~|core_no_fetch & |core_no_fetch_d1) & (location != cvm_topology::nil);
  assign m_core_no_fetchs[0].data.location = location;
  assign m_core_no_fetchs[0].data.val = core_no_fetch;

  // m_tick
  //assign m_ticks[0].valid =  ~|core_no_fetch & (tb_clocks % 100 == 0) & (location != cvm_topology::nil);
  assign m_ticks[0].valid =  ~|core_no_fetch & (location != cvm_topology::nil);
  assign m_ticks[0].data.location = location;
  assign m_ticks[0].data.cycle = tb_clocks;

  // -------------------------
  // C++->SV Callbacks
  // -------------------------


endmodule
