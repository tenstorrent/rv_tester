
  //******** JTAG ********//
module jtag_xtor
#(
    parameter int JTAG_DR_WIDTH             =      70
)(
  input clk,
  input reset,
  input bit [31:0] tap_sel,
  input rv_tester_pkg::jtag_if_out  jtag_resp,
  output rv_tester_pkg::jtag_if_t   jtag_req,
  output rv_tester_pkg::jtag_if_tck   jtag_tck_trst,
  input  bit[1:0]  command,
  input        jtag_enable,
  /* verilator lint_off MULTIDRIVEN */ 
  output reg       read_data_valid_reg,
  /* verilator lint_off MULTIDRIVEN */ 
  output bit jtag_busy,
  input bit [31:0] length,
  input  bit [JTAG_DR_WIDTH-1:0] jtag_tx,
  input  bit [63:0] misc_signals, 
  output bit [JTAG_DR_WIDTH-1:0] jtag_rx
);


 // JTAG state machine states
typedef enum logic [1:0] {
  IDLE   = 2'b10,
  SHIFT_DR = 2'b01,
  SHIFT_IR = 2'b00,
  UPDATE = 2'b11
} fsm_state_t;


  logic read_data_valid = '0;
  bit [31:0] counter = '0;
  bit [31:0] delay_counter = '0;

  bit read = '0;
  bit jtag_req_begin = '0;
  bit jtag_req_begin_d = '0;
  bit[1:0]  command_l = '0;

  bit [1:0]  state= 2'b10;
  bit [31:0] shiftCount= '0;
  bit ir ='0;
  bit dr ='0;
  
  bit pos_tdo_en;
  assign pos_tdo_en= ~jtag_resp.tdo_en;

  assign jtag_tck_trst.tck = clk;
  assign jtag_tck_trst.trst = reset;


always @(posedge clk) begin
  if (reset) begin
    state <= IDLE;
    shiftCount <= 32'b0;
    read_data_valid <= 1'b0;
    delay_counter <= 32'b0;
    jtag_req.tms <= 1'b0;
    jtag_req.tdi <= 1'b0;
  end else begin
    /* verilator lint_off CASEINCOMPLETE */
    if(jtag_req_begin_d)begin
      jtag_req_begin <= 1'b0;
      jtag_req_begin_d <= 1'b0;
    end 
    else if(jtag_enable)begin
      jtag_req_begin <= 1'b1;
      command_l <= command;
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
          jtag_busy <= 1'b1;
        end
        if (jtag_req_begin && delay_counter >= 32'd10) begin 
          // Interpret command and data, set state accordingly
          jtag_req_begin_d <= 1'b1;
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
          jtag_busy <= 1'b0;
        
        end
        
        read_data_valid <= 1'b0;
      end
      default: state <= IDLE;
    endcase
    /* verilator lint_on CASEINCOMPLETE */
  end
end

//driving tdo 
always @(posedge pos_tdo_en) begin
  counter <= 32'b0;
end

//for future use
always @(posedge clk) begin
  if (ir && ~jtag_resp.tdo_en) begin
    jtag_rx <= {jtag_rx[JTAG_DR_WIDTH-1:5],jtag_resp.tdo,jtag_rx[4:1]};
    read <= 1;
  end else if (dr && ~jtag_resp.tdo_en && (tap_sel == 1) ) begin  //DTM
    read_data_valid_reg <= 1'b0; 
    jtag_rx <= {jtag_rx[JTAG_DR_WIDTH-1:42],jtag_resp.tdo,jtag_rx[41 :1 ]};
    read <= 1;
  end else if (dr && ~jtag_resp.tdo_en && (tap_sel == 3) ) begin  //aclint
    read_data_valid_reg <= 1'b0; 
    jtag_rx <= {jtag_resp.tdo,jtag_rx[69 :1 ]};
    read <= 1;
  end else if (dr && ~jtag_resp.tdo_en && (tap_sel == 4) ) begin  //pmnw
    read_data_valid_reg <= 1'b0; 
    jtag_rx <= {jtag_rx[JTAG_DR_WIDTH-1],jtag_resp.tdo,jtag_rx[68 :1]};
    read <= 1;
  end else if (dr && ~jtag_resp.tdo_en && (tap_sel == 5)) begin  //smc
    read_data_valid_reg <= 1'b0; 
    jtag_rx <= {jtag_rx[JTAG_DR_WIDTH-1:38],jtag_resp.tdo,jtag_rx[37:1]};
    read <= 1;
  end else if (dr && ~jtag_resp.tdo_en && (tap_sel == 6)) begin  //trace 
    read_data_valid_reg <= 1'b0; 
    jtag_rx <= {jtag_rx[JTAG_DR_WIDTH-1:67],jtag_resp.tdo,jtag_rx[66 :1]};
    read <= 1;
  end else if (dr && ~jtag_resp.tdo_en && (tap_sel == 2)) begin      //axi
    read_data_valid_reg <= 1'b0; 
    jtag_rx <= {jtag_rx[JTAG_DR_WIDTH-1 : 66],jtag_resp.tdo,jtag_rx[65 :1]};
    read <= 1;
  end else if (dr && ~jtag_resp.tdo_en && (tap_sel == 7)) begin      //core h2 [TODO] update with correct behavior of H2 path
    read_data_valid_reg <= 1'b0; 
    jtag_rx <= {jtag_rx[JTAG_DR_WIDTH-1 : 66],jtag_resp.tdo,jtag_rx[65 :1]};
    read <= 1;
  end else begin
    if(read)begin
      $display("final jtag read from tdo=%h at time = %t",jtag_rx[63:0],$time);
      read_data_valid_reg <= 1'b1; 
      read <= 0;
    end
  end 

  if(read_data_valid_reg == 1'b1)begin
    read_data_valid_reg <= 1'b0; 
  end
end

//always @(posedge clk) begin
//  if(jtag_resp.tdo_en && ir && counter == 32'd3)begin
//    read_data_valid_reg <= 1'b1; 
//  end   
//  else if(jtag_resp.tdo_en && dr && counter == 32'd31)begin
//    read_data_valid_reg <= 1'b1; 
//  end else begin
//    read_data_valid_reg <= 1'b0; 
//  end  
//end
//
//always @(posedge clk)begin
//  if (jtag_resp.tdo_en ) begin
//    counter <= counter + 32'b1;
//  end 
//end

endmodule

