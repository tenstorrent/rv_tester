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
  `RV_TESTER_TRANSACTIONS_RESET_DRIVER_OUTPUT_PORTS
);

    import "DPI-C" context function void reset_driver_set_scope(int unsigned location);
    int unsigned location = cvm_topology::nil;
  
    logic reset_done;
        typedef longint unsigned LU;
    LU clocks = 0;
    
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
            if (location != cvm_topology::nil) begin
               $display("\nsv::RESET DRIVER pass scope from sv to cpp : location : %d\n",location);
              reset_driver_set_scope(location);
            end
            //terminate.terminate = '0;
            /* verilator lint_on BLKSEQ */
        end
    end

    //localparam longint unsigned TICKS = LU'(SW_CLOCK_UPDATE_PERIOD_PS)/LU'(CLOCK_PERIOD_PS);
    assign ticks[0].valid         = (clocks % 2 == 0) & (location != cvm_topology::nil);
    assign ticks[0].data.location = location;
    assign ticks[0].data.advance  = 2;

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
