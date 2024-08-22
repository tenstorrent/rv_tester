module rv_tester
    import rv_tester_params::*;
#(
    parameter bit EXTERNAL_CLOCK            =       0,
    `TOPOLOGY
) (
    input clk_ext [NCLKS-1:0],
    `_RV_TESTER_PORTS(output,input)
);

    longint eot_addr;
    byte    eot_status;
    byte    eot_syscall;

    export "DPI-C" function cosim_set_eot;
    function void cosim_set_eot(input longint unsigned addr, input byte status, input byte syscall);
       eot_addr    = addr;
       eot_status  = status;
       eot_syscall = syscall;
    endfunction

    typedef longint unsigned LU;

    localparam int unsigned NoAddrRules = 20;

    typedef struct packed {
        int unsigned idx;
        logic [topology.TOP.PLATFORM.AXI.ADDR_WIDTH-1:0] start_addr;
        logic [topology.TOP.PLATFORM.AXI.ADDR_WIDTH-1:0] end_addr;
      } xbar_rule_t;

    logic bypass_mem = 1;
    logic bypass_cache = 1;
    logic rv_tester_reset = '1;

    /* verilator lint_off UNOPTFLAT */
    logic [2:0] clock_mode = 3'b000;
    /* verilator lint_on UNOPTFLAT */
    logic  def_clk      [NCLKS-1:0];
    logic  profile1_clk [NCLKS-1:0];
    logic  profile2_clk [NCLKS-1:0];
    logic  profile3_clk [NCLKS-1:0];
    logic  profile4_clk [NCLKS-1:0];
    logic  profile5_clk [NCLKS-1:0];
    logic  profile6_clk [NCLKS-1:0];

    if (EXTERNAL_CLOCK) begin
        assign clk[TB_CLK_IDX] = clk_ext[TB_CLK_IDX];
        for (genvar c = 1; c < NCLKS; c++) begin
            assign clk[c] = force_ref_clk ? clk_ext[REF_CLK_IDX] : clk_ext[c];
        end
    end else begin
        for (genvar c = 0; c < NCLKS; c++) begin
            `ifdef CLK_MUX_UNSUPPORTED
             rv_tester_clkgen #(.CLOCK_FREQ_MHZ(CLOCK_FREQ_MHZ[c])) clkgen(.clk(clk[c]));
            `else
             rv_tester_clkgen #(.CLOCK_FREQ_MHZ(CLOCK_FREQ_MHZ[c])) clkgen(.clk(def_clk[c]));
             rv_tester_clkgen #(.CLOCK_FREQ_MHZ(PROFILE1_CLOCK_FREQ_MHZ[c])) profile1_clkgen(.clk(profile1_clk[c]));
             rv_tester_clkgen #(.CLOCK_FREQ_MHZ(PROFILE2_CLOCK_FREQ_MHZ[c])) profile2_clkgen(.clk(profile2_clk[c]));
             rv_tester_clkgen #(.CLOCK_FREQ_MHZ(PROFILE3_CLOCK_FREQ_MHZ[c])) profile3_clkgen(.clk(profile3_clk[c]));
             rv_tester_clkgen #(.CLOCK_FREQ_MHZ(PROFILE4_CLOCK_FREQ_MHZ[c])) profile4_clkgen(.clk(profile4_clk[c]));
             rv_tester_clkgen #(.CLOCK_FREQ_MHZ(PROFILE5_CLOCK_FREQ_MHZ[c])) profile5_clkgen(.clk(profile5_clk[c]));
             rv_tester_clkgen #(.CLOCK_FREQ_MHZ(PROFILE6_CLOCK_FREQ_MHZ[c])) profile6_clkgen(.clk(profile6_clk[c]));
 
            clk_mux_glitch_free #(
                .NUM_INPUTS(7),
                .CLOCK_DURING_RESET(1)
            ) i_clk_mux (
                .clks_i         ({profile6_clk[c], profile5_clk[c], profile4_clk[c], profile3_clk[c],profile2_clk[c], profile1_clk[c], def_clk[c]}),
                .test_clk_i     (1'b0),             // FIXME:Add test clock
                .test_en_i      (1'b0),             // FIXME:Add test enable
                .async_rstn_i   (~rv_tester_reset),
                .async_sel_i    (clock_mode),
                .clk_o          (clk[c])
            );
            `endif
         end
     end
    

    import "DPI-C" function void rv_tester_streaming_dpi_init();
    import "DPI-C" function int rv_tester_parse_flags(); // dummy return value so that this gets called immediately. need this to happen before any other DPIs are called.
    import "DPI-C" function void rv_tester_set_seed();
    import "DPI-C" context function void rv_tester_cvm_error_handler();
    import "DPI-C" context function void rv_tester_parse_memmap(int unsigned no_addr_rules);
    import "DPI-C" context function void rv_tester_build_registry();
    import "DPI-C" function byte unsigned rv_tester_shutdown_registry();
    import "DPI-C" context function bit rv_tester_flush_callbacks();
    import "DPI-C" function bit pwrmgmt_get_warm_reset_en(string mode);

    localparam int unsigned AxiIdWidthMstRv    = topology.TOP.PLATFORM.AXI.ID_WIDTH + $clog2(topology.TOP.PLATFORM.AXI.TOTAL) + 1;

    logic flush_complete;

    xbar_rule_t [NoAddrRules-1:0] addr_map, addr_map_final, addr_map_idx1;    
    bit perf = 0;
    /* verilator lint_off MULTIDRIVEN */
    logic [NCLKS-1:0] sys_reset = '1;
    logic dut_reset [NCLKS-1:0];
    /* verilator lint_on MULTIDRIVEN */
    logic init_pulse;
    logic warm_reset_pulse;
    int unsigned warm_reset_clocks = 0;
    int unsigned soc_clocks = 0;
    logic pwrmgmt_force_ref_clk;
    logic cold_reset;
    logic warm_reset;
    LU clocks = 0;
    bit cb_poll = '0;
    bit dyn_clk_switch = '0;
    bit cb_success = '1;
    logic call_finish;
    int num_reruns = -1;

    string warm_reset_string;
    logic warm_reset_en = 0;
    logic warm_reset_req;
    logic warm_reset_req_d1;
    logic warm_reset_now = 0;
    int num_resets = -1;
    int target_num_resets = 0;

    bit trace_en = 0;

    bit [NHARTS-1:0] poke_event_out;
    bit poke_event_in;
    bit cla_clk_halt = 0;
    bit jtag_en = 0;
    bit overlay_mmr_en = 0;
    logic trace_quiesced;
    logic jtag_quiesced;


    logic terminate_now;
    logic rerun_now;
    /* verilator lint_off UNOPTFLAT */
    rv_tester_pkg::terminate_t rv_tester_error_terminate;
    rv_tester_pkg::terminate_t sysmod_terminate;
    /* verilator lint_off UNOPTFLAT */
    rv_tester_pkg::terminate_t cosim_terminate [NHARTS-1:0];
    logic cosim_terminate_any;
    int instructions = 0;

    int quiesce_counter = 0;
    int trace_counter = 5000;
    int quiesce_timeout = 500;
    int flush_counter = 0;
    int flush_timeout = 25000;
    bit print_terminate_message = '1;

    int rand_dmi_driver_dly = 0;
    int dmi_poll_counter = 0; 
    int dmi_poll_timeout = 50000;
    logic dmi_poll_timeout_terminate;
    logic [31:0] dmi_commands_in_queue; 

    int trace_timeout = 50000;
    int freq_switch_ncycles = 7000;
    int clk_profile = 0;

    int assertion_test_cycle = 0;

    parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.ID, 0);



    bit gen_clocks = '0;
    string cvm_verbosity_string, gen_clocks_verbosity_string;
    int unsigned cvm_verbosity, gen_clocks_verbosity;


    assign terminate           = (rv_tester_error_terminate.terminate || ((sysmod_terminate.terminate || cosim_terminate_any || dmi_poll_timeout_terminate) && !sys_reset[TB_CLK_IDX]) || quiesce_counter > 0) && !rv_tester_reset;
    assign terminate_now       = (terminate && (quiesced || quiesce_counter >= quiesce_timeout) && (flush_complete || flush_counter >= flush_timeout) && ((dmi_commands_in_queue == '0) | (dmi_poll_counter > 'h1)) && (!trace_en || trace_quiesced || trace_counter >= trace_timeout) && (!jtag_en || jtag_quiesced )) || warm_reset_now; 

    
    assign rerun_now           = terminated && ((num_reruns > 0) || (warm_reset_en && (num_resets <= target_num_resets)));

  `ifndef CLK_MUX_UNSUPPORTED 
    always @(posedge clk[TB_CLK_IDX])begin
      if (rv_tester_reset)begin 
            clock_mode <= clk_profile[2:0];
      end
      /* verilator lint_off WIDTH */
      if(dyn_clk_switch & (clocks >10) &  ((clocks % freq_switch_ncycles) == 0)) begin
        //dynamically select clk from available profiles
        //this logic will generate the select pins of the mux ,which will switch between clks
        clock_mode <= clock_mode + 1'b1;
        if(clock_mode == 3'b111)
          clock_mode <= '0;
      end
       /* verilator lint_on WIDTH */
    end
    `endif

    /*
    * Don't put an DPI calls here, zebu gets confused when signals are driven
    * in the same block as zDPI and leads to weird bugs. Eg, triggers on
    * terminated stopped working, and rv_tester_reset stopped being depositable
    * from the tcl shell.
    */
    always @(posedge clk[TB_CLK_IDX]) begin

        rv_tester_reset <= rerun_now;
        clocks          <= clocks + 1;

        quiesce_counter <= quiesce_counter + int'(terminate);
        flush_counter   <= flush_counter + int'(quiesced);

        for (int i=0; i<NHARTS; i++) begin
          instructions  <= instructions + int'(pmci[i][INSTRUCTIONS]);
        end

        if (rv_tester_reset) begin
            quiesce_counter <= '0;
            flush_counter   <= '0;
            instructions    <= '0;
            dmi_poll_timeout_terminate <= '0;
        end

        if (terminate && terminated) begin
            num_resets      <= -1;
        end

        if(trace_en && (quiesce_counter >= quiesce_timeout)) begin
           trace_counter <= trace_counter + 1;
        end else if(trace_en) begin
          trace_counter <='0;
        end else if(!trace_en)begin
          trace_counter <= trace_timeout + 10;
        end

    end

    always @(posedge clk[TB_CLK_IDX]) begin
        if(rerun_now) begin
            $display("<%0d> [RVTESTER]: rerunning test %0d time(s)", clocks, num_reruns);
        end
    end

    /*
    * Group all zebu zDPI DPIs here
    * These are run on a separate thread than the slower zebi3
    */
    always @(posedge clk[TB_CLK_IDX]) begin
        if (rv_tester_reset) begin
            // Used for offine DPI
            rv_tester_streaming_dpi_init();
        end
    end

    /*
    * Group all zebu zemi3 DPIs here
    * These are run on a separate thread than the faster zDPI, so make sure
    * these are only run at rv_tester_reset, when no other zDPIs should be
    * called.
    */
    always @(posedge clk[TB_CLK_IDX]) begin

        automatic int _;

        if (rv_tester_reset) begin

            $display("[RVTESTER]: new test");
            _ = rv_tester_parse_flags();
            if (num_resets < 0)
                rv_tester_set_seed();
            rv_tester_cvm_error_handler();
            rv_tester_parse_memmap(NoAddrRules);

            /* verilator lint_off BLKSEQ */
            // zebu bug doesn't allow nested function calls, so create intermediate variables
            cvm_verbosity_string        = cvm_plusargs::get_string("cvm_verbosity");
            gen_clocks_verbosity_string = cvm_plusargs::get_string("gen_clocks_verbosity");
            cvm_verbosity               = cvm_logger::get_verbosity(cvm_verbosity_string);
            gen_clocks_verbosity        = cvm_logger::get_verbosity(gen_clocks_verbosity_string);
            warm_reset_string           = cvm_plusargs::get_string("warm_reset");
            warm_reset_en               = pwrmgmt_get_warm_reset_en(warm_reset_string);
            rv_tester_error_terminate.terminate = '0;
            /* verilator lint_on BLKSEQ */

            perf                 <= cvm_plusargs::get_bool("perf") != '0;
            rand_dmi_driver_dly  <= cvm_plusargs::get_int("rand_dmi_driver_dly"); 
            cb_poll              <= cvm_plusargs::get_bool("cb_async") == '0;
            quiesce_timeout      <= cvm_plusargs::get_int("quiesce_timeout");
            dmi_poll_timeout     <= cvm_plusargs::get_int("dmi_poll_timeout");
            trace_timeout        <= cvm_plusargs::get_int("trace_timeout");
            flush_timeout        <= cvm_plusargs::get_int("flush_timeout");
            freq_switch_ncycles  <= cvm_plusargs::get_int("freq_switch_ncycles");
            clk_profile          <= cvm_plusargs::get_int("clk_profile");
            dyn_clk_switch       <= cvm_plusargs::get_bool("dyn_clk_switch") != '0;
            call_finish          <= cvm_plusargs::get_bool("terminate_call_finish") != '0;
            gen_clocks           <= cvm_verbosity >= gen_clocks_verbosity;
            bypass_mem           <= cvm_plusargs::get_bool("bypass_mem") != '0;
            bypass_cache         <= cvm_plusargs::get_bool("bypass_cache") != '0;
            assertion_test_cycle <= cvm_plusargs::get_int("assertion_test_cycle");

            trace_en             <= cvm_plusargs::get_bool("trace_en") != '0;
            cla_clk_halt         <= cvm_plusargs::get_bool("cla_clk_halt") != '0;
            overlay_mmr_en       <= cvm_plusargs::get_bool("overlay_mmr_en") != '0;
            jtag_en              <= cvm_plusargs::get_bool("jtag_en") != '0;
            rand_dmi_driver_dly  <= cvm_plusargs::get_int("rand_dmi_driver_dly");

            $display("[RVTESTER]: reconstructing registry");
            rv_tester_build_registry();

        end
        clock_mode      <= clk_profile[2:0];
        num_reruns      <= num_reruns - int'(rerun_now);
        if (num_reruns < 0) begin
            num_reruns  <= cvm_plusargs::get_int("num_reruns");
        end

        num_resets <= num_resets + int'(warm_reset_now);
        if (warm_reset_en && (num_resets < 0)) begin
            num_resets          <= 0;
            target_num_resets   <= cvm_rand::get("warm_reset_count");
        end
    end

    /*
    * rv_tester_shutdown_registry could be called at the same time as
    * transactions (axi, cosim, etc). So it's put in a separate always block
    * from the rv_tester_reset group of zemi3 DPI. Otherwise zebu will make
    * rv_tester_shutdown_registry a zemi3 DPI and we'll have thread safety
    * issues with coinciding zDPIs from transactions.
    */
    always @(posedge clk[TB_CLK_IDX]) begin

        automatic logic shutdowned = '0;

        if (rv_tester_reset) begin
            print_terminate_message <= '1;
        end

        if(terminate_now && cla_clk_halt && !shutdown) begin
            $display("Error: CLK_HALT is not generated before test termination");
        end

        if (terminate_now && !terminated) begin

            if(tj_max_interrupt) begin
                $display("<%0d> [RVTESTER]: TJ Max interrupt detected. Terminting the test.", clocks);
            end
            if (print_terminate_message) begin
                if (warm_reset_now) begin
                    $display("<%0d> [RVTESTER]: starting warm reset", clocks);
                end else if (quiesced) begin
                    $display("<%0d> [RVTESTER]: exiting gracefully", clocks);
                end else if (quiesce_counter == 0) begin
                    $display("<%0d> [RVTESTER]: exiting immediately because +quiesce_counter=0", clocks);
                end else begin
                    $display("<%0d> [RVTESTER]: Error: Waiting to quiesce for more than %0d cycles", clocks, quiesce_timeout);
                end

            end

            shutdowned = rv_tester_shutdown_registry() != '0;

            if (!shutdowned) begin
                if (print_terminate_message) begin
                    $display("<%0d> [RVTESTER]: Could not shutdown, trying again until timeout", clocks);
                end
            end

            if (shutdowned && num_reruns == '0 && warm_reset_req == '0) begin
                $display("INFO_PASS_METRIC:{\"instruction_count\": %0d}", instructions);
                $display("INFO_PASS_REGR_METRIC:{\"name\": \"instructions\", \"value\":%0d, \"type\": \"i\", \"action\": \"sum\"}", instructions);
                $display("INFO_PASS:{\"clocks\": %0d}", clocks);

                if (call_finish) begin
                    $finish();
                end
            end
            print_terminate_message <= '0;
        end

        terminated <= !rv_tester_reset && (terminated || (terminate_now && shutdowned)) && !rerun_now;

        if (warm_reset_now) begin
            /* verilator lint_off BLKSEQ */
            warm_reset_clocks = soc_clocks;
            /* verilator lint_on BLKSEQ */
        end

        warm_reset_req_d1 <= warm_reset_req;
        warm_reset_now <= warm_reset_req & ~warm_reset_req_d1;

    end

    for (genvar c = 0; c < NCLKS; c++) begin
        always @(posedge dut_clk[c]) begin
            sys_reset[c] <= '0;
            if (rv_tester_reset)
                sys_reset <= '1;
        end
    end

    // We also assert reset at the end of the test to quiesce the DPIs.
    logic reset_pullup;
    assign reset_pullup = rv_tester_reset || sys_reset[TB_CLK_IDX] || terminate_now || terminated;

    assign reset[COLD_RESET_IDX] = cold_reset || init_pulse || (reset_pullup && !warm_reset_pulse);
    assign reset[WARM_RESET_IDX] = warm_reset;

    assign dut_reset[TB_CLK_IDX] = reset[COLD_RESET_IDX] || reset[WARM_RESET_IDX];
    assign dut_reset[CORE_CLK_IDX] = &core_no_fetch[NHARTS-1:0] | force_ref_clk;
    assign dut_reset[AXI_CLK_IDX] = &core_no_fetch[NHARTS-1:0] | force_ref_clk;
    assign dut_reset[SOC_CLK_IDX] = cold_reset;
    assign dut_reset[REF_CLK_IDX] = &core_no_fetch[NHARTS-1:0];

`ifdef NEGEDGE_UNSUPPORTED
    always@(posedge clk[TB_CLK_IDX]) begin
`else
    always@(negedge clk[TB_CLK_IDX]) begin
`endif
        if (cb_poll) begin
            /* verilator lint_off BLKSEQ */
            cb_success = rv_tester_flush_callbacks();
            /* verilator lint_on BLKSEQ */
        end
    end

    function void rv_tester_terminate ();
        $display("<%0d> rv_tester_terminate: attempting to terminate", clocks);
        rv_tester_error_terminate.terminate = '1;
    endfunction
    export "DPI-C" function rv_tester_terminate;

    `RV_TESTER_TRANSACTIONS_DOMAIN(1, dut_clk[CORE_CLK_IDX]);
    `RV_TESTER_TRANSACTIONS_DOMAIN(2, dut_clk[AXI_CLK_IDX]);
    `RV_TESTER_TRANSACTIONS_DOMAIN(3, dut_clk[SOC_CLK_IDX]);

    rv_tester_pkg::dm_write_t  trickbox_dmi_write;

    localparam int AXI_CLOCK_PERIOD = 1000000 / CLOCK_FREQ_MHZ[AXI_CLK_IDX];
    localparam int JTAG_CLOCK_PERIOD = 10*100;
    localparam int OVERLAY_CLOCK_PERIOD = 2*AXI_CLOCK_PERIOD;
    sysmod #(
        .CLOCK_PERIOD_PS(AXI_CLOCK_PERIOD),
        .JTAG_CLOCK_PERIOD_PS(JTAG_CLOCK_PERIOD),
        .OVERLAY_CLOCK_PERIOD_PS( OVERLAY_CLOCK_PERIOD),
        .SW_CLOCK_UPDATE_PERIOD_PS(SW_CLOCK_PERIOD_PS),
        .NUM(0),
        `TOPOLOGY_CFG,
        `RV_TESTER_TRANSACTIONS_SYSMOD_SOURCE_PARAMS(0)
    ) sysmod (
        .clk(dut_clk[AXI_CLK_IDX]),
        .reset(sys_reset[AXI_CLK_IDX]),
        .trace_quiesced(trace_quiesced),
        .jtag_quiesced(jtag_quiesced),
        .bootstrap,
        .dmi_write(trickbox_dmi_write),
        .event_triggers(event_triggers),
        .interrupt,
        .jtag_req,
        .jtag_tck_trst,
        .jtag_resp,
        .aplic_interrupt,
        .terminate(sysmod_terminate),
        `RV_TESTER_TRANSACTIONS_SYSMOD_SOURCE_PORTS(2, 0, 0)
    );

`ifndef DMI_TB_WRITES_UNSUPPORTED
    logic [7:0] misc_signals;
    logic dmi_status;
    
    dmi_driver i_dmi_driver(
        .clk(dut_clk[AXI_CLK_IDX]),
        .reset(~dut_reset[AXI_CLK_IDX]),
        .rand_dmi_driver_dly,
        .dmi_req_ready,
        .dmi_resp_valid,
        .dmi_resp,

        .dmi_req_valid,
        .dmi_req,
        .dmi_resp_ready,
        .dmi_status,
        .dmi_commands_in_queue,
        .misc_signals,

        .trickbox_dmi_write(trickbox_dmi_write)
    );

    dm_model #(
        .NUM(0),
        `TOPOLOGY_CFG,
        `RV_TESTER_TRANSACTIONS_DM_MODEL_SOURCE_PARAMS(0)
    ) i_dm_model(
        .clk(dut_clk[AXI_CLK_IDX]),
        .reset(sys_reset[TB_CLK_IDX]),
        .dmi_req(dmi_req),
        .dmi_req_valid(dmi_req_valid),
        .dmi_resp_valid(dmi_resp_valid),
        .dmi_resp(dmi_resp),
        .terminate,
        .dm_mem_tx_vld,
        .dm_mem_tx_we,
        .dm_mem_tx_addr,
        .dm_mem_tx_rd_data,
        .dm_mem_tx_wr_data,
        .dm_mem_tx_wr_data_be,
        .dmi_status,
        .dmi_commands_in_queue,
        .misc_signals,
        `RV_TESTER_TRANSACTIONS_DM_MODEL_SOURCE_PORTS(2,0,0)
    );

    always @(posedge dut_clk[AXI_CLK_IDX]) begin
        if (sys_reset[TB_CLK_IDX] | !dmi_status)
            dmi_poll_counter <= 0; 
        else if (dmi_status) begin
            dmi_poll_counter <= dmi_poll_counter + 1;

            if (dmi_poll_counter > dmi_poll_timeout) begin
                $display("<%0d> [RVTESTER]: Error: Debug poll timeout limit reached.", clocks);
                dmi_poll_timeout_terminate <= 1;
            end
            else if ((dmi_poll_counter >= 'h1) && terminate) begin
               $display("<%0d> [RVTESTER]: Debug poll stopped as terminate condition detected", clocks); 
            end
        end
    end

`endif

    // coverage
    arch_sample arch_sample ();

    assign poke_event_in = (poke_event_out != '0) ? 1'b1 : 1'b0;

`ifndef NO_COSIM
    for (genvar c = 0; c < NHARTS; c++) begin: cosim_inst
      cosim #(
          .NUM(c),
          .NRET(NRETS[c]),
          .NREAD(NREADS[c]),
          .NINSERT(NINSERTS[c]),
          .NWRITE(NWRITES[c]),
          .NBYPASS(NBYPASSES[c]),
          .NIFETCH(NIFETCHES[c]),
          .NIEVICT(NIEVICTS[c]),
          .NoAddrRules(NoAddrRules),
          .rule_t(xbar_rule_t),
          `TOPOLOGY_CFG,
          `RV_TESTER_TRANSACTIONS_COSIM_SOURCE_PARAMS(0)
      ) cosim (
          .tb_clk(clk[TB_CLK_IDX]),
          .clk(dut_clk[CORE_CLK_IDX]),
          .reset(sys_reset[TB_CLK_IDX]),
          .dut_reset(dut_reset[CORE_CLK_IDX]),
          .clocks,
          .rvfi(rvfi[NRETS_CUMSUM[c] +: NRETS[c]]),
          .csri(csri[c]),
          .mcmi_read(mcmi_read[NREADS_CUMSUM[c] +: NREADS[c]]),
          .mcmi_insert(mcmi_insert[NINSERTS_CUMSUM[c] +: NINSERTS[c]]),
          .mcmi_write(mcmi_write[NWRITES_CUMSUM[c] +: NWRITES[c]]),
          .mcmi_bypass(mcmi_bypass[NBYPASSES_CUMSUM[c] +: NBYPASSES[c]]),
          .mcmi_ifetch_req(mcmi_ifetch_req[NIFETCHES_CUMSUM[c] +: NIFETCHES[c]]),
          .mcmi_ifetch_resp(mcmi_ifetch_resp[NIFETCHES_CUMSUM[c] +: NIFETCHES[c]]),
          .mcmi_ievict(mcmi_ievict[NIEVICTS_CUMSUM[c] +: NIEVICTS[c]]),
          .wired_interrupt(interrupt_pend[c]),
          .imsic_interrupt(axi_msi), //FIXME
          .imsic_msi(axi_msi_packets[c]), //FIXME
          .imsic_ipi(axi_ipi_packets[c]), //FIXME
          .debug_mode(debug_mode[c]),
          .terminate(cosim_terminate[c]),
          .eot_addr(eot_addr),
          .addr_map(addr_map),
          .poke_event_out(poke_event_out[c]),
          .poke_event_in(poke_event_in),
          `RV_TESTER_TRANSACTIONS_COSIM_SOURCE_PORTS(1, c, 0)
      );
    end
`endif

    always @(posedge clk[TB_CLK_IDX]) begin
        if (eot_status != 0) 
        /* verilator lint_off ASSIGNIN */
            cosim_eot_addr <= eot_addr;
        /* verilator lint_on ASSIGNIN */
    end


    localparam RESET_TB_CLOCKS = 100;
    localparam RESET_SOC_CLOCKS = 20;
    assign init_pulse = (clocks < RESET_TB_CLOCKS);
    assign warm_reset_pulse = (soc_clocks > RESET_SOC_CLOCKS) && (soc_clocks < (warm_reset_clocks + RESET_SOC_CLOCKS));
    generate
        if (PWRMGMT_EN) begin : pwrmgmt
            pwrmgmt #(
                .NUM(0),
                `TOPOLOGY_CFG,
                `RV_TESTER_TRANSACTIONS_PWRMGMT_SOURCE_PARAMS(0)
            ) pwrmgmt (
                .tb_clk(clk[TB_CLK_IDX]),
                .tb_reset(sys_reset[TB_CLK_IDX]),
                .dut_clk(dut_clk),
                .dut_reset(dut_reset),
                .reset_count(num_resets),
                .target_reset_count(target_num_resets),
                .cold_reset(cold_reset),
                .warm_reset(warm_reset),
                .warm_reset_en(warm_reset_en),
                .warm_reset_req(warm_reset_req),
                .reset_hold(reset_hold),
                .force_ref_clk(pwrmgmt_force_ref_clk),
                `RV_TESTER_TRANSACTIONS_PWRMGMT_SOURCE_PORTS(3,0,0)
            );
            assign force_ref_clk = perf ? '0 : (pwrmgmt_force_ref_clk || init_pulse || warm_reset_pulse);
        end else begin
            assign cold_reset = (clocks < RESET_TB_CLOCKS);
            assign warm_reset = '0;
            assign force_ref_clk = '1;
        end
    endgenerate

    for (genvar c = 0; c < NHARTS; c++) begin: interrupts
        interrupts #(
            .NUM(c),
            `TOPOLOGY_CFG,
            `RV_TESTER_TRANSACTIONS_INTERRUPTS_SOURCE_PARAMS(0)
        ) interrupts (
            .tb_clk(clk[TB_CLK_IDX]),
            .tb_reset(sys_reset[TB_CLK_IDX]),
            .clk(dut_clk[AXI_CLK_IDX]),
            .reset(dut_reset[AXI_CLK_IDX]),
            .core_no_fetch(core_no_fetch[c]),
            .nmi(nmi[c]),
            `RV_TESTER_TRANSACTIONS_INTERRUPTS_SOURCE_PORTS(2,c,0)
        );
    end

    for (genvar c = 0; c < NHARTS; c++) begin: triggers
        triggers #(
            .NUM(c),
            `TOPOLOGY_CFG,
            `RV_TESTER_TRANSACTIONS_TRIGGERS_SOURCE_PARAMS(0)
        ) triggers (
            .tb_clk(clk[TB_CLK_IDX]),
            .tb_reset(sys_reset[TB_CLK_IDX]),
            .dut_clk(dut_clk[AXI_CLK_IDX]),
            .dut_reset(dut_reset[AXI_CLK_IDX]),
            .no_fetch(core_no_fetch[c]),
            .event_trigger_vec(event_triggers[c]),
            `RV_TESTER_TRANSACTIONS_TRIGGERS_SOURCE_PORTS(2,c,0)
        );
    end

    aplic_monitor #(
        .NUM(0),
        `TOPOLOGY_CFG,
        `RV_TESTER_TRANSACTIONS_APLIC_MONITOR_SOURCE_PARAMS(0)
    ) i_aplic_monitor(
        .clk(dut_clk[AXI_CLK_IDX]),
        .reset(sys_reset[TB_CLK_IDX]),
        .terminate,
        .aplic_pin_input(aplic_interrupt),
        .msi_axi_req('0),
        .axi_req_mst(aplic_mmr_axi_req_mst[0]),
        .axi_resp_mst(aplic_mmr_axi_rsp_mst[0]),
        //.axi_resp_mst('0),
        .misc_signals('0),
        `RV_TESTER_TRANSACTIONS_APLIC_MONITOR_SOURCE_PORTS(1,0,0)
    );

    aclint_checker #(
        .NUM(0),
        `TOPOLOGY_CFG,
        `RV_TESTER_TRANSACTIONS_ACLINT_CHECKER_SOURCE_PARAMS(0)
    ) i_aclint_checker(
        .tb_clk(clk[TB_CLK_IDX]),
        .cl_clk(dut_clk[CORE_CLK_IDX]),
        .rf_clk(dut_clk[REF_CLK_IDX]),
        .reset(sys_reset[TB_CLK_IDX]),
        .dut_reset(dut_reset[REF_CLK_IDX]),
        .terminate,
        .AcCrSynci(AcCrSynci),
        .AcReqPkti(AcReqPkti),
        .AcReqPktRfClki(AcReqPktRfClki),
        .rvfi(rvfi),
        .mcmi_bypass(mcmi_bypass),
        .AcMtimei(AcMtimei),
        .AcMtipi(AcMtipi),
        `RV_TESTER_TRANSACTIONS_ACLINT_CHECKER_SOURCE_PORTS(1,0,0)
    );

    always_comb begin
        cosim_terminate_any = '0;
        for (int i=0; i<NHARTS; i++) begin
            cosim_terminate_any |= cosim_terminate[i].terminate;
        end
    end

    for (genvar p = 0; p < NHARTS; p++) begin: pmu_inst
      pmu #(
          .NUM(p),
          .NRET(NRETS[p]),
          `TOPOLOGY_CFG,
          `RV_TESTER_TRANSACTIONS_PMU_SOURCE_PARAMS(0)
      ) pmu (
          .clk(dut_clk[CORE_CLK_IDX]),
          .sys_reset(sys_reset[CORE_CLK_IDX]),
          .reset(dut_reset[CORE_CLK_IDX]),
          .clocks,
          .pmci(pmci[p]),
          .sc_pmci(sc_pmci),
          .rvfi(rvfi[NRETS_CUMSUM[p] +: NRETS[p]]),
          .terminate,
          `RV_TESTER_TRANSACTIONS_PMU_SOURCE_PORTS(1, p, 0)
      );
    end

    assign tx_dom_1.logger_cycle_0s[0][0].valid = gen_clocks;
    assign tx_dom_1.logger_cycle_0s[0][0].data.location = location;
    assign tx_dom_1.logger_cycle_0s[0][0].data.clock = clocks;

    localparam NoOfMasters = ( topology.TOP.PLATFORM.AXI.TOTAL < 2 ) ? 2 : topology.TOP.PLATFORM.AXI.TOTAL ;
    for (genvar p = 0; p < NoOfMasters; p++) begin : axi_sw_slvs
        axi_sw #(
            .ADDR_WIDTH(topology.TOP.PLATFORM.AXI.ADDR_WIDTH),
            .DATA_WIDTH(topology.TOP.PLATFORM.AXI.DATA_WIDTH),
            .ID_WIDTH(AxiIdWidthMstRv),
            .STRB_WIDTH(topology.TOP.PLATFORM.AXI.STRB_WIDTH),
            .R_Q_MAX(topology.TOP.PLATFORM.AXI.R_Q_MAX),
            .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.AXI.ID, p)),
            `RV_TESTER_TRANSACTIONS_AXI_SW_SOURCE_PARAMS(0)
        ) axi_sw(
            .clk(dut_clk[AXI_CLK_IDX]),
            .sys_reset(sys_reset[AXI_CLK_IDX]),
            .reset_n(~dut_reset[AXI_CLK_IDX]),
            .axi_mst_ar_valid(axi_req_llc[p].ar_valid),
            .axi_mst_ar_id   (axi_req_llc[p].ar.id),
            .axi_mst_ar_addr (axi_req_llc[p].ar.addr),
            .axi_mst_ar_len  (axi_req_llc[p].ar.len),
            .axi_mst_ar_size (axi_req_llc[p].ar.size),
            .axi_mst_ar_lock (axi_req_llc[p].ar.lock),
            .axi_mst_ar_burst(axi_req_llc[p].ar.burst),
            .axi_mst_ar_cache (axi_req_llc[p].ar.cache), 
            .axi_mst_ar_prot  (axi_req_llc[p].ar.prot),
            .axi_mst_ar_qos   (axi_req_llc[p].ar.qos), 
            .axi_mst_ar_region(axi_req_llc[p].ar.region),
            .axi_mst_ar_user  (axi_req_llc[p].ar.user), 

            .axi_mst_aw_valid(axi_req_llc[p].aw_valid),
            .axi_mst_aw_id   (axi_req_llc[p].aw.id),
            .axi_mst_aw_addr (axi_req_llc[p].aw.addr),
            .axi_mst_aw_len  (axi_req_llc[p].aw.len),
            .axi_mst_aw_size (axi_req_llc[p].aw.size),
            .axi_mst_aw_burst(axi_req_llc[p].aw.burst),
            .axi_mst_aw_lock (axi_req_llc[p].aw.lock),
            .axi_mst_aw_atop (axi_req_llc[p].aw.atop),

            .axi_mst_w_valid(axi_req_llc[p].w_valid),
            .axi_mst_w_data (axi_req_llc[p].w.data),
            .axi_mst_w_strb (axi_req_llc[p].w.strb),
            .axi_mst_w_last (axi_req_llc[p].w.last),

            .axi_mst_b_ready(axi_req_llc[p].b_ready),
            .axi_mst_r_ready(axi_req_llc[p].r_ready),

            .axi_slv_b_valid(axi_rsp_llc[p].b_valid),
            .axi_slv_b_id   (axi_rsp_llc[p].b.id),
            .axi_slv_b_resp (axi_rsp_llc[p].b.resp),

            .axi_slv_r_valid(axi_rsp_llc[p].r_valid),
            .axi_slv_r_id   (axi_rsp_llc[p].r.id),
            .axi_slv_r_data (axi_rsp_llc[p].r.data),
            .axi_slv_r_resp (axi_rsp_llc[p].r.resp),
            .axi_slv_r_last (axi_rsp_llc[p].r.last),

            .axi_slv_aw_ready(axi_rsp_llc[p].aw_ready),
            .axi_slv_ar_ready(axi_rsp_llc[p].ar_ready),
            .axi_slv_w_ready (axi_rsp_llc[p].w_ready),
            `RV_TESTER_TRANSACTIONS_AXI_SW_SOURCE_PORTS(2, p, 0)
        );


        ext_mem_stall_checker stall_checker(
            .clk(dut_clk[AXI_CLK_IDX]),
            .reset_n(~dut_reset[AXI_CLK_IDX]),
            .axi_req(axi_req[p]),
            .axi_rsp(axi_rsp[p])
        );

    end

   localparam NoOfNcioMasters =  topology.TOP.PLATFORM.NCIO_AXI.TOTAL  ;
    for (genvar p = 0; p < NoOfNcioMasters; p++) begin : ncio_axi_sw_slvs
        axi_sw #(
            .ADDR_WIDTH(topology.TOP.PLATFORM.NCIO_AXI.ADDR_WIDTH),
            .DATA_WIDTH(topology.TOP.PLATFORM.NCIO_AXI.DATA_WIDTH),
            .ID_WIDTH(topology.TOP.PLATFORM.NCIO_AXI.ID_WIDTH),
            .STRB_WIDTH(topology.TOP.PLATFORM.NCIO_AXI.STRB_WIDTH),
            .R_Q_MAX(topology.TOP.PLATFORM.AXI.R_Q_MAX),
            .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.NCIO_AXI.ID, p)),
            `RV_TESTER_TRANSACTIONS_AXI_SW_SOURCE_PARAMS(1)
        ) ncio_axi_sw(
            .clk(dut_clk[AXI_CLK_IDX]),
            .sys_reset(sys_reset[AXI_CLK_IDX]),
            .reset_n(~dut_reset[AXI_CLK_IDX]),
            .axi_mst_ar_valid(ncio_axi_req[p].ar_valid),
            .axi_mst_ar_id   (ncio_axi_req[p].ar.id),
            .axi_mst_ar_addr (ncio_axi_req[p].ar.addr),
            .axi_mst_ar_len  (ncio_axi_req[p].ar.len),
            .axi_mst_ar_size (ncio_axi_req[p].ar.size),
            .axi_mst_ar_lock (ncio_axi_req[p].ar.lock),
            .axi_mst_ar_burst(ncio_axi_req[p].ar.burst),

            .axi_mst_aw_valid(ncio_axi_req[p].aw_valid),
            .axi_mst_aw_id   (ncio_axi_req[p].aw.id),
            .axi_mst_aw_addr (ncio_axi_req[p].aw.addr),
            .axi_mst_aw_len  (ncio_axi_req[p].aw.len),
            .axi_mst_aw_size (ncio_axi_req[p].aw.size),
            .axi_mst_aw_burst(ncio_axi_req[p].aw.burst),
            .axi_mst_aw_lock (ncio_axi_req[p].aw.lock),
            .axi_mst_aw_atop (ncio_axi_req[p].aw.atop),

            .axi_mst_w_valid(ncio_axi_req[p].w_valid),
            .axi_mst_w_data (ncio_axi_req[p].w.data),
            .axi_mst_w_strb (ncio_axi_req[p].w.strb),
            .axi_mst_w_last (ncio_axi_req[p].w.last),

            .axi_mst_b_ready(ncio_axi_req[p].b_ready),
            .axi_mst_r_ready(ncio_axi_req[p].r_ready),

            .axi_slv_b_valid(ncio_axi_rsp[p].b_valid),
            .axi_slv_b_id   (ncio_axi_rsp[p].b.id),
            .axi_slv_b_resp (ncio_axi_rsp[p].b.resp),

            .axi_slv_r_valid(ncio_axi_rsp[p].r_valid),
            .axi_slv_r_id   (ncio_axi_rsp[p].r.id),
            .axi_slv_r_data (ncio_axi_rsp[p].r.data),
            .axi_slv_r_resp (ncio_axi_rsp[p].r.resp),
            .axi_slv_r_last (ncio_axi_rsp[p].r.last),

            .axi_slv_aw_ready(ncio_axi_rsp[p].aw_ready),
            .axi_slv_ar_ready(ncio_axi_rsp[p].ar_ready),
            .axi_slv_w_ready (ncio_axi_rsp[p].w_ready),
            `RV_TESTER_TRANSACTIONS_AXI_SW_SOURCE_PORTS(2, p, 1)
        );
    end


   localparam NoOfAplicMomMsiMasters =  topology.TOP.PLATFORM.APLIC_MSI_AXI.TOTAL  ;
    for (genvar p = 0; p < NoOfAplicMomMsiMasters; p++) begin : aplic_msi_axi_sw_slvs
        axi_sw #(
            .ADDR_WIDTH(topology.TOP.PLATFORM.APLIC_MSI_AXI.ADDR_WIDTH),
            .DATA_WIDTH(topology.TOP.PLATFORM.APLIC_MSI_AXI.DATA_WIDTH),
            .ID_WIDTH(topology.TOP.PLATFORM.APLIC_MSI_AXI.ID_WIDTH),
            .STRB_WIDTH(topology.TOP.PLATFORM.APLIC_MSI_AXI.STRB_WIDTH),
            .R_Q_MAX(topology.TOP.PLATFORM.AXI.R_Q_MAX),
            .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.APLIC_MSI_AXI.ID, p)),
            `RV_TESTER_TRANSACTIONS_AXI_SW_SOURCE_PARAMS(2)
        ) aplic_msi_axi_sw(
            .clk(dut_clk[AXI_CLK_IDX]),
            .sys_reset(sys_reset[AXI_CLK_IDX]),
            .reset_n(~dut_reset[AXI_CLK_IDX]),
            .axi_mst_ar_valid(aplic_msi_axi_req[p].ar_valid),
            .axi_mst_ar_id   (aplic_msi_axi_req[p].ar.id),
            .axi_mst_ar_addr (aplic_msi_axi_req[p].ar.addr),
            .axi_mst_ar_len  (aplic_msi_axi_req[p].ar.len),
            .axi_mst_ar_size (aplic_msi_axi_req[p].ar.size),
            .axi_mst_ar_lock (aplic_msi_axi_req[p].ar.lock),
            .axi_mst_ar_burst(aplic_msi_axi_req[p].ar.burst),

            .axi_mst_aw_valid(aplic_msi_axi_req[p].aw_valid),
            .axi_mst_aw_id   (aplic_msi_axi_req[p].aw.id),
            .axi_mst_aw_addr (aplic_msi_axi_req[p].aw.addr),
            .axi_mst_aw_len  (aplic_msi_axi_req[p].aw.len),
            .axi_mst_aw_size (aplic_msi_axi_req[p].aw.size),
            .axi_mst_aw_burst(aplic_msi_axi_req[p].aw.burst),
            .axi_mst_aw_lock (aplic_msi_axi_req[p].aw.lock),
            .axi_mst_aw_atop (aplic_msi_axi_req[p].aw.atop),

            .axi_mst_w_valid(aplic_msi_axi_req[p].w_valid),
            .axi_mst_w_data (aplic_msi_axi_req[p].w.data),
            .axi_mst_w_strb (aplic_msi_axi_req[p].w.strb),
            .axi_mst_w_last (aplic_msi_axi_req[p].w.last),

            .axi_mst_b_ready(aplic_msi_axi_req[p].b_ready),
            .axi_mst_r_ready(aplic_msi_axi_req[p].r_ready),

            .axi_slv_b_valid(aplic_msi_axi_rsp[p].b_valid),
            .axi_slv_b_id   (aplic_msi_axi_rsp[p].b.id),
            .axi_slv_b_resp (aplic_msi_axi_rsp[p].b.resp),

            .axi_slv_r_valid(aplic_msi_axi_rsp[p].r_valid),
            .axi_slv_r_id   (aplic_msi_axi_rsp[p].r.id),
            .axi_slv_r_data (aplic_msi_axi_rsp[p].r.data),
            .axi_slv_r_resp (aplic_msi_axi_rsp[p].r.resp),
            .axi_slv_r_last (aplic_msi_axi_rsp[p].r.last),

            .axi_slv_aw_ready(aplic_msi_axi_rsp[p].aw_ready),
            .axi_slv_ar_ready(aplic_msi_axi_rsp[p].ar_ready),
            .axi_slv_w_ready (aplic_msi_axi_rsp[p].w_ready),
            `RV_TESTER_TRANSACTIONS_AXI_SW_SOURCE_PORTS(2, p, 2)
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
            .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.AXI_MST.ID, p)),
            `RV_TESTER_TRANSACTIONS_AXI_SW_MST_SOURCE_PARAMS(0)
        ) axi_sw_mst (
            .clk(dut_clk[AXI_CLK_IDX]),
            .sys_reset(sys_reset[AXI_CLK_IDX]),
            .reset_n(~dut_reset[AXI_CLK_IDX]),
            .axi_mst_ar_valid(axi_req_mst[p].ar_valid),
            .axi_mst_ar_id   (axi_req_mst[p].ar.id),
            .axi_mst_ar_addr (axi_req_mst[p].ar.addr),
            .axi_mst_ar_len  (axi_req_mst[p].ar.len),
            .axi_mst_ar_size (axi_req_mst[p].ar.size),
            .axi_mst_ar_lock (axi_req_mst[p].ar.lock),
            .axi_mst_ar_burst(axi_req_mst[p].ar.burst),
            .axi_mst_ar_cache (axi_req_mst[p].ar.cache), 
            .axi_mst_ar_prot  (axi_req_mst[p].ar.prot),
            .axi_mst_ar_qos   (axi_req_mst[p].ar.qos),
            .axi_mst_ar_region(axi_req_mst[p].ar.region),
            .axi_mst_ar_user  (axi_req_mst[p].ar.user),

            .axi_mst_aw_valid(axi_req_mst[p].aw_valid),
            .axi_mst_aw_id   (axi_req_mst[p].aw.id),
            .axi_mst_aw_addr (axi_req_mst[p].aw.addr),
            .axi_mst_aw_len  (axi_req_mst[p].aw.len),
            .axi_mst_aw_size (axi_req_mst[p].aw.size),
            .axi_mst_aw_burst(axi_req_mst[p].aw.burst),
            .axi_mst_aw_lock (axi_req_mst[p].aw.lock),
            .axi_mst_aw_cache(axi_req_mst[p].aw.cache), 
            .axi_mst_aw_prot (axi_req_mst[p].aw.prot), 
            .axi_mst_aw_qos  (axi_req_mst[p].aw.qos),  
            .axi_mst_aw_region(axi_req_mst[p].aw.region),
            .axi_mst_aw_atop (axi_req_mst[p].aw.atop),
            .axi_mst_aw_user (axi_req_mst[p].aw.user),

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
            `RV_TESTER_TRANSACTIONS_AXI_SW_MST_SOURCE_PORTS(2, p, 0)
        );
    end


    for (genvar p = 0; p < topology.TOP.PLATFORM.APLIC_MMR_AXI_MST.TOTAL; p++) begin : aplic_mmr_axi_sw_msts
        axi_sw_mst #(
            .ADDR_WIDTH(topology.TOP.PLATFORM.APLIC_MMR_AXI_MST.ADDR_WIDTH),
            .DATA_WIDTH(topology.TOP.PLATFORM.APLIC_MMR_AXI_MST.DATA_WIDTH),
            .ID_WIDTH(topology.TOP.PLATFORM.APLIC_MMR_AXI_MST.ID_WIDTH  ),
            .STRB_WIDTH(topology.TOP.PLATFORM.APLIC_MMR_AXI_MST.STRB_WIDTH),
            .AR_Q_MAX(topology.TOP.PLATFORM.APLIC_MMR_AXI_MST.AR_Q_MAX),
            .AW_Q_MAX(topology.TOP.PLATFORM.APLIC_MMR_AXI_MST.AW_Q_MAX),
            .W_Q_MAX(topology.TOP.PLATFORM.APLIC_MMR_AXI_MST.W_Q_MAX),
            .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.APLIC_MMR_AXI_MST.ID, p)),
            `RV_TESTER_TRANSACTIONS_AXI_SW_MST_SOURCE_PARAMS(1)
        ) aplic_mmr_sw_mst (
            .clk(dut_clk[AXI_CLK_IDX]),
            .sys_reset(sys_reset[AXI_CLK_IDX]),
            .reset_n(~dut_reset[AXI_CLK_IDX]),
            .axi_mst_ar_valid(aplic_mmr_axi_req_mst[p].ar_valid),
            .axi_mst_ar_id   (aplic_mmr_axi_req_mst[p].ar.id),
            .axi_mst_ar_addr (aplic_mmr_axi_req_mst[p].ar.addr),
            .axi_mst_ar_len  (aplic_mmr_axi_req_mst[p].ar.len),
            .axi_mst_ar_size (aplic_mmr_axi_req_mst[p].ar.size),
            .axi_mst_ar_lock (aplic_mmr_axi_req_mst[p].ar.lock),
            .axi_mst_ar_burst(aplic_mmr_axi_req_mst[p].ar.burst),

            .axi_mst_aw_valid(aplic_mmr_axi_req_mst[p].aw_valid),
            .axi_mst_aw_id   (aplic_mmr_axi_req_mst[p].aw.id),
            .axi_mst_aw_addr (aplic_mmr_axi_req_mst[p].aw.addr),
            .axi_mst_aw_len  (aplic_mmr_axi_req_mst[p].aw.len),
            .axi_mst_aw_size (aplic_mmr_axi_req_mst[p].aw.size),
            .axi_mst_aw_burst(aplic_mmr_axi_req_mst[p].aw.burst),
            .axi_mst_aw_lock (aplic_mmr_axi_req_mst[p].aw.lock),
            .axi_mst_aw_atop (aplic_mmr_axi_req_mst[p].aw.atop),

            .axi_mst_w_valid(aplic_mmr_axi_req_mst[p].w_valid),
            .axi_mst_w_data (aplic_mmr_axi_req_mst[p].w.data),
            .axi_mst_w_strb (aplic_mmr_axi_req_mst[p].w.strb),
            .axi_mst_w_last (aplic_mmr_axi_req_mst[p].w.last),

            .axi_mst_b_ready(aplic_mmr_axi_req_mst[p].b_ready),
            .axi_mst_r_ready(aplic_mmr_axi_req_mst[p].r_ready),

            .axi_slv_b_valid(aplic_mmr_axi_rsp_mst[p].b_valid),
            .axi_slv_b_id   (aplic_mmr_axi_rsp_mst[p].b.id),
            .axi_slv_b_resp (aplic_mmr_axi_rsp_mst[p].b.resp),

            .axi_slv_r_valid(aplic_mmr_axi_rsp_mst[p].r_valid),
            .axi_slv_r_id   (aplic_mmr_axi_rsp_mst[p].r.id),
            .axi_slv_r_data (aplic_mmr_axi_rsp_mst[p].r.data),
            .axi_slv_r_resp (aplic_mmr_axi_rsp_mst[p].r.resp),
            .axi_slv_r_last (aplic_mmr_axi_rsp_mst[p].r.last),

            .axi_slv_aw_ready(aplic_mmr_axi_rsp_mst[p].aw_ready),
            .axi_slv_ar_ready(aplic_mmr_axi_rsp_mst[p].ar_ready),
            .axi_slv_w_ready (aplic_mmr_axi_rsp_mst[p].w_ready),
            `RV_TESTER_TRANSACTIONS_AXI_SW_MST_SOURCE_PORTS(2, p, 1)
        );
    end
//SMC Connections

    for (genvar p = 0; p < topology.TOP.PLATFORM.SMC_AXI_MST.TOTAL; p++) begin : smc_axi_sw_msts
        axi_sw_mst #(
            .ADDR_WIDTH(topology.TOP.PLATFORM.SMC_AXI_MST.ADDR_WIDTH),
            .DATA_WIDTH(topology.TOP.PLATFORM.SMC_AXI_MST.DATA_WIDTH),
            .ID_WIDTH(topology.TOP.PLATFORM.SMC_AXI_MST.ID_WIDTH  ),
            .STRB_WIDTH(topology.TOP.PLATFORM.SMC_AXI_MST.STRB_WIDTH),
            .AR_Q_MAX(topology.TOP.PLATFORM.SMC_AXI_MST.AR_Q_MAX),
            .AW_Q_MAX(topology.TOP.PLATFORM.SMC_AXI_MST.AW_Q_MAX),
            .W_Q_MAX(topology.TOP.PLATFORM.SMC_AXI_MST.W_Q_MAX),
            .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.SMC_AXI_MST.ID, p)),
            `RV_TESTER_TRANSACTIONS_AXI_SW_MST_SOURCE_PARAMS(2)
        ) smc_sw_mst (
            .clk(dut_clk[SOC_CLK_IDX]),
            .sys_reset(sys_reset[SOC_CLK_IDX]),
            .reset_n(~dut_reset[SOC_CLK_IDX]),
            .axi_mst_ar_valid(smc_axi_req_mst[p].ar_valid),
            .axi_mst_ar_id   (smc_axi_req_mst[p].ar.id),
            .axi_mst_ar_addr (smc_axi_req_mst[p].ar.addr),
            .axi_mst_ar_len  (smc_axi_req_mst[p].ar.len),
            .axi_mst_ar_size (smc_axi_req_mst[p].ar.size),
            .axi_mst_ar_lock (smc_axi_req_mst[p].ar.lock),
            .axi_mst_ar_burst(smc_axi_req_mst[p].ar.burst),

            .axi_mst_aw_valid(smc_axi_req_mst[p].aw_valid),
            .axi_mst_aw_id   (smc_axi_req_mst[p].aw.id),
            .axi_mst_aw_addr (smc_axi_req_mst[p].aw.addr),
            .axi_mst_aw_len  (smc_axi_req_mst[p].aw.len),
            .axi_mst_aw_size (smc_axi_req_mst[p].aw.size),
            .axi_mst_aw_burst(smc_axi_req_mst[p].aw.burst),
            .axi_mst_aw_lock (smc_axi_req_mst[p].aw.lock),
            .axi_mst_aw_atop (smc_axi_req_mst[p].aw.atop),

            .axi_mst_w_valid(smc_axi_req_mst[p].w_valid),
            .axi_mst_w_data (smc_axi_req_mst[p].w.data),
            .axi_mst_w_strb (smc_axi_req_mst[p].w.strb),
            .axi_mst_w_last (smc_axi_req_mst[p].w.last),

            .axi_mst_b_ready(smc_axi_req_mst[p].b_ready),
            .axi_mst_r_ready(smc_axi_req_mst[p].r_ready),

            .axi_slv_b_valid(smc_axi_rsp_mst[p].b_valid),
            .axi_slv_b_id   (smc_axi_rsp_mst[p].b.id),
            .axi_slv_b_resp (smc_axi_rsp_mst[p].b.resp),

            .axi_slv_r_valid(smc_axi_rsp_mst[p].r_valid),
            .axi_slv_r_id   (smc_axi_rsp_mst[p].r.id),
            .axi_slv_r_data (smc_axi_rsp_mst[p].r.data),
            .axi_slv_r_resp (smc_axi_rsp_mst[p].r.resp),
            .axi_slv_r_last (smc_axi_rsp_mst[p].r.last),

            .axi_slv_aw_ready(smc_axi_rsp_mst[p].aw_ready),
            .axi_slv_ar_ready(smc_axi_rsp_mst[p].ar_ready),
            .axi_slv_w_ready (smc_axi_rsp_mst[p].w_ready),
            `RV_TESTER_TRANSACTIONS_AXI_SW_MST_SOURCE_PORTS(3, p, 2)
        );
    end

    //PLL 
    // Connections

    for (genvar p = 0; p < topology.TOP.PLATFORM.PLL_AXI_MST.TOTAL; p++) begin : pll_axi_sw_msts
        axi_sw_mst #(
            .ADDR_WIDTH(topology.TOP.PLATFORM.PLL_AXI_MST.ADDR_WIDTH),
            .DATA_WIDTH(topology.TOP.PLATFORM.PLL_AXI_MST.DATA_WIDTH),
            .ID_WIDTH(topology.TOP.PLATFORM.PLL_AXI_MST.ID_WIDTH  ),
            .STRB_WIDTH(topology.TOP.PLATFORM.PLL_AXI_MST.STRB_WIDTH),
            .AR_Q_MAX(topology.TOP.PLATFORM.PLL_AXI_MST.AR_Q_MAX),
            .AW_Q_MAX(topology.TOP.PLATFORM.PLL_AXI_MST.AW_Q_MAX),
            .W_Q_MAX(topology.TOP.PLATFORM.PLL_AXI_MST.W_Q_MAX),
            .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.PLL_AXI_MST.ID, p)),
            `RV_TESTER_TRANSACTIONS_AXI_SW_MST_SOURCE_PARAMS(3)
        ) pll_sw_mst (
            .clk(dut_clk[SOC_CLK_IDX]),
            .sys_reset(sys_reset[SOC_CLK_IDX]),
            .reset_n(~dut_reset[SOC_CLK_IDX]),
            .axi_mst_ar_valid(pll_axi_req_mst[p].ar_valid),
            .axi_mst_ar_id   (pll_axi_req_mst[p].ar.id),
            .axi_mst_ar_addr (pll_axi_req_mst[p].ar.addr),
            .axi_mst_ar_len  (pll_axi_req_mst[p].ar.len),
            .axi_mst_ar_size (pll_axi_req_mst[p].ar.size),
            .axi_mst_ar_lock (pll_axi_req_mst[p].ar.lock),
            .axi_mst_ar_burst(pll_axi_req_mst[p].ar.burst),

            .axi_mst_aw_valid(pll_axi_req_mst[p].aw_valid),
            .axi_mst_aw_id   (pll_axi_req_mst[p].aw.id),
            .axi_mst_aw_addr (pll_axi_req_mst[p].aw.addr),
            .axi_mst_aw_len  (pll_axi_req_mst[p].aw.len),
            .axi_mst_aw_size (pll_axi_req_mst[p].aw.size),
            .axi_mst_aw_burst(pll_axi_req_mst[p].aw.burst),
            .axi_mst_aw_lock (pll_axi_req_mst[p].aw.lock),
            .axi_mst_aw_atop (pll_axi_req_mst[p].aw.atop),

            .axi_mst_w_valid(pll_axi_req_mst[p].w_valid),
            .axi_mst_w_data (pll_axi_req_mst[p].w.data),
            .axi_mst_w_strb (pll_axi_req_mst[p].w.strb),
            .axi_mst_w_last (pll_axi_req_mst[p].w.last),

            .axi_mst_b_ready(pll_axi_req_mst[p].b_ready),
            .axi_mst_r_ready(pll_axi_req_mst[p].r_ready),

            .axi_slv_b_valid(pll_axi_rsp_mst[p].b_valid),
            .axi_slv_b_id   (pll_axi_rsp_mst[p].b.id),
            .axi_slv_b_resp (pll_axi_rsp_mst[p].b.resp),

            .axi_slv_r_valid(pll_axi_rsp_mst[p].r_valid),
            .axi_slv_r_id   (pll_axi_rsp_mst[p].r.id),
            .axi_slv_r_data (pll_axi_rsp_mst[p].r.data),
            .axi_slv_r_resp (pll_axi_rsp_mst[p].r.resp),
            .axi_slv_r_last (pll_axi_rsp_mst[p].r.last),

            .axi_slv_aw_ready(pll_axi_rsp_mst[p].aw_ready),
            .axi_slv_ar_ready(pll_axi_rsp_mst[p].ar_ready),
            .axi_slv_w_ready (pll_axi_rsp_mst[p].w_ready),
            `RV_TESTER_TRANSACTIONS_AXI_SW_MST_SOURCE_PORTS(3, p, 3)
        );
    end


    //PM_NW Connections

    for (genvar p = 0; p < topology.TOP.PLATFORM.PM_NW_AXI_MST.TOTAL; p++) begin : pm_nw_axi_sw_msts
        axi_sw_mst #(
            .ADDR_WIDTH(topology.TOP.PLATFORM.PM_NW_AXI_MST.ADDR_WIDTH),
            .DATA_WIDTH(topology.TOP.PLATFORM.PM_NW_AXI_MST.DATA_WIDTH),
            .ID_WIDTH(topology.TOP.PLATFORM.PM_NW_AXI_MST.ID_WIDTH  ),
            .STRB_WIDTH(topology.TOP.PLATFORM.PM_NW_AXI_MST.STRB_WIDTH),
            .AR_Q_MAX(topology.TOP.PLATFORM.PM_NW_AXI_MST.AR_Q_MAX),
            .AW_Q_MAX(topology.TOP.PLATFORM.PM_NW_AXI_MST.AW_Q_MAX),
            .W_Q_MAX(topology.TOP.PLATFORM.PM_NW_AXI_MST.W_Q_MAX),
            .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.PM_NW_AXI_MST.ID, p)),
            `RV_TESTER_TRANSACTIONS_AXI_SW_MST_SOURCE_PARAMS(4)
        ) pm_nw_sw_mst (
            .clk(dut_clk[SOC_CLK_IDX]),
            .sys_reset(sys_reset[SOC_CLK_IDX]),
            .reset_n(~dut_reset[SOC_CLK_IDX]),
            .axi_mst_ar_valid(pm_nw_axi_req_mst[p].ar_valid),
            .axi_mst_ar_id   (pm_nw_axi_req_mst[p].ar.id),
            .axi_mst_ar_addr (pm_nw_axi_req_mst[p].ar.addr),
            .axi_mst_ar_len  (pm_nw_axi_req_mst[p].ar.len),
            .axi_mst_ar_size (pm_nw_axi_req_mst[p].ar.size),
            .axi_mst_ar_lock (pm_nw_axi_req_mst[p].ar.lock),
            .axi_mst_ar_burst(pm_nw_axi_req_mst[p].ar.burst),

            .axi_mst_aw_valid(pm_nw_axi_req_mst[p].aw_valid),
            .axi_mst_aw_id   (pm_nw_axi_req_mst[p].aw.id),
            .axi_mst_aw_addr (pm_nw_axi_req_mst[p].aw.addr),
            .axi_mst_aw_len  (pm_nw_axi_req_mst[p].aw.len),
            .axi_mst_aw_size (pm_nw_axi_req_mst[p].aw.size),
            .axi_mst_aw_burst(pm_nw_axi_req_mst[p].aw.burst),
            .axi_mst_aw_lock (pm_nw_axi_req_mst[p].aw.lock),
            .axi_mst_aw_atop (pm_nw_axi_req_mst[p].aw.atop),

            .axi_mst_w_valid(pm_nw_axi_req_mst[p].w_valid),
            .axi_mst_w_data (pm_nw_axi_req_mst[p].w.data),
            .axi_mst_w_strb (pm_nw_axi_req_mst[p].w.strb),
            .axi_mst_w_last (pm_nw_axi_req_mst[p].w.last),

            .axi_mst_b_ready(pm_nw_axi_req_mst[p].b_ready),
            .axi_mst_r_ready(pm_nw_axi_req_mst[p].r_ready),

            .axi_slv_b_valid(pm_nw_axi_rsp_mst[p].b_valid),
            .axi_slv_b_id   (pm_nw_axi_rsp_mst[p].b.id),
            .axi_slv_b_resp (pm_nw_axi_rsp_mst[p].b.resp),

            .axi_slv_r_valid(pm_nw_axi_rsp_mst[p].r_valid),
            .axi_slv_r_id   (pm_nw_axi_rsp_mst[p].r.id),
            .axi_slv_r_data (pm_nw_axi_rsp_mst[p].r.data),
            .axi_slv_r_resp (pm_nw_axi_rsp_mst[p].r.resp),
            .axi_slv_r_last (pm_nw_axi_rsp_mst[p].r.last),

            .axi_slv_aw_ready(pm_nw_axi_rsp_mst[p].aw_ready),
            .axi_slv_ar_ready(pm_nw_axi_rsp_mst[p].ar_ready),
            .axi_slv_w_ready (pm_nw_axi_rsp_mst[p].w_ready),
            `RV_TESTER_TRANSACTIONS_AXI_SW_MST_SOURCE_PORTS(3, p, 4)
        );
    end

    //new memory - LLC

    typedef logic [AxiIdWidthMstRv-1:0] id_mst_rv;
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

    mst_req_rv axi_req_llc [NoOfMasters-1:0];
    mst_resp_rv axi_rsp_llc [NoOfMasters-1:0];



    function automatic void rv_tester_set_address_map(int unsigned i, longint unsigned start_addr, longint unsigned end_addr, int unsigned device);
        localparam int unsigned AW = topology.TOP.PLATFORM.AXI.ADDR_WIDTH;
        addr_map[i] = '{
            idx       : device         ,
            start_addr: AW'(start_addr),
            end_addr  : AW'(end_addr  )
        };

        addr_map_idx1[i] = '{
            idx       : 1              ,
            start_addr: AW'(start_addr),
            end_addr  : AW'(end_addr  )
        };

    endfunction

    assign addr_map_final = (bypass_cache == 0)?addr_map:addr_map_idx1;

    export "DPI-C" function rv_tester_set_address_map;

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
        .mst_resp_t             ( mst_resp_rv ),
	.rule_t			( xbar_rule_t ),
	.NoAddrRules		( NoAddrRules ),
	.NumMastersMem		( NoOfMasters )
    ) rv_tester_mem(
        .clk                    ( dut_clk[AXI_CLK_IDX] ),
        .rst_n                  ( ~dut_reset[AXI_CLK_IDX] ),
        .axi_req_up             ( axi_req ),
        .axi_resp_up            ( axi_rsp ),
        .axi_req_mst_up         ( axi_req_llc ),
        .axi_resp_mst_up        ( axi_rsp_llc ),
	.addr_map		( addr_map_final ),
        .bypass_mem		( bypass_mem ),
	.flush_cache		( quiesced ),
	.flush_complete		( flush_complete ),
	.bist_status_done	()
    );

    always @(posedge clk[TB_CLK_IDX]) begin
        assert(assertion_test_cycle == '0 || clocks != LU'(assertion_test_cycle)) else $error("assertion test");
    end

endmodule
