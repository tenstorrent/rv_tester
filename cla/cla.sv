module cla
import rv_tester_params::*;
#(
  parameter int NUM = -1,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_CLA_OUTPUT_PARAMS
)
(
  input logic tb_clk,
  input logic tb_reset,
  input logic clk,
  input logic reset,
  input logic [NHARTS-1:0] core_no_fetch,
  input logic terminate_from_rv_tester,
  output logic terminate_cla_seq,
  `RV_TESTER_TRANSACTIONS_CLA_OUTPUT_PORTS
);

  import "DPI-C" context function void cla_set_scope(int unsigned location);
  import "DPI-C" function bit cla_cfg_seq_en_func();
  import "DPI-C" function void cla_send_elf_terminate();

  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.CLA.ID, NUM);
  bit cla_cfg_en = 0;
  logic terminate_from_rv_tester_d1;

  always @(posedge tb_clk) begin
    if (tb_reset) begin
      /* verilator lint_off BLKSEQ */
      cla_cfg_en = cla_cfg_seq_en_func();
      terminate_cla_seq = '0;
      /* verilator lint_on BLKSEQ */
      if (location != cvm_topology::nil) begin
        cla_set_scope(location);
      end
    end

    if(terminate_from_rv_tester && ~terminate_from_rv_tester_d1) begin
      cla_send_elf_terminate();
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
    terminate_from_rv_tester_d1 <= terminate_from_rv_tester;
    core_no_fetch_d1 <= core_no_fetch;
    dut_clocks <= dut_clocks + 1;
  end

  // m_cla_core_no_fetch
  assign m_core_no_fetchs[0].valid = (~|core_no_fetch & |core_no_fetch_d1) & (location != cvm_topology::nil);
  assign m_core_no_fetchs[0].data.location = location;
  assign m_core_no_fetchs[0].data.val = core_no_fetch;

  // m_cla_tick
  assign m_ticks[0].valid = cla_cfg_en & ~|core_no_fetch & (location != cvm_topology::nil);
  assign m_ticks[0].data.location = location;
  assign m_ticks[0].data.cycle = tb_clocks;

  // -------------------------
  // C++->SV Callbacks
  // -------------------------
  export "DPI-C" function terminate_cla_cfg_seq_func;

  function void terminate_cla_cfg_seq_func(bit terminate_test);
      /* verilator lint_off BLKSEQ */
      terminate_cla_seq = terminate_test;
      /* verilator lint_on BLKSEQ */
  endfunction

endmodule
