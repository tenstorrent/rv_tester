module sysmod #(
    parameter int CLOCK_PERIOD_PS           =     500,
    parameter int SW_CLOCK_UPDATE_PERIOD_PS = 100_000,
    rv_tester_pkg::cfg_t CFG                =      '0,
    parameter int  NUM                      =      -1,
    `RV_TESTER_PARAMETERS(CFG),
    `TOPOLOGY
)(
    input clk,
    input reset,
    input longint unsigned clocks,
    output bootstrap_t bootstrap,
    output rv_tester_pkg::interrupt_t interrupt,
    output terminate
);
    import "DPI-C" context function void sysmod_set_scope(int unsigned loc);
    import "DPI-C" function void sysmod_tick(int unsigned loc, longint unsigned advance);

    typedef longint unsigned LU;
    int unsigned loc = cvm_topology::nil;

    always @(posedge clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            loc = cvm_topology::get_location(topology.PLATFORM, 0);
            sysmod_set_scope(loc);
            /* verilator lint_on BLKSEQ */
        end
    end

    assign bootstrap.boot_addr = 1 << 31;

    logic ready_to_terminate = '0;
    logic terminate_after_n_clocks = '0;
    logic terminated = '0;
    byte unsigned call_finish_on_terminate = '0;
    always @(posedge clk) begin
        if (reset) begin
            /* verilator lint_off BLKANDNBLK */
            ready_to_terminate <= '0;
            terminate_after_n_clocks <= '0;
            terminated <= '0;
            /* verilator lint_on BLKANDNBLK */
        end
        if (ready_to_terminate) begin
          terminate_after_n_clocks <= terminate_after_n_clocks + 1;
        end
        // Call finish after n clocks
        if (terminate_after_n_clocks == 1) begin
          // exit gracefully
          if (call_finish_on_terminate != 0) begin
              $finish();
          end
          terminated <= '1;
        end
    end

    function automatic void sysmod_terminate (byte unsigned call_finish);
        call_finish_on_terminate = call_finish;
        ready_to_terminate = '1;
    endfunction
    export "DPI-C" function sysmod_terminate;

    assign terminate = ready_to_terminate;

    localparam longint unsigned TICKS = LU'(SW_CLOCK_UPDATE_PERIOD_PS)/LU'(CLOCK_PERIOD_PS);
    always @(posedge clk) begin
        if (0 == (clocks % TICKS)) begin
            if (loc != cvm_topology::nil) begin
              sysmod_tick(loc, TICKS);
            end
        end
    end

    rv_tester_pkg::interrupt_t /* verilator lint_off BLKANDNBLK */ interrupt_d /* verilator lint_on BLKANDNBLK */;
    rv_tester_pkg::interrupt_t interrupt_q;
    assign interrupt = interrupt_q;

    function automatic sysmod_timer_interrupt (unsigned hartid, unsigned val);
      $display("[SYSMOD] mti = %0d", val);
      interrupt_d.mti = val;
    endfunction
    export "DPI-C" function sysmod_timer_interrupt;

    function automatic sysmod_sw_interrupt (unsigned hartid, unsigned val);
      $display("[SYSMOD] msi = %0d", val);
      interrupt_d.msi = val;
    endfunction
    export "DPI-C" function sysmod_sw_interrupt;

    function automatic sysmod_tbox_interrupt (int unsigned hartid, int unsigned intr_select,int unsigned intr_value);
      $display("\n[SYSMOD] trickbox interrupt select %0d with value %0d\n",intr_select,intr_value);
      for(int i =0;i<6;i++)begin
        if(intr_select[i])
          interrupt_d[i] = intr_value[i];
      end
    endfunction
    export "DPI-C" function sysmod_tbox_interrupt;

    always @(posedge clk) begin
        interrupt_q <= interrupt_d;
        if (reset) begin
            interrupt_d <= '0;
        end
    end

endmodule
