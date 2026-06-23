module rv_tester
  import rv_tester_params::*,
    pmu_core_pkg::INSTRUCTIONS;
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

  localparam int unsigned NoAddrRules = 100;

  typedef struct packed {
    int unsigned idx;
    logic [topology.TOP.PLATFORM.AXI_SW[AXI_IDX].ADDR_WIDTH-1:0] start_addr;
    logic [topology.TOP.PLATFORM.AXI_SW[AXI_IDX].ADDR_WIDTH-1:0] end_addr;
  } xbar_rule_t;

  bit flag_force_ref_clk;
  bit force_ref_clk_d1;
  bit force_ref_clk_d2;
  logic rv_tester_reset = '1;
  assign rv_tester_reset_ = rv_tester_reset;

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


        rv_tester_clk_mux #(.NUM_INPUTS(7)) i_clk_mux (
            .clks_i      ({profile6_clk[c], profile5_clk[c], profile4_clk[c],
                           profile3_clk[c], profile2_clk[c], profile1_clk[c], def_clk[c]}),
            .async_sel_i (clock_mode),
            .clk_o       (clk[c])
        );
      end
`endif
    end
  end

  //import "DPI-C" function void rv_tester_streaming_dpi_init();
  import "DPI-C" function void rv_tester_streaming_dpi_shutdown();
  import "DPI-C" function int rv_tester_parse_flags(); // dummy return value so that this gets called immediately. need this to happen before any other DPIs are called.
  import "DPI-C" function void rv_tester_set_seed();
  import "DPI-C" context function void rv_tester_cvm_error_handler();
  import "DPI-C" context function void rv_tester_parse_memmap(int unsigned no_addr_rules, int num_ways, int num_sets, int num_blocks, int addr_width, int data_width);
  import "DPI-C" context function void rv_tester_build_registry();
  import "DPI-C" context function void rv_tester_domain0_build_registry();
  import "DPI-C" function byte unsigned rv_tester_shutdown_registry(bit unconditional_terminate);
  import "DPI-C" function byte unsigned rv_tester_domain1_shutdown_registry();
  import "DPI-C" context function bit rv_tester_flush_callbacks();
  import "DPI-C" function longint unsigned eot_get_addr();
  import "DPI-C" context function bit rv_tester_perf_calc(int init, int reset_done, int term, LU clocks);
  import "DPI-C" context function void rv_tester_clock_monitor(LU clocks, int unsigned clock_mode);

  localparam int unsigned MaxInFlightReadReq = topology.TOP.PLATFORM.MAX_IN_FLIGHT_READ_REQ;
  localparam int unsigned MaxBeatsPerBurst = topology.TOP.PLATFORM.MAX_BEATS_PER_BURST;
  localparam bit RespDelayModule = 1;

  xbar_rule_t [NoAddrRules-1:0] addr_map;
  bit perf = 0;
  logic sys_reset_any;
  logic shifted_dut_reset_req, shifted_dut_reset_req_d1;
  logic dut_reset_req_d1;
  logic fml_shutdowned;
  logic init_pulse;
  logic warm_reset_pulse;
  int unsigned warm_reset_clocks = 0;
  int unsigned soc_clocks = 0;
  //LU clocks = 0;
  LU axi_clocks;
  bit perf_init_done = 1'b0;       // init done
  bit perf_reset_done = 1'b0;       // reset done
  bit perf_retn  = 1'b0;
  int perf_count  = 0;
  int perf_period = 0;
  int debug_enable = 0;
  bit cb_poll = '0;
  bit dyn_clk_switch = '0;
  bit cb_success = '1;
  logic call_finish;
  int num_reruns = -1;
  int dm_build_count = 0;

  logic warm_reset_req_d1;
  logic dmi_warm_reset;

  int num_builds = -1;

  bit [NHARTS-1:0] poke_event_out;
  bit poke_event_in;
  bit overlay_mmr_en = 0;

  logic terminate_1T = '0;
  localparam int unsigned CVM_DONE_DELAY_CYCLES = 500;
  localparam int unsigned CVM_DONE_DRAINED_DELAY_CYCLES = 500;
  logic [CVM_DONE_DELAY_CYCLES-1:0] cvm_done_terminate_delay_sr;
  logic [CVM_DONE_DRAINED_DELAY_CYCLES-1:0] cvm_done_drained_delay_sr;
  logic cvm_done_drained;
  logic terminated_1T = '0;
  logic rerun_now;
  /* verilator lint_off UNOPTFLAT */
  rv_tester_pkg::terminate_t rv_tester_error_terminate;
  /* verilator lint_off UNOPTFLAT */
  rv_tester_pkg::terminate_t cosim_terminate [NHARTS-1:0];
  logic cosim_terminate_any;
  longint unsigned instructions = 0;

  int quiesce_counter = 0;
  int quiesce_timeout;
  bit print_terminate_message = '1;
  bit dm_registery_terminate_message = '1;
  int ndmreset_ack_delay = 0;

  int trace_timeout;
  int freq_switch_ncycles;
  int clk_profile = 0;

  int assertion_test_cycle = 0;

  logic streaming_dpi_shutdowned = 0;

  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.ID, 0);


  bit gen_clocks = '0;
  bit gen_timestamp = '0;
  logic [63:0] current_time;
  int unsigned cvm_verbosity, cvm_debug_verbosity, curr_cvm_verbosity;
  LU cvm_debug_cycle_on = '0;
  LU cvm_debug_cycle_off = '0;
  //logic dut_terminate_any;

  bit rv_tester_reset_dut_clk;
  bit rv_tester_reset_core_clk_d1;
  bit rv_tester_reset_core_clk_d2;
  bit rv_tester_reset_core_clk_d3;
  logic rv_tester_streaming_dpi_init;
  bit rv_tester_dpi_init_done;
  
  // Termination condition variables for better readability
  logic sysmod_cosim_dmi_terminate;

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


  // Extract common termination conditions for better readability
  assign sysmod_cosim_dmi_terminate = (sysmod_terminate || cosim_terminate_any || dmi_poll_timeout_terminate) && !sys_reset_any;
  assign core_terminate_conditions = dut_terminate || rv_tester_error_terminate.terminate || sysmod_cosim_dmi_terminate;
`ifdef UVM_MACROS_SVH
  assign terminate           = uvm_done && (core_terminate_conditions || quiesce_counter > 0) && !rv_tester_reset && !warm_reset;
`else    
  assign terminate           = (core_terminate_conditions || quiesce_counter > 0) && !rv_tester_reset && !warm_reset;
`endif   
  assign terminate_now       = (unconditional_terminate && sysmod_terminate) || (cvm_done_drained &&terminate_1T && (quiesced || (quiesce_timeout != 0 && (quiesce_counter >= quiesce_timeout))) && !warm_reset) || dut_terminate || warm_reset_now;

  assign rerun_now           = terminated && !terminated_1T && num_reruns != -1 && ((num_reruns > 0) || (warm_reset_en && (num_resets <= target_num_resets)) || shifted_dut_reset_req);

  // Assert `cvm_done` CVM_DONE_DELAY_CYCLES TB clk cycles after `terminate_1T` (uvm/CVM handshake).
  assign cvm_done = cvm_done_terminate_delay_sr[CVM_DONE_DELAY_CYCLES-1];
  // Assert `cvm_done_drained` CVM_DONE_DRAINED_DELAY_CYCLES TB clk cycles after `cvm_done`.
  assign cvm_done_drained = cvm_done_drained_delay_sr[CVM_DONE_DRAINED_DELAY_CYCLES-1];

`ifndef CLK_MUX_UNSUPPORTED
  always @(posedge dut_clk[TB_CLK_IDX])begin
    if (rv_tester_reset & !rerun_now_ff)begin
      clock_mode <= clk_profile[2:0];
      rv_tester_clock_monitor(clocks, {29'b0, clk_profile[2:0]});
    end
    /* verilator lint_off WIDTH */
    else if(dyn_clk_switch & (clocks >10) &  (freq_switch_ncycles != 0 && (clocks % freq_switch_ncycles) == 0)) begin
      //dynamically select clk from available profiles
      //this logic will generate the select pins of the mux ,which will switch between clks
      if(clock_mode == 'b110) begin
        clock_mode <= 'b1;
        rv_tester_clock_monitor(clocks, 'b1);
      end else begin
        clock_mode <= clock_mode + 1'b1;
        rv_tester_clock_monitor(clocks, {29'b0, clock_mode + 1'b1});
      end
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

    for (int i=0; i<NHARTS; i++) begin
      instructions  <= instructions + LU'(pmci[i][INSTRUCTIONS]);
    end

    if (rv_tester_reset) begin
      quiesce_counter <= '0;
      instructions    <= '0;
      if (num_resets < 0) begin
        clocks <= '0;
      end
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
      //rv_tester_streaming_dpi_init();
      //rv_tester_reset_dut_clk <= 1'b1;
    end
    if (terminated && !streaming_dpi_shutdowned) begin
      // Used for zebu offline DPI to shutdown the registry
      rv_tester_streaming_dpi_shutdown();
      streaming_dpi_shutdowned <= 1;
    end
  end
  always @(posedge dut_clk[TB_CLK_IDX] or posedge rv_tester_dpi_init_done) begin
    if (rv_tester_dpi_init_done) begin
      rv_tester_reset_dut_clk <= 1'b0;
    end
    else begin
      if (rv_tester_reset) begin
        rv_tester_reset_dut_clk <= 1'b1;
      end
    end
  end
  // rv_tester_reset_dut_clk comes in from TB_CLK and is sync-ed to CORE_CLK here
  // rv_tester_streaming_dpi_init occurs on d2=0 and d1=1 is used by rv_tester_transactions to call init funciton
  // rv_tester_dpi_init_done occurs on d3=0 and d2=1 (1 clock after init).  
  // We need this timing to insure init is called before reset is done.
  always @(posedge dut_clk[CORE_CLK_IDX]) begin
    rv_tester_reset_core_clk_d1 <= rv_tester_reset_dut_clk;
    rv_tester_reset_core_clk_d2 <= rv_tester_reset_core_clk_d1;
    rv_tester_reset_core_clk_d3 <= rv_tester_reset_core_clk_d2;
  end
  assign rv_tester_streaming_dpi_init = rv_tester_reset_core_clk_d1 & ~rv_tester_reset_core_clk_d2;
  assign rv_tester_dpi_init_done      = rv_tester_reset_core_clk_d2 & ~rv_tester_reset_core_clk_d3;
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

    if (rv_tester_reset || rvt_reload) begin
      if (rv_tester_reset || (rvt_reload && $test$plusargs("whisper_loadfrom")))
      begin
        $display("[RVTESTER]: new test");
        _ = rv_tester_parse_flags();
        if (num_resets < 0)
          rv_tester_set_seed();
        rv_tester_cvm_error_handler();
  
        /* verilator lint_off BLKSEQ */
        rv_tester_error_terminate.terminate = '0;
        /* verilator lint_on BLKSEQ */
  
  
        if (rv_tester_reset) begin
          if(num_builds < 0) begin
            $display("[RVTESTER]: constructing Full registry");
            rv_tester_build_registry();
            num_builds <= 0;
          end else begin
            $display("[RVTESTER]: constructing registry for domain:0 (without DM Model and others)");
            rv_tester_domain0_build_registry();
          end
        end
        rv_tester_parse_memmap(NoAddrRules, 0, 0, 0, topology.TOP.PLATFORM.AXI_SW[AXI_IDX].ADDR_WIDTH, topology.TOP.PLATFORM.AXI_SW[AXI_IDX].DATA_WIDTH);
  
        /* verilator lint_off BLKSEQ */
        // zebu bug doesn't allow nested function calls, so create intermediate variables
        // Using nested function calls in cvm as Palladium doesn't support strings
        _cvm_verbosity              = cvm_logger::get_verbosity_from_plusargs("cvm_verbosity");
        _gen_clocks_verbosity       = cvm_logger::get_verbosity_from_plusargs("gen_clocks_verbosity");
        _gen_timestamp_verbosity    = cvm_logger::get_verbosity_from_plusargs("gen_timestamp_verbosity");
        perf_period                 = cvm_plusargs::get_int("perf_period");
        /* verilator lint_on BLKSEQ */
  
  
        eot_addr                        <= eot_get_addr();
        eot_status                      <= 1;
        eot_syscall                     <= 0;
        perf                            <= cvm_plusargs::get_bool("perf") != '0;
        cb_poll                         <= cvm_plusargs::get_bool("cb_async") == '0;
        quiesce_timeout                 <= cvm_plusargs::get_int("quiesce_timeout");
        ndmreset_ack_delay              <= cvm_plusargs::get_int("ndmreset_ack_delay");
        trace_timeout                   <= cvm_plusargs::get_int("trace_timeout");
        freq_switch_ncycles             <= cvm_plusargs::get_int("freq_switch_ncycles");
        clk_profile                     <= cvm_plusargs::get_int("clk_profile");
        dyn_clk_switch                  <= cvm_plusargs::get_bool("dyn_clk_switch") != '0;
        call_finish                     <= cvm_plusargs::get_bool("terminate_call_finish") != '0;
        gen_clocks                      <= _cvm_verbosity >= _gen_clocks_verbosity;
        gen_timestamp                   <= _cvm_verbosity >= _gen_timestamp_verbosity;
        assertion_test_cycle            <= cvm_plusargs::get_int("assertion_test_cycle");
        overlay_mmr_en                  <= cvm_plusargs::get_bool("overlay_mmr_en") != '0;
        perf_count                      <= '0;
  
        cvm_verbosity        <= _cvm_verbosity;
        curr_cvm_verbosity   <= _cvm_verbosity;
        cvm_debug_verbosity  <= cvm_logger::get_verbosity_from_plusargs("cvm_debug_verbosity");
        cvm_debug_cycle_on   <= cvm_plusargs::get_ulongint("cvm_debug_cycle_on");
        cvm_debug_cycle_off  <= cvm_plusargs::get_ulongint("cvm_debug_cycle_off");
      end
    end
    if (!dyn_clk_switch) clock_mode <= clk_profile[2:0];
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
  always @(posedge dut_clk[TB_CLK_IDX]) begin

    automatic logic shutdowned = '0;
    automatic logic domain1_shutdowned = '0;
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

      shutdowned = rv_tester_shutdown_registry(unconditional_terminate) != '0;
`ifndef SVA_S_EVENTUALLY_UNSUPPORTED
      fml_shutdowned = shutdowned;
`endif
      if(num_resets > target_num_resets)begin
        domain1_shutdowned = rv_tester_domain1_shutdown_registry() != '0;
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

  always_ff @(posedge dut_clk[TB_CLK_IDX]) begin
    if (rv_tester_reset) begin
      cvm_done_terminate_delay_sr <= '0;
      cvm_done_drained_delay_sr <= '0;
    end else begin
      if (terminate_1T === 'b1) begin
        cvm_done_terminate_delay_sr <= {cvm_done_terminate_delay_sr[CVM_DONE_DELAY_CYCLES-2:0], terminate_1T};
      end else begin
        cvm_done_terminate_delay_sr <= {cvm_done_terminate_delay_sr[CVM_DONE_DELAY_CYCLES-2:0], '0};
      end
      if(cvm_done === 'b1) begin
        cvm_done_drained_delay_sr <= {cvm_done_drained_delay_sr[CVM_DONE_DRAINED_DELAY_CYCLES-2:0],
                                      cvm_done_terminate_delay_sr[CVM_DONE_DELAY_CYCLES-1]};
      end else begin
        cvm_done_drained_delay_sr <= {cvm_done_drained_delay_sr[CVM_DONE_DRAINED_DELAY_CYCLES-2:0], '0};
      end
    end
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
  logic dut_reset_axi_latched = 1'b0;
  logic warm_reset_deasserted = 1'b0;


  always @(posedge dut_clk[TB_CLK_IDX]) begin
    dut_reset_axi_latched <= dut_reset[AXI_CLK_IDX];
    if(cold_reset === 1'b0)begin
      if (ndmreset_ack_clocks_latched && dut_reset_axi_latched && !dut_reset[AXI_CLK_IDX])  begin
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

  /* RVDE-27024
   * function void rv_tester_set_clock_mode (input int unsigned new_clock_mode);
   *     if (new_clock_mode <= 3'b110) begin
   *         clock_mode <= new_clock_mode[2:0];
   *         $display("<%0d> rv_tester_set_clock_mode: clock_mode changed to %0d", clocks, new_clock_mode);
   *     end else begin
   *         $display("<%0d> rv_tester_set_clock_mode: Invalid clock_mode %0d, valid range is 0-6", clocks, new_clock_mode);
   *     end
   * endfunction
   * export "DPI-C" function rv_tester_set_clock_mode;
   */

  `RV_TESTER_TRANSACTIONS_DOMAIN(1, dut_clk[CORE_CLK_IDX]);
  `RV_TESTER_TRANSACTIONS_DOMAIN(2, dut_clk[AXI_CLK_IDX]);
  `RV_TESTER_TRANSACTIONS_DOMAIN(3, dut_clk[SOC_CLK_IDX]);

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
                     .bootstrap,
                     .dmi_write(dmi_write),
                     .event_triggers(event_triggers),
                     .interrupt,
                     .terminate(sysmod_terminate),
                     `RV_TESTER_TRANSACTIONS_SYSMOD_SOURCE_PORTS(2, 0, 0)
                     );

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
                     .timeCsr(timeCsr[c]),
                     .MTIP(MTIP[c]),
                     .imsic_msi(imsic_msi[c]),
                     .debug_mode(debug_mode[c]),
                     .haltreq(DM_DebugReq_Valids[c]),
                     .terminate(cosim_terminate[c]),
                     .eot_addr(eot_addr),
                     .addr_map(addr_map),
                     .poke_event_out(poke_event_out[c]),
                     .poke_event_in(poke_event_in),
                     .cycles_since_retire(cycles_since_retire[c]),
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

  // Generated AXI interface instantiations from topology
  `AXI_INSTANTIATION

    localparam int unsigned XbarAddrWidth = topology.TOP.PLATFORM.AXI_SW[AXI_IDX].ADDR_WIDTH;
  function automatic void rv_tester_set_address_map(int unsigned i, longint unsigned start_addr, longint unsigned end_addr, int unsigned device);
    addr_map[i] = '{
                    idx       : device                       ,
                    start_addr: XbarAddrWidth'(start_addr)   ,
                    end_addr  : XbarAddrWidth'(end_addr  )
                    };

  endfunction

  export "DPI-C" function rv_tester_set_address_map;

  always @(posedge dut_clk[TB_CLK_IDX]) begin
    assert(assertion_test_cycle == '0 || clocks != 64'(assertion_test_cycle)) else $error("assertion test");
  end

endmodule
