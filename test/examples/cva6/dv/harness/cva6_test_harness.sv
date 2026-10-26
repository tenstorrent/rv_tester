// SPDX-FileCopyrightText: © 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

`include "rvfi_types.svh"

module cva6_test_harness
    import rv_tester_params::*;
#(
    parameter config_pkg::cva6_cfg_t CVA6Cfg =
        build_config_pkg::build_config(cva6_config_pkg::cva6_cfg),
    `TOPOLOGY
) (
    `RV_TESTER_PORTS
);

    localparam NRET = topology.TOP.PLATFORM.COSIM.RVFI.NRETS[0];

    if (NRET != CVA6Cfg.NrCommitPorts) $error("NRET misconfiguration, needs to be %0d", CVA6Cfg.NrCommitPorts);
    if (topology.TOP.PLATFORM.AXI_SW[AXI_IDX].ADDR_WIDTH != ariane_axi::AddrWidth) $error("AXI_ADDR_WIDTH misconfiguration, needs to be %0d", ariane_axi::AddrWidth);
    if (topology.TOP.PLATFORM.AXI_SW[AXI_IDX].DATA_WIDTH != ariane_axi::DataWidth) $error("AXI_DATA_WIDTH misconfiguration, needs to be %0d", ariane_axi::DataWidth);
    if (topology.TOP.PLATFORM.AXI_SW[AXI_IDX].ID_WIDTH   != ariane_axi::IdWidth  ) $error("AXI_ID_WIDTH   misconfiguration, needs to be %0d", ariane_axi::IdWidth  );

    // RVFI probe -> retired-instruction types
    localparam type rvfi_probes_instr_t = `RVFI_PROBES_INSTR_T(CVA6Cfg);
    localparam type rvfi_probes_csr_t   = `RVFI_PROBES_CSR_T(CVA6Cfg);
    localparam type rvfi_probes_t = struct packed {
        rvfi_probes_csr_t   csr;
        rvfi_probes_instr_t instr;
    };
    localparam type rvfi_instr_t    = `RVFI_INSTR_T(CVA6Cfg);
    localparam type rvfi_csr_elmt_t = `RVFI_CSR_ELMT_T(CVA6Cfg);
    localparam type rvfi_csr_t      = `RVFI_CSR_T(CVA6Cfg, rvfi_csr_elmt_t);
    localparam type rvfi_to_iti_t   = `RVFI_TO_ITI_T(CVA6Cfg);

    rvfi_probes_t                             cva6_rvfi_probes;
    rvfi_instr_t [CVA6Cfg.NrCommitPorts-1:0]  rvfi_instr;

    // Monotonic retire tag: counter incremented per valid retirement
    logic [63:0] retire_tag_q;

    logic warm_reset_en_   =  0;
    int num_resets_        = -1;
    int target_num_resets_ =  0;
    logic [rv_tester_params::NHOLDS-1:0]  reset_hold;

    // Retired-instruction stream -> rv_tester RVFI struct.
    always_comb begin
        logic [63:0] tag;
        tag = retire_tag_q;
        for (int i = 0; i < NRET; i++) begin
            rvfi[i].valid = rvfi_instr[i].valid;
            rvfi[i].comp = '0;
            rvfi[i].last_uop = '1;
            // cva6_rvfi never drives .order; MCM needs a unique seq, so TB supplies it.
            rvfi[i].order = tag;
            rvfi[i].insn = rvfi_instr[i].insn;
            rvfi[i].uop = {32'h0, rvfi_instr[i].insn};
            rvfi[i].trap = rvfi_instr[i].trap;
            rvfi[i].cause = rvfi_instr[i].cause;
            rvfi[i].halt = rvfi_instr[i].halt;
            rvfi[i].intr = rvfi_instr[i].intr;
            rvfi[i].mode = {2'h0, rvfi_instr[i].mode};
            rvfi[i].ixl = rvfi_instr[i].ixl;
            rvfi[i].rd_addr = {1'b0, rvfi_instr[i].rd_addr};
            rvfi[i].rd_wdata = rvfi_instr[i].rd_wdata;
            rvfi[i].pc_rdata = rvfi_instr[i].pc_rdata;
            rvfi[i].pc_wdata = rvfi_instr[i].pc_wdata;
            rvfi[i].mem_addr = rvfi_instr[i].mem_addr;
            rvfi[i].mem_paddr = '1;
            rvfi[i].mem_rmask = rvfi_instr[i].mem_rmask;
            rvfi[i].mem_wmask = rvfi_instr[i].mem_wmask;
            rvfi[i].mem_rdata = rvfi_instr[i].mem_rdata;
            rvfi[i].mem_wdata = rvfi_instr[i].mem_wdata;
            rvfi[i].csr_wmask = '0;
            rvfi[i].csr_rmask = '0;
            rvfi[i].vrd_valid = '0;
            rvfi[i].frd_valid = '0;
            if (rvfi_instr[i].valid) tag = tag + 64'd1;
        end
    end

    // Retire tag: bumped per valid retirement, held in reset
    always_ff @(posedge dut_clk[CORE_CLK_IDX]) begin
        if (reset[COLD_RESET_IDX]) begin
            retire_tag_q <= '0;
        end else begin
            int unsigned n_valid;
            n_valid = 0;
            for (int i = 0; i < NRET; i++) begin
                if (rvfi_instr[i].valid) n_valid = n_valid + 1;
            end
            retire_tag_q <= retire_tag_q + n_valid;
        end
    end

    // AXI: CVA6 noc master -> rv_tester AXI slot 0
    ariane_axi::req_t  cva6_req;
    ariane_axi::resp_t cva6_rsp;

    always_comb begin
        axi_req[0].ar_valid  = cva6_req.ar_valid ;
        axi_req[0].ar.id     = cva6_req.ar.id    ;
        axi_req[0].ar.addr   = cva6_req.ar.addr  ;
        axi_req[0].ar.len    = cva6_req.ar.len   ;
        axi_req[0].ar.size   = cva6_req.ar.size  ;
        axi_req[0].ar.burst  = cva6_req.ar.burst ;
        axi_req[0].ar.lock   = cva6_req.ar.lock  ;
        axi_req[0].ar.cache  = cva6_req.ar.cache ;
        axi_req[0].ar.prot   = cva6_req.ar.prot  ;
        axi_req[0].ar.region = cva6_req.ar.region;
        axi_req[0].ar.qos    = cva6_req.ar.qos   ;
        axi_req[0].ar.user   = 1'(cva6_req.ar.user); // FIXME parameterize user width in rv_tester

        axi_req[0].aw_valid = cva6_req.aw_valid;
        axi_req[0].aw.id    = cva6_req.aw.id   ;
        axi_req[0].aw.addr  = cva6_req.aw.addr ;
        axi_req[0].aw.len   = cva6_req.aw.len  ;
        axi_req[0].aw.size  = cva6_req.aw.size ;
        axi_req[0].aw.burst = cva6_req.aw.burst;
        axi_req[0].aw.lock  = cva6_req.aw.lock ;
        axi_req[0].aw.atop  = cva6_req.aw.atop ;
        axi_req[0].aw.user   = 1'(cva6_req.aw.user); // FIXME parameterize user width in rv_tester

        axi_req[0].w_valid  = cva6_req.w_valid;
        axi_req[0].w.data   = cva6_req.w.data ;
        axi_req[0].w.strb   = cva6_req.w.strb ;
        axi_req[0].w.last   = cva6_req.w.last ;

        axi_req[0].b_ready  = cva6_req.b_ready;
        axi_req[0].r_ready  = cva6_req.r_ready;
    end

    for(genvar p = 0;
        p < topology.TOP.PLATFORM.AXI_SW[NCIO_AXI_IDX].SHARD;
        p++) begin : g_ncio_quiet
          assign ncio_axi_req[p].ar_valid           = 1'b0;
          assign ncio_axi_req[p].r_ready            = 1'b0;
          assign ncio_axi_req[p].aw_valid           = 1'b0;
          assign ncio_axi_req[p].w_valid            = 1'b0;
          assign ncio_axi_req[p].b_ready            = 1'b0;
    end

    always_comb begin
        // Slot 0 of the AXI mem group is driven by cva6 above; quiesce
        // the rest of the group (if any).
        for (int i = 1; i < topology.TOP.PLATFORM.AXI_SW[AXI_IDX].SHARD; i++) begin
            axi_req[i] = '0;
        end
    end

    always_comb begin
        cva6_rsp.b_valid  = axi_rsp[0].b_valid ;
        cva6_rsp.b.id     = axi_rsp[0].b.id    ;
        cva6_rsp.b.resp   = axi_rsp[0].b.resp  ;

        cva6_rsp.r_valid  = axi_rsp[0].r_valid ;
        cva6_rsp.r.id     = axi_rsp[0].r.id    ;
        cva6_rsp.r.data   = axi_rsp[0].r.data  ;
        cva6_rsp.r.resp   = axi_rsp[0].r.resp  ;
        cva6_rsp.r.last   = axi_rsp[0].r.last  ;

        cva6_rsp.aw_ready = axi_rsp[0].aw_ready;
        cva6_rsp.ar_ready = axi_rsp[0].ar_ready;
        cva6_rsp.w_ready  = axi_rsp[0].w_ready ;
    end

    assign quiesced = '1;

    // ------------------------------------------------------------------
    // CVA6 core (openhwgroup `ariane` wrapper: cva6 + cvxif tie-off)
    // ------------------------------------------------------------------
    ariane #(
      .CVA6Cfg             ( CVA6Cfg             ),
      .rvfi_probes_instr_t ( rvfi_probes_instr_t ),
      .rvfi_probes_csr_t   ( rvfi_probes_csr_t   ),
      .rvfi_probes_t       ( rvfi_probes_t       ),
      .noc_req_t           ( ariane_axi::req_t   ),
      .noc_resp_t          ( ariane_axi::resp_t  )
    ) i_ariane (
        .clk_i        ( dut_clk[CORE_CLK_IDX]      ),
        .rst_ni       ( ~reset[COLD_RESET_IDX]     ),
        .boot_addr_i  ( bootstrap.boot_addr        ),
        .hart_id_i    ( '0                         ),
        // {supervisor external, machine external}
        .irq_i        ( {interrupt[0].sei, interrupt[0].mei} ),
        .ipi_i        ( interrupt[0].ssi || interrupt[0].msi ),
        .time_irq_i   ( interrupt[0].sti || interrupt[0].mti ),
        .debug_req_i  ( '0                         ),
        .rvfi_probes_o( cva6_rvfi_probes           ),
        .noc_req_o    ( cva6_req                   ),
        .noc_resp_i   ( cva6_rsp                   )
    );

    // Expand RVFI probes into the retired-instruction stream consumed above.
    cva6_rvfi #(
      .CVA6Cfg             ( CVA6Cfg             ),
      .rvfi_instr_t        ( rvfi_instr_t        ),
      .rvfi_csr_t          ( rvfi_csr_t          ),
      .rvfi_probes_instr_t ( rvfi_probes_instr_t ),
      .rvfi_probes_csr_t   ( rvfi_probes_csr_t   ),
      .rvfi_probes_t       ( rvfi_probes_t       ),
      .rvfi_to_iti_t       ( rvfi_to_iti_t       )
    ) i_cva6_rvfi (
        .clk_i         ( dut_clk[CORE_CLK_IDX]  ),
        .rst_ni        ( ~reset[COLD_RESET_IDX] ),
        .rvfi_probes_i ( cva6_rvfi_probes       ),
        .rvfi_instr_o  ( rvfi_instr             ),
        .rvfi_to_iti_o ( /* unused */           ),
        .rvfi_csr_o    ( /* unused */           )
    );

    assign  dut_clk = clk;
    assign  core_no_fetch = reset[COLD_RESET_IDX];

    assign  dut_terminate = '0;
    assign  dut_reset_req = '0;

    assign dmi_poll_timeout_terminate = '0;
    assign warm_reset_en     = '0;
    assign warm_reset        = '0;
    assign num_resets        = -1;
    assign target_num_resets =  0;
    assign unconditional_terminate = '0;
    assign warm_reset_req = '0;
    logic reset_window_   = 1;
    assign reset_window   = reset_window_;
    assign disable_checks = '0;

    int unsigned order = '0;
    int unsigned reset_deassert_cycle = 100;  // When to deassert cold_reset and reset_window
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
            $uniq_prior_checkoff();  // FIXME: unclear why this is needed; should be off by default
        end
    `endif

endmodule
