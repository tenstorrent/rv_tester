module pwrmgmt
import rv_tester_params::*;
#(
  parameter int NUM = -1,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_PWRMGMT_OUTPUT_PARAMS
)
(
  input logic init,
  input logic tb_clk,
  input logic tb_reset,
  input logic dut_clk [NCLKS-1:0],
  input logic dut_reset [NCLKS-1:0],
  input logic [NHARTS-1:0] core_no_fetch,
  output logic cold_reset,
  output logic warm_reset,
  output logic [NHOLDS-1:0] reset_hold,
  output logic force_ref_clk,
  `RV_TESTER_TRANSACTIONS_PWRMGMT_OUTPUT_PORTS
);

  import "DPI-C" context function void pwrmgmt_set_scope(int unsigned location);
  import "DPI-C" function bit pwrmgmt_get_warm_reset_en(string mode);

  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.PWRMGMT.ID, NUM);
  int unsigned warm_reset_count;
  string warm_reset_mode;
  bit warm_reset_en;
  always @(posedge tb_clk) begin
    if (tb_reset) begin
      /* verilator lint_off BLKSEQ */
      if (location != cvm_topology::nil) begin
        pwrmgmt_set_scope(location);
        warm_reset_mode = cvm_plusargs::get_string("warm_reset");
        warm_reset_en = pwrmgmt_get_warm_reset_en(warm_reset_mode);
        warm_reset_count = cvm_rand::get("warm_reset_count");
      end
      /* verilator lint_on BLKSEQ */
    end
  end

  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------

  int unsigned tb_clocks = 0;
  int unsigned soc_clocks = 0;
  int unsigned warm_reset_interval = 0;
  bit warm_reset_tick = 0;
  logic [NHARTS-1:0] core_no_fetch_d1;
  logic warm_reset_d1;
  logic force_ref_clk_d1;

  always @(posedge tb_clk) begin
    if (warm_reset_en & (warm_reset_count != 0) & ~force_ref_clk) begin
      tb_clocks <= tb_clocks + 1;
      if (force_ref_clk_d1) begin
        warm_reset_interval <= cvm_rand::get("warm_reset_interval");
      end
    end
    else if (warm_reset_tick) begin
      tb_clocks <= 0;
    end
  end

  always @(posedge dut_clk[SOC_CLK_IDX]) begin
    core_no_fetch_d1 <= core_no_fetch;
    warm_reset_d1 <= warm_reset;
    force_ref_clk_d1 <= force_ref_clk;
    soc_clocks <= soc_clocks + 1;
    warm_reset_tick <= 0;
    if ((tb_clocks > warm_reset_interval) & ~force_ref_clk) begin
      warm_reset_tick <= 1;
    end
    /* verilator lint_off BLKSEQ */
    if (warm_reset & ~warm_reset_d1) begin
      warm_reset_count = warm_reset_count - 1;
    end
    /* verilator lint_on BLKSEQ */
  end

  // m_tick
  // - during reset sequence till force_ref_clk is deasserted, send every clock
  // - after rest sequence, send a tick only to start a warm reset
  assign m_ticks[0].valid =  (init | force_ref_clk | warm_reset_tick) & (location != cvm_topology::nil);
  assign m_ticks[0].data.location = location;
  assign m_ticks[0].data.cycle = soc_clocks;

  // m_force_ref_clk
  assign m_force_ref_clks[0].valid = (~force_ref_clk & force_ref_clk_d1) & (location != cvm_topology::nil);
  assign m_force_ref_clks[0].data.location = location;
  assign m_force_ref_clks[0].data.cycle = soc_clocks;

  // -------------------------
  // C++->SV Callbacks
  // -------------------------

  export "DPI-C" function pwrmgmt_init;
  export "DPI-C" function pwrmgmt_cold_reset;
  export "DPI-C" function pwrmgmt_warm_reset;
  export "DPI-C" function pwrmgmt_reset_hold;
  export "DPI-C" function pwrmgmt_force_ref_clk;

  function void pwrmgmt_init();
      force_ref_clk = '1;
      cold_reset    = '1;
      warm_reset    = '0;
      reset_hold    = '0;
  endfunction

  function void pwrmgmt_cold_reset(bit val);
      cold_reset = val;
  endfunction

  function void pwrmgmt_warm_reset(bit val);
      warm_reset = val;
  endfunction

  function void pwrmgmt_reset_hold(bit sram, bit debug, bit critical);
      reset_hold[SRAM_HOLD_IDX] = sram;
      reset_hold[DEBUG_HOLD_IDX] = debug;
      reset_hold[CRITICAL_HOLD_IDX] = critical;
  endfunction

  function void pwrmgmt_force_ref_clk(bit val);
      force_ref_clk = val;
  endfunction

endmodule
