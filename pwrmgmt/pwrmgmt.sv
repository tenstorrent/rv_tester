module pwrmgmt
import rv_tester_params::*;
#(
  parameter int NUM = -1,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_PWRMGMT_OUTPUT_PARAMS
)
(
  input logic tb_clk,
  input logic tb_reset,
  input logic dut_clk [NCLKS-1:0],
  input logic [NDOMAINS-1:0] dut_reset,
  input logic [NHARTS-1:0] core_no_fetch,
  output logic cold_reset,
  output logic warm_reset,
  output logic [NHOLDS-1:0] reset_hold,
  output logic force_ref_clk,
  `RV_TESTER_TRANSACTIONS_PWRMGMT_OUTPUT_PORTS
);

  import "DPI-C" context function void pwrmgmt_set_scope(int unsigned location);
  import "DPI-C" function int unsigned warm_reset_rand_get(string plusarg);

  int unsigned location = cvm_topology::nil;
  always @(posedge tb_clk) begin
    if (tb_reset) begin
      /* verilator lint_off BLKSEQ */
      location = cvm_topology::get_location(topology_pkg::mods.TOP.PLATFORM.PWRMGMT.ID, NUM);
      if (location != cvm_topology::nil) begin
        pwrmgmt_set_scope(location);
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

  always @(posedge tb_clk) begin
    if (~&core_no_fetch) begin
      tb_clocks <= tb_clocks + 1;
      if (|core_no_fetch_d1) begin
        warm_reset_interval <= warm_reset_rand_get("warm_reset_interval");
      end
    end
    else if (warm_reset_tick) begin
      tb_clocks <= 0;
    end
  end

  always @(posedge dut_clk[SOC_CLK_IDX]) begin
    core_no_fetch_d1 <= core_no_fetch;
    soc_clocks <= soc_clocks + 1;
    warm_reset_tick <= 0;
    if (tb_clocks >= warm_reset_interval) begin
      warm_reset_tick <= 1;
    end
  end

  // m_tick
  // - during reset sequence till core_no_fetch is deasserted, send every clock
  // - after rest sequence, send a tick only to start a warm reset
  assign m_ticks[0].valid = (|core_no_fetch | warm_reset_tick) & (location != cvm_topology::nil);
  assign m_ticks[0].data.location = location;
  assign m_ticks[0].data.cycle = soc_clocks;

  // m_nofetch
  assign m_nofetchs[0].valid = (~&core_no_fetch & |core_no_fetch_d1) & (location != cvm_topology::nil);
  assign m_nofetchs[0].data.location = location;
  assign m_nofetchs[0].data.cycle = soc_clocks;

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
