// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// ACLINT timer model, SV side (C++ side: aclint.cpp owns the MMRs and drives
// this module over DPI from the scope captured by aclint_register_scope).
// mtime free-runs +ref_time_incr per reference edge; MTIP is mtime >= mtimecmp
// per hart, mtip_cl the OR of all nine compares (RTL MTIP[8]).
// aclint_time_sync is the two-phase commit strobe (spec 7.5.2), armed by the
// C++ side at X+Y-Z; the legacy broadcast produces none.
// aclint_ctime_wr/_data report each model CTIME write so checkers can tell a
// model load from a software write.
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
    output logic              mtip_cl
  );

  // mtimecmp0-7 plus mtimecmp8 (cluster wakeup), independent of the populated
  // harts; MTIP is driven for populated harts only.
  localparam int NUM_MTIMECMP = 9;
  localparam int NUM_CORE_MTIMECMP = NUM_MTIMECMP - 1;

  // RTL CSR_MTIMECMP*_RESET_VALUE; mirrored in aclint.h.
  localparam logic [63:0] MTIMECMP_RESET = 64'hFFFFFFFF_FFFFFFFF;

  // o_local_time_sync pulse width in reference cycles (spec 7.5.2).
  localparam int unsigned LOCAL_SYNC_PULSE_CYCLES = 16;
  // RTL evaluates the commit only after the staged write left the AXI master
  // (LS_STAGE/LS_WAIT_MASTER); same lead here.
  localparam int unsigned LOCAL_SYNC_STAGE_CYCLES = 4;

  // Tick increment from the core c_mscfg1 (AclintProgRefTimeIncr), CSR reset default.
  logic [7:0] ref_time_incr_dpi = 8'd10;

  function void aclint_set_ref_time_incr (byte unsigned ref_time_incr);
    ref_time_incr_dpi = (ref_time_incr == 8'd0) ? 8'd10 : ref_time_incr;
  endfunction
  export "DPI-C" function aclint_set_ref_time_incr;

  logic [63:0] mtime_q              = '0;
  logic [63:0] mtime_dpi            = '0;
  int unsigned mtime_wr_req         = 0;
  int unsigned mtime_wr_ack         = 0;
  logic [63:0] mtimecmp [NUM_MTIMECMP-1:0] = '{default: MTIMECMP_RESET};

  // A staged SW write is visible at once, as the RTL updates AcCsrMtime before it broadcasts.
  assign mtime = (mtime_wr_req != mtime_wr_ack) ? mtime_dpi : mtime_q;

  // Per-register req/ack staging keeps one non-blocking driver on mtimecmp and
  // keeps writes to distinct registers within one reference period apart.
  logic [63:0] mtimecmp_dpi     [NUM_MTIMECMP-1:0] = '{default: MTIMECMP_RESET};
  int unsigned mtimecmp_wr_req  [NUM_MTIMECMP-1:0] = '{default: 0};
  int unsigned mtimecmp_wr_ack  [NUM_MTIMECMP-1:0] = '{default: 0};

  function void aclint_set_mtimecmp (int unsigned hartid, longint unsigned val);
    mtimecmp_dpi[hartid] = val;
    mtimecmp_wr_req[hartid]++;
  endfunction
  export "DPI-C" function aclint_set_mtimecmp;

  function void aclint_set_mtime (longint unsigned val);
    mtime_dpi = val;
    mtime_wr_req++;
  endfunction
  export "DPI-C" function aclint_set_mtime;

  function longint unsigned aclint_get_mtime ();
    return (mtime_wr_req != mtime_wr_ack) ? mtime_dpi : mtime_q;
  endfunction
  export "DPI-C" function aclint_get_mtime;

  // Commit mtime armed by the C++ side; the strobe fires in the reference domain.
  int unsigned sync_req        = 0;
  int unsigned sync_ack        = 0;
  logic [63:0] sync_commit_dpi = '0;
  logic [63:0] sync_commit_q   = '0;
  logic [63:0] sync_target_q   = '0;
  logic        sync_armed      = 1'b0;
  logic [4:0]  sync_pulse_cnt  = '0;
  logic [3:0]  sync_stage_cnt  = '0;

  function void aclint_sync_start (longint unsigned commit_mtime);
    sync_commit_dpi = commit_mtime;
    sync_req++;
  endfunction
  export "DPI-C" function aclint_sync_start;

  function int aclint_sync_active ();
    return ((sync_req != sync_ack) || sync_armed || (sync_pulse_cnt != '0)) ? 1 : 0;
  endfunction
  export "DPI-C" function aclint_sync_active;

  // One clk pulse per issued CTIME write, for the checkers.
  int unsigned ctime_wr_req = 0;
  int unsigned ctime_wr_ack = 0;
  logic        aclint_ctime_wr_q = '0;
  logic [63:0] ctime_wr_data_dpi = '0;
  logic [63:0] aclint_ctime_wr_data_q = '0;
  assign aclint_ctime_wr      = aclint_ctime_wr_q;
  assign aclint_ctime_wr_data = aclint_ctime_wr_data_q;

  function void aclint_note_ctime_wr (longint unsigned value);
    ctime_wr_data_dpi = value;
    ctime_wr_req++;
  endfunction
  export "DPI-C" function aclint_note_ctime_wr;

  import "DPI-C" context function void aclint_register_scope ();
  initial aclint_register_scope();

  // Commit strobe start, reported with the staged target.
  import "DPI-C" context function void aclint_sync_pulse (longint unsigned mtime, longint unsigned target);

  // Staged SW writes commit here so mtime_q keeps a single synchronous driver;
  // the req/ack hand-off avoids mixing blocking and non-blocking assignments (BLKANDNBLK).
  always @(posedge aclint_ref_clk) begin
    if (aclint_ref_reset) begin
      mtime_q      <= '0;
      mtime_wr_ack <= mtime_wr_req;
    end else if (mtime_wr_req != mtime_wr_ack) begin
      mtime_q      <= mtime_dpi;
      mtime_wr_ack <= mtime_wr_req;
    end else begin
      mtime_q <= mtime_q + 64'(ref_time_incr_dpi);
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
      sync_target_q  <= ctime_wr_data_dpi;
      sync_ack       <= sync_req;
    end else if (sync_armed && (sync_stage_cnt != '0)) begin
      sync_stage_cnt <= sync_stage_cnt - 4'd1;
    end else if (sync_armed && (mtime_q >= sync_commit_q)) begin
      sync_armed     <= 1'b0;
      sync_pulse_cnt <= 5'(LOCAL_SYNC_PULSE_CYCLES);
      aclint_sync_pulse(mtime_q, sync_target_q);
    end else if (sync_pulse_cnt != '0) begin
      sync_pulse_cnt <= sync_pulse_cnt - 5'd1;
    end
  end
  assign aclint_time_sync = (sync_pulse_cnt != '0);

  // One strobe per reference edge, detected in the model clk domain; the CC
  // boundary expects the pulse already synchronised, so the edge detect stays
  // here until an asynchronous crossing exists there.
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
