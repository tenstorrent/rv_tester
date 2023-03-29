module sysmod #(
    parameter int CLOCK_PERIOD_PS           =     500,
    parameter int SW_CLOCK_UPDATE_PERIOD_PS = 100_000,
    parameter int  NUM                      =      -1,
    `TOPOLOGY,
    `RV_TESTER_PARAMETERS(topology)
)(
    input clk,
    input reset,
    input longint unsigned clocks,
    output bootstrap_t bootstrap,
    output rv_tester_pkg::interrupt_t interrupt,
    output rv_tester_pkg::dm_write_t  dmi_write,
    output rv_tester_pkg::terminate_t terminate
);
    import "DPI-C" context function void sysmod_set_scope(int unsigned location);
    import "DPI-C" function void sysmod_tick(int unsigned location, longint unsigned advance);
    import "DPI-C" function int sysmod_tick_with_return(int unsigned location, longint unsigned advance);

    typedef longint unsigned LU;
    int unsigned location = cvm_topology::nil;
    bit sysmod_tick_async = '1;

    /* verilator lint_off BLKANDNBLK */
    bit dmi_write_begin = '0;
    bit dmi_write_begin_d = '0;
    bit dmi_write_end = '0;
    bit [63:0] dm_wdata = '0;
    /* verilator lint_on BLKANDNBLK */

    always @(posedge clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            sysmod_tick_async = cvm_plusargs::get_bool("sysmod_tick_async") != '0;
            location = cvm_topology::get_location(topology.PLATFORM.id, 0);
            sysmod_set_scope(location);
            /* verilator lint_on BLKSEQ */
        end
        terminate.terminate <= '0;
    end

    assign bootstrap.boot_addr = 1 << 31;

    function void sysmod_terminate (byte unsigned call_finish);
        terminate.terminate <= '1;
        terminate.call_finish = call_finish[0];
    endfunction
    export "DPI-C" function sysmod_terminate;

    localparam longint unsigned TICKS = LU'(SW_CLOCK_UPDATE_PERIOD_PS)/LU'(CLOCK_PERIOD_PS);
    always @(posedge clk) begin
        if (0 == (clocks % TICKS)) begin
            if (location != cvm_topology::nil) begin
                if (sysmod_tick_async) begin
                    sysmod_tick(location, TICKS);
                end else begin
                    void'(sysmod_tick_with_return(location, TICKS));
                end
            end
        end
    end

    rv_tester_pkg::interrupt_t /* verilator lint_off BLKANDNBLK */ interrupt_d /* verilator lint_on BLKANDNBLK */;
    rv_tester_pkg::interrupt_t interrupt_q;
    assign interrupt = interrupt_q;

    function void sysmod_timer_interrupt (unsigned hartid, unsigned val);
      $display("[SYSMOD] mti = %0d", val);
      interrupt_d.mti = val;
    endfunction
    export "DPI-C" function sysmod_timer_interrupt;

    function void sysmod_sw_interrupt (unsigned hartid, unsigned val);
      $display("[SYSMOD] msi = %0d", val);
      interrupt_d.msi = val;
    endfunction
    export "DPI-C" function sysmod_sw_interrupt;

    function void sysmod_tbox_interrupt (int unsigned hartid, int unsigned intr_select,int unsigned intr_value);
      $display("\n[SYSMOD] trickbox interrupt select %0d with value %0d\n",intr_select,intr_value);
      for(int i =0;i<6;i++)begin
        if(intr_select[i])
          interrupt_d[i] = intr_value[i];
      end
    endfunction
    export "DPI-C" function sysmod_tbox_interrupt;

    function sysmod_dmi_write (int unsigned hartid, int unsigned upper_value,int unsigned lower_value);
      $display("\n[SYSMOD] trickbox DMI write upper value: %d lower value: %d\n",upper_value,lower_value);
      dmi_write_begin = '1;
      dm_wdata = {upper_value,lower_value};
    endfunction
    export "DPI-C"  function sysmod_dmi_write;

    always @(posedge clk) begin
        interrupt_q <= interrupt_d;
        if (reset) begin
            interrupt_d <= '0;
            dmi_write   <= '0;
        end
        else if(dmi_write_end)begin
            dmi_write.dm_wdata <= '0;
            dmi_write.dm_wvalid <= '0;
            dmi_write_begin <= '0;
            dmi_write_end <= '0;
            $display("\n[SYSMOD] trickbox DMI Deassert write : %d time: %t\ n",dmi_write.dm_wdata,$time);
        end
        else if(dmi_write_begin)begin
            dmi_write.dm_wvalid <= '1;
            dmi_write.dm_wdata <= dm_wdata;
            dmi_write_end <='1;
            $display("\n[SYSMOD] trickbox DMI Assert write : %d time: %t\ n",dmi_write.dm_wdata,$time);
        end

    end

endmodule
