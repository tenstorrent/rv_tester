// SPDX-FileCopyrightText: © 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// RVFI generator: unpacks C910's up-to-3-instruction ROB packets into per-instruction retire stream.
// SIMULATION/LOCKSTEP ONLY (VERILATOR COSIM: multi-write-port behavioural arrays; flattened buses).
// Flattened bus convention: field f of slot i occupies bus[i*W +: W].
module ct_rvfi_gen #(
  parameter NENT  = 4,       // ROB create entries (packets) per cycle
  parameter NRET  = 3,       // retire packet lanes (ROB retire width)
  parameter NOUT  = 3,       // per-instruction output records / cycle (3-wide)
  parameter NWB   = 5,       // register writeback capture ports
  parameter MAXPK = 3,       // max instructions per packet
  parameter XLEN  = 64,
  parameter VLEN  = 40,      // C910 virtual/physical address width (byte PC)
  parameter IIDW  = 7,       // ROB iid width (128 entries)
  parameter PREGW = 7        // physical register index width
) (
  input                       cpuclk,
  input                       rst_b,

  // Dispatch: NENT create entries (packets, program order)
  input  [NENT-1:0]           disp_ent_vld,
  input  [NENT*IIDW-1:0]      disp_ent_iid,
  input  [NENT*2-1:0]         disp_ent_num,
  input  [NENT*3-1:0]         disp_slot_len,
  input  [NENT*5-1:0]         disp_slot_rd_areg,
  input  [NENT-1:0]           disp_slot_rd_we,
  input  [NENT-1:0]           disp_slot_rd_fpr,
  input  [NENT*PREGW-1:0]     disp_slot_preg,
  input  [NENT*32-1:0]        disp_slot_insn,

  // Register writeback (preg-keyed)
  input  [NWB-1:0]            wb_vld,
  input  [NWB*PREGW-1:0]      wb_preg,
  input  [NWB*XLEN-1:0]       wb_data,

  // Retire (per packet lane, program order)
  input  [NRET-1:0]           retire_vld,
  input  [NRET*IIDW-1:0]      retire_iid,
  input  [NRET*VLEN-1:0]      retire_base_pc,
  input  [NRET-1:0]           retire_split,
  input  [NRET-1:0]           retire_trap,
  input  [NRET*XLEN-1:0]      retire_cause,
  input  [NRET-1:0]           retire_intr,
  input  [NRET*2-1:0]         retire_mode,

  // Flattened per-instruction RVFI export
  output [NOUT-1:0]           rvfi_valid,
  output [NOUT*32-1:0]        rvfi_insn,
  output [NOUT*XLEN-1:0]      rvfi_pc_rdata,
  output [NOUT*XLEN-1:0]      rvfi_pc_wdata,
  output [NOUT*5-1:0]         rvfi_rd_addr,
  output [NOUT-1:0]           rvfi_rd_we,
  output [NOUT-1:0]           rvfi_rd_fpr,
  output [NOUT*XLEN-1:0]      rvfi_rd_wdata,
  output [NOUT*XLEN-1:0]      rvfi_mem_addr,
  output [NOUT*8-1:0]         rvfi_mem_rmask,
  output [NOUT*8-1:0]         rvfi_mem_wmask,
  output [NOUT*XLEN-1:0]      rvfi_mem_rdata,
  output [NOUT*XLEN-1:0]      rvfi_mem_wdata,
  output [NOUT-1:0]           rvfi_trap,
  output [NOUT*XLEN-1:0]      rvfi_cause,
  output [NOUT-1:0]           rvfi_intr,
  output [NOUT*2-1:0]         rvfi_mode,
  output [NOUT*2-1:0]         rvfi_ixl,
  output [NOUT-1:0]           rvfi_last_uop
);

  localparam DEPTH = (1 << IIDW);
  localparam PDEPTH = (1 << PREGW);

  // IID-keyed packet table
  reg [1:0]           t_num    [DEPTH-1:0];
  reg [MAXPK*3-1:0]   t_len    [DEPTH-1:0];
  reg [MAXPK*5-1:0]   t_rd_areg[DEPTH-1:0];
  reg [MAXPK-1:0]     t_rd_we  [DEPTH-1:0];
  reg [MAXPK-1:0]     t_rd_fpr [DEPTH-1:0];
  reg [MAXPK*PREGW-1:0] t_preg [DEPTH-1:0];
  reg [MAXPK*32-1:0]  t_insn   [DEPTH-1:0];
  // Physical-register-keyed writeback data
  reg [XLEN-1:0]      t_preg_data [PDEPTH-1:0];
  // Per-preg "written" scoreboard (for non-blocking load retire)
  reg [PDEPTH-1:0]    t_preg_ready;

  integer e, k, w;
  integer start_slot;

  // Capture dispatch metadata + writeback data
  always @(posedge cpuclk) begin
    start_slot = 0;
    for (e = 0; e < NENT; e = e + 1) begin
      if (disp_ent_vld[e]) begin
        t_num[disp_ent_iid[e*IIDW +: IIDW]] <= disp_ent_num[e*2 +: 2];
        for (k = 0; k < MAXPK; k = k + 1) begin
          t_len   [disp_ent_iid[e*IIDW +: IIDW]][k*3 +: 3]       <= disp_slot_len[((start_slot+k) % NENT)*3 +: 3];
          t_rd_areg[disp_ent_iid[e*IIDW +: IIDW]][k*5 +: 5]      <= disp_slot_rd_areg[((start_slot+k) % NENT)*5 +: 5];
          t_rd_we [disp_ent_iid[e*IIDW +: IIDW]][k]              <= disp_slot_rd_we[(start_slot+k) % NENT];
          t_rd_fpr[disp_ent_iid[e*IIDW +: IIDW]][k]              <= disp_slot_rd_fpr[(start_slot+k) % NENT];
          t_preg  [disp_ent_iid[e*IIDW +: IIDW]][k*PREGW +: PREGW] <= disp_slot_preg[((start_slot+k) % NENT)*PREGW +: PREGW];
          t_insn  [disp_ent_iid[e*IIDW +: IIDW]][k*32 +: 32]      <= disp_slot_insn[((start_slot+k) % NENT)*32 +: 32];
          if (k < disp_ent_num[e*2 +: 2])
            t_preg_ready[disp_slot_preg[((start_slot+k) % NENT)*PREGW +: PREGW]] <= 1'b0;
        end
        start_slot = start_slot + disp_ent_num[e*2 +: 2];
      end
    end
    // Writeback after dispatch so same-cycle write wins
    for (w = 0; w < NWB; w = w + 1) begin
      if (wb_vld[w]) begin
        t_preg_data [wb_preg[w*PREGW +: PREGW]] <= wb_data[w*XLEN +: XLEN];
        t_preg_ready[wb_preg[w*PREGW +: PREGW]] <= 1'b1;
      end
    end
  end

  // Retire read-out: unpack packets, flatten to <=NOUT records
  reg [NOUT-1:0]      o_valid;
  reg [NOUT*32-1:0]   o_insn;
  reg [NOUT*XLEN-1:0] o_pc_rdata;
  reg [NOUT*XLEN-1:0] o_pc_wdata;
  reg [NOUT*5-1:0]    o_rd_addr;
  reg [NOUT-1:0]      o_rd_we;
  reg [NOUT-1:0]      o_rd_fpr;
  reg [NOUT*XLEN-1:0] o_rd_wdata;
  reg [NOUT-1:0]      o_trap;
  reg [NOUT*XLEN-1:0] o_cause;
  reg [NOUT-1:0]      o_intr;
  reg [NOUT*2-1:0]    o_mode;
  reg [NOUT-1:0]      o_last_uop;

  // In-order retire FIFO: stall non-blocking load retires until result writes back
  localparam MAXR   = NRET * MAXPK;
  localparam FDEPTH = 128;
  localparam FIDXW  = 7;
  localparam FPTRW  = 8;

  reg [XLEN-1:0]  f_pcr  [FDEPTH-1:0];
  reg [XLEN-1:0]  f_pcw  [FDEPTH-1:0];
  reg [4:0]       f_rda  [FDEPTH-1:0];
  reg             f_rdwe [FDEPTH-1:0];
  reg             f_rdfpr[FDEPTH-1:0];
  reg [PREGW-1:0] f_preg [FDEPTH-1:0];
  reg             f_trap [FDEPTH-1:0];
  reg [XLEN-1:0]  f_cause[FDEPTH-1:0];
  reg             f_intr [FDEPTH-1:0];
  reg [1:0]       f_mode [FDEPTH-1:0];
  reg             f_luop [FDEPTH-1:0];
  reg [31:0]      f_insn [FDEPTH-1:0];
  reg [FPTRW-1:0] fhead, ftail;

  // Push candidates (this cycle's retired sub-records, compacted)
  reg [XLEN-1:0]  p_pcr  [MAXR-1:0];
  reg [XLEN-1:0]  p_pcw  [MAXR-1:0];
  reg [4:0]       p_rda  [MAXR-1:0];
  reg             p_rdwe [MAXR-1:0];
  reg             p_rdfpr[MAXR-1:0];
  reg [PREGW-1:0] p_preg [MAXR-1:0];
  reg             p_trap [MAXR-1:0];
  reg [XLEN-1:0]  p_cause[MAXR-1:0];
  reg             p_intr [MAXR-1:0];
  reg [1:0]       p_mode [MAXR-1:0];
  reg             p_luop [MAXR-1:0];
  reg [31:0]      p_insn [MAXR-1:0];
  integer         p_cnt;

  integer l, oi, di, pi, fi;
  reg [FPTRW-1:0] idx;
  reg [FPTRW-1:0] occ;    // occupancy, computed in pointer width so it wraps
  reg             stopped;
  reg [IIDW-1:0]   iid;
  reg [1:0]        num;
  reg [VLEN-1:0]   base_pc;
  reg [VLEN-1:0]   cur_pc;
  reg [PREGW-1:0]  preg;
  reg [XLEN-1:0]   pc_sx;
  reg [XLEN-1:0]   nxt_sx;
  reg [VLEN-1:0]   nxt_pc;
  reg [XLEN-1:0]   rd_val;

  // Read preg result with same-cycle writeback forwarding
  function [XLEN-1:0] preg_read;
    input [PREGW-1:0] p;
    integer wi;
    begin
      preg_read = t_preg_data[p];
      for (wi = 0; wi < NWB; wi = wi + 1) begin
        if (wb_vld[wi] && (wb_preg[wi*PREGW +: PREGW] == p))
          preg_read = wb_data[wi*XLEN +: XLEN];
      end
    end
  endfunction

  // Preg result ready: written since dispatch or this cycle
  function preg_ready_f;
    input [PREGW-1:0] p;
    integer wi;
    begin
      preg_ready_f = t_preg_ready[p];
      for (wi = 0; wi < NWB; wi = wi + 1) begin
        if (wb_vld[wi] && (wb_preg[wi*PREGW +: PREGW] == p))
          preg_ready_f = 1'b1;
      end
    end
  endfunction

  // Push flatten: unpack retiring packets into p_* records
  always @(*) begin
    for (pi = 0; pi < MAXR; pi = pi + 1) begin
      p_pcr[pi]  = {XLEN{1'b0}}; p_pcw[pi]  = {XLEN{1'b0}};
      p_rda[pi]  = 5'b0;         p_rdwe[pi] = 1'b0;
      p_rdfpr[pi]= 1'b0;         p_preg[pi] = {PREGW{1'b0}};
      p_trap[pi] = 1'b0;         p_cause[pi]= {XLEN{1'b0}};
      p_intr[pi] = 1'b0;         p_mode[pi] = 2'b0;
      p_luop[pi] = 1'b1;         p_insn[pi] = 32'b0;
    end
    oi = 0;
    for (l = 0; l < NRET; l = l + 1) begin
      if (retire_vld[l]) begin
        iid     = retire_iid[l*IIDW +: IIDW];
        num     = t_num[iid];
        base_pc = retire_base_pc[l*VLEN +: VLEN];
        cur_pc  = base_pc;
        for (k = 0; k < MAXPK; k = k + 1) begin
          if ((k < num) && (oi < MAXR)) begin
            preg   = t_preg[iid][k*PREGW +: PREGW];
            nxt_pc = cur_pc + {{(VLEN-4){1'b0}}, t_len[iid][k*3 +: 3], 1'b0};
            p_pcr[oi]  = {{(XLEN-VLEN){cur_pc[VLEN-1]}}, cur_pc};
            p_pcw[oi]  = {{(XLEN-VLEN){nxt_pc[VLEN-1]}}, nxt_pc};
            p_rda[oi]  = t_rd_areg[iid][k*5 +: 5];
            // Exclude x0 writes; else the retire FIFO stalls on a preg that never writes back.
            p_rdwe[oi] = t_rd_we[iid][k] && (t_rd_areg[iid][k*5 +: 5] != 5'b0)
                         && !t_rd_fpr[iid][k];
            p_rdfpr[oi]= t_rd_fpr[iid][k];
            p_preg[oi] = preg;
            // Trap/cause/intr on last sub of packet
            if (k == (num - 1)) begin
              p_trap[oi]  = retire_trap[l];
              p_cause[oi] = retire_cause[l*XLEN +: XLEN];
              p_intr[oi]  = retire_intr[l];
            end
            p_mode[oi] = retire_mode[l*2 +: 2];
            p_insn[oi] = t_insn[iid][k*32 +: 32];
            // retire_split = non-final uop of a cracked insn (jal/jalr/amo); last_uop=0 so cosim coalesces it.
            p_luop[oi] = ~retire_split[l];
            cur_pc = nxt_pc;
            oi = oi + 1;
          end
        end
      end
    end
    p_cnt = oi;
  end

  // Pop: drain head records whose destination result is ready
  always @(*) begin
    o_valid    = {NOUT{1'b0}};
    o_insn     = {(NOUT*32){1'b0}};
    o_pc_rdata = {(NOUT*XLEN){1'b0}};
    o_pc_wdata = {(NOUT*XLEN){1'b0}};
    o_rd_addr  = {(NOUT*5){1'b0}};
    o_rd_we    = {NOUT{1'b0}};
    o_rd_fpr   = {NOUT{1'b0}};
    o_rd_wdata = {(NOUT*XLEN){1'b0}};
    o_trap     = {NOUT{1'b0}};
    o_cause    = {(NOUT*XLEN){1'b0}};
    o_intr     = {NOUT{1'b0}};
    o_mode     = {(NOUT*2){1'b0}};
    o_last_uop = {NOUT{1'b1}};
    stopped    = 1'b0;
    occ        = ftail - fhead;
    for (di = 0; di < NOUT; di = di + 1) begin
      idx = (fhead + di[FPTRW-1:0]) & {{(FPTRW-FIDXW){1'b0}}, {FIDXW{1'b1}}};
      if (!stopped && (occ > di[FPTRW-1:0]) &&
          (!f_rdwe[idx[FIDXW-1:0]] || preg_ready_f(f_preg[idx[FIDXW-1:0]]))) begin
        o_valid[di]                 = 1'b1;
        o_pc_rdata[di*XLEN +: XLEN] = f_pcr[idx[FIDXW-1:0]];
        o_pc_wdata[di*XLEN +: XLEN] = f_pcw[idx[FIDXW-1:0]];
        o_rd_addr[di*5 +: 5]        = f_rda[idx[FIDXW-1:0]];
        o_rd_we[di]                 = f_rdwe[idx[FIDXW-1:0]];
        o_rd_fpr[di]                = f_rdfpr[idx[FIDXW-1:0]];
        o_rd_wdata[di*XLEN +: XLEN] = f_rdwe[idx[FIDXW-1:0]] ? preg_read(f_preg[idx[FIDXW-1:0]]) : {XLEN{1'b0}};
        o_trap[di]                  = f_trap[idx[FIDXW-1:0]];
        o_cause[di*XLEN +: XLEN]    = f_cause[idx[FIDXW-1:0]];
        o_intr[di]                  = f_intr[idx[FIDXW-1:0]];
        o_mode[di*2 +: 2]           = f_mode[idx[FIDXW-1:0]];
        o_last_uop[di]              = f_luop[idx[FIDXW-1:0]];
        o_insn[di*32 +: 32]         = f_insn[idx[FIDXW-1:0]];
      end else begin
        stopped = 1'b1;
      end
    end
  end

  // Count of records drained this cycle
  reg [FPTRW-1:0] drained;
  always @(*) begin
    drained = {FPTRW{1'b0}};
    for (di = 0; di < NOUT; di = di + 1)
      if (o_valid[di]) drained = drained + 1'b1;
  end


  // FIFO pointer/storage update
  always @(posedge cpuclk) begin
    if (rst_b == 1'b0) begin
      fhead <= {FPTRW{1'b0}};
      ftail <= {FPTRW{1'b0}};
    end else begin
      for (fi = 0; fi < MAXR; fi = fi + 1) begin
        if (fi < p_cnt) begin
          idx                      = (ftail + fi[FPTRW-1:0]) & {{(FPTRW-FIDXW){1'b0}}, {FIDXW{1'b1}}};
          f_pcr  [idx[FIDXW-1:0]] <= p_pcr[fi];
          f_pcw  [idx[FIDXW-1:0]] <= p_pcw[fi];
          f_rda  [idx[FIDXW-1:0]] <= p_rda[fi];
          f_rdwe [idx[FIDXW-1:0]] <= p_rdwe[fi];
          f_rdfpr[idx[FIDXW-1:0]] <= p_rdfpr[fi];
          f_preg [idx[FIDXW-1:0]] <= p_preg[fi];
          f_trap [idx[FIDXW-1:0]] <= p_trap[fi];
          f_cause[idx[FIDXW-1:0]] <= p_cause[fi];
          f_intr [idx[FIDXW-1:0]] <= p_intr[fi];
          f_mode [idx[FIDXW-1:0]] <= p_mode[fi];
          f_luop [idx[FIDXW-1:0]] <= p_luop[fi];
          f_insn [idx[FIDXW-1:0]] <= p_insn[fi];
        end
      end
      ftail <= ftail + p_cnt[FPTRW-1:0];
      fhead <= fhead + drained;
    end
  end

  assign rvfi_valid    = o_valid;
  assign rvfi_insn     = o_insn;
  assign rvfi_pc_rdata = o_pc_rdata;
  assign rvfi_pc_wdata = o_pc_wdata;
  assign rvfi_rd_addr  = o_rd_addr;
  assign rvfi_rd_we    = o_rd_we;
  assign rvfi_rd_fpr   = o_rd_fpr;
  assign rvfi_rd_wdata = o_rd_wdata;
  assign rvfi_mem_addr  = {(NOUT*XLEN){1'b0}};
  assign rvfi_mem_rmask = {(NOUT*8){1'b0}};
  assign rvfi_mem_wmask = {(NOUT*8){1'b0}};
  assign rvfi_mem_rdata = {(NOUT*XLEN){1'b0}};
  assign rvfi_mem_wdata = {(NOUT*XLEN){1'b0}};
  assign rvfi_trap     = o_trap;
  assign rvfi_cause    = o_cause;
  assign rvfi_intr     = o_intr;
  assign rvfi_mode     = o_mode;
  assign rvfi_last_uop = o_last_uop;

  genvar g;
  generate
    for (g = 0; g < NOUT; g = g + 1) begin : g_ixl
      assign rvfi_ixl[g*2 +: 2] = 2'b10;
    end
  endgenerate

endmodule
