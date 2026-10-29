// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// ACLINT timer model (SV side; the C++ side is src/sysmod/aclint/aclint.cpp).
//   - mtime is a single 64-bit MMR: it free-runs +cfg_ref_time_incr per
//     reference-clock edge. A SW mtime write (DPI set_mtime) is staged and
//     committed synchronously on the next reference edge so mtime_q has a
//     single driver; mtime and get_mtime show the staged value meanwhile.
//   - MTIP per hart is the compare mtime >= mtimecmp[hart]; mtip_cl is the OR
//     of all nine compares (RTL MTIP[8], cluster wakeup).
//   - aclint_ref_pulse is a single-cycle reference strobe (one per reference
//     edge) broadcast to the cores. The CC boundary expects the pulse already
//     synchronized into its clock domain, so the edge detect stays here until
//     an asynchronous crossing is added at that boundary.
//   - aclint_time_sync is the two-phase local sync commit strobe (spec 7.5.2):
//     the C++ side stages X+Y into the cores and arms a commit value X+Y-Z;
//     the strobe asserts for LOCAL_SYNC_PULSE_CYCLES reference cycles once
//     mtime reaches it. The legacy single-step broadcast produces no strobe.
//   - aclint_ctime_wr pulses once per CTIME write the C++ side issues and
//     aclint_ctime_wr_data holds the value written, so checkers can tell a
//     model load from a software CTIME write.
// The C++ side owns the MMR read/write/broadcast and the configuration; it
// drives the model through the DPI functions below. The scope captured by
// sysmod_aclint_register_scope() lets it call them from its transactor-context
// handlers.
module aclint_model #(
    parameter int NHARTS = 1
  ) (
    input  logic              clk,
    input  logic              reset,
    input  logic              aclint_ref_clk,
    input  logic              aclint_ref_reset,
    output logic              aclint_ref_pulse,
    output logic              aclint_time_sync,
    output logic              aclint_ctime_wr,
    output logic [63:0]       aclint_ctime_wr_data,
    output logic [63:0]       mtime,
    output logic [NHARTS-1:0] mtip,
    output logic              mtip_cl,
    output logic              cfg_local_sync,
    output logic [15:0]       cfg_local_sync_latency,
    output logic [7:0]        cfg_timer_update_latency,
    output logic [7:0]        cfg_ref_time_incr
  );

  // mtimecmp0-7 (8 physical cores) plus mtimecmp8 (cluster wakeup) are always
  // implemented, independent of how many harts are populated. MTIP is still
  // only driven for the populated harts. A hart id at or above the core
  // compare count belongs to another cluster and has no slot in this model.
  localparam int NUM_MTIMECMP = 9;
  localparam int NUM_CORE_MTIMECMP = NUM_MTIMECMP - 1;

  // RTL CSR_MTIMECMP*_RESET_VALUE; kept in step with aclint.h.
  localparam logic [63:0] MTIMECMP_RESET = 64'hFFFFFFFF_FFFFFFFF;

  // o_local_time_sync pulse width in reference cycles (spec 7.5.2).
  localparam int unsigned LOCAL_SYNC_PULSE_CYCLES = 16;
  // The RTL sequencer evaluates the commit only after the staged CTIME write
  // has left the AXI master (LS_STAGE/LS_WAIT_MASTER), so mtime has moved on
  // from the written value by then; the same lead is applied here.
  localparam int unsigned LOCAL_SYNC_STAGE_CYCLES = 4;

  // Configuration handed over by the C++ side at construction (aclint_cfg.h);
  // the defaults are the legacy 100 MHz reference behaviour.
  logic        cfg_local_sync_dpi           = 1'b0;
  logic [15:0] cfg_local_sync_latency_dpi   = '0;
  logic [7:0]  cfg_timer_update_latency_dpi = '0;
  logic [7:0]  cfg_ref_time_incr_dpi        = 8'd10;
  assign cfg_local_sync           = cfg_local_sync_dpi;
  assign cfg_local_sync_latency   = cfg_local_sync_latency_dpi;
  assign cfg_timer_update_latency = cfg_timer_update_latency_dpi;
  assign cfg_ref_time_incr        = cfg_ref_time_incr_dpi;

  function void sysmod_aclint_set_cfg (byte unsigned local_sync, shortint unsigned local_sync_latency,
                                       byte unsigned timer_update_latency, byte unsigned ref_time_incr);
    cfg_local_sync_dpi           = local_sync[0];
    cfg_local_sync_latency_dpi   = local_sync_latency;
    cfg_timer_update_latency_dpi = timer_update_latency;
    cfg_ref_time_incr_dpi        = (ref_time_incr == 8'd0) ? 8'd10 : ref_time_incr;
  endfunction
  export "DPI-C" function sysmod_aclint_set_cfg;

  logic [63:0] mtime_q              = '0;
  logic [63:0] mtime_dpi            = '0;
  int unsigned mtime_wr_req         = 0;
  int unsigned mtime_wr_ack         = 0;
  logic [63:0] mtimecmp [NUM_MTIMECMP-1:0] = '{default: MTIMECMP_RESET};

  // A staged SW write is visible on mtime at once, as the RTL updates
  // AcCsrMtime before it broadcasts; mtime_q catches up on the next edge.
  assign mtime = (mtime_wr_req != mtime_wr_ack) ? mtime_dpi : mtime_q;

  // Compare writes are staged per register and committed on the reference edge
  // below, so mtimecmp keeps a single non-blocking driver and can be reset
  // there. Per-register req/ack pairs keep writes to distinct registers from
  // collapsing when they land within one reference period.
  logic [63:0] mtimecmp_dpi     [NUM_MTIMECMP-1:0] = '{default: MTIMECMP_RESET};
  int unsigned mtimecmp_wr_req  [NUM_MTIMECMP-1:0] = '{default: 0};
  int unsigned mtimecmp_wr_ack  [NUM_MTIMECMP-1:0] = '{default: 0};

  function void sysmod_aclint_set_mtimecmp (int unsigned hartid, longint unsigned val);
    mtimecmp_dpi[hartid] = val;
    mtimecmp_wr_req[hartid]++;
  endfunction
  export "DPI-C" function sysmod_aclint_set_mtimecmp;

  function void sysmod_aclint_set_mtime (longint unsigned val);
    mtime_dpi = val;
    mtime_wr_req++;
  endfunction
  export "DPI-C" function sysmod_aclint_set_mtime;

  function longint unsigned sysmod_aclint_get_mtime ();
    return (mtime_wr_req != mtime_wr_ack) ? mtime_dpi : mtime_q;
  endfunction
  export "DPI-C" function sysmod_aclint_get_mtime;

  // Two-phase local sync commit: the C++ side arms the commit mtime through a
  // req/ack counter pair; the strobe is generated in the reference domain
  // once mtime reaches it.
  int unsigned sync_req        = 0;
  int unsigned sync_ack        = 0;
  logic [63:0] sync_commit_dpi = '0;
  logic [63:0] sync_commit_q   = '0;
  logic        sync_armed      = 1'b0;
  logic [4:0]  sync_pulse_cnt  = '0;
  logic [3:0]  sync_stage_cnt  = '0;

  function void sysmod_aclint_sync_start (longint unsigned commit_mtime);
    sync_commit_dpi = commit_mtime;
    sync_req++;
  endfunction
  export "DPI-C" function sysmod_aclint_sync_start;

  function int sysmod_aclint_sync_active ();
    return ((sync_req != sync_ack) || sync_armed || (sync_pulse_cnt != '0)) ? 1 : 0;
  endfunction
  export "DPI-C" function sysmod_aclint_sync_active;

  // CTIME write strobe towards the checkers (one clk pulse per issued write).
  int unsigned ctime_wr_req = 0;
  int unsigned ctime_wr_ack = 0;
  logic        aclint_ctime_wr_q = '0;
  logic [63:0] ctime_wr_data_dpi = '0;
  logic [63:0] aclint_ctime_wr_data_q = '0;
  assign aclint_ctime_wr      = aclint_ctime_wr_q;
  assign aclint_ctime_wr_data = aclint_ctime_wr_data_q;

  function void sysmod_aclint_note_ctime_wr (longint unsigned value);
    ctime_wr_data_dpi = value;
    ctime_wr_req++;
  endfunction
  export "DPI-C" function sysmod_aclint_note_ctime_wr;

  import "DPI-C" context function void sysmod_aclint_register_scope ();
  initial sysmod_aclint_register_scope();

  // mtime advances cfg_ref_time_incr per reference-clock edge, matching the RTL
  // ACLINT. A staged SW write commits here so mtime_q keeps a single
  // synchronous driver. The write is handed off via a req/ack counter pair
  // (same idiom as the strobes below) so no variable mixes blocking and
  // non-blocking assignments, which verilator rejects (BLKANDNBLK).
  always @(posedge aclint_ref_clk) begin
    if (aclint_ref_reset) begin
      mtime_q      <= '0;
      mtime_wr_ack <= mtime_wr_req;
    end else if (mtime_wr_req != mtime_wr_ack) begin
      mtime_q      <= mtime_dpi;
      mtime_wr_ack <= mtime_wr_req;
    end else begin
      mtime_q <= mtime_q + 64'(cfg_ref_time_incr_dpi);
    end
  end

  always @(posedge aclint_ref_clk) begin
    for (int i = 0; i < NUM_MTIMECMP; i++) begin
      if (aclint_ref_reset) begin
        mtimecmp[i]        <= MTIMECMP_RESET;
        mtimecmp_wr_ack[i] <= mtimecmp_wr_req[i];
      end else if (mtimecmp_wr_req[i] != mtimecmp_wr_ack[i]) begin
        mtimecmp[i]        <= mtimecmp_dpi[i];
        mtimecmp_wr_ack[i] <= mtimecmp_wr_req[i];
      end
    end
  end

  // Commit strobe sequencer (RTL aclint_local_sync LS_WAIT_COMMIT/LS_PULSE).
  always @(posedge aclint_ref_clk) begin
    if (aclint_ref_reset) begin
      sync_armed     <= 1'b0;
      sync_pulse_cnt <= '0;
      sync_stage_cnt <= '0;
      sync_ack       <= sync_req;
    end else if (sync_req != sync_ack) begin
      sync_armed     <= 1'b1;
      sync_stage_cnt <= 4'(LOCAL_SYNC_STAGE_CYCLES);
      sync_commit_q  <= sync_commit_dpi;
      sync_ack       <= sync_req;
    end else if (sync_armed && (sync_stage_cnt != '0)) begin
      sync_stage_cnt <= sync_stage_cnt - 4'd1;
    end else if (sync_armed && (mtime_q >= sync_commit_q)) begin
      sync_armed     <= 1'b0;
      sync_pulse_cnt <= 5'(LOCAL_SYNC_PULSE_CYCLES);
    end else if (sync_pulse_cnt != '0) begin
      sync_pulse_cnt <= sync_pulse_cnt - 5'd1;
    end
  end
  assign aclint_time_sync = (sync_pulse_cnt != '0);

  // aclint_ref_pulse is a single-cycle strobe aligned to the aclint_ref_clk
  // rising edge (posedge-detected in the model clk domain, which equals the core
  // clk under the CCX clock override). One pulse per reference edge makes the RTL
  // CoreTime advance once per tick in step with mtime.
  logic [1:0] ref_clk_sync = '0;
  always @(posedge clk) begin
    if (reset) ref_clk_sync <= '0;
    else       ref_clk_sync <= {ref_clk_sync[0], aclint_ref_clk};
  end
  assign aclint_ref_pulse = ref_clk_sync[0] & ~ref_clk_sync[1];

  logic [NUM_MTIMECMP-1:0] mtip_cmp;
  for (genvar h = 0; h < NUM_MTIMECMP; h++) begin : gen_mtip_cmp
    assign mtip_cmp[h] = (mtime_q >= mtimecmp[h]);
  end
  for (genvar h = 0; h < NHARTS; h++) begin : gen_mtip
    if (h < NUM_CORE_MTIMECMP) begin : gen_core
      assign mtip[h] = mtip_cmp[h];
    end else begin : gen_no_compare
      assign mtip[h] = 1'b0;
    end
  end
  assign mtip_cl = |mtip_cmp;

  always @(posedge clk) begin
    if (reset) begin
      aclint_ctime_wr_q <= '0;
      ctime_wr_ack      <= ctime_wr_req;
    end else begin
      aclint_ctime_wr_q <= (ctime_wr_req != ctime_wr_ack);
      if (ctime_wr_req != ctime_wr_ack) aclint_ctime_wr_data_q <= ctime_wr_data_dpi;
      ctime_wr_ack      <= ctime_wr_req;
    end
  end
endmodule
