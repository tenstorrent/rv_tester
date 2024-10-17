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
  input longint unsigned clocks,
  `RV_TESTER_TRANSACTIONS_SNOOP_GEN_OUTPUT_PORTS
);

  import "DPI-C" context function void snoop_gen_set_scope(int unsigned location);

  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.SNOOP_GEN.ID, NUM);
  always @(posedge clk) begin
    if (sys_reset) begin
      if (location != cvm_topology::nil) begin
        snoop_gen_set_scope(location);
      end
    end
  end

 



endmodule
