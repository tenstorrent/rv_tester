module overlay_driver




import rv_tester_params::*;
#(
  parameter int NUM = -1,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_OVERLAY_DRIVER_OUTPUT_PARAMS
)
(
  input logic clk,
  input logic reset,
  input logic dut_clk,
  input logic dut_reset,
  input logic no_fetch,

  `RV_TESTER_TRANSACTIONS_OVERLAY_DRIVER_OUTPUT_PORTS
);
  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.OVERLAY_DRIVER.ID, NUM);
  import "DPI-C" context function void overlay_driver_set_scope(int unsigned location);
   // -------------------------
  // C++->SV Callbacks
  // -------------------------


  
  int unsigned push_idx;
  logic reset_d1;


  

 


  ////////////////////////////////////

  always @(posedge clk) begin
    reset_d1 <= reset;
    if (~reset & reset_d1) begin
      if (location != cvm_topology::nil) begin
        overlay_driver_set_scope(location);
      end
    end
  end

  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------

 

  int unsigned dut_clocks = 0;
  always @(posedge dut_clk) begin
    dut_clocks <= dut_clocks + 1;
  end

  // m_overlay_driver_tick
  assign m_overlay_driver_ticks[0].valid = ~dut_reset & ~no_fetch;
  assign m_overlay_driver_ticks[0].data.location = location;
  assign m_overlay_driver_ticks[0].data.cycle =  dut_clocks;


endmodule
