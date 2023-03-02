`include "covergroups/cov_model.sv"

`ifndef COVERAGE_UNSUPPORTED
typedef struct {
  longint unsigned cp;
  longint unsigned val;
} cp_pkt;
`endif

module arch_sample#( 
    parameter int NUM = -1
)(
    input clk,
    input reset
);

`ifndef COVERAGE_UNSUPPORTED
  cg_privilege_mode           cg0 = new();
  //cg_pagingmode               cg0_paging = new();
  cg_fpaginglevel             cg1_paging = new();
  cg_dpaginglevel             cg2_paging = new();
`endif 

    always @(posedge clk) begin
        if (reset) begin
          // * // 
        end
    end

  function void cov_sample(
      longint unsigned cp,
      longint unsigned val
  );
`ifndef COVERAGE_UNSUPPORTED
    case($unsigned(cp))
      POINT_FPAGINGLEVEL : begin
        $cast(fpaginglevel_var,  val);
        cg1_paging.sample(fpaginglevel_var);
      end
      POINT_DPAGINGLEVEL : begin
        $cast(dpaginglevel_var,  val);
        cg2_paging.sample(dpaginglevel_var); 
      end
    endcase 
`endif
  endfunction  

  export "DPI-C" function cov_sample;

endmodule
