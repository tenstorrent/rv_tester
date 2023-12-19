module sysmod
import rv_tester_params::*;
#(
    parameter int CLOCK_PERIOD_PS           =     500,
    parameter int SW_CLOCK_UPDATE_PERIOD_PS = 100_000,
    parameter int NUM                       =      -1,
    `TOPOLOGY,
    `RV_TESTER_TRANSACTIONS_SYSMOD_OUTPUT_PARAMS
)(
    input clk,
    input reset,
    input longint unsigned clocks,
    output rv_tester_params::bootstrap_t bootstrap,
    output rv_tester_pkg::interrupt_t interrupt [NHARTS-1:0],
    output rv_tester_pkg::dm_write_t  dmi_write,
    output rv_tester_pkg::terminate_t terminate,
    `RV_TESTER_TRANSACTIONS_SYSMOD_OUTPUT_PORTS
);
    import "DPI-C" context function void sysmod_set_scope(int unsigned location);

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
            location = cvm_topology::get_location(topology.TOP.PLATFORM.SYSMOD.ID, NUM);
            if (location != cvm_topology::nil) begin
              sysmod_set_scope(location);
            end
            terminate.terminate = '0;
            /* verilator lint_on BLKSEQ */
        end
    end

    assign bootstrap.boot_addr = 1 << 31;

    function void sysmod_terminate ();
        $display("attempting to terminate");
        terminate.terminate = '1;
    endfunction
    export "DPI-C" function sysmod_terminate;

    localparam longint unsigned TICKS = LU'(SW_CLOCK_UPDATE_PERIOD_PS)/LU'(CLOCK_PERIOD_PS);
    assign ticks[0].valid         = (0 == (clocks % TICKS)) & (location != cvm_topology::nil);
    assign ticks[0].data.location = location;
    assign ticks[0].data.advance  = TICKS;

    rv_tester_pkg::interrupt_t interrupt_d [NHARTS-1:0] = '{default: '0}; // FIXME how to reset these?
    rv_tester_pkg::interrupt_t interrupt_q [NHARTS-1:0];
    assign interrupt = interrupt_q;

    function void sysmod_timer_interrupt (unsigned hartid, unsigned val);
      interrupt_d[hartid].mti = val;
    endfunction
    export "DPI-C" function sysmod_timer_interrupt;

    function void sysmod_sw_interrupt (unsigned hartid, unsigned val);
      interrupt_d[hartid].msi = val;
    endfunction
   
       export "DPI-C" function sysmod_aplic_dir_interrupt;

    function void sysmod_aplic_dir_interrupt (longint val[16]);
      //interrupt_d[hartid].msi = val;
      $display("\nHello aplic\n");
      for(int i =0;i<16;i++)begin
        $display("\n APLIC DATA %h \n",val[i]);
      end
    endfunction

    export "DPI-C" function sysmod_sw_interrupt;

    function void sysmod_tbox_interrupt (int unsigned hartid, int unsigned intr_select,int unsigned intr_value);
      for(int i =0;i<6;i++)begin
        if(intr_select[i])
          interrupt_d[hartid][i] = intr_value[i];
      end
    endfunction
    export "DPI-C" function sysmod_tbox_interrupt;

    function sysmod_dmi_write (int unsigned hartid, int unsigned upper_value,int unsigned lower_value);
      dmi_write_begin = '1;
      dm_wdata = {upper_value,lower_value};
    endfunction
    export "DPI-C"  function sysmod_dmi_write;

    always @(posedge clk) begin
        interrupt_q <= interrupt_d;
        if (reset) begin
            dmi_write   <= '0;
        end
        else if(dmi_write_end)begin
            dmi_write.dm_wdata <= '0;
            dmi_write.dm_wvalid <= '0;
            dmi_write_begin <= '0;
            dmi_write_end <= '0;
        end
        else if(dmi_write_begin)begin
            dmi_write.dm_wvalid <= '1;
            dmi_write.dm_wdata <= dm_wdata;
            dmi_write_end <='1;
        end

    end

endmodule
