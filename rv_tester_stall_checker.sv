module ext_mem_stall_checker
import rv_tester_params::*;
#(
)
(
  `_RV_TESTER_STALL_CHECKER_PORTS(input,output)
);
  
  import "DPI-C" function int unsigned ext_mem_rv_tester_get_stall_timeout();
  import "DPI-C" function void stall_checker_rv_tester_error(string error, int unsigned threshold);

  int unsigned ext_mem_stall_timeout = '0;

  logic reset_n_1T;
  always @(posedge clk) begin
    reset_n_1T <= reset_n;
    if (!reset_n) begin
      ext_mem_stall_timeout         <= '0;
    end else if (!reset_n_1T) begin
      // first cycle after reset deassertion
      ext_mem_stall_timeout         <= ext_mem_rv_tester_get_stall_timeout();
    end
  end

  logic [31:0] ext_mem_outstanding_requests;
  logic [31:0] ext_mem_outstanding_response;
  logic [31:0] ext_mem_stall_counter;
  logic read_req, write_req, write_rsp, read_rsp;

  logic fail;
  logic failed = '0;

  assign read_req = axi_req.ar_valid & axi_rsp.ar_ready;
  assign write_req = axi_req.aw_valid & axi_rsp.aw_ready;
  assign write_rsp = axi_rsp.b_valid & axi_req.b_ready;
  assign read_rsp = axi_rsp.r_valid & axi_req.r_ready & axi_rsp.r.last;

  always @(posedge clk) begin
    if(!reset_n || !reset_n_1T) begin
      ext_mem_outstanding_requests <= 0;
      ext_mem_outstanding_response <= 0;
      ext_mem_stall_counter <= 0;
    end else begin
      if(read_req & write_req) ext_mem_outstanding_requests <= ext_mem_outstanding_requests + 2;
      else if(read_req | write_req) ext_mem_outstanding_requests <= ext_mem_outstanding_requests + 1;
      if(read_rsp & write_rsp) ext_mem_outstanding_response <= ext_mem_outstanding_response + 2;
      else if(read_rsp | write_rsp) ext_mem_outstanding_response <= ext_mem_outstanding_response + 1;
      if(ext_mem_outstanding_requests != ext_mem_outstanding_response && !(read_rsp | write_rsp)) ext_mem_stall_counter <= ext_mem_stall_counter + 1;
      else ext_mem_stall_counter <= 0;
      failed <= failed || fail;
    end
  end

  assign fail = reset_n && reset_n_1T && ext_mem_stall_timeout != '0 && ext_mem_stall_counter > ext_mem_stall_timeout;

  always @(posedge clk) begin
    if (fail && !failed) begin
      /* verilator lint_off WIDTH */
        stall_checker_rv_tester_error("External Mem/IO timeout", ext_mem_stall_timeout);
      /* verilator lint_on WIDTH */
    end
  end


endmodule
