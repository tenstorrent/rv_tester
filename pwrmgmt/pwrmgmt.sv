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
  input logic [NDOMAINS-1:0] dut_reset,
  input int reset_count,
  input int target_reset_count,
  input logic warm_reset_en,
  output logic cold_reset,
  output logic warm_reset,
  output logic warm_reset_now,
  output logic [NHOLDS-1:0] reset_hold,
  output logic force_ref_clk,
  `RV_TESTER_TRANSACTIONS_PWRMGMT_OUTPUT_PORTS
);

  import "DPI-C" context function void pwrmgmt_set_scope(int unsigned location);

  int unsigned location = cvm_topology::nil;
  int unsigned warm_reset_interval = 0;
  logic tb_reset_d1 = 0;
  always @(posedge tb_clk) begin
    tb_reset_d1 <= tb_reset;
    if (tb_reset & ~tb_reset_d1) begin
      /* verilator lint_off BLKSEQ */
      location = cvm_topology::get_location(topology_pkg::mods.TOP.PLATFORM.PWRMGMT.ID, NUM);
      if (location != cvm_topology::nil) begin
        pwrmgmt_set_scope(location);
        if (reset_count <= 0)
          pwrmgmt_init();
        if (warm_reset_en)
          warm_reset_interval <= cvm_rand::get("warm_reset_interval");
      end
      /* verilator lint_on BLKSEQ */
    end
  end

  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------

  int unsigned tb_clocks = 0;
  int unsigned soc_clocks = 0;
  logic warm_reset_tick = 0;
  logic force_ref_clk_d1;

  always @(posedge tb_clk) begin
    force_ref_clk_d1 <= force_ref_clk;
    if (warm_reset_tick) begin
      tb_clocks <= 0;
    end else if (warm_reset_en & (reset_count < target_reset_count) & ~force_ref_clk) begin
      tb_clocks <= tb_clocks + 1;
      if (force_ref_clk_d1) begin
        warm_reset_interval <= cvm_rand::get("warm_reset_interval");
        $display("[%0d] [pwrmgmt] Target warm reset interval: %0d TB clocks", tb_clocks, warm_reset_interval);
      end
    end
  end

  always @(posedge dut_clk[SOC_CLK_IDX]) begin
    soc_clocks <= soc_clocks + 1;
    warm_reset_tick <= 0;
    if (warm_reset_en & (reset_count < target_reset_count) & (tb_clocks > warm_reset_interval) & ~force_ref_clk) begin
      $display("[%0d] [pwrmgmt] Warm reset now", tb_clocks);
      warm_reset_tick <= 1;
    end
  end

  assign warm_reset_now = warm_reset_tick;

  // m_tick
  // - during reset sequence till force_ref_clk is deasserted, send every clock
  // - after rest sequence, send a tick only to start a warm reset
  logic tick_valid;
  assign tick_valid = (init | force_ref_clk) & (location != cvm_topology::nil);
  assign m_ticks[0].valid = tick_valid;
  assign m_ticks[0].data.location = location;
  assign m_ticks[0].data.cycle = tick_valid ? soc_clocks : 0;

  // -------------------------
  // C++->SV Callbacks
  // -------------------------

  export "DPI-C" function pwrmgmt_init;
  export "DPI-C" function pwrmgmt_cold_reset;
  export "DPI-C" function pwrmgmt_warm_reset;
  export "DPI-C" function pwrmgmt_reset_hold;
  export "DPI-C" function pwrmgmt_force_ref_clk;

  function void pwrmgmt_init();
      /* verilator lint_off BLKSEQ */
      force_ref_clk = '1;
      cold_reset    = '1;
      warm_reset    = '0;
      reset_hold    = '0;
      /* verilator lint_on BLKSEQ */
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
