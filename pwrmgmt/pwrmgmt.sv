module pwrmgmt
import rv_tester_params::*;
#(
  parameter int NUM = -1,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_PWRMGMT_OUTPUT_PARAMS
)
(
  input logic clk [NCLKS-1:0],
  input logic reset [NCLKS-1:0],
  input logic sys_reset [NCLKS-1:0],
  input int reset_count,
  input int target_reset_count,
  input logic warm_reset_en,
  input logic tj_shutdown,
  input logic tj_max,
  input logic pll_dfs_done,
  input logic pll_shutdown_done,
  input logic terminate,
  output logic cold_reset,
  output logic warm_reset,
  output logic warm_reset_req,
  output logic [NHOLDS-1:0] reset_hold,
  output logic force_ref_clk,
  input logic core_no_fetch,
  `RV_TESTER_TRANSACTIONS_PWRMGMT_OUTPUT_PORTS
);

  import "DPI-C" context function void pwrmgmt_set_scope(int unsigned location);
  import "DPI-C" function void pwrmgmt_set_reset_count(int unsigned location, int count);

  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.PWRMGMT.ID, NUM);
  bit tj_max_shutdown_seq_en;
  int unsigned warm_reset_interval = 0;
  int unsigned warm_reset_clocks = 0;

  always @(posedge clk[TB_CLK_IDX]) begin
    if (sys_reset[TB_CLK_IDX]) begin
      /* verilator lint_off BLKSEQ */
      if (location != cvm_topology::nil) begin
        tj_max_shutdown_seq_en  <= (cvm_plusargs::get_bool("tj_max") != '0) || (cvm_plusargs::get_bool("tj_shutdown") != '0);
        pwrmgmt_set_scope(location);
        pwrmgmt_set_reset_count(location, reset_count);
        thub_blocking_sequence_tick_internal(0);
        if (reset_count < 0)
          //FIXME pwrmgmt_force_ref_clk(1);
          pwrmgmt_init_internal();
        if (warm_reset_en) begin
          warm_reset_interval = cvm_rand::get("warm_reset_interval");
          $display("[%0d] [pwrmgmt] Target warm reset count: %0d, current count: %0d, current interval: %0d TB clocks",
            warm_reset_clocks, target_reset_count, reset_count, warm_reset_interval);
        end
      end
      /* verilator lint_on BLKSEQ */
    end
  end

  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------

  int unsigned soc_clocks = 0;
  logic warm_reset_tick = 0;
  logic force_ref_clk_d1;
  logic cold_reset_d1;
  logic pll_dfs_done_d1;
  logic pll_shutdown_done_d1;
  bit tj_seq_ack;

  always @(posedge clk[TB_CLK_IDX]) begin
    if (warm_reset_tick) begin
      warm_reset_clocks <= 0;
    end else if (warm_reset_en & (reset_count < target_reset_count) & ~core_no_fetch & ~warm_reset) begin
      warm_reset_clocks <= warm_reset_clocks + 1;
    end
  end

  always @(posedge clk[SOC_CLK_IDX]) begin
    cold_reset_d1 <= cold_reset;
    force_ref_clk_d1 <= force_ref_clk;
    soc_clocks <= soc_clocks + 1;
    warm_reset_tick <= 0;
    pll_dfs_done_d1 <= pll_dfs_done;
    pll_shutdown_done_d1 <= pll_shutdown_done;
    if (!terminate & warm_reset_en & (reset_count < target_reset_count) & (warm_reset_clocks > warm_reset_interval) & ~core_no_fetch) begin
      $display("[%0d] [pwrmgmt] Warm reset now", warm_reset_clocks);
      warm_reset_tick <= 1;
    end
  end

  assign warm_reset_req = warm_reset_tick;

  // m_tick
  // - during reset sequence till force_ref_clk is deasserted, send every clock
  // - after reset sequence, send a tick only to start a warm reset
  logic tick_valid;
  assign tick_valid = (core_no_fetch | force_ref_clk | warm_reset_req) & (location != cvm_topology::nil);
  assign m_ticks[0].valid = tick_valid;
  assign m_ticks[0].data.location = location;
  assign m_ticks[0].data.cycle = tick_valid ? soc_clocks : 0;

  logic thub_blocking_seq_tick,tick_reset;
  assign tick_reset = core_no_fetch | force_ref_clk | warm_reset_req;

  // m_pcontrol_tick
  logic pcontrol_tick;
  rv_tester_tick_generator #(.NAME("pcontrol")) pcontrol_tick_generator (.clk(clk[SOC_CLK_IDX]), .reset(tick_reset), .inhibit('0), .tick(pcontrol_tick), .last());
  assign m_pcontrol_ticks[0].valid = pcontrol_tick && (location != cvm_topology::nil);
  assign m_pcontrol_ticks[0].data.location = location;

  // m_dfs_tick
  logic dfs_tick;
  rv_tester_tick_generator #(.NAME("dfs")) dfs_tick_generator (.clk(clk[SOC_CLK_IDX]), .reset(core_no_fetch), .inhibit('0), .tick(dfs_tick), .last());
  assign m_dfs_ticks[0].valid = dfs_tick && (location != cvm_topology::nil);
  assign m_dfs_ticks[0].data.location = location;

   // m_thub_tick
  logic thub_tick;
  rv_tester_tick_generator #(.NAME("thub")) thub_tick_generator (.clk(clk[SOC_CLK_IDX]), .reset(tick_reset), .inhibit(thub_blocking_seq_tick), .tick(thub_tick), .last());
  assign m_thub_ticks[0].valid = thub_tick && (location != cvm_topology::nil);
  assign m_thub_ticks[0].data.location = location;

 // m_cold_reset_ack
  logic cold_reset_ack_valid;
  assign cold_reset_ack_valid = (cold_reset_d1 && ~cold_reset);
  assign m_cold_reset_acks[0].valid = cold_reset_ack_valid && (location != cvm_topology::nil);
  assign m_cold_reset_acks[0].data.location = location;

  // m_force_ref_clk_ack
  logic force_ref_clk_ack_valid;
  assign force_ref_clk_ack_valid = (force_ref_clk_d1 && ~force_ref_clk);
  assign m_force_ref_clk_acks[0].valid = force_ref_clk_ack_valid && (location != cvm_topology::nil);
  assign m_force_ref_clk_acks[0].data.location = location;

   // m_tj_shutdown_ack
  logic pll_dfs_done_valid;
  assign pll_dfs_done_valid = (~pll_dfs_done_d1 && pll_dfs_done);
  assign m_tj_shutdown_acks[0].valid = tj_shutdown && pll_dfs_done_valid && (location != cvm_topology::nil);
  assign m_tj_shutdown_acks[0].data.location = location;

   // m_tj_max_ack
  logic pll_shotdown_done_valid;
  assign pll_shotdown_done_valid = (~pll_shutdown_done_d1 && pll_shutdown_done);
  assign m_tj_max_acks[0].valid = tj_max && pll_shotdown_done_valid && (location != cvm_topology::nil);
  assign m_tj_max_acks[0].data.location = location;

  // -------------------------
  // C++->SV Callbacks
  // -------------------------

  export "DPI-C" function pwrmgmt_init;
  export "DPI-C" function pwrmgmt_cold_reset;
  export "DPI-C" function pwrmgmt_warm_reset;
  export "DPI-C" function pwrmgmt_reset_hold;
  export "DPI-C" function pwrmgmt_force_ref_clk;

  function void pwrmgmt_init_internal();
      /* verilator lint_off BLKSEQ */
      force_ref_clk = '1;
      cold_reset    = '1;
      warm_reset    = '0;
      reset_hold    = '0;
      /* verilator lint_on BLKSEQ */
  endfunction


  function void pwrmgmt_init();
      pwrmgmt_init_internal();
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
      /* verilator lint_off BLKSEQ */
      force_ref_clk = val;
      /* verilator lint_on BLKSEQ */
  endfunction

  export "DPI-C" function thub_blocking_sequence_tick;

  function void thub_blocking_sequence_tick_internal(bit val);
      /* verilator lint_off BLKSEQ */
      thub_blocking_seq_tick = val;
      /* verilator lint_on BLKSEQ */
  endfunction

  function void thub_blocking_sequence_tick(bit val);
      thub_blocking_sequence_tick_internal(val);
  endfunction

  export "DPI-C" function func_tj_seq_ack;
  function void func_tj_seq_ack(bit val);
      tj_seq_ack = val;
  endfunction

  final begin
    if(thub_tick && tj_max_shutdown_seq_en && !tj_seq_ack) begin
      $display("ERROR: TJ_max/Shutdown didn't completed with Proper Ack...");
    end
  end


endmodule
