// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// ACLINT timer model (SV side; the C++ side is src/sysmod/aclint/aclint.cpp).
//   - mtime is a single 64-bit MMR: it free-runs +10 per reference-clock edge.
//     A SW mtime write (DPI set_mtime) is staged and committed synchronously on
//     the next reference edge so mtime_q has a single driver; get_mtime returns
//     the staged value in the meantime, so the C++ read/broadcast still observes
//     the written value.
//   - MTIP per hart is the compare mtime >= mtimecmp.
//   - aclint_ref_pulse is a single-cycle reference strobe (one per reference
//     edge) broadcast to the cores.
//   - aclint_time_sync is a one-cycle strobe emitted on each C++ time broadcast.
// The C++ side owns the MMR read/write/broadcast; it drives mtime/mtimecmp and
// the timesync strobe through the DPI functions below. The scope captured by
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
    output logic [63:0]       mtime,
    output logic [NHARTS-1:0] mtip
  );

  logic [63:0] mtime_q              = '0;
  logic [63:0] mtime_dpi            = '0;
  logic        mtime_dpi_pending    = 1'b0;
  logic [63:0] mtimecmp [NHARTS-1:0] = '{default: '1};

  assign mtime = mtime_q;

  function void sysmod_aclint_set_mtimecmp (int unsigned hartid, longint unsigned val);
    mtimecmp[hartid] = val;
  endfunction
  export "DPI-C" function sysmod_aclint_set_mtimecmp;

  function void sysmod_aclint_set_mtime (longint unsigned val);
    mtime_dpi         = val;
    mtime_dpi_pending = 1'b1;
  endfunction
  export "DPI-C" function sysmod_aclint_set_mtime;

  function longint unsigned sysmod_aclint_get_mtime ();
    return mtime_dpi_pending ? mtime_dpi : mtime_q;
  endfunction
  export "DPI-C" function sysmod_aclint_get_mtime;

  // TimeSync strobe : the C++ side bumps ts_sync_req on each
  // time broadcast; a one-cycle pulse is emitted when the request count differs
  // from the acknowledged count.
  int unsigned ts_sync_req = 0;
  int unsigned ts_sync_ack = 0;
  logic        aclint_time_sync_q = '0;
  assign aclint_time_sync = aclint_time_sync_q;

  function void sysmod_aclint_pulse_timesync ();
    ts_sync_req++;
  endfunction
  export "DPI-C" function sysmod_aclint_pulse_timesync;

  import "DPI-C" context function void sysmod_aclint_register_scope ();
  initial sysmod_aclint_register_scope();

  // mtime advances +10 per reference-clock edge, matching the RTL ACLINT. A
  // staged SW write commits here so mtime_q keeps a single synchronous driver.
  always @(posedge aclint_ref_clk) begin
    if (aclint_ref_reset) begin
      mtime_q <= '0;
    end else if (mtime_dpi_pending) begin
      mtime_q           <= mtime_dpi;
      mtime_dpi_pending <= 1'b0;
    end else begin
      mtime_q <= mtime_q + 64'd10;
    end
  end

  // aclint_ref_pulse is a single-cycle strobe aligned to the aclint_ref_clk
  // rising edge (posedge-detected in the model clk domain, which equals the core
  // clk under the CCX clock override). One pulse per reference edge makes the RTL
  // CoreTime advance +10 once per tick in step with mtime
  logic [1:0] ref_clk_sync = '0;
  always @(posedge clk) begin
    if (reset) ref_clk_sync <= '0;
    else       ref_clk_sync <= {ref_clk_sync[0], aclint_ref_clk};
  end
  assign aclint_ref_pulse = ref_clk_sync[0] & ~ref_clk_sync[1];

  for (genvar h = 0; h < NHARTS; h++) begin : gen_mtip
    assign mtip[h] = (mtime_q >= mtimecmp[h]);
  end

  always @(posedge clk) begin
    if (reset) begin
      aclint_time_sync_q <= '0;
      ts_sync_ack        <= ts_sync_req;
    end else begin
      aclint_time_sync_q <= (ts_sync_req != ts_sync_ack);
      ts_sync_ack        <= ts_sync_req;
    end
  end
endmodule
