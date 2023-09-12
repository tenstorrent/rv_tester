module rv_tester
import rv_tester_params::*;
#(
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

    import "DPI-C" function int rv_tester_parse_flags(); // dummy return value so that this gets called immediately. need this to happen before any other DPIs are called.
    import "DPI-C" context function void rv_tester_cvm_error_handler();
    import "DPI-C" function void rv_tester_parse_memmap();
    import "DPI-C" function void rv_tester_build_registry();
    import "DPI-C" function int rv_tester_shutdown_registry(); // dummy return value so that this gets called immediately to end the test.
    import "DPI-C" context function bit rv_tester_flush_callbacks();

    localparam int unsigned AxiIdWidthMst_rv    = topology.TOP.PLATFORM.AXI.ID_WIDTH + $clog2(topology.TOP.PLATFORM.AXI.TOTAL) + 1;


    logic rv_tester_reset = '1;
    logic sysmod_reset = '0;
    LU clocks = 0;
    bit cb_poll = '0;
    bit cb_success = '1;
    logic call_finish;
    int num_reruns = -1;

    logic terminate_now;
    logic rerun_now;
    rv_tester_pkg::terminate_t rv_tester_error_terminate;
    rv_tester_pkg::terminate_t sysmod_terminate;

    int quiesce_counter = 0;
    int quiesce_timeout = 500;

    int unsigned location = cvm_topology::nil;

    bit gen_clocks = '0;
    string cvm_verbosity_string, gen_clocks_verbosity_string;
    int unsigned cvm_verbosity, gen_clocks_verbosity;

    assign terminate           = (rv_tester_error_terminate.terminate || (sysmod_terminate.terminate && !sysmod_reset) || quiesce_counter > 0) && !rv_tester_reset;
    assign terminate_now       = terminate && (quiesced || quiesce_counter >= quiesce_timeout);
    assign rerun_now           = terminated && num_reruns != 0;

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
            $display("<%0d> [RVTESTER]: rerunning test %0d time(s)", clocks, num_reruns);
        end
    end

    /*
    * Group all zebu zemi3 DPIs here
    * These are run on a separate thread than the faster zDPI, so make sure
    * these are only run at rv_tester_reset, when no other zDPIs should be
    * called.
    */
    always @(posedge clk) begin

        automatic int _;

        if (rv_tester_reset) begin

            $display("[RVTESTER]: new test");
            _ = rv_tester_parse_flags();
            rv_tester_cvm_error_handler();
            rv_tester_parse_memmap();

            /* verilator lint_off BLKSEQ */
            // zebu bug doesn't allow nested function calls, so create intermediate variables
            cvm_verbosity_string        = cvm_plusargs::get_string("cvm_verbosity");
            gen_clocks_verbosity_string = cvm_plusargs::get_string("gen_clocks_verbosity");
            cvm_verbosity               = cvm_logger::get_verbosity(cvm_verbosity_string);
            gen_clocks_verbosity        = cvm_logger::get_verbosity(gen_clocks_verbosity_string);
            location                    = cvm_topology::get_location(topology_pkg::mods.TOP.PLATFORM.ID, 0);
            rv_tester_error_terminate.terminate = '0;
            /* verilator lint_on BLKSEQ */

            cb_poll             <= cvm_plusargs::get_bool("cb_async") == '0;
            quiesce_timeout     <= cvm_plusargs::get_int("quiesce_timeout");
            call_finish         <= cvm_plusargs::get_bool("terminate_call_finish") != '0;
            gen_clocks          <= cvm_verbosity >= gen_clocks_verbosity;


            $display("[RVTESTER]: reconstructing registry");
            rv_tester_build_registry();
        end

        num_reruns      <= num_reruns - int'(rerun_now);
        if (num_reruns < 0) begin
            num_reruns  <= cvm_plusargs::get_int("num_reruns");
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

        automatic int _;

        if (terminate_now && !terminated) begin

            if (quiesced) begin
                $display("<%0d> [RVTESTER]: exiting gracefully", clocks);
            end else if (quiesce_counter == 0) begin
                $display("<%0d> [RVTESTER]: exiting immediately because +quiesce_counter=0", clocks);
            end else begin
                $display("<%0d> Error: Waiting to quiesce for more than %0d cycles", clocks, quiesce_timeout);
            end

            _ = rv_tester_shutdown_registry();

            if (call_finish && num_reruns == '0) begin
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

    function void rv_tester_terminate ();
        $display("rv_tester_terminate: attempting to terminate");
        rv_tester_error_terminate.terminate = '1;
    endfunction
    export "DPI-C" function rv_tester_terminate;

    `RV_TESTER_TRANSACTIONS_DOMAIN(1, clk);
    `RV_TESTER_TRANSACTIONS_DOMAIN(2, clk);

    rv_tester_pkg::dm_write_t  trickbox_dmi_write;

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
        .dmi_write(trickbox_dmi_write),
        .interrupt,
        .terminate(sysmod_terminate),
        `RV_TESTER_TRANSACTIONS_SOURCE_SYSMOD(2, 0)
    );

`ifndef DMI_TB_WRITES_UNSUPPORTED
    logic [7:0] misc_signals;

    dmi_driver i_dmi_driver(
        .clk,
        .reset,
        .dmi_req_ready,
        .dmi_resp_valid,
        .dmi_resp,

        .dmi_req_valid,
        .dmi_req,
        .dmi_resp_ready,
        .misc_signals,

        .trickbox_dmi_write(trickbox_dmi_write)
    );

    dm_model #(
        .NUM(0),
        `TOPOLOGY_CFG
    ) i_dm_model(
        .clk,
        .reset(sysmod_reset),
        .dmi_req(dmi_req),
        .dmi_req_valid(dmi_req_valid),
        .dmi_resp_valid(dmi_resp_valid),
        .dmi_resp(dmi_resp),
        .terminate,
        .axi_req_mst(axi_req_mst[0]),
        .axi_resp_mst(axi_rsp_mst[0]),
        .misc_signals,
        `RV_TESTER_TRANSACTIONS_SOURCE_DM_MODEL(1,0)
    );
`endif

    // coverage
    arch_sample arch_sample ();

`ifndef NO_COSIM
    for (genvar c = 0; c < NHARTS; c++) begin: cosim_inst
      cosim #(
          .NUM(c),
          .NRET(NRETS[c]),
          .NREAD(NREADS[c]),
          .NINSERT(NINSERTS[c]),
          .NWRITE(NWRITES[c]),
          `TOPOLOGY_CFG
      ) cosim (
          .clk,
          .reset(sysmod_reset),
          .clocks,
          .rvfi(rvfi[NRETS_CUMSUM[c] +: NRETS[c]]),
          .mcmi_read(mcmi_read[NREADS_CUMSUM[c] +: NREADS[c]]),
          .mcmi_insert(mcmi_insert[NINSERTS_CUMSUM[c] +: NINSERTS[c]]),
          .mcmi_write(mcmi_write[NWRITES_CUMSUM[c] +: NWRITES[c]]),
          .interrupt(interrupt[c]),
          .debug_mode(debug_mode[c]),
          `RV_TESTER_TRANSACTIONS_SOURCE_COSIM(1, c)
      );
    end
`endif

    for (genvar p = 0; p < NHARTS; p++) begin: pmu_inst
      pmu #(
          .NUM(p),
          .NRET(NRETS[p]),
          `TOPOLOGY_CFG
      ) pmu (
          .clk,
          .reset(sysmod_reset),
          .clocks,
          .pmci(pmci[p]),
          .rvfi(rvfi[NRETS_CUMSUM[p] +: NRETS[p]]),
          .terminate,
          `RV_TESTER_TRANSACTIONS_SOURCE_PMU(1, p)
      );
    end

    assign tx_dom_1.logger_cycles[0][0].valid = gen_clocks;
    assign tx_dom_1.logger_cycles[0][0].data.location = location;
    assign tx_dom_1.logger_cycles[0][0].data.clock = clocks;

    
        axi_sw #(
            .ADDR_WIDTH(topology.TOP.PLATFORM.AXI.ADDR_WIDTH),
            .DATA_WIDTH(topology.TOP.PLATFORM.AXI.DATA_WIDTH),
            .ID_WIDTH(AxiIdWidthMst_rv),
            .STRB_WIDTH(topology.TOP.PLATFORM.AXI.STRB_WIDTH),
            .R_Q_MAX(topology.TOP.PLATFORM.AXI.R_Q_MAX),
            .TOPO_ID(topology.TOP.PLATFORM.AXI.ID),
            .NUM(0)
        ) axi_sw(
            .clk,
            .reset_n(~reset),
            .sys_reset(sysmod_reset),
            .axi_mst_ar_valid(axi_req_llc.ar_valid),
            .axi_mst_ar_id   (axi_req_llc.ar.id),
            .axi_mst_ar_addr (axi_req_llc.ar.addr),
            .axi_mst_ar_len  (axi_req_llc.ar.len),
            .axi_mst_ar_size (axi_req_llc.ar.size),
            .axi_mst_ar_lock (axi_req_llc.ar.lock),
            .axi_mst_ar_burst(axi_req_llc.ar.burst),

            .axi_mst_aw_valid(axi_req_llc.aw_valid),
            .axi_mst_aw_id   (axi_req_llc.aw.id),
            .axi_mst_aw_addr (axi_req_llc.aw.addr),
            .axi_mst_aw_len  (axi_req_llc.aw.len),
            .axi_mst_aw_size (axi_req_llc.aw.size),
            .axi_mst_aw_burst(axi_req_llc.aw.burst),
            .axi_mst_aw_lock (axi_req_llc.aw.lock),
            .axi_mst_aw_atop (axi_req_llc.aw.atop),

            .axi_mst_w_valid(axi_req_llc.w_valid),
            .axi_mst_w_data (axi_req_llc.w.data),
            .axi_mst_w_strb (axi_req_llc.w.strb),
            .axi_mst_w_last (axi_req_llc.w.last),

            .axi_mst_b_ready(axi_req_llc.b_ready),
            .axi_mst_r_ready(axi_req_llc.r_ready),

            .axi_slv_b_valid(axi_rsp_llc.b_valid),
            .axi_slv_b_id   (axi_rsp_llc.b.id),
            .axi_slv_b_resp (axi_rsp_llc.b.resp),

            .axi_slv_r_valid(axi_rsp_llc.r_valid),
            .axi_slv_r_id   (axi_rsp_llc.r.id),
            .axi_slv_r_data (axi_rsp_llc.r.data),
            .axi_slv_r_resp (axi_rsp_llc.r.resp),
            .axi_slv_r_last (axi_rsp_llc.r.last),

            .axi_slv_aw_ready(axi_rsp_llc.aw_ready),
            .axi_slv_ar_ready(axi_rsp_llc.ar_ready),
            .axi_slv_w_ready (axi_rsp_llc.w_ready),
            `RV_TESTER_TRANSACTIONS_SOURCE_AXI_SW(2, 0)
        );

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
            .axi_mst_ar_id   (axi_req_mst[p].ar.id),
            .axi_mst_ar_addr (axi_req_mst[p].ar.addr),
            .axi_mst_ar_len  (axi_req_mst[p].ar.len),
            .axi_mst_ar_size (axi_req_mst[p].ar.size),
            .axi_mst_ar_lock (axi_req_mst[p].ar.lock),
            .axi_mst_ar_burst(axi_req_mst[p].ar.burst),

            .axi_mst_aw_valid(axi_req_mst[p].aw_valid),
            .axi_mst_aw_id   (axi_req_mst[p].aw.id),
            .axi_mst_aw_addr (axi_req_mst[p].aw.addr),
            .axi_mst_aw_len  (axi_req_mst[p].aw.len),
            .axi_mst_aw_size (axi_req_mst[p].aw.size),
            .axi_mst_aw_burst(axi_req_mst[p].aw.burst),
            .axi_mst_aw_lock (axi_req_mst[p].aw.lock),
            .axi_mst_aw_atop (axi_req_mst[p].aw.atop),

            .axi_mst_w_valid(axi_req_mst[p].w_valid),
            .axi_mst_w_data (axi_req_mst[p].w.data),
            .axi_mst_w_strb (axi_req_mst[p].w.strb),
            .axi_mst_w_last (axi_req_mst[p].w.last),

            .axi_mst_b_ready(axi_req_mst[p].b_ready),
            .axi_mst_r_ready(axi_req_mst[p].r_ready),

            .axi_slv_b_valid(axi_rsp_mst[p].b_valid),
            .axi_slv_b_id   (axi_rsp_mst[p].b.id),
            .axi_slv_b_resp (axi_rsp_mst[p].b.resp),

            .axi_slv_r_valid(axi_rsp_mst[p].r_valid),
            .axi_slv_r_id   (axi_rsp_mst[p].r.id),
            .axi_slv_r_data (axi_rsp_mst[p].r.data),
            .axi_slv_r_resp (axi_rsp_mst[p].r.resp),
            .axi_slv_r_last (axi_rsp_mst[p].r.last),

            .axi_slv_aw_ready(axi_rsp_mst[p].aw_ready),
            .axi_slv_ar_ready(axi_rsp_mst[p].ar_ready),
            .axi_slv_w_ready (axi_rsp_mst[p].w_ready),
            `RV_TESTER_TRANSACTIONS_SOURCE_AXI_SW_MST(2, p)
        );
    end

//new memory - LLC

typedef logic [AxiIdWidthMst_rv-1:0] id_mst_rv;
typedef logic [topology.TOP.PLATFORM.AXI.ID_WIDTH-1:0] id_slv_rv;
typedef logic [topology.TOP.PLATFORM.AXI.ADDR_WIDTH-1:0] addr_rv;
typedef logic [topology.TOP.PLATFORM.AXI.DATA_WIDTH-1:0] data_rv;
typedef logic [topology.TOP.PLATFORM.AXI.STRB_WIDTH-1:0] strb_rv;
typedef logic [AXI_USER_ID_WIDTH-1:0] user_rv;

`AXI_TYPEDEF_AW_CHAN_T(mst_aw_chan_rv, addr_rv, id_mst_rv, user_rv)
`AXI_TYPEDEF_AW_CHAN_T(slv_aw_chan_rv, addr_rv, id_slv_rv, user_rv)
`AXI_TYPEDEF_W_CHAN_T(w_chan_rv, data_rv, strb_rv, user_rv)
`AXI_TYPEDEF_B_CHAN_T(mst_b_chan_rv, id_mst_rv, user_rv)
`AXI_TYPEDEF_B_CHAN_T(slv_b_chan_rv, id_slv_rv, user_rv)
`AXI_TYPEDEF_AR_CHAN_T(mst_ar_chan_rv, addr_rv, id_mst_rv, user_rv)
`AXI_TYPEDEF_AR_CHAN_T(slv_ar_chan_rv, addr_rv, id_slv_rv, user_rv)
`AXI_TYPEDEF_R_CHAN_T(mst_r_chan_rv, data_rv, id_mst_rv, user_rv)
`AXI_TYPEDEF_R_CHAN_T(slv_r_chan_rv, data_rv, id_slv_rv, user_rv)
`AXI_TYPEDEF_REQ_T(mst_req_rv, mst_aw_chan_rv, w_chan_rv, mst_ar_chan_rv)
`AXI_TYPEDEF_REQ_T(slv_req_rv, slv_aw_chan_rv, w_chan_rv, slv_ar_chan_rv)
`AXI_TYPEDEF_RESP_T(mst_resp_rv, mst_b_chan_rv, mst_r_chan_rv)
`AXI_TYPEDEF_RESP_T(slv_resp_rv, slv_b_chan_rv, slv_r_chan_rv)

mst_req_rv axi_req_llc;
mst_resp_rv axi_rsp_llc;

rv_tester_mem #(
.NumMasters             ( topology.TOP.PLATFORM.AXI.TOTAL ),
.AxiIdWidth             ( topology.TOP.PLATFORM.AXI.ID_WIDTH ),
.AxiDataWidth           ( topology.TOP.PLATFORM.AXI.DATA_WIDTH ),
.AxiAddrWidth           ( topology.TOP.PLATFORM.AXI.ADDR_WIDTH ),
.AxiStrbWidth           ( topology.TOP.PLATFORM.AXI.STRB_WIDTH ),
.AxiUserWidth		( AXI_USER_ID_WIDTH ),
.NumLines_LLC           ( 128 ),
.NumBlocks_LLC          ( 4 ),
.SetAssociativity_LLC   ( 4 ),
.slv_req_t              ( slv_req_rv  ),
.slv_resp_t             ( slv_resp_rv ),
.mst_req_t              ( mst_req_rv  ),
.mst_resp_t             ( mst_resp_rv )
) inst(
.clk                    ( clk ),
.rst_n                  ( reset ),
.axi_req                ( axi_req ),
.axi_resp               ( axi_rsp ),
.axi_req_mst            ( axi_req_llc ),
.axi_resp_mst           ( axi_rsp_llc )
);


endmodule
