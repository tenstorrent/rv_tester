module rv_tester #(
    parameter int RESET_CLOCKS              =      10,
    parameter bit EXTERNAL_CLOCK            =       0,
    parameter int CLOCK_PERIOD_PS           =     500,
    parameter int SW_CLOCK_UPDATE_PERIOD_PS = 100_000,
    `TOPOLOGY
) (
    input clk_ext,
    `_RV_TESTER_PORTS(output,input)
);

    typedef longint unsigned LU;

    if (EXTERNAL_CLOCK) begin
        assign clk = clk_ext;
    end else begin
        rv_tester_clkgen clkgen(.*);
    end

    import "DPI-C" context function void rv_tester_parse_flags(); // context forces zebu to serialize this call. this needs to happen at the start of the test before other DPIs.
    import "DPI-C" context function void rv_tester_cvm_error_handler();
    import "DPI-C" function void rv_tester_parse_memmap();
    import "DPI-C" function void rv_tester_build_registry();
    import "DPI-C" function void rv_tester_shutdown_registry();
    import "DPI-C" context function bit rv_tester_flush_callbacks();

    logic rv_tester_reset = '1;
    logic sysmod_reset = '0;
    LU clocks = 0;
    bit cb_poll = '0;
    bit cb_success = '1;
    logic call_finish;
    int rerun_test = -1;

    logic terminate_now;
    logic rerun_now;
    rv_tester_pkg::terminate_t rv_tester_error_terminate;
    rv_tester_pkg::terminate_t sysmod_terminate;

    int quiesce_counter = 0;
    int quiesce_timeout = 500;

    int unsigned location = cvm_topology::nil;

    bit gen_clocks = '0;
    string cvm_verbosity, gen_clocks_verbosity;

    assign terminate           = (rv_tester_error_terminate.terminate || (sysmod_terminate.terminate && !sysmod_reset) || quiesce_counter > 0) && !rv_tester_reset;
    assign terminate_now       = terminate && (quiesced || quiesce_counter >= quiesce_timeout);
    assign rerun_now           = terminated && rerun_test != 0;

    /*
    * Don't put an DPI calls here, zebu gets confused when signals are driven
    * in the same block as zDPI and leads to weird bugs. Eg, triggers on
    * terminated stopped working, and rv_tester_reset stopped being depositable
    * from the tcl shell.
    */
    always @(posedge clk) begin

        rv_tester_reset <= rerun_now;
        sysmod_reset    <= '0;
        clocks          <= clocks + 1;

        quiesce_counter <= quiesce_counter + int'(terminate);
        terminated      <= (terminate_now || terminated) && !rerun_now;

        if (rv_tester_reset) begin
            clocks          <= '0;
            sysmod_reset    <= '1;
            quiesce_counter <= '0;
            terminated      <= '0;
        end

    end

    always @(posedge clk) begin
        if(rerun_now) begin
            $display("<%0d> [RVTESTER]: rerunning test %0d time(s)", clocks, rerun_test);
        end
    end

    /*
    * Group all zebu zemi3 DPIs here
    * These are run on a separate thread than the faster zDPI, so make sure
    * these are only run at rv_tester_reset, when no other zDPIs should be
    * called.
    */
    always @(posedge clk) begin

        if (rv_tester_reset) begin

            $display("[RVTESTER]: new test");
            rv_tester_parse_flags();
            rv_tester_cvm_error_handler();
            rv_tester_parse_memmap();

            /* verilator lint_off BLKSEQ */
            // zebu bug doesn't allow nested function calls, so create intermediate variables
            cvm_verbosity        = cvm_plusargs::get_string("cvm_verbosity");
            gen_clocks_verbosity = cvm_plusargs::get_string("gen_clocks_verbosity");
            location             = cvm_topology::get_location(topology_pkg::mods.TOP.PLATFORM.ID, 0);
            /* verilator lint_on BLKSEQ */

            cb_poll             <= cvm_plusargs::get_bool("cb_async") == '0;
            quiesce_timeout     <= cvm_plusargs::get_int("quiesce_timeout");
            call_finish         <= cvm_plusargs::get_bool("terminate_call_finish") != '0;
            gen_clocks          <= cvm_logger::get_verbosity(cvm_verbosity) >= cvm_logger::get_verbosity(gen_clocks_verbosity);

            $display("[RVTESTER]: reconstructing registry");
            rv_tester_build_registry();
        end

        rerun_test      <= rerun_test - int'(rerun_now);
        if (rerun_test < 0) begin
            rerun_test  <= cvm_plusargs::get_int("rerun_test");
        end

    end

    /*
    * rv_tester_shutdown_registry could be called at the same time as
    * transactions (axi, cosim, etc). So it's put in a separate always block
    * from the rv_tester_reset group of zemi3 DPI. Otherwise zebu will make
    * rv_tester_shutdown_registry a zemi3 DPI and we'll have thread safety
    * issues with coinciding zDPIs from transactions.
    */
    always @(posedge clk) begin

        if (terminate_now && !terminated) begin

            if (quiesced) begin
                $display("<%0d> [RVTESTER]: exiting gracefully", clocks);
            end else if (quiesce_counter == 0) begin
                $display("<%0d> [RVTESTER]: exiting immediately because +quiesce_counter=0", clocks);
            end else begin
                $display("<%0d> Error: Waiting to quiesce for more than %0d cycles", clocks, quiesce_timeout);
            end

            rv_tester_shutdown_registry();

            if (call_finish && rerun_test == '0) begin
                $finish();
            end

        end
    end

    assign reset = clocks < LU'(RESET_CLOCKS) || rv_tester_reset || sysmod_reset;

`ifdef NEGEDGE_UNSUPPORTED
    always@(posedge clk) begin
`else
    always@(negedge clk) begin
`endif
        if (cb_poll) begin
            /* verilator lint_off BLKSEQ */
            cb_success = rv_tester_flush_callbacks();
            /* verilator lint_on BLKSEQ */
        end
    end

    always @(posedge clk) begin
        /* verilator lint_off BLKANDNBLK */
        rv_tester_error_terminate.terminate <= '0;
        /* verilator lint_on BLKANDNBLK */
    end


    function void rv_tester_terminate ();
      rv_tester_error_terminate.terminate = '1;
    endfunction
    export "DPI-C" function rv_tester_terminate;

    `RV_TESTER_TRANSACTIONS_DOMAIN(1, clk);

    sysmod #(
        .CLOCK_PERIOD_PS(CLOCK_PERIOD_PS),
        .SW_CLOCK_UPDATE_PERIOD_PS(SW_CLOCK_UPDATE_PERIOD_PS),
        .NUM(0),
        `TOPOLOGY_CFG
    ) sysmod (
        .clk,
        .reset(sysmod_reset),
        .clocks,
        .bootstrap,
        .dmi_write,
        .interrupt,
        .terminate(sysmod_terminate),
        `RV_TESTER_TRANSACTIONS_SOURCE_SYSMOD(1, 0)
    );

    // coverage
    arch_sample arch_sample ();

`ifndef NO_COSIM
    cosim #(
        .NUM(0),
        `TOPOLOGY_CFG
    ) cosim (
        .clk,
        .reset(sysmod_reset),
        .clocks,
        .rvfi(rvfi_instr),
        .mcmi_store(mcmi_store),
        .interrupt,
        .debug_mode,
        `RV_TESTER_TRANSACTIONS_SOURCE_COSIM(1, 0)
    );
`endif

    assign tx_dom_1.logger_cycles[0][0].valid = gen_clocks;
    assign tx_dom_1.logger_cycles[0][0].data.location = location;
    assign tx_dom_1.logger_cycles[0][0].data.clock = clocks;

    for (genvar p = 0; p < topology.TOP.PLATFORM.AXI.TOTAL; p++) begin : axi_sw_slvs
        axi_sw #(
            .ADDR_WIDTH(topology.TOP.PLATFORM.AXI.ADDR_WIDTH),
            .DATA_WIDTH(topology.TOP.PLATFORM.AXI.DATA_WIDTH),
            .ID_WIDTH(topology.TOP.PLATFORM.AXI.ID_WIDTH  ),
            .STRB_WIDTH(topology.TOP.PLATFORM.AXI.STRB_WIDTH),
            .R_Q_MAX(topology.TOP.PLATFORM.AXI.R_Q_MAX),
            .TOPO_ID(topology.TOP.PLATFORM.AXI.ID),
            .NUM(p)
        ) axi_sw(
            .clk,
            .reset_n(~reset),
            .sys_reset(sysmod_reset),
            .axi_mst_ar_valid(axi_req[p].ar_valid),
            .axi_mst_ar_id   (axi_req[p].ar_id),
            .axi_mst_ar_addr (axi_req[p].ar_addr),
            .axi_mst_ar_len  (axi_req[p].ar_len),
            .axi_mst_ar_size (axi_req[p].ar_size),
            .axi_mst_ar_lock (axi_req[p].ar_lock),
            .axi_mst_ar_burst(axi_req[p].ar_burst),

            .axi_mst_aw_valid(axi_req[p].aw_valid),
            .axi_mst_aw_id   (axi_req[p].aw_id),
            .axi_mst_aw_addr (axi_req[p].aw_addr),
            .axi_mst_aw_len  (axi_req[p].aw_len),
            .axi_mst_aw_size (axi_req[p].aw_size),
            .axi_mst_aw_burst(axi_req[p].aw_burst),
            .axi_mst_aw_lock (axi_req[p].aw_lock),
            .axi_mst_aw_atop (axi_req[p].aw_atop),

            .axi_mst_w_valid(axi_req[p].w_valid),
            .axi_mst_w_data (axi_req[p].w_data),
            .axi_mst_w_strb (axi_req[p].w_strb),
            .axi_mst_w_last (axi_req[p].w_last),

            .axi_mst_b_ready(axi_req[p].b_ready),
            .axi_mst_r_ready(axi_req[p].r_ready),

            .axi_slv_b_valid(axi_rsp[p].b_valid),
            .axi_slv_b_id   (axi_rsp[p].b_id),
            .axi_slv_b_resp (axi_rsp[p].b_resp),

            .axi_slv_r_valid(axi_rsp[p].r_valid),
            .axi_slv_r_id   (axi_rsp[p].r_id),
            .axi_slv_r_data (axi_rsp[p].r_data),
            .axi_slv_r_resp (axi_rsp[p].r_resp),
            .axi_slv_r_last (axi_rsp[p].r_last),

            .axi_slv_aw_ready(axi_rsp[p].aw_ready),
            .axi_slv_ar_ready(axi_rsp[p].ar_ready),
            .axi_slv_w_ready (axi_rsp[p].w_ready),
            `RV_TESTER_TRANSACTIONS_SOURCE_AXI_SW(1, p)
        );
    end

    for (genvar p = 0; p < topology.TOP.PLATFORM.AXI_MST.TOTAL; p++) begin : axi_sw_msts
        axi_sw_mst #(
            .ADDR_WIDTH(topology.TOP.PLATFORM.AXI_MST.ADDR_WIDTH),
            .DATA_WIDTH(topology.TOP.PLATFORM.AXI_MST.DATA_WIDTH),
            .ID_WIDTH(topology.TOP.PLATFORM.AXI_MST.ID_WIDTH  ),
            .STRB_WIDTH(topology.TOP.PLATFORM.AXI_MST.STRB_WIDTH),
            .AR_Q_MAX(topology.TOP.PLATFORM.AXI_MST.AR_Q_MAX),
            .AW_Q_MAX(topology.TOP.PLATFORM.AXI_MST.AW_Q_MAX),
            .W_Q_MAX(topology.TOP.PLATFORM.AXI_MST.W_Q_MAX),
            .TOPO_ID(topology.TOP.PLATFORM.AXI_MST.ID),
            .NUM(p)
        ) axi_sw_mst (
            .clk,
            .reset_n(~reset),
            .sys_reset(sysmod_reset),
            .axi_mst_ar_valid(axi_req_mst[p].ar_valid),
            .axi_mst_ar_id   (axi_req_mst[p].ar_id),
            .axi_mst_ar_addr (axi_req_mst[p].ar_addr),
            .axi_mst_ar_len  (axi_req_mst[p].ar_len),
            .axi_mst_ar_size (axi_req_mst[p].ar_size),
            .axi_mst_ar_lock (axi_req_mst[p].ar_lock),
            .axi_mst_ar_burst(axi_req_mst[p].ar_burst),

            .axi_mst_aw_valid(axi_req_mst[p].aw_valid),
            .axi_mst_aw_id   (axi_req_mst[p].aw_id),
            .axi_mst_aw_addr (axi_req_mst[p].aw_addr),
            .axi_mst_aw_len  (axi_req_mst[p].aw_len),
            .axi_mst_aw_size (axi_req_mst[p].aw_size),
            .axi_mst_aw_burst(axi_req_mst[p].aw_burst),
            .axi_mst_aw_lock (axi_req_mst[p].aw_lock),
            .axi_mst_aw_atop (axi_req_mst[p].aw_atop),

            .axi_mst_w_valid(axi_req_mst[p].w_valid),
            .axi_mst_w_data (axi_req_mst[p].w_data),
            .axi_mst_w_strb (axi_req_mst[p].w_strb),
            .axi_mst_w_last (axi_req_mst[p].w_last),

            .axi_mst_b_ready(axi_req_mst[p].b_ready),
            .axi_mst_r_ready(axi_req_mst[p].r_ready),

            .axi_slv_b_valid(axi_rsp_mst[p].b_valid),
            .axi_slv_b_id   (axi_rsp_mst[p].b_id),
            .axi_slv_b_resp (axi_rsp_mst[p].b_resp),

            .axi_slv_r_valid(axi_rsp_mst[p].r_valid),
            .axi_slv_r_id   (axi_rsp_mst[p].r_id),
            .axi_slv_r_data (axi_rsp_mst[p].r_data),
            .axi_slv_r_resp (axi_rsp_mst[p].r_resp),
            .axi_slv_r_last (axi_rsp_mst[p].r_last),

            .axi_slv_aw_ready(axi_rsp_mst[p].aw_ready),
            .axi_slv_ar_ready(axi_rsp_mst[p].ar_ready),
            .axi_slv_w_ready (axi_rsp_mst[p].w_ready),
            `RV_TESTER_TRANSACTIONS_SOURCE_AXI_SW_MST(1, p)
        );
    end

endmodule
