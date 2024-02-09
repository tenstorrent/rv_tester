  //******** JTAG ********//
module jtag_xtor(
    input clk,
    input reset,
    output rv_tester_pkg::jtag_if_t  jtag_req,
    input  bit[1:0]  command,
    input        jtag_enable,
    output        read_data_valid_reg,
    input  bit [63:0] jtag_tx,
    input  bit [63:0] misc_signals,
    output bit [63:0] jtag_rx
);


  // JTAG state machine states
  parameter IDLE = 2'b00;
  parameter SHIFT_DR = 2'b01;
  parameter SHIFT_IR = 2'b10;
  parameter UPDATE = 2'b11;
  parameter DR_WIDTH = 32'd32;
  parameter IR_WIDTH = 32'd4;


    bit [31:0] read_data = '0;
    bit read = '0;
    logic [1:0] jtag_cmd = 2'b00;
    logic [31:0] jtag_data_in = 32'h1234abcd;
    bit[3:0] opcode= '0;
    bit ConfigureIR='0;
    bit jtag_req_begin = '0;
    bit jtag_req_begin_d = '0;
    bit jtag_req_end = '0;
    bit[1:0]  command_l = '0;

    bit [63:0] clk_trig = '0;
    bit [1:0]  state= '0;
    bit [31:0] shiftCount= '0;
  // JTAG controller
  always @(posedge clk) begin
    if (reset) begin
      state <= IDLE;
      shiftCount <= 32'b0;
      read_data_valid_reg <= 1'b0;
      jtag_req.tdo <= 1'b0;
    end else begin
      /* verilator lint_off CASEINCOMPLETE */
      if(jtag_req_begin_d)begin
        jtag_req_begin <= 1'b0;
      end 
      else if(jtag_enable)begin
        jtag_req_begin <= 1'b1;
        command_l <= command;
      end
      case (state)
        IDLE: begin
          jtag_req.tms <= 1'b0;
          jtag_req.tdi <= 1'b0;
          read_data_valid_reg <= 1'b0;
          shiftCount <= 0;
          if (jtag_req_begin) begin // 
            // Interpret command and data, set state accordingly
            jtag_req_begin_d <= 1'b1;
            case (command_l)
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
          if(shiftCount >= 32'd3) begin
            jtag_req.tdi <= jtag_tx[shiftCount-3];
          end
          
          shiftCount <= shiftCount + 1;
          if (shiftCount == 32'd3 + IR_WIDTH) begin
            state <= UPDATE;
            command_l <= UPDATE;
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
          read_data_valid_reg <= 1'b1;
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
        //$display("final jtag read from tdo=%h",jtag_rx);
      end
    end 
  end

endmodule