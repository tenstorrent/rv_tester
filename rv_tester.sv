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

    typedef longint unsigned LU;

    localparam int unsigned NoAddrRules = 25;

    typedef struct packed {
        int unsigned idx;
        logic [topology.TOP.PLATFORM.AXI.ADDR_WIDTH-1:0] start_addr;
        logic [topology.TOP.PLATFORM.AXI.ADDR_WIDTH-1:0] end_addr;
      } xbar_rule_t;

    bit flag_force_ref_clk;
    bit force_ref_clk_d1;
    bit force_ref_clk_d2;
    logic rv_tester_enable_llc = 0;
    logic rv_tester_mem_bypass_cache = 1;
    logic rv_tester_reset = '1;
    logic dm_model_bypass = 1;
    int unsigned rv_tester_mem_delay = 20;

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

    logic fastest_clk;
    localparam bit [6:0][NCLKS-1:0][31:0] ALL_PROFILE_FREQS = {
        PROFILE1_CLOCK_FREQ_MHZ,
        PROFILE2_CLOCK_FREQ_MHZ,
        PROFILE3_CLOCK_FREQ_MHZ,
        PROFILE4_CLOCK_FREQ_MHZ,
        PROFILE5_CLOCK_FREQ_MHZ,
        PROFILE6_CLOCK_FREQ_MHZ,
        CLOCK_FREQ_MHZ
    };

    function automatic int unsigned find_overall_max_freq(bit [6:0][NCLKS-1:0][31:0] all_freqs);
        int unsigned max_val = 0;
        foreach (all_freqs[i, j]) begin
            if (all_freqs[i][j] > max_val) begin
                max_val = all_freqs[i][j];
            end
        end
        return max_val;
    endfunction

    localparam int max_clock_freq_overall = find_overall_max_freq(ALL_PROFILE_FREQS);

    `ifdef PALLADIUM_CAKE1X
        IXCclkgen #(max_clock_freq_overall) uclk (fastest_clk);
    `else
        assign fastest_clk = '0;
    `endif

    if (EXTERNAL_CLOCK) begin
        assign clk[TB_CLK_IDX] = clk_ext[TB_CLK_IDX];
        for (genvar c = 1; c < NCLKS; c++) begin
            assign clk[c] = force_ref_clk_d2 ? clk_ext[REF_CLK_IDX] : clk_ext[c];
        end
    end else begin
        for (genvar c = 0; c < NCLKS; c++) begin
            `ifdef CLK_MUX_UNSUPPORTED
            rv_tester_clkgen #(.CLOCK_FREQ_MHZ(CLOCK_FREQ_MHZ[c]), .FASTEST_FREQ_MHZ(max_clock_freq_overall)) clkgen(.clk(clk[c]), .fastest_clk(fastest_clk));

            `else
            if(c == REF_CLK_IDX) begin
                rv_tester_clkgen #(.CLOCK_FREQ_MHZ(CLOCK_FREQ_MHZ[REF_CLK_IDX]), .FASTEST_FREQ_MHZ(max_clock_freq_overall)) clkgen(.clk(clk[REF_CLK_IDX]), .fastest_clk(fastest_clk));
            end else if (c == TB_CLK_IDX ) begin
                rv_tester_clkgen #(.CLOCK_FREQ_MHZ(CLOCK_FREQ_MHZ[TB_CLK_IDX]), .FASTEST_FREQ_MHZ(max_clock_freq_overall)) clkgen(.clk(clk[TB_CLK_IDX]), .fastest_clk(fastest_clk));
            end else begin 
                rv_tester_clkgen #(.CLOCK_FREQ_MHZ(CLOCK_FREQ_MHZ[c]), .FASTEST_FREQ_MHZ(max_clock_freq_overall)) clkgen(.clk(def_clk[c]), .fastest_clk(fastest_clk));
                rv_tester_clkgen #(.CLOCK_FREQ_MHZ(PROFILE1_CLOCK_FREQ_MHZ[c]), .FASTEST_FREQ_MHZ(max_clock_freq_overall)) profile1_clkgen(.clk(profile1_clk[c]), .fastest_clk(fastest_clk));
                rv_tester_clkgen #(.CLOCK_FREQ_MHZ(PROFILE2_CLOCK_FREQ_MHZ[c]), .FASTEST_FREQ_MHZ(max_clock_freq_overall)) profile2_clkgen(.clk(profile2_clk[c]), .fastest_clk(fastest_clk));
                rv_tester_clkgen #(.CLOCK_FREQ_MHZ(PROFILE3_CLOCK_FREQ_MHZ[c]), .FASTEST_FREQ_MHZ(max_clock_freq_overall)) profile3_clkgen(.clk(profile3_clk[c]), .fastest_clk(fastest_clk));
                rv_tester_clkgen #(.CLOCK_FREQ_MHZ(PROFILE4_CLOCK_FREQ_MHZ[c]), .FASTEST_FREQ_MHZ(max_clock_freq_overall)) profile4_clkgen(.clk(profile4_clk[c]), .fastest_clk(fastest_clk));
                rv_tester_clkgen #(.CLOCK_FREQ_MHZ(PROFILE5_CLOCK_FREQ_MHZ[c]), .FASTEST_FREQ_MHZ(max_clock_freq_overall)) profile5_clkgen(.clk(profile5_clk[c]), .fastest_clk(fastest_clk));
                rv_tester_clkgen #(.CLOCK_FREQ_MHZ(PROFILE6_CLOCK_FREQ_MHZ[c]), .FASTEST_FREQ_MHZ(max_clock_freq_overall)) profile6_clkgen(.clk(profile6_clk[c]), .fastest_clk(fastest_clk));


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
            end
            `endif
         end
     end

    import "DPI-C" function void rv_tester_streaming_dpi_init();
    import "DPI-C" function void rv_tester_streaming_dpi_shutdown();
    import "DPI-C" function int rv_tester_parse_flags(); // dummy return value so that this gets called immediately. need this to happen before any other DPIs are called.
    import "DPI-C" function void rv_tester_set_seed();
    import "DPI-C" context function void rv_tester_cvm_error_handler();
    import "DPI-C" context function void rv_tester_parse_memmap(int unsigned no_addr_rules, int num_ways, int num_sets, int num_blocks, int addr_width, int data_width);
    import "DPI-C" context function void rv_tester_build_registry();
    import "DPI-C" context function void rv_tester_no_dm_build_registry();
    import "DPI-C" function byte unsigned rv_tester_shutdown_registry();
    import "DPI-C" context function void rv_tester_dm_build_registry();
    import "DPI-C" function byte unsigned rv_tester_dm_shutdown_registry();
    import "DPI-C" context function bit rv_tester_flush_callbacks();
    import "DPI-C" function bit pwrmgmt_get_pwrmgmt_en_from_plusargs(string mode);
    import "DPI-C" function longint unsigned eot_get_addr();
    import "DPI-C" context function bit rv_tester_perf_calc(int init, int reset_done, int term, LU clocks);

    localparam int unsigned MaxInFlightReadReq = topology.TOP.PLATFORM.MAX_IN_FLIGHT_READ_REQ;
    localparam int unsigned MaxBeatsPerBurst = topology.TOP.PLATFORM.MAX_BEATS_PER_BURST;
    localparam int unsigned AxiIdWidthMstRv    = topology.TOP.PLATFORM.AXI.ID_WIDTH + $clog2(topology.TOP.PLATFORM.AXI.TOTAL) + 1;
    localparam bit RespDelayModule = 0;

    logic flush_complete;

    xbar_rule_t [NoAddrRules-1:0] addr_map, addr_map_final, addr_map_idx1;
    bit perf = 0;
    /* verilator lint_off MULTIDRIVEN */
    logic sys_reset [NCLKS-1:0];
    /* verilator lint_on MULTIDRIVEN */
    logic sys_reset_any;
    logic shifted_dut_reset_req, shifted_dut_reset_req_d1;
    logic dut_reset_req_d1;
    logic fml_shutdowned;
    logic init_pulse;
    logic warm_reset_pulse;
    int unsigned warm_reset_clocks = 0;
    int unsigned soc_clocks = 0;
    logic pwrmgmt_force_ref_clk;
    logic terminate_ntrace_test;
    logic terminate_dst_trace_seq;
    logic terminate_cla_seq;
    logic reset_window;
    logic cold_reset;
    logic warm_reset;
    //LU clocks = 0;
    LU axi_clocks;
    bit perf_init_done = 1'b0;       // init done
    bit perf_reset_done = 1'b0;       // reset done
    bit perf_retn  = 1'b0;
    int perf_count  = 0;
    int perf_period = 0;
    bit cb_poll = '0;
    bit dyn_clk_switch = '0;
    bit cb_success = '1;
    logic call_finish;
    int num_reruns = -1;
    int dm_build_count = 0;

    logic warm_reset_en = 0;
    logic warm_reset_req_d1;
    logic warm_reset_now = 0;
    logic dmi_warm_reset;

    int num_resets = -1;
    int num_builds = -1;
    int target_num_resets = 0;

    bit trace_en = 0;
    bit cla_en = 0;

    bit [NHARTS-1:0] poke_event_out;
    bit poke_event_in;
    bit jtag_en = 0;
    bit overlay_mmr_en = 0;
    logic trace_quiesced;
    logic jtag_quiesced;


    logic terminate_1T = '0;
    logic terminated_1T = '0;
    logic rerun_now;
    /* verilator lint_off UNOPTFLAT */
    rv_tester_pkg::terminate_t rv_tester_error_terminate;
    rv_tester_pkg::terminate_t sysmod_terminate;
    /* verilator lint_off UNOPTFLAT */
    rv_tester_pkg::terminate_t cosim_terminate [NHARTS-1:0];
    logic cosim_terminate_any;
    longint unsigned instructions = 0;

    int quiesce_counter = 0;
    int trace_counter = 5000;
    int quiesce_timeout = 500;
    int flush_counter = 0;
    int flush_timeout = 25000;
    bit print_terminate_message = '1;
    bit dm_registery_terminate_message = '1;
    int ndmreset_ack_delay = 0;

    int debug_enable = 0;
    bit dmi_driver_dbg_enable;
    int hart_enable_mask = 0;
    int num_harts = 0;
    bit ntrace_stop_on_wrap = 0;
    bit cluster_axi_sp_perf = 0;
    int rand_dmi_driver_dly = 0;
    int sdtrig_multitrigger = 0;
    int num_dm_randpc = 0;
    int num_dm_randload = 0;
    int num_dm_randstore = 0;
    int trigger_config = 0;
    bit priority_singlestep = 0;
    bit disable_haltpoll = 0;
    bit disable_abscmdpoll = 0;
    bit disable_triggerpoll = 0;
    int dm_single_step_count = 0;
    int dmi_poll_counter = 0;
    int dmi_poll_timeout = 50000;
    int disable_dmi_responce_ready = 0;
    logic dmi_poll_timeout_terminate;
    logic [31:0] dmi_commands_in_queue;
    bit sdtrig_display = 0;
    bit nonexistent_hart = 0;
    int abscmd_hang_counter = 0;
    bit warm_reset_directed_en = 0;

    int trace_timeout = 50000;
    int freq_switch_ncycles = 7000;
    int clk_profile = 0;

    int assertion_test_cycle = 0;

    logic streaming_dpi_shutdowned = 0;

    parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.ID, 0);

    localparam int AxiLLC_SetAssociativity = 32'd4;
    localparam int AxiLLC_NumLines = 32'd128;
    localparam int blocks_in_cacheline = 512/topology.TOP.PLATFORM.AXI.DATA_WIDTH;
    localparam int AxiLLC_NumBlocks = blocks_in_cacheline < 2 ? 2 : blocks_in_cacheline;
    if (topology.TOP.PLATFORM.AXI.DATA_WIDTH > 512 || (512 % topology.TOP.PLATFORM.AXI.DATA_WIDTH) != 0) begin
        $error("axi data width %0d larger than 64 byts or not divisible into 64 bytes", topology.TOP.PLATFORM.AXI.DATA_WIDTH);
    end


    bit gen_clocks = '0;
    bit gen_timestamp = '0;
    logic [63:0] current_time;
    int unsigned cvm_verbosity, cvm_debug_verbosity, curr_cvm_verbosity;
    LU cvm_debug_cycle_on = '0;
    LU cvm_debug_cycle_off = '0;
    logic dut_terminate_any;
    logic ntrace_terminate;
    
    // Termination condition variables for better readability
    logic sysmod_cosim_dmi_terminate;
    logic core_terminate_conditions;

    reg [49:0] dut_reset_req_shift_reg;

    logic rerun_now_ff;

    always @(posedge dut_clk[AXI_CLK_IDX] or posedge cold_reset) begin
        /* verilator lint_off SYNCASYNCNET */
        if (cold_reset)
            dut_reset_req_shift_reg <= {50{1'b0}};
        else
            dut_reset_req_shift_reg <= {dut_reset_req_shift_reg[48:0], dut_reset_req};
            /* verilator lint_on SYNCASYNCNET */
    end

    assign shifted_dut_reset_req = dut_reset_req_shift_reg[49];

    assign dut_terminate_any = dut_terminate;

    // Extract common termination conditions for better readability
    assign sysmod_cosim_dmi_terminate = (sysmod_terminate.terminate || cosim_terminate_any || dmi_poll_timeout_terminate) && !sys_reset_any;
    assign core_terminate_conditions = dut_terminate_any || rv_tester_error_terminate.terminate || sysmod_cosim_dmi_terminate;
    
    assign ntrace_terminate    = (terminate_ntrace_test & ntrace_stop_on_wrap) || !ntrace_stop_on_wrap;
    assign terminate           = (core_terminate_conditions || quiesce_counter > 0) && !rv_tester_reset && !warm_reset && ntrace_terminate;
    assign terminate_now       = (terminate_1T && (quiesced || ((quiesce_counter >= quiesce_timeout) && !warm_reset)) && (flush_complete || flush_counter >= flush_timeout) && ((dmi_commands_in_queue <= 'h1) | (dmi_poll_counter > 'h1)) && (!trace_en || trace_quiesced || terminate_dst_trace_seq) && (!cla_en || terminate_cla_seq )  && (!jtag_en || jtag_quiesced )) || dut_terminate_any || warm_reset_now;

    assign rerun_now           = terminated && !terminated_1T && ((num_reruns > 0) || (warm_reset_en && (num_resets <= target_num_resets)) || shifted_dut_reset_req);

    assign dmi_driver_dbg_enable = ((debug_enable == 'h1) || (debug_enable == 'h3));

  `ifndef CLK_MUX_UNSUPPORTED
    always @(posedge dut_clk[TB_CLK_IDX])begin
      if (rv_tester_reset & !rerun_now_ff)begin
            clock_mode <= clk_profile[2:0];
      end
      /* verilator lint_off WIDTH */
      else if(dyn_clk_switch & (clocks >10) &  ((clocks % freq_switch_ncycles) == 0)) begin
        //dynamically select clk from available profiles
        //this logic will generate the select pins of the mux ,which will switch between clks
        if(clock_mode == 3'b110)
            clock_mode <= 'b1;
        else
            clock_mode <= clock_mode + 1'b1;
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
    always @(posedge dut_clk[TB_CLK_IDX]) begin

        rv_tester_reset <= rerun_now;
        rerun_now_ff <= rerun_now;
        clocks          <= clocks + 1;


        `ifndef NO_TIMESTAMP
            current_time <= $time;
        `else
            current_time <= '0;
        `endif

        quiesce_counter <= quiesce_counter + int'(terminate);
        flush_counter   <= flush_counter + int'(quiesced);

        for (int i=0; i<NHARTS; i++) begin
          instructions  <= instructions + LU'(pmci[i][INSTRUCTIONS]);
        end

        if (rv_tester_reset) begin
            quiesce_counter <= '0;
            flush_counter   <= '0;
            instructions    <= '0;
            dmi_poll_timeout_terminate <= '0;
            if (num_resets < 0) begin
                clocks <= '0;
            end
        end

        num_resets <= num_resets + int'(warm_reset_now);
        if (warm_reset_en && (num_resets < 0)) begin
            num_resets          <= 0;
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

    always @(posedge dut_clk[TB_CLK_IDX]) begin
        if (!rv_tester_reset && cvm_debug_cycle_off > 0) begin
            if (curr_cvm_verbosity != cvm_debug_verbosity && clocks >= cvm_debug_cycle_on && clocks <= cvm_debug_cycle_off) begin
                cvm_logger::set_verbosity(cvm_debug_verbosity);
                curr_cvm_verbosity <= cvm_debug_verbosity;
            end else if (curr_cvm_verbosity != cvm_verbosity && clocks >= cvm_debug_cycle_off) begin
                cvm_logger::set_verbosity(cvm_verbosity);
                curr_cvm_verbosity <= cvm_verbosity;
            end
        end
    end


    always @(posedge dut_clk[TB_CLK_IDX]) begin
        if(rerun_now) begin
            $display("<%0d> [RVTESTER]: rerunning test %0d time(s)", clocks, num_reruns);
        end
    end

    /*
    * Group all zebu zDPI DPIs here
    * These are run on a separate thread than the slower zebi3
    */
    always @(posedge dut_clk[TB_CLK_IDX]) begin
        if (rv_tester_reset) begin
            streaming_dpi_shutdowned <= 0;

            // Used for offine DPI
            rv_tester_streaming_dpi_init();
        end
        if (terminated && !streaming_dpi_shutdowned) begin
            // Used for zebu offline DPI to shutdown the registry
            rv_tester_streaming_dpi_shutdown();
            streaming_dpi_shutdowned <= 1;
        end
    end
    /*
    * 2-way DPI call used to periodically calculate the model performance
    *   - perf_period: controls how often performance measurement it made.
    *       perf_period >  0 : measurement occurs periodically AND at the end of the test (termination)
    *       perf_period <= 0 : measurement only occurs at the end of the test (termination)
    */
    always @(posedge dut_clk[TB_CLK_IDX]) begin
        if (perf_init_done == 1'b0) begin
            if (rv_tester_reset) begin
                perf_count <= perf_period - 32'h1;
                perf_retn  <= rv_tester_perf_calc(1,0,0, clocks);
                perf_init_done <= 1'b1;
                perf_reset_done <= 1'b0;
            end
        end
        else begin
            if (rv_tester_reset == 1'b0) begin
                if (perf_reset_done == 1'b0) begin
                   perf_retn  <= rv_tester_perf_calc(0,1,0, clocks);
                   perf_reset_done <= 1'b1;
                   perf_count <= perf_period - 32'h1;
                end
            end
            else
            if (perf_reset_done == 1'b1) begin
                perf_init_done <= 1'b0;
            end

            if (terminate) begin
                perf_retn  <= rv_tester_perf_calc(0,0,1, clocks);
                perf_init_done <= 1'b0;
            end
            else begin
                if (((perf_count == '0) & (perf_period > '0))) begin
                    perf_count <= perf_period - 32'h1;
                    perf_retn  <= rv_tester_perf_calc(0,0,0, clocks);
                end
                else begin
                   perf_count <= perf_count - 32'h1;
                end
            end
        end
    end

    /*
    * Group all zebu zemi3 DPIs here
    * These are run on a separate thread than the faster zDPI, so make sure
    * these are only run at rv_tester_reset, when no other zDPIs should be
    * called.
    */
    always @(posedge dut_clk[TB_CLK_IDX]) begin

        automatic int _, _cvm_verbosity, _gen_clocks_verbosity, _gen_timestamp_verbosity;

        if (rv_tester_reset) begin

            $display("[RVTESTER]: new test");
            _ = rv_tester_parse_flags();
            if (num_resets < 0)
                rv_tester_set_seed();
            rv_tester_cvm_error_handler();

            if(num_builds < 0) begin
               $display("[RVTESTER]: constructing Full registry");
               rv_tester_build_registry();
               num_builds <= 0;
            end
            else begin
               $display("[RVTESTER]: constructing registry without DM Model");
               rv_tester_no_dm_build_registry();
            end
            rv_tester_parse_memmap(NoAddrRules, AxiLLC_SetAssociativity, AxiLLC_NumLines, AxiLLC_NumBlocks, topology.TOP.PLATFORM.AXI.ADDR_WIDTH + 1 /* cache has one more bit */, topology.TOP.PLATFORM.AXI.DATA_WIDTH);

            /* verilator lint_off BLKSEQ */
            // zebu bug doesn't allow nested function calls, so create intermediate variables
            // Using nested function calls in cvm as Palladium doesn't support strings
            _cvm_verbosity              = cvm_logger::get_verbosity_from_plusargs("cvm_verbosity");
            _gen_clocks_verbosity       = cvm_logger::get_verbosity_from_plusargs("gen_clocks_verbosity");
            _gen_timestamp_verbosity    = cvm_logger::get_verbosity_from_plusargs("gen_timestamp_verbosity");
            warm_reset_en               <= pwrmgmt_get_pwrmgmt_en_from_plusargs("warm_reset");
            rv_tester_error_terminate.terminate = '0;
            perf_period                 = cvm_plusargs::get_int("perf_period");
            /* verilator lint_on BLKSEQ */

      
            eot_addr                        <= eot_get_addr();
            eot_status                      <= 1;
            eot_syscall                     <= 0;
            perf                            <= cvm_plusargs::get_bool("perf") != '0;
            flag_force_ref_clk              <= cvm_plusargs::get_bool("force_ref_clk") != '0;
            rand_dmi_driver_dly             <= cvm_plusargs::get_int("rand_dmi_driver_dly");
            num_dm_randpc                   <= cvm_plusargs::get_int("iss_select_num_randpc");
            num_dm_randload                 <= cvm_plusargs::get_int("iss_select_num_randload");
            num_dm_randstore                <= cvm_plusargs::get_int("iss_select_num_randstore");
            trigger_config                  <= cvm_plusargs::get_int("trigger_config");
            priority_singlestep             <= cvm_plusargs::get_bool("priority_singlestep") != '0;
            disable_haltpoll                <= cvm_plusargs::get_bool("disable_haltpoll") != '0;
            disable_abscmdpoll              <= cvm_plusargs::get_bool("disable_abscmdpoll") != '0;
            disable_triggerpoll             <= cvm_plusargs::get_bool("disable_triggerpoll") != '0;
            nonexistent_hart                <= cvm_plusargs::get_bool("nonexistent_hart") != '0;
            sdtrig_multitrigger             <= cvm_plusargs::get_int("sdtrig_multitrigger");
            dm_single_step_count            <= cvm_plusargs::get_int("dm_single_step_count");
            cb_poll                         <= cvm_plusargs::get_bool("cb_async") == '0;
            quiesce_timeout                 <= cvm_plusargs::get_int("quiesce_timeout");
            dmi_poll_timeout                <= cvm_plusargs::get_int("dmi_poll_timeout");
            disable_dmi_responce_ready      <= cvm_plusargs::get_int("disable_dmi_responce_ready");
            ndmreset_ack_delay              <= cvm_plusargs::get_int("ndmreset_ack_delay");
            trace_timeout                   <= cvm_plusargs::get_int("trace_timeout");
            flush_timeout                   <= cvm_plusargs::get_int("flush_timeout");
            freq_switch_ncycles             <= cvm_plusargs::get_int("freq_switch_ncycles");
            clk_profile                     <= cvm_plusargs::get_int("clk_profile");
            dyn_clk_switch                  <= cvm_plusargs::get_bool("dyn_clk_switch") != '0;
            call_finish                     <= cvm_plusargs::get_bool("terminate_call_finish") != '0;
            gen_clocks                      <= _cvm_verbosity >= _gen_clocks_verbosity;
            gen_timestamp                   <= _cvm_verbosity >= _gen_timestamp_verbosity;
            rv_tester_enable_llc            <= cvm_plusargs::get_bool("rv_tester_enable_llc") != '0;
            rv_tester_mem_bypass_cache      <= cvm_plusargs::get_bool("rv_tester_mem_bypass_cache") != '0;
            rv_tester_mem_delay             <= cvm_plusargs::get_int("rv_tester_mem_delay");
            assertion_test_cycle            <= cvm_plusargs::get_int("assertion_test_cycle");
            sdtrig_display                  <= cvm_plusargs::get_bool("sdtrig_display") != '0;
            dm_model_bypass                 <= cvm_plusargs::get_bool("dm_model_check_bypass") != '0;
            debug_enable                    <= cvm_plusargs::get_int("debug_enable");
            trace_en                        <= cvm_plusargs::get_bool("trace_en") != '0;
            cla_en                          <= (cvm_plusargs::get_bool("cla_rand_nmi_trig_en") != '0 ||  cvm_plusargs::get_bool("cla_nmi") != '0);
            overlay_mmr_en                  <= cvm_plusargs::get_bool("overlay_mmr_en") != '0;
            jtag_en                         <= cvm_plusargs::get_bool("jtag_en") != '0;
            rand_dmi_driver_dly             <= cvm_plusargs::get_int("rand_dmi_driver_dly");
            hart_enable_mask                <= cvm_plusargs::get_int("hart_enable_mask");
            perf_count                      <= '0;
            ntrace_stop_on_wrap             <= cvm_plusargs::get_bool("ntrace_stop_on_wrap_seq_en") != '0;
            num_harts                       <= cvm_plusargs::get_int("num_harts");
            cluster_axi_sp_perf             <= cvm_plusargs::get_bool("cluster_axi_sp_perf") != '0;
            abscmd_hang_counter             <= cvm_plusargs::get_int("abscmd_hang_counter");
            warm_reset_directed_en          <= cvm_plusargs::get_bool("warm_reset_directed_en") != '0;

            cvm_verbosity        <= _cvm_verbosity;
            curr_cvm_verbosity   <= _cvm_verbosity;
            cvm_debug_verbosity  <= cvm_logger::get_verbosity_from_plusargs("cvm_debug_verbosity");
            cvm_debug_cycle_on   <= cvm_plusargs::get_ulongint("cvm_debug_cycle_on");
            cvm_debug_cycle_off  <= cvm_plusargs::get_ulongint("cvm_debug_cycle_off");

        end
        clock_mode           <= clk_profile[2:0];
        num_reruns      <= num_reruns - int'(rerun_now);
        if (num_reruns < 0) begin
            num_reruns  <= cvm_plusargs::get_int("num_reruns");
        end

        if ((warm_reset_en  || warm_reset_directed_en) && (num_resets < 0)) begin
            target_num_resets   <= cvm_rand::get("warm_reset_count");
        end
        
        // Deassert warm_reset_en when any termination condition is met
        if (core_terminate_conditions) begin
            warm_reset_en <= 0;
        end


    end

    /*
    * rv_tester_shutdown_registry could be called at the same time as
    * transactions (axi, cosim, etc). So it's put in a separate always block
    * from the rv_tester_reset group of zemi3 DPI. Otherwise zebu will make
    * rv_tester_shutdown_registry a zemi3 DPI and we'll have thread safety
    * issues with coinciding zDPIs from transactions.
    */
    always @(posedge dut_clk[TB_CLK_IDX]) begin

        automatic logic shutdowned = '0;
        automatic logic dm_shutdowned = '0;
        `ifndef SVA_S_EVENTUALLY_UNSUPPORTED
        fml_shutdowned = 1'b0;
        `endif
        if (rv_tester_reset) begin
            print_terminate_message <= '1;
        end
        if(cold_reset) begin //
          dm_registery_terminate_message <= '1;
        end
        if (terminate_now && !terminated) begin

            if (print_terminate_message) begin
                if (warm_reset_now) begin
                    $display("<%0d> [RVTESTER]: starting warm reset", clocks);
                end else if (dut_terminate) begin
                    $display("<%0d> [RVTESTER]: exiting due to dut_terminate", clocks);
                end else if (quiesced) begin
                    $display("<%0d> [RVTESTER]: exiting gracefully", clocks);
                end else if (quiesce_counter == 0) begin
                    $display("<%0d> [RVTESTER]: exiting immediately because +quiesce_counter=0", clocks);
                end else begin
                    $display("\n<%0d> [RVTESTER]: Error: Waiting to quiesce for more than %0d cycles", clocks, quiesce_timeout);
                end

            end

            shutdowned = rv_tester_shutdown_registry() != '0;
            `ifndef SVA_S_EVENTUALLY_UNSUPPORTED
            fml_shutdowned = shutdowned;
            `endif
            if(num_resets > target_num_resets)begin
            dm_shutdowned = rv_tester_dm_shutdown_registry() != '0;
            end
            if (!shutdowned) begin
                if (print_terminate_message) begin
                    $display("<%0d> [RVTESTER]: Could not shutdown, trying again until timeout", clocks);
                end
            end

            if (shutdowned && num_reruns == '0 && !warm_reset_req && !shifted_dut_reset_req) begin
                $display("INFO_PASS:{\"clocks\": %0d}", clocks);
                $display("INFO_PASS_METRIC:{\"axi_clocks\": %0d}", axi_clocks);
                $display("INFO_PASS_METRIC:{\"instruction_count\": %0d}", instructions);
                $display("INFO_PASS_REGR_METRIC:{\"name\": \"instructions\", \"value\":%0d, \"type\": \"i\", \"action\": \"sum\"}", instructions);

                if (call_finish) begin
                    $finish();
                end
            end
            print_terminate_message <= '0;
        end

        terminate_1T <= terminate;
        terminated <= !rv_tester_reset && (terminated || (terminate_now && shutdowned));
        terminated_1T <= terminated;

    end

    // sys_reset per clock domain
    logic sys_reset_pending [NCLKS-1:0];
    logic terminate_sync    [NCLKS-1:0];
    for (genvar c = 0; c < NCLKS; c++) begin
        if (c != TB_CLK_IDX) begin
            rv_tester_cdc_pulse cdc_pulse (
                .clk_a (dut_clk[TB_CLK_IDX]),
                .clk_b (dut_clk[c]),
                .pulse_a (rv_tester_reset),
                .pulse_b (sys_reset[c]),
                .pulse_pending_or_asserted_a (sys_reset_pending[c])
            );
 
            rv_tester_sync3 terminate_sync3 (
                .clk (dut_clk[c]),
                .d   (terminate),
                .q   (terminate_sync[c])
            );

        end else begin
            always_ff @(posedge dut_clk[TB_CLK_IDX]) begin
                sys_reset[c] <= rv_tester_reset;
            end
            assign sys_reset_pending[c] = sys_reset[c];
            assign terminate_sync   [c] = terminate;
        end
    end

    // Clock counts
    always_ff @(posedge dut_clk[AXI_CLK_IDX]) begin
        if (dut_reset[AXI_CLK_IDX]) begin
            axi_clocks <= 0;
        end else begin
            axi_clocks <= axi_clocks + 1;
        end
    end

    always_comb begin
        sys_reset_any = '0;
        for (int c = 0; c < NCLKS; c++) begin
            sys_reset_any |= sys_reset_pending[c];
        end
    end

    // soc clock counter
    always @(posedge dut_clk[SOC_CLK_IDX]) begin
        soc_clocks <= soc_clocks + 1;
    end

    // dut_reset = force_ref_clk delayed by 2 clocks
    always @(posedge dut_clk[REF_CLK_IDX]) begin
        force_ref_clk_d1 <= force_ref_clk;
        force_ref_clk_d2 <= force_ref_clk_d1;
    end

    // We also assert reset at the end of the test to quiesce the DPIs.
    logic reset_pullup;
    logic cold_reset_pullup = 0;
    logic warm_reset_pullup = 0;
    assign reset_pullup = rv_tester_reset || terminate_now || terminated;

    assign reset[COLD_RESET_IDX] = cold_reset || cold_reset_pullup;
    assign reset[WARM_RESET_IDX] = warm_reset;

    assign dut_reset[TB_CLK_IDX] =  reset[COLD_RESET_IDX] || reset[WARM_RESET_IDX];
    assign dut_reset[CORE_CLK_IDX] =&core_no_fetch || reset[WARM_RESET_IDX] || warm_reset_pullup;
    assign dut_reset[AXI_CLK_IDX] = reset_window || reset[WARM_RESET_IDX] || warm_reset_pullup;
    assign dut_reset[SOC_CLK_IDX] = reset[COLD_RESET_IDX];
    assign dut_reset[REF_CLK_IDX] = reset_window;

    always@(posedge dut_clk[TB_CLK_IDX]) begin
        if (reset_pullup)
            if (!warm_reset_req && !(dut_reset_req || shifted_dut_reset_req))
                cold_reset_pullup <= '1;
            else
                warm_reset_pullup <= '1;
        if (cold_reset) begin
            cold_reset_pullup <= '0;
            warm_reset_pullup <= '0;
        end
        if (warm_reset)
            warm_reset_pullup <= '0;
    end

    // posedge on dut_reset_req should trigger a warm reset
    always @(posedge dut_clk[AXI_CLK_IDX]) begin
        dut_reset_req_d1 <= dut_reset_req;
        shifted_dut_reset_req_d1 <= shifted_dut_reset_req;
        if (warm_reset_now) begin
            /* verilator lint_off BLKSEQ */
            warm_reset_clocks = soc_clocks;
            /* verilator lint_on BLKSEQ */
        end

        warm_reset_req_d1 <= warm_reset_req;
        warm_reset_now <= (warm_reset_req & ~warm_reset_req_d1) || (shifted_dut_reset_req & ~shifted_dut_reset_req_d1);
    end
    assign dut_reset_req_active = shifted_dut_reset_req && warm_reset_pullup;

    //ndmreset ack delay logic
    LU ndmreset_ack_clocks;
    logic ndmreset_ack_clocks_latched = 1'b0;
    logic warm_reset_latched;
    logic warm_reset_deasserted = 1'b0;


    always @(posedge dut_clk[TB_CLK_IDX]) begin
        warm_reset_latched <= warm_reset;  // Track previous state of warm_reset
        if(cold_reset === 1'b0)begin
        if ((warm_reset_latched == 1'b1) && (warm_reset == 1'b0))  begin
            warm_reset_deasserted <= 1'b1;
        end
        if (!dut_reset_req && warm_reset_deasserted)  begin
            ndmreset_ack_clocks_latched <= 1'b0;
            ndmreset_ack <= 1'b0;
            warm_reset_deasserted <= 1'b0;
        end else if (dut_reset_req && !ndmreset_ack_clocks_latched) begin
            ndmreset_ack_clocks <= clocks;
            ndmreset_ack_clocks_latched <= 1'b1;
        end
        /* verilator lint_off WIDTHEXPAND */
        else if (ndmreset_ack_clocks_latched && (clocks >= (ndmreset_ack_clocks + ndmreset_ack_delay))) begin
        /* verilator lint_on WIDTHEXPAND */
            ndmreset_ack <= 1'b1;
        end
        end
        else begin
            ndmreset_ack_clocks_latched <= 1'b0;
            ndmreset_ack <= 1'b0;
        end
    end


`ifdef NEGEDGE_UNSUPPORTED
    always@(posedge dut_clk[TB_CLK_IDX]) begin
`else
    always@(negedge dut_clk[TB_CLK_IDX]) begin
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

    function void rv_tester_set_clock_mode (input int unsigned new_clock_mode);
        if (new_clock_mode <= 3'b110) begin
            clock_mode <= new_clock_mode[2:0];
            $display("<%0d> rv_tester_set_clock_mode: clock_mode changed to %0d", clocks, new_clock_mode);
        end else begin
            $display("<%0d> rv_tester_set_clock_mode: Invalid clock_mode %0d, valid range is 0-6", clocks, new_clock_mode);
        end
    endfunction
    export "DPI-C" function rv_tester_set_clock_mode;

    `RV_TESTER_TRANSACTIONS_DOMAIN(1, dut_clk[CORE_CLK_IDX]);
    `RV_TESTER_TRANSACTIONS_DOMAIN(2, dut_clk[AXI_CLK_IDX]);
    `RV_TESTER_TRANSACTIONS_DOMAIN(3, dut_clk[SOC_CLK_IDX]);

    rv_tester_pkg::dm_write_t  trickbox_dmi_write;

    // Writeback logic
    logic [1:0] mcm_writeback_valid[7:0]; // since it can be present for 8 cosim instances
    assign mcm_writeback_valid[0] = writeback_cl_valid;
    for(genvar i = 1; i < 8; i++) begin
        assign mcm_writeback_valid[i] = 2'b00;
    end

    // Dfetch logic
    logic [1:0] mcm_dfetch_valid[7:0]; // since it can be present for 8 cosim instances
    assign mcm_dfetch_valid[0] = dfetch_cl_valid;
    for(genvar i = 1; i < 8; i++) begin
        assign mcm_dfetch_valid[i] = 2'b00;
    end
    

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
        .dut_reset_req,
        .dut_core_reset(dut_reset[CORE_CLK_IDX]),
        .trace_quiesced(trace_quiesced),
        .bootstrap,
        .dmi_write(trickbox_dmi_write),
        .event_triggers(event_triggers),
        .interrupt,
        .terminate(sysmod_terminate),
        `RV_TESTER_TRANSACTIONS_SYSMOD_SOURCE_PORTS(2, 0, 0)
    );

`ifndef DMI_TB_WRITES_UNSUPPORTED
    logic [7:0] misc_signals;
    logic dmi_status;

    dmi_driver i_dmi_driver(
        .clk(dut_clk[AXI_CLK_IDX]),
        .core_clk(dut_clk[CORE_CLK_IDX]),
        .reset_n(~(reset[WARM_RESET_IDX] || reset[COLD_RESET_IDX]) || reset_hold[DEBUG_HOLD_IDX]),
        .dmi_warm_reset(~reset[WARM_RESET_IDX]),
        .dmi_driver_dbg_enable,
        .rand_dmi_driver_dly,
        .hart_enable_mask,
        .dm_single_step_count,
        .sdtrig_multitrigger,
        .num_dm_randpc,
        .num_dm_randload,
        .num_dm_randstore,
        .trigger_config,
        .priority_singlestep,
        .disable_haltpoll,
        .disable_abscmdpoll,
        .disable_triggerpoll,
        .terminate(sysmod_terminate.terminate),
        .num_harts,
        .sdtrig_display,
        .nonexistent_hart,
        .abscmd_hang_counter,

        .dmi_req_ready,
        .dmi_resp_valid,
        .dmi_resp,
        .disable_dmi_responce_ready,

        .dmi_req_valid,
        .dmi_req,
        .dmi_resp_ready,
        .dmi_status,
        .dmi_commands_in_queue,
        .misc_signals,
        .DM_DebugReq_Valids(DM_DebugReq_Valids),

        .trickbox_dmi_write(trickbox_dmi_write),
        .rvfi(rvfi)
    );

    dm_model #(
        .NUM(0),
        `TOPOLOGY_CFG,
        `RV_TESTER_TRANSACTIONS_DM_MODEL_SOURCE_PARAMS(0)
    ) i_dm_model(
        .clk(dut_clk[AXI_CLK_IDX]),

        //.reset(sys_reset[TB_CLK_IDX]),
        .reset(~(~reset[WARM_RESET_IDX] || reset_hold[DEBUG_HOLD_IDX])),
        .dmi_req(dmi_tx_req),
        .dmi_req_valid(dmi_tx_req_vld),
        .dmi_resp_valid(dmi_tx_resp_vld),
        .dmi_resp(dmi_tx_resp),
        .terminate,
        .dm_mem_tx_vld,
        .dm_mem_tx_we,
        .dm_mem_tx_addr,
        .dm_mem_tx_rd_data,
        .dm_mem_tx_wr_data,
        .dm_mem_tx_wr_data_be,
        .dmi_status,
        .dmi_commands_in_queue,
        .dmi_warm_reset(~reset[WARM_RESET_IDX]),
        .misc_signals,
        .DM_DebugReq_Valids(DM_DebugReq_Valids),
        `RV_TESTER_TRANSACTIONS_DM_MODEL_SOURCE_PORTS(2,0,0)
    );

    always @(posedge dut_clk[AXI_CLK_IDX]) begin
        if (sys_reset[TB_CLK_IDX] | !dmi_status)
            dmi_poll_counter <= 0;
        else if (dmi_status) begin
            dmi_poll_counter <= dmi_poll_counter + 1;

            if (dmi_poll_counter > dmi_poll_timeout) begin
                $display("\n<%0d> [RVTESTER]: Error: Debug poll timeout limit reached.", clocks);
                dmi_poll_timeout_terminate <= 1;
            end
            else if ((dmi_poll_counter >= 'h1) && terminate && !dm_model_bypass) begin
               $display("<%0d> [RVTESTER]: Debug poll stopped as terminate condition detected", clocks);
            end
        end
    end

`endif



    // coverage
    arch_sample arch_sample ();

    assign poke_event_in = (poke_event_out != '0) ? 1'b1 : 1'b0;

    logic [NHARTS-1:0] boot_done;
`ifndef NO_COSIM
    `ifndef CACHE_MODEL_EN
    // Dummy variables to prevent X - props #FIXME : remove later when making cache model default
    logic [51:0] devict_addr;
    logic [51:0] writeback_addr [1:0];
    logic [51:0] dfetch_addr [1:0];
    assign devict_addr = '0;
    for(genvar i = 0; i < 2; i++) begin : no_cache_model_init
        assign writeback_addr[i] = '0;
        assign dfetch_addr[i] = '0;
    end
    `endif 
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
          .NCSRI(MAX_NCSRI),
          .NoAddrRules(NoAddrRules),
          .rule_t(xbar_rule_t),
          `TOPOLOGY_CFG,
          `RV_TESTER_TRANSACTIONS_COSIM_SOURCE_PARAMS(0)
      ) cosim (
          .tb_clk(dut_clk[TB_CLK_IDX]),
          .clk(dut_clk[CORE_CLK_IDX]),
          .reset(sys_reset[TB_CLK_IDX] | reset_window),
          .dut_core_reset(dut_reset[CORE_CLK_IDX]),
          .dut_reset(dut_reset[TB_CLK_IDX]),
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
          .nmi_pend(nmi_pend[c]),
          .interrupt_pend(interrupt_pend[c]),
          .mtime(mtime),
          .imsic_msi(imsic_msi[c]),
          .debug_mode(debug_mode[c]),
          .haltreq(DM_DebugReq_Valids[c]),
          .terminate(cosim_terminate[c]),
          .eot_addr(eot_addr),
          .addr_map(addr_map),
          .poke_event_out(poke_event_out[c]),
          .poke_event_in(poke_event_in),
          .disable_checks(disable_checks),
          .boot_done(boot_done[c]),
          `ifdef CACHE_MODEL_EN
          .devict_cl_valid(devict_cl_valid[c]),
          .devict_cl_addr(devict_cl_addr[c]),
          .flush_cl_valid(flush_cl_valid[c]),
          .flush_cl_addr(flush_cl_addr[c]),
          .writeback_cl_valid(mcm_writeback_valid[c]),
          .writeback_cl_addr(writeback_cl_addr),
          .dfetch_cl_valid(mcm_dfetch_valid[c]),
          .dfetch_cl_addr(dfetch_cl_addr),
          `else
          .devict_cl_valid(),
          .devict_cl_addr(devict_addr),
          .flush_cl_valid('0),
          .flush_cl_addr('0),
          .writeback_cl_valid('0),
          .writeback_cl_addr(writeback_addr),
          .dfetch_cl_valid('0),
          .dfetch_cl_addr(dfetch_addr),
          // Tying to 0 to prevent X-propagation
          `endif

          
          `RV_TESTER_TRANSACTIONS_COSIM_SOURCE_PORTS(1, c, 0)
      );
    end
    
    assign boot_done_all = &boot_done;
`endif

    always @(posedge dut_clk[TB_CLK_IDX]) begin
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
                .clk(dut_clk),
                .reset(dut_reset),
                .sys_reset(sys_reset),
                .reset_count(num_resets),
                .target_reset_count(target_num_resets),
                .cold_reset(cold_reset),
                .warm_reset(warm_reset),
                .warm_reset_en(warm_reset_en),
                .warm_reset_req(warm_reset_req),
                .reset_hold(reset_hold),
                .force_ref_clk(pwrmgmt_force_ref_clk),
                .core_no_fetch(&core_no_fetch),
                .tj_shutdown(tj_shutdown),
                .tj_max(tj_max),
                .pll_dfs_done(pll_dfs_done),
                .pll_shutdown_done(pll_shutdown_done),
                .terminate(terminate),
                `RV_TESTER_TRANSACTIONS_PWRMGMT_SOURCE_PORTS(3,0,0)
            );
            assign reset_window = pwrmgmt_force_ref_clk || init_pulse || warm_reset_pulse;
            assign force_ref_clk = flag_force_ref_clk ? reset_window : '0;
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
            .clk(dut_clk[AXI_CLK_IDX]),
            .sys_reset(sys_reset[AXI_CLK_IDX]),
            .reset(dut_reset[AXI_CLK_IDX]),
            .clocks,
            .boot_done(boot_done[c]),
            .nmi(nmi[c].nmi),
            `RV_TESTER_TRANSACTIONS_INTERRUPTS_SOURCE_PORTS(2,c,0)
        );
    end
    jtag_driver #(
          .NUM(0),
          `TOPOLOGY_CFG,
          `RV_TESTER_TRANSACTIONS_JTAG_DRIVER_SOURCE_PARAMS(0)
        )jtag_driver
        (
            .clk(dut_clk[REF_CLK_IDX]),
            .reset(reset[COLD_RESET_IDX]),
            .warm_reset(reset[WARM_RESET_IDX]),
            .dut_clk(dut_clk[TB_CLK_IDX]),
            .dut_reset(dut_reset[TB_CLK_IDX]),
            .no_fetch(core_no_fetch[0]),
            .jtag_driver_en(jtag_en||dmi_driver_dbg_enable),
            .jtag_quiesced(jtag_quiesced),
            .jtag_req,
            .jtag_tck_trst,
            .jtag_resp,
          `RV_TESTER_TRANSACTIONS_JTAG_DRIVER_SOURCE_PORTS(1,0,0)
        );


    overlay_driver #(
          .NUM(0),
          `TOPOLOGY_CFG,
          `RV_TESTER_TRANSACTIONS_OVERLAY_DRIVER_SOURCE_PARAMS(0)
        )overlay_driver
        (
            .clk(dut_clk[AXI_CLK_IDX]),
            .reset(dut_reset[AXI_CLK_IDX]),
            .dut_clk(dut_clk[AXI_CLK_IDX]),
            .dut_reset(dut_reset[AXI_CLK_IDX]),
            .no_fetch(core_no_fetch[0]),
            .cluster_axi_sp_perf(cluster_axi_sp_perf),
          `RV_TESTER_TRANSACTIONS_OVERLAY_DRIVER_SOURCE_PORTS(2,0,0)
        );

    snoop_gen #(
            .NUM(0),
            `TOPOLOGY_CFG,
            `RV_TESTER_TRANSACTIONS_SNOOP_GEN_SOURCE_PARAMS(0)
    ) snoop_gen (
            .clk(dut_clk[AXI_CLK_IDX]),
            .sys_reset(sys_reset[AXI_CLK_IDX]),
            .reset(dut_reset[AXI_CLK_IDX]),
            .clocks,
            .core_no_fetch(core_no_fetch),
            `RV_TESTER_TRANSACTIONS_SNOOP_GEN_SOURCE_PORTS(2,0,0)
    );

    trace #(
       .NUM(0),
       `TOPOLOGY_CFG,
       `RV_TESTER_TRANSACTIONS_TRACE_SOURCE_PARAMS(0)
    ) trace (
        .tb_clk(clk[TB_CLK_IDX]),
        .tb_reset(sys_reset[TB_CLK_IDX]),
        .clk(dut_clk[AXI_CLK_IDX]),
        .reset(dut_reset[AXI_CLK_IDX]),
        .core_no_fetch(core_no_fetch),
        .terminate_ntrace_test(terminate_ntrace_test),
        .terminate_dst_trace_seq(terminate_dst_trace_seq),
        `RV_TESTER_TRANSACTIONS_TRACE_SOURCE_PORTS(2,0,0)
    );

    cla #(
       .NUM(0),
       `TOPOLOGY_CFG,
       `RV_TESTER_TRANSACTIONS_CLA_SOURCE_PARAMS(0)
    ) cla (
        .tb_clk(clk[TB_CLK_IDX]),
        .tb_reset(sys_reset[TB_CLK_IDX]),
        .clk(dut_clk[AXI_CLK_IDX]),
        .reset(dut_reset[AXI_CLK_IDX]),
        .core_no_fetch(core_no_fetch),
        .terminate_from_rv_tester(terminate),
        .terminate_cla_seq(terminate_cla_seq),
        `RV_TESTER_TRANSACTIONS_CLA_SOURCE_PORTS(2,0,0)
    );
    for (genvar c = 0; c < NHARTS; c++) begin: triggers
        triggers #(
            .NUM(c),
            `TOPOLOGY_CFG,
            `RV_TESTER_TRANSACTIONS_TRIGGERS_SOURCE_PARAMS(0)
        ) triggers (
            .tb_clk(dut_clk[TB_CLK_IDX]),
            .tb_reset(sys_reset[TB_CLK_IDX]),
            .clk(dut_clk[AXI_CLK_IDX]),
            .reset(dut_reset[AXI_CLK_IDX]),
            .event_trigger_vec(event_triggers),
            `RV_TESTER_TRANSACTIONS_TRIGGERS_SOURCE_PORTS(2,c,0)
        );
    end

    aclint_checker #(
        .NUM(0),
        `TOPOLOGY_CFG,
        `RV_TESTER_TRANSACTIONS_ACLINT_CHECKER_SOURCE_PARAMS(0)
    ) i_aclint_checker(
        .tb_clk(dut_clk[TB_CLK_IDX]),
        .cl_clk(dut_clk[CORE_CLK_IDX]),
        .rf_clk(dut_clk[REF_CLK_IDX]),
        .reset(sys_reset[TB_CLK_IDX]),
        .cold_resetn(~cold_reset),
        .warm_reset(AcWarmReset),
        .dut_reset(dut_reset[REF_CLK_IDX]),
        .terminated(terminated),
        .terminate_now(terminate_now),
        .AcChk_pll_interrupts_in(AcChk_pll_interrupts_in),
        .AcCrSynci(AcCrSynci),
        .AcReqPkti(AcReqPkti),
        .AcReqPktRfClki(AcReqPktRfClki),
        .rvfi(rvfi),
        .AcMtimei(AcMtimei),
        .AcMtipi(AcMtipi),
        .SmcMtipi(SmcMtipi),
        `RV_TESTER_TRANSACTIONS_ACLINT_CHECKER_SOURCE_PORTS(1,0,0)
    );

    always_comb begin
        cosim_terminate_any = '0;
        for (int i=0; i<NHARTS; i++) begin
            cosim_terminate_any |= cosim_terminate[i].terminate;
        end
    end

    for (genvar p = 0; p < NHARTS; p++) begin: pmu_inst
      if (p == 0) begin : pmu_c0
        pmu #(
            .NUM(p),
            .NRET(NRETS[p]),
            .SC_PMCI_ENABLED(p == 0),
            `TOPOLOGY_CFG,
            `RV_TESTER_TRANSACTIONS_PMU_CORE_SOURCE_PARAMS(0),
            `RV_TESTER_TRANSACTIONS_PMU_SC_SOURCE_PARAMS(0)
        ) pmu (
            .clk(dut_clk[CORE_CLK_IDX]),
            .sys_reset(sys_reset[CORE_CLK_IDX]),
            .reset(dut_reset[CORE_CLK_IDX]),
            .clocks,
            .pmci(pmci[p]),
            .hpmi(hpmi[p]),
            .sc_pmci(sc_pmci),
            .rvfi(rvfi[NRETS_CUMSUM[p] +: NRETS[p]]),
            .terminate(terminate_sync[CORE_CLK_IDX]),
            `RV_TESTER_TRANSACTIONS_PMU_CORE_SOURCE_PORTS(1, p, 0),
            `RV_TESTER_TRANSACTIONS_PMU_SC_SOURCE_PORTS(1, p, 0)
        );
      end else begin : pmu_cX
        pmu #(
            .NUM(p),
            .NRET(NRETS[p]),
            .SC_PMCI_ENABLED(p == 0),
            `TOPOLOGY_CFG,
            `RV_TESTER_TRANSACTIONS_PMU_CORE_SOURCE_PARAMS(0)
        ) pmu (
            .clk(dut_clk[CORE_CLK_IDX]),
            .sys_reset(sys_reset[CORE_CLK_IDX]),
            .reset(dut_reset[CORE_CLK_IDX]),
            .clocks,
            .pmci(pmci[p]),
            .hpmi(hpmi[p]),
            .sc_pmci(),
            .rvfi(rvfi[NRETS_CUMSUM[p] +: NRETS[p]]),
            .terminate(terminate_sync[CORE_CLK_IDX]),
            `RV_TESTER_TRANSACTIONS_PMU_CORE_SOURCE_PORTS(1, p, 0)
        );
      end
    end

    assign tx_dom_1.logger_cycle_0s[0][0].valid = gen_clocks;
    assign tx_dom_1.logger_cycle_0s[0][0].data.location = location;
    assign tx_dom_1.logger_cycle_0s[0][0].data.clock = clocks;

    assign tx_dom_1.logger_timestamp_0s[0][0].valid = gen_timestamp && (current_time != 0);
    assign tx_dom_1.logger_timestamp_0s[0][0].data.location = location;
    assign tx_dom_1.logger_timestamp_0s[0][0].data.timeval = current_time;

    localparam NoOfMasters = ( topology.TOP.PLATFORM.AXI.TOTAL < 2 ) ? 2 : topology.TOP.PLATFORM.AXI.TOTAL ;
    for (genvar p = 0; p < NoOfMasters; p++) begin : axi_sw_slvs
        localparam string tag = $sformatf("coh_slv%0d", p);
        axi_sw #(
            .ADDR_WIDTH(topology.TOP.PLATFORM.AXI.ADDR_WIDTH),
            .DATA_WIDTH(topology.TOP.PLATFORM.AXI.DATA_WIDTH),
            .ID_WIDTH(AxiIdWidthMstRv),
            .STRB_WIDTH(topology.TOP.PLATFORM.AXI.STRB_WIDTH),
            .R_Q_MAX(topology.TOP.PLATFORM.AXI.R_Q_MAX),
            .B_Q_MAX(topology.TOP.PLATFORM.AXI.B_Q_MAX),
            .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.AXI.ID, p)),
            .tag(tag),
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
        localparam string tag = $sformatf("non_coh_slv%0d", p);
        axi_sw #(
            .ADDR_WIDTH(topology.TOP.PLATFORM.NCIO_AXI.ADDR_WIDTH),
            .DATA_WIDTH(topology.TOP.PLATFORM.NCIO_AXI.DATA_WIDTH),
            .ID_WIDTH(topology.TOP.PLATFORM.NCIO_AXI.ID_WIDTH),
            .STRB_WIDTH(topology.TOP.PLATFORM.NCIO_AXI.STRB_WIDTH),
            .R_Q_MAX(topology.TOP.PLATFORM.AXI.R_Q_MAX),
            .B_Q_MAX(topology.TOP.PLATFORM.AXI.B_Q_MAX),
            .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.NCIO_AXI.ID, p)),
            .tag(tag),
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

    for (genvar p = 0; p < topology.TOP.PLATFORM.AXI_MST.TOTAL; p++) begin : axi_sw_msts
        localparam string tag = $sformatf("non_coh_mst%0d", p);
        axi_sw_mst #(
            .ADDR_WIDTH(topology.TOP.PLATFORM.AXI_MST.ADDR_WIDTH),
            .DATA_WIDTH(topology.TOP.PLATFORM.AXI_MST.DATA_WIDTH),
            .ID_WIDTH(topology.TOP.PLATFORM.AXI_MST.ID_WIDTH  ),
            .STRB_WIDTH(topology.TOP.PLATFORM.AXI_MST.STRB_WIDTH),
            .USER_WIDTH(topology.TOP.PLATFORM.AXI_MST.USER_WIDTH),
            .AR_Q_MAX(topology.TOP.PLATFORM.AXI_MST.AR_Q_MAX),
            .AW_Q_MAX(topology.TOP.PLATFORM.AXI_MST.AW_Q_MAX),
            .W_Q_MAX(topology.TOP.PLATFORM.AXI_MST.W_Q_MAX),
            .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.AXI_MST.ID, p)),
            .tag(tag),
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

    for (genvar p = 0; p < topology.TOP.PLATFORM.SMC_AXI_MST.TOTAL; p++) begin : smc_axi_sw_msts
        localparam string tag = $sformatf("smc_mst%0d", p);
        axi_sw_mst #(
            .ADDR_WIDTH(topology.TOP.PLATFORM.SMC_AXI_MST.ADDR_WIDTH),
            .DATA_WIDTH(topology.TOP.PLATFORM.SMC_AXI_MST.DATA_WIDTH),
            .ID_WIDTH(topology.TOP.PLATFORM.SMC_AXI_MST.ID_WIDTH  ),
            .STRB_WIDTH(topology.TOP.PLATFORM.SMC_AXI_MST.STRB_WIDTH),
            .USER_WIDTH(topology.TOP.PLATFORM.SMC_AXI_MST.USER_WIDTH),
            .AR_Q_MAX(topology.TOP.PLATFORM.SMC_AXI_MST.AR_Q_MAX),
            .AW_Q_MAX(topology.TOP.PLATFORM.SMC_AXI_MST.AW_Q_MAX),
            .W_Q_MAX(topology.TOP.PLATFORM.SMC_AXI_MST.W_Q_MAX),
            .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.SMC_AXI_MST.ID, p)),
            .tag(tag),
            `RV_TESTER_TRANSACTIONS_AXI_SW_MST_SOURCE_PARAMS(1)
        ) smc_sw_mst (
            .clk(dut_clk[SOC_CLK_IDX]),
            .sys_reset(sys_reset[SOC_CLK_IDX]),
            .reset_n(~(dut_reset[SOC_CLK_IDX] | warm_reset_pullup)),
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
            `RV_TESTER_TRANSACTIONS_AXI_SW_MST_SOURCE_PORTS(3, p, 1)
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

    string preload_data_file_arr [0:AxiLLC_SetAssociativity - 1]; // Declare an array for the preload data file names
    string preload_tag_file_arr [0:AxiLLC_SetAssociativity - 1]; // Declare an array for the preload tag file names

    // Palladium doesn't want localparam int unsigned inside a function
    localparam int unsigned AXI_AW = topology.TOP.PLATFORM.AXI.ADDR_WIDTH;
    function automatic void rv_tester_set_address_map(int unsigned i, longint unsigned start_addr, longint unsigned end_addr, int unsigned device);
        addr_map[i] = '{
            idx       : device         ,
            start_addr: AXI_AW'(start_addr),
            end_addr  : AXI_AW'(end_addr  )
        };

        addr_map_idx1[i] = '{
            idx       : 1              ,
            start_addr: AXI_AW'(start_addr),
            end_addr  : AXI_AW'(end_addr  )
        };

    endfunction

    assign addr_map_final = (rv_tester_mem_bypass_cache == 0)?addr_map:addr_map_idx1;

    export "DPI-C" function rv_tester_set_address_map;

    `ifndef NO_PRELOAD
    function void set_preload_data_file(int unsigned way, string file);
        if (way < AxiLLC_SetAssociativity) begin
            preload_data_file_arr[way] = file;
            $display("%0t Preload data file for way %0d set to: %s", $time, way, file);
        end else begin
            $display("Error: Attempted to set preload file for invalid way %0d", way);
        end
    endfunction
    `else
        function void set_preload_data_file(); // some tools have problems with string arguments
            $display("Error: Compiled with NO_PRELOAD defined");
        endfunction
    `endif

    export "DPI-C" function set_preload_data_file;


    `ifndef NO_PRELOAD
    function void set_preload_tag_file(int unsigned way, string file);
        if (way < AxiLLC_SetAssociativity) begin
            preload_tag_file_arr[way] = file;
            $display("Preload data file for way %0d set to: %s", way, file);
        end else begin
            $display("Error: Attempted to set preload file for invalid way %0d", way);
        end
    endfunction
    `else
        function void set_preload_tag_file(); // some tools have problems with string arguments
            $display("Error: Compiled with NO_PRELOAD defined");
        endfunction
    `endif

    export "DPI-C" function set_preload_tag_file;

    rv_tester_mem #(
        .NumMasters             ( topology.TOP.PLATFORM.AXI.TOTAL ),
        .AxiIdWidth             ( topology.TOP.PLATFORM.AXI.ID_WIDTH ),
        .AxiDataWidth           ( topology.TOP.PLATFORM.AXI.DATA_WIDTH ),
        .AxiAddrWidth           ( topology.TOP.PLATFORM.AXI.ADDR_WIDTH ),
        .AxiStrbWidth           ( topology.TOP.PLATFORM.AXI.STRB_WIDTH ),
        .AxiUserWidth           ( AXI_USER_ID_WIDTH ),
        .NumLines_LLC           ( AxiLLC_NumLines ),
        .NumBlocks_LLC          ( AxiLLC_NumBlocks ),
        .SetAssociativity_LLC   ( AxiLLC_SetAssociativity ),
        .slv_req_t              ( slv_req_rv  ),
        .slv_resp_t             ( slv_resp_rv ),
        .mst_req_t              ( mst_req_rv  ),
        .mst_resp_t             ( mst_resp_rv ),
        .rule_t                 ( xbar_rule_t ),
        .NoAddrRules            ( NoAddrRules ),
        .NumMastersMem          ( NoOfMasters ),
        .MaxInFlightReadReq     ( MaxInFlightReadReq ),
        .MaxBeatsPerBurst       ( MaxBeatsPerBurst ),
        .RespDelayModule        ( RespDelayModule )
        ) rv_tester_mem(
        .clk                    ( dut_clk[AXI_CLK_IDX] ),
        .rst_n                  ( ~dut_reset[AXI_CLK_IDX] ),
        .axi_req_up             ( axi_req ),
        .axi_resp_up            ( axi_rsp ),
        .axi_req_mst_up         ( axi_req_llc ),
        .axi_resp_mst_up        ( axi_rsp_llc ),
        .addr_map               ( addr_map_final ),
        .rv_tester_enable_llc   ( rv_tester_enable_llc ),
        .rv_tester_mem_delay    ( rv_tester_mem_delay ),
        .flush_cache            ( terminate && quiesced ),
        .flush_complete         ( flush_complete ),
        .bist_status_done       ()
        `ifndef NO_PRELOAD
            , .preload_file_data_arr  ( preload_data_file_arr )
            , .preload_file_tag_arr   ( preload_tag_file_arr )
        `endif
    );

    always @(posedge dut_clk[TB_CLK_IDX]) begin
        assert(assertion_test_cycle == '0 || clocks != 64'(assertion_test_cycle)) else $error("assertion test");
    end

endmodule
