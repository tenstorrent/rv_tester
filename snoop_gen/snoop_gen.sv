module snoop_gen
import rv_tester_params::*;
#(
  parameter int NUM = -1,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_SNOOP_GEN_OUTPUT_PARAMS
)
(
  input logic clk,
  input logic sys_reset,
  input logic reset,
  input logic [63:0] clocks,
  input logic [NHARTS-1:0] core_no_fetch,
  `RV_TESTER_TRANSACTIONS_SNOOP_GEN_OUTPUT_PORTS
);

  import "DPI-C" context function void snoop_gen_set_scope(int unsigned location);
  import "DPI-C" function bit get_rand_snoop_en();

  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.SNOOP_GEN.ID, NUM);
  bit rand_snoop_en = '0;

  always @(posedge clk) begin
    if (sys_reset) begin
      /* verilator lint_off BLKSEQ */
      rand_snoop_en = get_rand_snoop_en();
      /* verilator lint_on BLKSEQ */
      if (location != cvm_topology::nil) begin
        snoop_gen_set_scope(location);
      end
    end
  end

  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------

  // m_tick
  assign m_ticks[0].valid = rand_snoop_en & ~|core_no_fetch & (location != cvm_topology::nil);
  assign m_ticks[0].data.location = location;



endmodule
