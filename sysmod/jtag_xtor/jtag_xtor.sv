  //******** JTAG ********//
module jtag_xtor(
  input clk,
  input reset,
  /* verilator lint_off MULTIDRIVEN */ 
  /* verilator lint_off UNOPTFLAT */
  input rv_tester_pkg::jtag_if_out  jtag_resp,
  output rv_tester_pkg::jtag_if_t   jtag_req,
  /* verilator lint_on UNOPTFLAT */
  /* verilator lint_off MULTIDRIVEN */ 
  input  bit[1:0]  command,
  input        jtag_enable,
  /* verilator lint_off MULTIDRIVEN */ 
  output reg       read_data_valid_reg,
  /* verilator lint_off MULTIDRIVEN */ 
  input  bit [63:0] jtag_tx,
  input  bit [63:0] misc_signals, //0th bit is used as tdo_enable
  output bit [63:0] jtag_rx
);


 // JTAG state machine states
typedef enum logic [1:0] {
  IDLE   = 2'b10,
  SHIFT_DR = 2'b01,
  SHIFT_IR = 2'b00,
  UPDATE = 2'b11
} fsm_state_t;

parameter DR_WIDTH = 32'd32;
parameter IR_WIDTH = 32'd4;


  logic read_data_valid = '0;
  bit valid_delayed = '0;
  bit [1:0] delay_counter = '0;
  bit [31:0] counter = '0;

  bit read = '0;
  bit jtag_req_begin = '0;
  bit jtag_req_begin_d = '0;
  bit[1:0]  command_l = '0;
  bit i_en = '0;

  bit [1:0]  state= '0;
  bit [31:0] shiftCount= '0;
  bit ir ='0;
  bit dr ='0;
  // bit delay_tdi ='0;
  // bit delay_tdi1 ='0;
  // bit valid_i ='0;
  // bit valid_i1 ='0;

// JTAG controller

tt_clkgater tt_clkgater0(clk,i_en, 1'b0,jtag_req.tck);


always @(posedge clk) begin
  if (reset) begin
    state <= IDLE;
    shiftCount <= 32'b0;
    read_data_valid <= 1'b0;
    // jtag_req.tck <= 1'b0;
    i_en <= 1'b0;
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
        if (jtag_req_begin) begin 
          // Interpret command and data, set state accordingly
          jtag_req_begin_d <= 1'b1;
          case (command_l)
            2'b10: begin
                    state <= IDLE;
                    i_en <= 1'b0;
                  end
            2'b01: begin
                    state <= SHIFT_DR; // to configure dr
                    i_en <= 1'b1;
                  end
            2'b00:begin
                   state <= SHIFT_IR; // to configure ir
                   i_en <= 1'b1;
                  end
            2'b11:begin 
                    state <= UPDATE;
                    i_en <= 1'b1;
                  end
            default: state <= IDLE;
          endcase
        end
        else begin
          i_en <= 1'b0;
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
        
        if(shiftCount >= 32'd2) begin
          read_data_valid<= 1'b1;
          jtag_req.tdi <= jtag_tx[shiftCount-2];
        end
        shiftCount <= shiftCount + 1;
        if (shiftCount == 32'd1 + DR_WIDTH) begin
          state <= UPDATE;
          command_l <= UPDATE;
          shiftCount <=0;
        end
        i_en <= 1'b1;
        // jtag_req.tck <= 1'b1;
      end
      SHIFT_IR: begin
        if(shiftCount <= 32'd1)begin
          jtag_req.tms <= 1'b1;
        end 
        else begin
          jtag_req.tms <= 0;
        end
        if(shiftCount >= 32'd3) begin
          read_data_valid<= 1'b1;
          jtag_req.tdi <= jtag_tx[shiftCount-3];
        end

        ir <=  1'b1;
        
        shiftCount <= shiftCount + 1;
        if (shiftCount == 32'd2 + IR_WIDTH) begin
          state <= UPDATE;
          command_l <= UPDATE;
          shiftCount <=0;
        end
        i_en <= 1'b1;
      end
      UPDATE: begin
        
        jtag_req.tdi <= 1'b0;

        shiftCount <= shiftCount + 1;
        if (shiftCount <= 32'd4) begin
          jtag_req.tms <= 1'b1;
        end

        if (shiftCount == 32'd5) begin
          jtag_req.tms <= 1'b0;
          state <= IDLE;
          shiftCount <= 0;
        end
        
        read_data_valid <= 1'b0;
        i_en <= 1'b1;
      end
      default: begin 
        state <= IDLE;
        i_en <= 1'b0;
      end

    endcase
    /* verilator lint_on CASEINCOMPLETE */
  end
end

//driving tdo by driver
always @(posedge jtag_resp.tdo_en) begin
  counter <= 32'b0;
end
// always @(posedge clk) begin
//   if (reset) begin
//     jtag_resp.tdo <= 1'b0; // Reset to a known state
//   end else begin
//     delay_tdi <= jtag_req.tdi;
//     valid_i <= read_data_valid;
//     delay_tdi1 <= delay_tdi;
//     valid_i1 <= valid_i;
//     // jtag_resp.tdo <= delay_tdi1;
//     // jtag_resp.tdo_en <= valid_i1; 
//   end
// end



//for future use
always @(posedge clk) begin
  if (ir && jtag_resp.tdo_en) begin
    jtag_rx <= {jtag_rx[63:4],jtag_resp.tdo,jtag_rx[3:1]};
    read <= 1;
  end else if (dr && jtag_resp.tdo_en) begin
    jtag_rx <= {jtag_rx[63:32],jtag_resp.tdo,jtag_rx[31:1]};
    read <= 1;
  end else begin
    if(read)begin
      // $display("final jtag read from tdo=%h",jtag_rx);
      jtag_rx <= 0;
    end
  end 

  if(read_data_valid_reg == 1'b1)begin
    read_data_valid_reg <= 1'b0; 
  end
end

always @(posedge clk) begin
  if(jtag_resp.tdo_en && ir && counter == 32'd3)begin
    read_data_valid_reg <= 1'b1; 
  end   
  else if(jtag_resp.tdo_en && dr && counter == 32'd31)begin
    read_data_valid_reg <= 1'b1; 
  end else begin
    read_data_valid_reg <= 1'b0; 
  end  
end

always @(posedge clk)begin
  if (jtag_resp.tdo_en ) begin
    counter <= counter + 32'b1;
  end 
end

endmodule
