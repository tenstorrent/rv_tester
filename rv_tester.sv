module rv_tester #(
    parameter int RESET_CLOCKS              =      10,
    parameter bit EXTERNAL_CLOCK            =       0,
    parameter int CLOCK_PERIOD_PS           =     500,
    parameter int SW_CLOCK_UPDATE_PERIOD_PS = 100_000,
    `TOPOLOGY,
    `RV_TESTER_PARAMETERS(topology)
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
    import "DPI-C" function void rv_tester_parse_memmap();
    import "DPI-C" function void rv_tester_reset_registry();
    import "DPI-C" context function void rv_tester_flush_callbacks();

    logic rv_tester_reset = '1;
    logic sysmod_reset = '0;
    LU clocks = 0;
    bit cb_poll = '0;

    logic wait_for_quiesced = '0;
    logic ready_to_terminate;
    logic call_finish;
    rv_tester_pkg::terminate_t sysmod_terminate;

    logic [31:0] quiesce_counter = 0;
    logic [31:0] quiesce_timeout = 500;

`ifndef NO_COSIM
    rv_tester_pkg::terminate_t cosim_terminate;
    assign ready_to_terminate = sysmod_terminate.terminate || cosim_terminate.terminate;
    assign call_finish = sysmod_terminate.call_finish || cosim_terminate.call_finish;
`else
    assign ready_to_terminate = sysmod_terminate.terminate;
    assign call_finish = sysmod_terminate.call_finish;
`endif

    assign terminate = wait_for_quiesced;

    always @(posedge clk) begin
        rv_tester_reset <= '0;
        sysmod_reset <= 0;
        clocks <= clocks + 1;
        if (rv_tester_reset) begin
            $display("[RVTESTER]: new test");
            rv_tester_parse_flags();
            rv_tester_parse_memmap();
            $display("[RVTESTER]: reconstructing registry");
            rv_tester_reset_registry();
            clocks <= 0;
            sysmod_reset <= '1;

            /* verilator lint_off BLKSEQ */
            cb_poll <= cvm_plusargs::get_bool("cb_async") != '1;
            /* verilator lint_on BLKSEQ */
        end
        if (cb_poll) begin
            rv_tester_flush_callbacks();
        end
        if (ready_to_terminate && call_finish) begin
            wait_for_quiesced <= '1;
        end
        if (wait_for_quiesced) begin
            if (quiesced) begin
              // exit gracefully
              $display("[RVTESTER]: exiting gracefully");
              $finish();
            end else begin
              quiesce_counter <= quiesce_counter + 1;
            end
            if (quiesce_counter >= quiesce_timeout) begin
              $display("Error: Waiting to quiesce for more than %0d cycles", quiesce_timeout);
              $finish();
            end
        end
    end
    assign reset = clocks < LU'(RESET_CLOCKS) || rv_tester_reset || sysmod_reset;

    sysmod#(
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
        .terminate(sysmod_terminate)
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
        .terminate(cosim_terminate)
    );
`endif

    for (genvar p = 0; p < topology.CORE.AXI_PORTS; p++) begin
        axi_sw #(
            .ADDR_WIDTH(topology.CORE.AXI_ADDR_WIDTH),
            .DATA_WIDTH(topology.CORE.AXI_DATA_WIDTH),
            .ID_WIDTH  (topology.CORE.AXI_ID_WIDTH  ),
            .SYSMOD_NUM(0)
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
            .axi_slv_w_ready (axi_rsp[p].w_ready)
        );
    end

endmodule
