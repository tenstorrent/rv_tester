module reset_driver #(
  parameter int NUM = -1,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_RESET_DRIVER_OUTPUT_PARAMS
)(
  input clk,
  input tb_clk,
  input reset,
  input bit terminate,
  input logic [7:0] misc_signals,
  output cold_reset_n,
  output warm_reset_n,
  output sram_hold,
  output debug_hold,
  output critical_hold,
  `RV_TESTER_TRANSACTIONS_RESET_DRIVER_OUTPUT_PORTS
);

    import "DPI-C" context function void reset_driver_set_scope(int unsigned location);
    int unsigned location = cvm_topology::nil;
    int unsigned cold_reset_cycles = 0;
    int unsigned warm_reset_cycles = 0;
    int unsigned reset_chk_threshold_period = 2;
    int unsigned reset_chk_period = 500;
    bit mid_sim_reset_en = 0;
    bit mid_sim_warm_reset_en = 0;
  
    logic reset_done;
        typedef longint unsigned LU;
    LU clocks = 0;
    LU init_clocks = 500;
    
    bit sysmod_tick_async = '1;
    bit [3:0] o_resets;
    bit [3:0] o_holds;
    bit flag = 0;
        /* verilator lint_on BLKANDNBLK */
    always @(posedge clk) begin
        clocks <= clocks + 1;
        if(!flag)begin
          clocks <= 0;
          flag <= 1;
        end
        if (clocks ==2) begin
            //clocks <= 0;
            /* verilator lint_off BLKSEQ */
            sysmod_tick_async = cvm_plusargs::get_bool("sysmod_tick_async") != '0;
            location = cvm_topology::get_location(topology.TOP.PLATFORM.RESET_DRIVER.ID, NUM);
            cold_reset_cycles        <= cvm_plusargs::get_int("mid_sim_reset_period");
            warm_reset_cycles        <= cvm_plusargs::get_int("mid_sim_warm_reset_period");
            reset_chk_threshold_period        <= cvm_plusargs::get_int("reset_chk_threshold_period");
            reset_chk_period        <= cvm_plusargs::get_int("reset_chk_period");
            mid_sim_reset_en          = cvm_plusargs::get_bool("mid_sim_reset_en") != '0;
            mid_sim_warm_reset_en     = cvm_plusargs::get_bool("mid_sim_warm_reset_en") != '0;
            if (location != cvm_topology::nil) begin
               $display("\nsv::RESET DRIVER pass scope from sv to cpp : location : %d\n",location);
              reset_driver_set_scope(location);
            end
            //terminate.terminate = '0;
            /* verilator lint_on BLKSEQ */
        end
    end

    //localparam longint unsigned TICKS = LU'(SW_CLOCK_UPDATE_PERIOD_PS)/LU'(CLOCK_PERIOD_PS);
    /* verilator lint_off WIDTHEXPAND */
    assign ticks[0].valid         =  (location != cvm_topology::nil) & ((clocks < init_clocks)|(mid_sim_reset_en & ((clocks % (cold_reset_cycles - reset_chk_threshold_period)) < reset_chk_period))| (mid_sim_warm_reset_en & ((clocks % (warm_reset_cycles - reset_chk_threshold_period)) < reset_chk_period))) ;
    //assign ticks[0].valid         = (clocks % 2 == 0) & (location != cvm_topology::nil) & ((clocks < init_clocks)|(mid_sim_reset_en & ((clocks % cold_reset_cycles) < 500))| (mid_sim_warm_reset_en & ((clocks % warm_reset_cycles) < 500))) ;
    /* verilator lint_on WIDTHEXPAND */
    assign ticks[0].data.location = location;
    assign ticks[0].data.num_clocks  = clocks;

    //assign pins
    assign sram_hold = o_holds[0];
    assign critical_hold = o_holds[1];
    assign debug_hold = o_holds[2];
    assign cold_reset_n = o_resets[0]; 
    assign warm_reset_n = o_resets[2];
    export "DPI-C" function reset_driver_drive_resets;
    function void reset_driver_drive_resets (int unsigned reset_pins);
      //trace_quiesced_q = trace_info_s[0];
      $display("\n RESET DRIVER DRIVING RESETS %h",reset_pins);
      o_resets = reset_pins[3:0];
    endfunction
    
    function void reset_driver_drive_holds (int unsigned hold_pins);
      //trace_quiesced_q = trace_info_s[0];
      $display("\n RESET DRIVER DRIVING HOLDS %h",hold_pins);
      o_holds = hold_pins[3:0];
    endfunction
    export "DPI-C" function reset_driver_drive_holds;

endmodule
