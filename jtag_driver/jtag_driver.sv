module jtag_driver




import rv_tester_params::*;
#(
  parameter int NUM = -1,
  parameter int JTAG_DR_WIDTH             =      1344,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_JTAG_DRIVER_OUTPUT_PARAMS
)
(
  input logic clk,
  input logic reset,
  input logic warm_reset,
  input logic dut_clk,
  input logic dut_reset,
  input logic no_fetch,
  input logic jtag_driver_en,
  output logic jtag_socket,
  output rv_tester_pkg::jtag_if_t   jtag_req,
  input rv_tester_pkg::jtag_if_out  jtag_resp,
  output rv_tester_pkg::jtag_if_tck   jtag_tck_trst,
  output logic jtag_quiesced,

  `RV_TESTER_TRANSACTIONS_JTAG_DRIVER_OUTPUT_PORTS
);
  parameter int unsigned location = cvm_topology_gen::get_location (cvm_topology_gen::mods.TOP.PLATFORM.JTAG_DRIVER.ID, NUM);
  import "DPI-C" context function void jtag_driver_set_scope(int unsigned location);
  import "DPI-C" function bit jtag_driver_get_en_from_plusargs(string mode);
   // -------------------------
  // C++->SV Callbacks
  // -------------------------

  export "DPI-C" function jtag_driver_jtag_socket;
  export "DPI-C" function drive_jtag_req; 
  export "DPI-C" function drive_jtag_req_socket; 

  function void jtag_driver_jtag_socket(bit val);
  endfunction
  
  int unsigned push_idx;
  logic reset_d1;
  bit jtag_socket_en;
  int unsigned jtag_socket_count;
  int unsigned jtag_socket_interval;
  int unsigned jtag_socket_width;

  /////////////////////////////////////
  bit [31:0] tap_sel;
   bit[1:0]  command;
   bit       read_data_valid_reg;
   bit jtag_busy;
   bit [31:0] length;
   bit [JTAG_DR_WIDTH-1:0] jtag_tx;
   bit [63:0] misc_signals; 
   bit [JTAG_DR_WIDTH-1:0] jtag_rx;
  
   bit        jtag_enable_begin;
   bit        jtag_enable_begin_cpp;
   bit        jtag_enable_begin_sv;
   bit        jtag_enable_d = '0;
   bit        jtag_enable_end;
   bit        jtag_test_reset = '0;

   bit        jtag_reset_begin;
   bit        jtag_reset_begin_cpp;
   bit        jtag_reset_begin_sv;
   bit        jtag_soft_reset;
   bit        jtag_hard_reset;
   bit        [2:0] reset_counter;       // Counter for up to 1000 cycles
   bit        [9:0] reset_delay_count;        // Counter for 5 cycles of trst assertion
   bit        [9:0] reset_delay_counter;       
   bit        reset_counter_active;      // Indicates if the reset counter is active
   bit        trst_active;               // Indicates if trst is being asserted
   bit        hard_reset_pending;        // Indicates if a hard reset is pending
   bit        soft_reset_pending;        // Indicates if a soft reset is pending
   


   initial begin
     jtag_enable_end = 0;
     jtag_enable_begin_cpp = 0;
     jtag_enable_begin_sv = 0;
     jtag_quiesced = 1'b0;
     jtag_reset_begin_cpp = 0;
     jtag_reset_begin_sv = 0;
   end
  

 
   bit jtag_rdatas_jtag_busy;


  ////////////////////////////////////

  always @(posedge clk) begin
    reset_d1 <= reset|warm_reset;
    if (reset || jtag_test_reset || jtag_hard_reset  || !jtag_driver_en) begin
      jtag_req.tms <= '0;
      jtag_req.tdi <= '0;
    end
    if (~(reset|warm_reset) & reset_d1) begin
      if (location != cvm_topology::nil) begin
        jtag_driver_set_scope(location);
        jtag_socket_en <= jtag_driver_get_en_from_plusargs("jtag_driver_mode");
      end
    end
  end

  // -------------------------
  // SV->C++ Messages/Packets
  // -------------------------

  bit read = '0;
  int unsigned tb_clocks = 0;
  bit jtag_socket_start = 0;
  bit jtag_socket_end = 0;
  bit jtag_socket_in_progress = 0;
//  bit jtag_tx_in_progress;
  bit jtag_tx_in_progress_l;
  always @(posedge clk) begin
    if (reset ||jtag_test_reset) begin
      jtag_enable_begin_sv <= jtag_enable_begin_cpp;
      jtag_enable_end <= '0;
    end else begin
      if (jtag_socket_en  && ~|no_fetch) begin
        tb_clocks <= tb_clocks + 1;
        jtag_socket_start <= '0;
        jtag_socket_end <= '0;
      end
       //JTAG
      if(jtag_enable_end)begin
        jtag_enable_begin_sv <= jtag_enable_begin_cpp;
        jtag_enable_end <= '0;
      end
      else if(jtag_enable_begin)begin
        jtag_enable_end <='1;
      end
    end
  end

  int unsigned dut_clocks = 0;
  int unsigned cycles = 0;
  always @(posedge dut_clk) begin
    dut_clocks <= dut_clocks + 1;
    if ((m_jtag_driver_ticks[0].valid) || warm_reset)
      cycles <= 0;
    else
      cycles <= cycles+1;
  end

  // m_jtag_driver_tick
  assign m_jtag_driver_ticks[0].valid = ~dut_reset & ((dut_clocks % 200) == 0) & ~(jtag_busy | jtag_enable_begin) & (cycles!=0) ;
  assign m_jtag_driver_ticks[0].data.location = location;
  assign m_jtag_driver_ticks[0].data.cycle = jtag_socket_en?((jtag_socket_start | jtag_socket_end) ? dut_clocks : '0):cycles;
  
  assign jtag_rdatas[0].valid         = read_data_valid_reg;
  assign jtag_rdatas[0].data.location = location;
  /* verilator lint_off WIDTHEXPAND */
  assign jtag_rdatas[0].data.rdata     = jtag_rx;//upper32 bits for future use


  assign jtag_rdatas_jtag_busy = jtag_busy ;
 // jtag xtor

 // JTAG state machine states
typedef enum logic [1:0] {
  IDLE   = 2'b10,
  SHIFT_DR = 2'b01,
  SHIFT_IR = 2'b00,
  UPDATE = 2'b11
} fsm_state_t;


  logic read_data_valid = '0;
  bit [31:0] delay_counter = '0;

  bit jtag_req_begin = '0;
  bit jtag_req_begin_d = '0;
  bit[1:0]  command_l = '0;

  fsm_state_t  state = IDLE;
  bit [31:0] shiftCount= '0;
  bit ir ='0;
  bit dr ='0;
  
  bit pos_tdo_en;

  assign jtag_pkt_acks[0].valid         = (state == UPDATE);
  assign jtag_pkt_acks[0].data.location = location;
  /* verilator lint_off WIDTHEXPAND */
  assign jtag_pkt_acks[0].data.complete     = (state == UPDATE);//upper32 bits for future use

  /* verilator lint_on WIDTHEXPAND */

  function drive_jtag_req(int unsigned jtag_cmd_ip,longint upper_value,longint lower_value,int unsigned reg_length, int unsigned jtag_quit , int unsigned tap_cfg_sel);

    if(jtag_quit === 0 )begin
      jtag_enable_begin_cpp = (jtag_enable_begin_cpp ^ 1'b1);
      command = jtag_cmd_ip[1:0];
      /* verilator lint_off WIDTHEXPAND */
      jtag_tx = {upper_value[5:0],lower_value};
      /* verilator lint_on WIDTHEXPAND */
      tap_sel = tap_cfg_sel;
      length = reg_length[31:0];
      jtag_quiesced = 1'b0;
      $display("[JTAG_DRIVER.SV] JTAG driver %h %h %h %h %h",upper_value, lower_value,reg_length,tap_sel,tap_cfg_sel);
    end
    else if(jtag_quit === 1 )begin
      jtag_quiesced = 1'b1;
      $display("[JTAG_DRIVER.SV] JTAG quit was given in %0d %t",jtag_quit,$time);
    end
    /* verilator lint_off WIDTHEXPAND */
    else if(jtag_quit === 2 )begin
    /* verilator lint_on WIDTHEXPAND */
      //jtag_hard_reset = 1'b1;
      jtag_reset_begin_cpp =  (jtag_reset_begin_cpp ^ 1'b1);
      hard_reset_pending = 1'b1;
      soft_reset_pending = 1'b0;
      reset_delay_count = lower_value[9:0]; // Set the delay counter for hard reset
      $display("[JTAG_DRIVER.SV] JTAG hard reset was requested in %0d %t",jtag_quit,$time);
    end
    /* verilator lint_off WIDTHEXPAND */
    else if(jtag_quit === 3 )begin
    /* verilator lint_on WIDTHEXPAND */
      //jtag_soft_reset = 1'b1;
      jtag_reset_begin_cpp =  (jtag_reset_begin_cpp ^ 1'b1);
      soft_reset_pending = 1'b1;
      hard_reset_pending = 1'b0;
      reset_delay_count = lower_value[9:0]; // Set the delay counter for hard reset
      $display("[JTAG_DRIVER.SV] JTAG soft reset was requested in %0d %t",jtag_quit,$time);
    end
  endfunction

  function drive_jtag_req_socket(int unsigned jtag_cmd_ip,longint unsigned lower_value[21],int unsigned reg_length, int unsigned jtag_quit , int unsigned tap_cfg_sel);
    if(jtag_quit === 0 )begin
      jtag_enable_begin_cpp = (jtag_enable_begin_cpp ^ 1'b1);
      command = jtag_cmd_ip[1:0];
      jtag_tx = {lower_value[20], lower_value[19], lower_value[18], lower_value[17], lower_value[16], 
      lower_value[15], lower_value[14], lower_value[13], lower_value[12], lower_value[11],
      lower_value[10], lower_value[9], lower_value[8], lower_value[7], lower_value[6], 
      lower_value[5], lower_value[4], lower_value[3], lower_value[2], lower_value[1], 
      lower_value[0]};
      tap_sel = tap_cfg_sel;
      length = reg_length[31:0];
      jtag_quiesced = 1'b0;
      $display("[JTAG_DRIVER.SV] JTAG driver socket  %h %h %h %h at time = %t\n", lower_value[0],reg_length,tap_sel,tap_cfg_sel, $time);

      /*
      $display("[JTAG_DRIVER.SV] JTAG DRIVER socket jtag_tx %h \n",jtag_tx);
      for(int i=0;i<21;i++)begin
      $display("[JTAG_DRIVER.SV] JTAG DRIVER socket input data [%d]=%h \n",i,lower_value[i]);
      end
      */

    end
    else if(jtag_quit === 1 )begin
      jtag_quiesced = 1'b1;
      $display("[JTAG_DRIVER.SV] JTAG quit was given in %0d %t",jtag_quit,$time);
    end
  endfunction
  assign pos_tdo_en= ~jtag_resp.tdo_en;

  assign jtag_tck_trst.tck = jtag_driver_en ? clk: 1'b0;
  assign jtag_tck_trst.trst = (reset & ~((dut_clocks > 30) && (dut_clocks <100))) || trst_active;

assign jtag_enable_begin = jtag_enable_begin_cpp ^ jtag_enable_begin_sv;
assign jtag_reset_begin = jtag_reset_begin_cpp ^ jtag_reset_begin_sv;

always @(posedge clk) begin
  if (jtag_reset_begin) begin
        reset_delay_counter <= reset_delay_counter + 1; // Increment counter
        if (reset_delay_counter == reset_delay_count) begin
            reset_delay_counter <= 0;
            jtag_reset_begin_sv <= jtag_reset_begin_cpp;
          if (hard_reset_pending) begin
            jtag_hard_reset <= 1'b1; // Set the hard reset signal
            $display("[JTAG_DRIVER.SV] Hard reset asserted at time = %t", $time);
          end else if (soft_reset_pending) begin
            jtag_soft_reset <= 1'b1; // Set soft reset signal
            $display("[JTAG_DRIVER.SV] Soft reset asserted at time = %t", $time);
          end
        end
  end
  if (!jtag_driver_en) begin
    jtag_req.tms <= 1'b0;
    jtag_req.tdi <= 1'b0;
    trst_active  <=  1'b1;
  end else if (reset || jtag_hard_reset || jtag_soft_reset) begin
    state <= IDLE;
    shiftCount <= 32'b0;
    read_data_valid <= 1'b0;
    delay_counter <= 32'b0;
    jtag_req.tdi <= 1'b0;
   
    if (jtag_hard_reset) begin
      if (reset_counter == 0) begin
        reset_counter <= 3'b101; // Initialize counter for 5 cycles
        trst_active <= 1'b1; // Pull trst pin down
      end else begin
        reset_counter <= reset_counter - 1; // Decrement counter
        if (reset_counter == 1) begin
          trst_active <= 1'b0; // Release trst pin after 5 cycles
          jtag_hard_reset <= 1'b0; // Clear hard reset signal
          jtag_req.tms <= 1'b0;
        end
      end
    end else if (jtag_soft_reset) begin
      if (reset_counter == 0) begin
        reset_counter <= 3'b101; // Initialize counter for 6 cycles
         jtag_req.tms <= 1'b1; // Keep TMS high for 5 cycles
      end else begin
        reset_counter <= reset_counter - 1; // Decrement counter
        if (reset_counter == 1) begin
            jtag_soft_reset <= 1'b0; // Clear soft reset after 5 cycles
            jtag_req.tms <= 1'b0;
        end
      end
    end else begin
      jtag_req.tms <= 1'b0;
    end
  end else begin
    trst_active   <= 0 ;
    reset_counter <= 0 ;
    /* verilator lint_off CASEINCOMPLETE */
    if(jtag_req_begin_d)begin
      jtag_req_begin <= 1'b0;
      jtag_req_begin_d <= 1'b0;
    end 
    else if(jtag_enable_begin)begin
      jtag_req_begin <= 1'b1;
      command_l <= command;
      jtag_busy <= 1'b1;
    end
    case (state)
      IDLE: begin
        jtag_req.tms <= 1'b0;
        jtag_req.tdi <= 1'b0;
        read_data_valid <= 1'b0;
        shiftCount <= 0;
        if(ir == 1'b1) begin
          ir <=  1'b0;
        end 
        if(dr == 1'b1) begin
          dr <=  1'b0;
        end 
        if(delay_counter < 32'd10) begin
          delay_counter <= delay_counter + 32'b1;
        end
        else if (jtag_req_begin && delay_counter >= 32'd10) begin 
          // Interpret command and data, set state accordingly
          jtag_req_begin_d <= 1'b1;
          delay_counter <= 0;
          case (command_l)
            2'b10: begin
                    state <= IDLE;
                    jtag_busy <= 1'b0;
                  end
            2'b01: begin
                    state <= SHIFT_DR; // to configure dr
                    jtag_busy <= 1'b1;
                  end
            2'b00:begin
                   state <= SHIFT_IR; // to configure ir
                   jtag_busy <= 1'b1;
                  end
            2'b11:begin 
                    state <= UPDATE;
                    jtag_busy <= 1'b1;
                  end
            default: state <= IDLE;
          endcase
        end
        else begin
          jtag_busy <= 1'b0;
        end
      end
      SHIFT_DR: begin
        if(shiftCount == 32'd0)begin
          jtag_req.tms <= 1'b1;
        end 
        else begin
          jtag_req.tms <= 1'b0;
        end

        dr <=  1'b1;
        
        if(shiftCount >= 32'd3) begin
          read_data_valid<= 1'b1;
          jtag_req.tdi <= jtag_tx[shiftCount-3];
        end
        shiftCount <= shiftCount + 1;
        if (shiftCount == 32'd2 + length -1'd1) begin
          state <= UPDATE;
          command_l <= UPDATE;
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
        if(shiftCount >= 32'd4) begin
          read_data_valid<= 1'b1;
          jtag_req.tdi <= jtag_tx[shiftCount-4];
        end

        ir <=  1'b1;
        
        shiftCount <= shiftCount + 1;
        if (shiftCount == 32'd3 + length -1'd1) begin
          state <= UPDATE;
          command_l <= UPDATE;
          shiftCount <=0;
        end
      end
      UPDATE: begin
        
        if (shiftCount == 32'd0) 
           jtag_req.tdi <= jtag_tx[length - 1'd1];
        else
           jtag_req.tdi <= 1'b0;

        shiftCount <= shiftCount + 1;
        if (shiftCount <= 32'd1) begin
          jtag_req.tms <= 1'b1;
        end

        if (shiftCount == 32'd2) begin
          jtag_req.tms <= 1'b0;
          state <= IDLE;
          shiftCount <= 0;
        end
        
        read_data_valid <= 1'b0;
        /* verilator lint_off BLKSEQ */
        //jtag_tx_in_progress = 0; 
        /* verilator lint_on BLKSEQ */
      end
      default: state <= IDLE;
    endcase
    /* verilator lint_on CASEINCOMPLETE */
  end
end

//driving tdo 

always @(posedge clk) begin
  if (reset) begin
    push_idx <= 32'b0;
  end else begin
    if(read_data_valid_reg == 1'b1)begin
      read_data_valid_reg <= 1'b0; 
      jtag_rx <= 1344'b0;
    end
    if (ir && ~jtag_resp.tdo_en) begin
      jtag_rx <= {jtag_rx[JTAG_DR_WIDTH-2:0],jtag_resp.tdo};
      read <= 1;
      read_data_valid_reg <= 1'b0; 
    end else if (dr && ~jtag_resp.tdo_en ) begin  
      read_data_valid_reg <= 1'b1; 
      jtag_rx <= {jtag_rx[JTAG_DR_WIDTH-2:0],jtag_resp.tdo};
      push_idx <= push_idx +1;
      read <= 1;
      read_data_valid_reg <= 1'b0; 
    end else begin
      if(read)begin
        $display("final jtag read from tdo=%h at time = %t",jtag_rx[511:0],$time);
        read_data_valid_reg <= 1'b1; 
        read <= 0;
        push_idx <= 0;
      end
    end
  end
end

jtag_tx_stable_when_not_in_idle: assert property(@(posedge clk) disable iff (!reset || state == IDLE) $stable(jtag_tx))
        else $error("jtag tx data got modified while the data is being shited out");

endmodule
