// SPDX-FileCopyrightText: © 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// Bridge between XuanTie C910 dual-core top and rv_tester (single-hart lockstep: core0 DUT, core1 reset).
module openc910_test_harness
    import rv_tester_params::*;
#(
    `TOPOLOGY
) (
    `RV_TESTER_PORTS
);

    localparam int NRET = topology.TOP.PLATFORM.COSIM.RVFI.NRETS[0];

    // C910 AXI geometry (see rv_tester_axi.yml).
    localparam int C910_AXI_ADDR = 40;
    localparam int C910_AXI_DATA = 128;
    localparam int C910_AXI_ID   = 8;
    localparam int C910_AXI_STRB = 16;

    // Flattened RVFI export from openC910 (core0, NRET=3)
    logic [NRET-1:0]            core0_rvfi_valid;
    logic [NRET*32-1:0]         core0_rvfi_insn;
    logic [NRET*64-1:0]         core0_rvfi_pc_rdata;
    logic [NRET*64-1:0]         core0_rvfi_pc_wdata;
    logic [NRET*5-1:0]          core0_rvfi_rd_addr;
    logic [NRET-1:0]            core0_rvfi_rd_we;
    logic [NRET-1:0]            core0_rvfi_rd_fpr;
    logic [NRET*64-1:0]         core0_rvfi_rd_wdata;
    logic [NRET*64-1:0]         core0_rvfi_mem_addr;
    logic [NRET*8-1:0]          core0_rvfi_mem_rmask;
    logic [NRET*8-1:0]          core0_rvfi_mem_wmask;
    logic [NRET*64-1:0]         core0_rvfi_mem_rdata;
    logic [NRET*64-1:0]         core0_rvfi_mem_wdata;
    logic [NRET-1:0]            core0_rvfi_trap;
    logic [NRET*64-1:0]         core0_rvfi_cause;
    logic [NRET-1:0]            core0_rvfi_intr;
    logic [NRET*2-1:0]          core0_rvfi_mode;
    logic [NRET*2-1:0]          core0_rvfi_ixl;
    logic [NRET-1:0]            core0_rvfi_last_uop;

    // Monotonic retire tag: counter incremented per valid retirement
    logic [63:0] retire_tag_q;

    // RVFI export -> rv_tester RVFI struct
    always_comb begin
        logic [63:0] tag;
        tag = retire_tag_q;
        for (int i = 0; i < NRET; i++) begin
            rvfi[i].valid    = core0_rvfi_valid[i];
            rvfi[i].comp     = '0;
            rvfi[i].last_uop = core0_rvfi_last_uop[i];
            rvfi[i].order    = tag;
            rvfi[i].insn     = core0_rvfi_insn[i*32 +: 32];
            rvfi[i].uop      = {32'h0, core0_rvfi_insn[i*32 +: 32]};
            rvfi[i].trap     = core0_rvfi_trap[i];
            rvfi[i].cause    = core0_rvfi_cause[i*64 +: 64];
            rvfi[i].halt     = '0;
            rvfi[i].intr     = core0_rvfi_intr[i];
            rvfi[i].mode     = core0_rvfi_mode[i*2 +: 2];
            rvfi[i].ixl      = core0_rvfi_ixl[i*2 +: 2];
            rvfi[i].rd_addr  = (core0_rvfi_rd_we[i] && !core0_rvfi_rd_fpr[i]) ?
                               {1'b0, core0_rvfi_rd_addr[i*5 +: 5]} : '0;
            rvfi[i].rd_wdata = (core0_rvfi_rd_we[i] && !core0_rvfi_rd_fpr[i]) ?
                               core0_rvfi_rd_wdata[i*64 +: 64] : '0;
            rvfi[i].pc_rdata = core0_rvfi_pc_rdata[i*64 +: 64];
            rvfi[i].pc_wdata = core0_rvfi_pc_wdata[i*64 +: 64];
            rvfi[i].mem_addr  = core0_rvfi_mem_addr[i*64 +: 64];
            rvfi[i].mem_paddr = '1;
            rvfi[i].mem_rmask = core0_rvfi_mem_rmask[i*8 +: 8];
            rvfi[i].mem_wmask = core0_rvfi_mem_wmask[i*8 +: 8];
            rvfi[i].mem_rdata = core0_rvfi_mem_rdata[i*64 +: 64];
            rvfi[i].mem_wdata = core0_rvfi_mem_wdata[i*64 +: 64];
            rvfi[i].csr_wmask = '0;
            rvfi[i].csr_rmask = '0;
            rvfi[i].vrd_valid = '0;
            rvfi[i].frd_valid = core0_rvfi_rd_we[i] && core0_rvfi_rd_fpr[i];
            rvfi[i].frd_addr  = core0_rvfi_rd_addr[i*5 +: 5];
            rvfi[i].frd_wdata = core0_rvfi_rd_wdata[i*64 +: 64];
            if (core0_rvfi_valid[i]) tag = tag + 64'd1;
        end
    end

    always_ff @(posedge dut_clk[CORE_CLK_IDX]) begin
        if (reset[COLD_RESET_IDX]) begin
            retire_tag_q <= '0;
        end else begin
            int unsigned n_valid;
            n_valid = 0;
            for (int i = 0; i < NRET; i++) begin
                if (core0_rvfi_valid[i]) n_valid = n_valid + 1;
            end
            retire_tag_q <= retire_tag_q + n_valid;
        end
    end

    // AXI: C910 biu plain-AXI4 master -> rv_tester AXI slot 0
    logic [C910_AXI_ADDR-1:0] biu_pad_araddr;
    logic [1:0]               biu_pad_arburst;
    logic [3:0]               biu_pad_arcache;
    logic [C910_AXI_ID-1:0]   biu_pad_arid;
    logic [7:0]               biu_pad_arlen;
    logic                     biu_pad_arlock;
    logic [2:0]               biu_pad_arprot;
    logic [2:0]               biu_pad_arsize;
    logic                     biu_pad_arvalid;
    logic [C910_AXI_ADDR-1:0] biu_pad_awaddr;
    logic [1:0]               biu_pad_awburst;
    logic [3:0]               biu_pad_awcache;
    logic [C910_AXI_ID-1:0]   biu_pad_awid;
    logic [7:0]               biu_pad_awlen;
    logic                     biu_pad_awlock;
    logic [2:0]               biu_pad_awprot;
    logic [2:0]               biu_pad_awsize;
    logic                     biu_pad_awvalid;
    logic                     biu_pad_bready;
    logic                     biu_pad_cactive;
    logic                     biu_pad_csysack;
    logic                     biu_pad_rready;
    logic [C910_AXI_DATA-1:0] biu_pad_wdata;
    logic                     biu_pad_wlast;
    logic [C910_AXI_STRB-1:0] biu_pad_wstrb;
    logic                     biu_pad_wvalid;

    always_comb begin
        axi_req[0] = '0;

        axi_req[0].ar_valid  = biu_pad_arvalid;
        axi_req[0].ar.id     = biu_pad_arid;
        axi_req[0].ar.addr   = biu_pad_araddr;
        axi_req[0].ar.len    = biu_pad_arlen;
        axi_req[0].ar.size   = biu_pad_arsize;
        axi_req[0].ar.burst  = biu_pad_arburst;
        axi_req[0].ar.lock   = biu_pad_arlock;
        axi_req[0].ar.cache  = biu_pad_arcache;
        axi_req[0].ar.prot   = biu_pad_arprot;
        axi_req[0].ar.region = '0;
        axi_req[0].ar.qos    = '0;
        axi_req[0].ar.user   = '0;

        axi_req[0].aw_valid = biu_pad_awvalid;
        axi_req[0].aw.id    = biu_pad_awid;
        axi_req[0].aw.addr  = biu_pad_awaddr;
        axi_req[0].aw.len   = biu_pad_awlen;
        axi_req[0].aw.size  = biu_pad_awsize;
        axi_req[0].aw.burst = biu_pad_awburst;
        axi_req[0].aw.lock  = biu_pad_awlock;
        axi_req[0].aw.cache = biu_pad_awcache;
        axi_req[0].aw.prot  = biu_pad_awprot;
        axi_req[0].aw.atop  = '0;
        axi_req[0].aw.user  = '0;

        axi_req[0].w_valid  = biu_pad_wvalid;
        axi_req[0].w.data   = biu_pad_wdata;
        axi_req[0].w.strb   = biu_pad_wstrb;
        axi_req[0].w.last   = biu_pad_wlast;

        axi_req[0].b_ready  = biu_pad_bready;
        axi_req[0].r_ready  = biu_pad_rready;
    end

    // Response back to C910
    logic                     pad_biu_arready;
    logic                     pad_biu_awready;
    logic [C910_AXI_ID-1:0]   pad_biu_bid;
    logic [1:0]               pad_biu_bresp;
    logic                     pad_biu_bvalid;
    logic                     pad_biu_csysreq;
    logic [C910_AXI_DATA-1:0] pad_biu_rdata;
    logic [C910_AXI_ID-1:0]   pad_biu_rid;
    logic                     pad_biu_rlast;
    logic [1:0]               pad_biu_rresp;
    logic                     pad_biu_rvalid;
    logic                     pad_biu_wready;

    always_comb begin
        pad_biu_arready = axi_rsp[0].ar_ready;
        pad_biu_awready = axi_rsp[0].aw_ready;
        pad_biu_wready  = axi_rsp[0].w_ready;

        pad_biu_bvalid  = axi_rsp[0].b_valid;
        pad_biu_bid     = axi_rsp[0].b.id;
        pad_biu_bresp   = axi_rsp[0].b.resp;

        pad_biu_rvalid  = axi_rsp[0].r_valid;
        pad_biu_rid     = axi_rsp[0].r.id;
        pad_biu_rdata   = axi_rsp[0].r.data;
        pad_biu_rresp   = axi_rsp[0].r.resp;
        pad_biu_rlast   = axi_rsp[0].r.last;

        pad_biu_csysreq = 1'b1;
    end

    // Quiesce rest of AXI mem group + NCIO group
    always_comb begin
        for (int i = 1; i < topology.TOP.PLATFORM.AXI_SW[AXI_IDX].SHARD; i++) begin
            axi_req[i] = '0;
        end
    end
    for (genvar p = 0;
         p < topology.TOP.PLATFORM.AXI_SW[NCIO_AXI_IDX].SHARD;
         p++) begin : g_ncio_quiet
        assign ncio_axi_req[p].ar_valid = 1'b0;
        assign ncio_axi_req[p].r_ready  = 1'b0;
        assign ncio_axi_req[p].aw_valid = 1'b0;
        assign ncio_axi_req[p].w_valid  = 1'b0;
        assign ncio_axi_req[p].b_ready  = 1'b0;
    end

    assign quiesced = '1;

    // Free-running system counter for C910's internal CLINT time
    logic [63:0] sys_cnt_q;
    always_ff @(posedge dut_clk[CORE_CLK_IDX]) begin
        if (reset[COLD_RESET_IDX]) sys_cnt_q <= '0;
        else                       sys_cnt_q <= sys_cnt_q + 64'd1;
    end

    // C910 DUT (dual-core MP top; core1 held in reset)
    openC910 i_openC910 (
        .pll_cpu_clk                 ( dut_clk[CORE_CLK_IDX] ),
        .axim_clk_en                 ( 1'b1                  ),
        .pad_cpu_rst_b               ( ~reset[COLD_RESET_IDX] ),
        .pad_core0_rst_b             ( ~reset[COLD_RESET_IDX] ),
        .pad_core1_rst_b             ( 1'b0                   ),
        .pad_yy_dft_clk_rst_b        ( 1'b1                   ),
        .pad_yy_scan_rst_b           ( 1'b1                   ),
        .pad_core0_rvba              ( bootstrap.boot_addr[39:0] ),
        .pad_core1_rvba              ( 40'h0                  ),
        .pad_core0_hartid            ( 3'd0                   ),
        .pad_core1_hartid            ( 3'd1                   ),
        .pad_cpu_apb_base            ( 40'h0                  ),
        .pad_cpu_sys_cnt             ( sys_cnt_q              ),
        .pad_core0_dbg_mask          ( 1'b0                   ),
        .pad_core0_dbgrq_b           ( 1'b1                   ),
        .pad_core1_dbg_mask          ( 1'b0                   ),
        .pad_core1_dbgrq_b           ( 1'b1                   ),
        .pad_cpu_l2cache_flush_req   ( 1'b0                   ),
        .pad_had_jtg_tclk            ( 1'b0                   ),
        .pad_had_jtg_tdi             ( 1'b0                   ),
        .pad_had_jtg_tms             ( 1'b0                   ),
        .pad_had_jtg_trst_b          ( 1'b1                   ),
        .pad_l2c_data_mbist_clk_ratio( 3'd0                   ),
        .pad_l2c_tag_mbist_clk_ratio ( 3'd0                   ),
        .pad_yy_icg_scan_en          ( 1'b0                   ),
        .pad_yy_mbist_mode           ( 1'b0                   ),
        .pad_yy_scan_enable          ( 1'b0                   ),
        .pad_yy_scan_mode            ( 1'b0                   ),
        // PLIC external interrupts tied off for smoke bring-up.
        .pad_plic_int_cfg            ( 144'h0                 ),
        .pad_plic_int_vld            ( 144'h0                 ),
        .biu_pad_araddr              ( biu_pad_araddr         ),
        .biu_pad_arburst             ( biu_pad_arburst        ),
        .biu_pad_arcache             ( biu_pad_arcache        ),
        .biu_pad_arid                ( biu_pad_arid           ),
        .biu_pad_arlen               ( biu_pad_arlen          ),
        .biu_pad_arlock              ( biu_pad_arlock         ),
        .biu_pad_arprot              ( biu_pad_arprot         ),
        .biu_pad_arsize              ( biu_pad_arsize         ),
        .biu_pad_arvalid             ( biu_pad_arvalid        ),
        .biu_pad_awaddr              ( biu_pad_awaddr         ),
        .biu_pad_awburst             ( biu_pad_awburst        ),
        .biu_pad_awcache             ( biu_pad_awcache        ),
        .biu_pad_awid                ( biu_pad_awid           ),
        .biu_pad_awlen               ( biu_pad_awlen          ),
        .biu_pad_awlock              ( biu_pad_awlock         ),
        .biu_pad_awprot              ( biu_pad_awprot         ),
        .biu_pad_awsize              ( biu_pad_awsize         ),
        .biu_pad_awvalid             ( biu_pad_awvalid        ),
        .biu_pad_bready              ( biu_pad_bready         ),
        .biu_pad_cactive             ( biu_pad_cactive        ),
        .biu_pad_csysack             ( biu_pad_csysack        ),
        .biu_pad_rready              ( biu_pad_rready         ),
        .biu_pad_wdata               ( biu_pad_wdata          ),
        .biu_pad_wlast               ( biu_pad_wlast          ),
        .biu_pad_wstrb               ( biu_pad_wstrb          ),
        .biu_pad_wvalid              ( biu_pad_wvalid         ),
        .pad_biu_arready             ( pad_biu_arready        ),
        .pad_biu_awready             ( pad_biu_awready        ),
        .pad_biu_bid                 ( pad_biu_bid            ),
        .pad_biu_bresp               ( pad_biu_bresp          ),
        .pad_biu_bvalid              ( pad_biu_bvalid         ),
        .pad_biu_csysreq             ( pad_biu_csysreq        ),
        .pad_biu_rdata               ( pad_biu_rdata          ),
        .pad_biu_rid                 ( pad_biu_rid            ),
        .pad_biu_rlast               ( pad_biu_rlast          ),
        .pad_biu_rresp               ( pad_biu_rresp          ),
        .pad_biu_rvalid              ( pad_biu_rvalid         ),
        .pad_biu_wready              ( pad_biu_wready         ),
        // Observed outputs left unconnected (retire pads superseded by RVFI).
        .core0_pad_jdb_pm            (                        ),
        .core0_pad_lpmd_b            (                        ),
        .core0_pad_mstatus           (                        ),
        .core0_pad_retire0           (                        ),
        .core0_pad_retire0_pc        (                        ),
        .core0_pad_retire1           (                        ),
        .core0_pad_retire1_pc        (                        ),
        .core0_pad_retire2           (                        ),
        .core0_pad_retire2_pc        (                        ),
        .core1_pad_jdb_pm            (                        ),
        .core1_pad_lpmd_b            (                        ),
        .core1_pad_mstatus           (                        ),
        .core1_pad_retire0           (                        ),
        .core1_pad_retire0_pc        (                        ),
        .core1_pad_retire1           (                        ),
        .core1_pad_retire1_pc        (                        ),
        .core1_pad_retire2           (                        ),
        .core1_pad_retire2_pc        (                        ),
        .cpu_debug_port              (                        ),
        .cpu_pad_l2cache_flush_done  (                        ),
        .cpu_pad_no_op               (                        ),
        .had_pad_jtg_tdo             (                        ),
        .had_pad_jtg_tdo_en          (                        )
        `ifdef RVFI
        ,
        .core0_rvfi_valid            ( core0_rvfi_valid       ),
        .core0_rvfi_insn             ( core0_rvfi_insn        ),
        .core0_rvfi_pc_rdata         ( core0_rvfi_pc_rdata    ),
        .core0_rvfi_pc_wdata         ( core0_rvfi_pc_wdata    ),
        .core0_rvfi_rd_addr          ( core0_rvfi_rd_addr     ),
        .core0_rvfi_rd_we            ( core0_rvfi_rd_we       ),
        .core0_rvfi_rd_fpr           ( core0_rvfi_rd_fpr      ),
        .core0_rvfi_rd_wdata         ( core0_rvfi_rd_wdata    ),
        .core0_rvfi_mem_addr         ( core0_rvfi_mem_addr    ),
        .core0_rvfi_mem_rmask        ( core0_rvfi_mem_rmask   ),
        .core0_rvfi_mem_wmask        ( core0_rvfi_mem_wmask   ),
        .core0_rvfi_mem_rdata        ( core0_rvfi_mem_rdata   ),
        .core0_rvfi_mem_wdata        ( core0_rvfi_mem_wdata   ),
        .core0_rvfi_trap             ( core0_rvfi_trap        ),
        .core0_rvfi_cause            ( core0_rvfi_cause       ),
        .core0_rvfi_intr             ( core0_rvfi_intr        ),
        .core0_rvfi_mode             ( core0_rvfi_mode        ),
        .core0_rvfi_ixl              ( core0_rvfi_ixl         ),
        .core0_rvfi_last_uop         ( core0_rvfi_last_uop    )
        `endif
    );

    assign  dut_clk = clk;
    assign  core_no_fetch = reset[COLD_RESET_IDX];

    assign  dut_terminate = '0;
    assign  dut_reset_req = '0;
    assign  dmi_poll_timeout_terminate = '0;
    assign  warm_reset_en     = '0;
    assign  warm_reset        = '0;
    assign  num_resets        = -1;
    assign  target_num_resets =  0;
    assign  unconditional_terminate = '0;
    assign  warm_reset_req = '0;
    logic reset_window_ = 1;
    assign reset_window   = reset_window_;
    assign disable_checks = '0;

    int unsigned order = '0;
    int unsigned reset_deassert_cycle = 100;
    always @(posedge dut_clk[CORE_CLK_IDX]) begin
      order <= order + 1;
        if (order < reset_deassert_cycle) begin
            cold_reset <= '1;
        end
        else if (order == reset_deassert_cycle) begin
            cold_reset <= '0;
        end
        else if (order == reset_deassert_cycle + 100) begin
            reset_window_ <= 0;
        end
        else begin
            cold_reset <= '0;
        end
    end

    `ifdef VCS
        initial begin
            $uniq_prior_checkoff();
        end
    `endif

endmodule
