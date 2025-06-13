module sysmod
import rv_tester_params::*;
#(
    parameter int CLOCK_PERIOD_PS              =     500,
    parameter int JTAG_CLOCK_PERIOD_PS         =     100,
    parameter int OVERLAY_CLOCK_PERIOD_PS      =     100,
    parameter int SW_CLOCK_UPDATE_PERIOD_PS    = 100_000,
    parameter int NUM                          =      -1,
    parameter int JTAG_DR_WIDTH                =      70,
    `TOPOLOGY,
    `RV_TESTER_TRANSACTIONS_SYSMOD_OUTPUT_PARAMS
)(
    input clk,
    input reset,
    input dut_reset_req,
    input dut_core_reset,
    output logic trace_quiesced,
    //output logic jtag_quiesced,
    output rv_tester_params::bootstrap_t bootstrap,
    output rv_tester_pkg::interrupt_t interrupt [NHARTS-1:0],
    output rv_tester_pkg::dm_write_t  dmi_write,
    input  event_trigger_intf_t event_triggers [NHARTS-1:0],
    //output rv_tester_pkg::jtag_if_t  jtag_req,
    //output rv_tester_pkg::jtag_if_tck  jtag_tck_trst,
    //input rv_tester_pkg::jtag_if_out  jtag_resp,
    output rv_tester_pkg::terminate_t terminate,
    `RV_TESTER_TRANSACTIONS_SYSMOD_OUTPUT_PORTS
);
    import "DPI-C" context function void sysmod_set_scope(int unsigned location);

    typedef longint unsigned LU;
    LU clocks = 0;
    parameter int unsigned location = cvm_topology_gen::get_location (topology.TOP.PLATFORM.SYSMOD.ID, NUM);
    bit sysmod_tick_async = '1;

    /* verilator lint_off UNOPTFLAT */
    logic trace_quiesced_q = 0;
    /* verilator lint_on UNOPTFLAT */

    /* verilator lint_off BLKANDNBLK */
    bit dmi_write_begin = '0;
    bit dmi_write_begin_d = '0;
    bit dmi_write_end = '0;
    bit [63:0] dm_wdata = '0;

      /* verilator lint_on BLKANDNBLK */


    // jtag_xtor #(.JTAG_DR_WIDTH(JTAG_DR_WIDTH))  i_jtag_xtor(
    //     .clk(clk),
    //     .reset(reset),
    //     .tap_sel(tap_sel),
    //     .command(command),
    //     .jtag_req(jtag_req),
    //     .jtag_resp(jtag_resp),
    //     .jtag_tck_trst(jtag_tck_trst),
    //     .jtag_busy(jtag_busy),
    //     .jtag_enable(jtag_enable_begin),
    //     .read_data_valid_reg(read_data_valid_reg),
    //     .length(length),
    //     .jtag_tx(jtag_tx),
    //     .jtag_rx(jtag_rx),
    //     .misc_signals('0)
    // );

    /* verilator lint_on BLKANDNBLK */
    always @(posedge clk) begin
        clocks <= clocks + 1;
        if (reset) begin
            $display("[sysmod]: reset");
            clocks <= 0;
            /* verilator lint_off BLKSEQ */
            //jtag_quiesced = 0;
            sysmod_tick_async = cvm_plusargs::get_bool("sysmod_tick_async") != '0;
            if (location != cvm_topology::nil) begin
              sysmod_set_scope(location);
            end
            terminate.terminate = '0;
            /* verilator lint_on BLKSEQ */
        end
    end

    assign trace_quiesced = trace_quiesced_q;
    assign bootstrap.boot_addr = 1 << 31;

    function void sysmod_terminate ();
        $display("[sysmod]: attempting to terminate");
        terminate.terminate = '1;
    endfunction
    export "DPI-C" function sysmod_terminate;

    localparam longint unsigned TICKS = LU'(SW_CLOCK_UPDATE_PERIOD_PS)/LU'(CLOCK_PERIOD_PS);
    assign ticks[0].valid         = ((0 == (clocks % TICKS)) || dut_reset_req) & (~dut_core_reset) & (clocks > TICKS )& (location != cvm_topology::nil);
    assign ticks[0].data.location = location;
    assign ticks[0].data.advance  = TICKS;
    assign ticks[0].data.clocks   = clocks;
    assign ticks[0].data.divisor  = TICKS;
    assign ticks[0].data.dut_reset_req = dut_reset_req & (clocks > 100);

    localparam longint unsigned JTAG_TICKS = LU'(SW_CLOCK_UPDATE_PERIOD_PS)/LU'(JTAG_CLOCK_PERIOD_PS);
    assign jtag_ticks[0].valid         = (0 == (clocks % JTAG_TICKS)) & (location != cvm_topology::nil);
    assign jtag_ticks[0].data.location = location;
    assign jtag_ticks[0].data.advance  = JTAG_TICKS;

    localparam longint unsigned OVERLAY_TICKS = LU'(SW_CLOCK_UPDATE_PERIOD_PS)/LU'(OVERLAY_CLOCK_PERIOD_PS);
    assign overlay_ticks[0].valid         = (0 == (clocks % OVERLAY_TICKS)) & (location != cvm_topology::nil);
    assign overlay_ticks[0].data.location = location;
    assign overlay_ticks[0].data.advance  = OVERLAY_TICKS;

    rv_tester_pkg::interrupt_t interrupt_d [NHARTS-1:0] = '{default: '0}; // FIXME how to reset these?
    rv_tester_pkg::interrupt_t interrupt_q [NHARTS-1:0];
    assign interrupt = interrupt_q;

    function void sysmod_timer_interrupt (int unsigned hartid, int unsigned val, longint unsigned mtime_val);
      interrupt_d[hartid].mti = val[0];
      interrupt_d[hartid].mtime = mtime_val;
    endfunction
    export "DPI-C" function sysmod_timer_interrupt;

    function void sysmod_sw_interrupt (int unsigned hartid, int unsigned val);
      interrupt_d[hartid].msi = val[0];
    endfunction

    export "DPI-C" function sysmod_sw_interrupt;

    function void sysmod_tbox_interrupt (int unsigned hartid, int unsigned intr_select,int unsigned intr_value);
      for(int i =0;i<6;i++)begin
        if(intr_select[i])
          interrupt_d[hartid][i] = intr_value[i];
      end
    endfunction
    export "DPI-C" function sysmod_tbox_interrupt;

    function void sysmod_trace_info (int unsigned trace_info_s);
      trace_quiesced_q = trace_info_s[0];
    endfunction
    export "DPI-C" function sysmod_trace_info;

    function sysmod_dmi_write (int unsigned hartid, int unsigned upper_value,int unsigned lower_value);
      dmi_write_begin = '1;
      dm_wdata = {upper_value,lower_value};
    endfunction
    export "DPI-C"  function sysmod_dmi_write;

    // function sysmod_jtag_req (int unsigned jtag_cmd_ip,longint upper_value,longint lower_value,int unsigned reg_length, int unsigned jtag_quit , int unsigned tap_cfg_sel);
    //   if(jtag_quit[0] === 1'b0 )begin
    //     jtag_enable_begin = 1'b1;
    //     command = jtag_cmd_ip[1:0];
    //     jtag_tx = {upper_value[5:0],lower_value};
    //     tap_sel = tap_cfg_sel;
    //     length = reg_length[31:0];
    //     //jtag_quiesced = 1'b0;
    //     $display("[SYSMOD.SV] JTAG driver %h %h %h %h %h",upper_value, lower_value,reg_length,tap_sel,tap_cfg_sel);
    //   end
    //   else if(jtag_quit[0] === 1'b1 )begin
    //     //jtag_quiesced = 1'b1;
    //     $display("[SYSMOD.SV] JTAG quit was given in %0d %t",jtag_quit[0],$time);
    //   end
    // endfunction
    // export "DPI-C"  function sysmod_jtag_req;


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

  // assign jtag_rdatas[0].valid         = read_data_valid_reg;
  // assign jtag_rdatas[0].data.location = location;
  // assign jtag_rdatas[0].data.rdata     = jtag_rx;//upper32 bits for future use
  // assign jtag_rdatas_jtag_busy = jtag_busy ;

  // Currently we have only NHARTS C2 triggers, new triggers shall send message on event_triggerss[NHARTS+:NHARTS] and increment rv_tester_transactions.yml
  for (genvar n = 0; n < NHARTS; n++) begin: tboxtrigc2
  assign event_triggerss[n].valid         = event_triggers[n][C2].valid;
  assign event_triggerss[n].data.location = location;
  assign event_triggerss[n].data.data     = event_triggers[n][C2].data;
  assign event_triggerss[n].data.addr     = event_triggers[n][C2].addr;
  end
endmodule
