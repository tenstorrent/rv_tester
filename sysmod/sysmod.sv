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
    output rv_tester_pkg::aplic_interrupt_t aplic_interrupt,
    output rv_tester_pkg::dm_write_t  dmi_write,
    output rv_tester_pkg::jtag_if_t  jtag_req,
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
    /* verilator lint_off BLKANDNBLK */

    bit jtag_req_begin = '0;
    bit jtag_req_begin_d = '0;
    bit jtag_req_end = '0;
    bit [63:0] jtag_tx = '0;
    bit [63:0] jtag_rx = '0;
    bit [63:0] clk_trig = '0;
    bit [1:0]  state= '0;
    bit [31:0] shiftCount= '0;
    bit [1:0]  command= '0;
    bit        jtag_enable = '0;
    bit        read_data_valid_reg='0;
    bit [31:0] read_data = '0;
    bit read = '0;
    
    logic [1:0] jtag_cmd = 2'b00;
    logic [31:0] jtag_data_in = 32'h1234abcd;
    bit[3:0] opcode= '0;
    bit ConfigureIR='0;
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

    function void sysmod_timer_interrupt (int unsigned hartid, int unsigned val);
      interrupt_d[hartid].mti = val[0];
    endfunction
    export "DPI-C" function sysmod_timer_interrupt;

    function void sysmod_sw_interrupt (int unsigned hartid, int unsigned val);
      interrupt_d[hartid].msi = val[0];
    endfunction
   
       export "DPI-C" function sysmod_aplic_dir_interrupt;

    function void sysmod_aplic_dir_interrupt (longint val[16]);
      //interrupt_d[hartid].msi = val;
      //$display("\nSYSMOD.SV Hello aplic\n");
      for(int i =0;i<16;i++)begin
        //$display("\nSYSMOD.SV APLIC DATA  at index %h %h \n",i,val[i]);
        aplic_interrupt.pins[64*i +: 64] = val[i];
      end
       // $display("\n APLIC DATA  FINAL at  %h \n",aplic_interrupt.pins);
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

    function sysmod_jtag_req (int unsigned upper_value,int unsigned lower_value);
      //dmi_write_begin = '1;
      //dm_wdata = {upper_value,lower_value};
       jtag_enable = 1'b1;
       command = upper_value[1:0];
       jtag_tx = {32'h0,lower_value};
      $display("[SYSMOD.SV] JTAG driver %h %h",upper_value, lower_value);
    endfunction
    export "DPI-C"  function sysmod_jtag_req;


    always @(posedge clk) begin
        interrupt_q <= interrupt_d;
        clk_trig <= clk_trig  + 1'b1;
        //tck <= ~tck;
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
        //**** JTAG ***//
         
   //******** JTAG ********//
    end
         
  //******** JTAG ********//

  // JTAG state machine states
  parameter IDLE = 2'b00;
  parameter SHIFT_DR = 2'b01;
  parameter SHIFT_IR = 2'b10;
  parameter UPDATE = 2'b11;
  parameter DR_WIDTH = 32'd32;
  parameter IR_WIDTH = 32'd4;

  // JTAG controller
  always @(posedge clk) begin
    if (reset) begin
      state <= IDLE;
      shiftCount <= 32'b0;
      read_data_valid_reg <= 1'b0;
      jtag_req.tdo <= 1'b0;
    end else begin
      /* verilator lint_off CASEINCOMPLETE */
      case (state)
        IDLE: begin
          jtag_req.tms <= 1'b0;
          jtag_req.tdi <= 1'b0;
          shiftCount <= 0;
          if (jtag_enable) begin // 
            // Interpret command and data, set state accordingly
            jtag_enable <= 1'b0;
            case (command)
              2'b00: state <= IDLE;
              2'b01: state <= SHIFT_DR; // to configure dr
              2'b10: state <= SHIFT_IR;// to configure ir
              2'b11: state <= UPDATE;
              default: state <= IDLE;
            endcase
          end
        end
        SHIFT_DR: begin
          if(shiftCount == 32'd0)begin
            jtag_req.tms <= 1'b1;
          end 
          else begin
            jtag_req.tms <= 1'b0;
          end
          
          if(shiftCount >= 32'd2) begin
            jtag_req.tdi <= jtag_tx[shiftCount-2];
          end
          shiftCount <= shiftCount + 1;
          if (shiftCount == 2 + DR_WIDTH) begin
            state <= UPDATE;
            command <= UPDATE;
            shiftCount <=0;
          end
          
        end
        SHIFT_IR: begin
          if(shiftCount <= 32'd1)begin
            jtag_req.tms <= 1'b1;
          end 
          else begin
            jtag_req.tms <= 0;
          end
          if(shiftCount >= 32'd3) begin
            jtag_req.tdi <= jtag_tx[shiftCount-3];
          end
          
          shiftCount <= shiftCount + 1;
          if (shiftCount == 32'd3 + IR_WIDTH) begin
            state <= UPDATE;
            command <= UPDATE;
            shiftCount <=0;
          end
          
        end
        UPDATE: begin
          jtag_req.tms <= 1'b1;
          jtag_req.tdi <= 1'b0;
          shiftCount <= shiftCount + 1;
          if (shiftCount == 32'd2) begin
            state <= IDLE;
            shiftCount <= 0;
          end
        end
        default: state <= IDLE;
      endcase
      /* verilator lint_on CASEINCOMPLETE */
    end
  end
  
  //for future use
  always @(posedge clk) begin
    if (read_data_valid_reg) begin
      jtag_rx <= {jtag_rx[62:0],jtag_req.tdo};
      read <= 1;
    end else begin
      if(read)begin
        $display("final jtag read from tdo=%h",jtag_rx);
      end
    end 
  end
  
  assign jtag_rdatas[0].valid         = read_data_valid_reg;
  assign jtag_rdatas[0].data.location = location;
  assign jtag_rdatas[0].data.rdata     = {32'h0,read_data[31:0]};//upper32 bits for future use

endmodule
