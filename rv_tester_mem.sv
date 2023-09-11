///////////////includes///////////////////////////////

`include "axi_llc/typedef.svh"

/////////////////////////////////////////////////////

module rv_tester_mem #(

parameter int unsigned NumMasters           = 32'd1,
parameter int unsigned AxiIdWidth           = 32'd8,
parameter int unsigned AxiDataWidth         = 32'd64,
parameter int unsigned AxiAddrWidth         = 32'd64,
parameter int unsigned AxiStrbWidth          = 32'd8, 
parameter int unsigned SetAssociativity_LLC = 32'd1,
parameter int unsigned NumLines_LLC         = 32'd8,
parameter int unsigned NumBlocks_LLC        = 32'd256,
parameter int unsigned RegWidth_LLC         = 64

)
(

input  logic                            clk,
input  logic                            rst_n,
//from core
input rv_tester_params::axi_req_t       axi_req      [rv_tester_params::AXI_TOTAL-1:0],        
output  rv_tester_params::axi_rsp_t       axi_rsp      [rv_tester_params::AXI_TOTAL-1:0],       
//to main memory
output  rv_tester_params::axi_req_mst_t   axi_req_mst  [rv_tester_params::AXI_MST_TOTAL-1:0],    
input rv_tester_params::axi_rsp_mst_t   axi_rsp_mst  [rv_tester_params::AXI_MST_TOTAL-1:0]

);

////////////////local parameters/////////////////////

//for AXI-Interconnect
localparam int unsigned AxiUserWidth      = 5;
localparam int unsigned Pipeline          = 32'b1;
localparam int unsigned AxiIdUsed         = 32'd3;
localparam bit UniqueIds                  = 1'b0;
localparam int unsigned NumSlaves         = 32'd1;
localparam bit ATOPS                      = 1'b1;
localparam int unsigned AxiIdWidthMst     = AxiIdWidth + $clog2(NumMasters);


localparam axi_pkg::xbar_cfg_t xbar_cfg = '{
    NoSlvPorts:         NumMasters,
    NoMstPorts:         NumSlaves,
    MaxMstTrans:        10,
    MaxSlvTrans:        6,
    FallThrough:        1'b0,
    LatencyMode:        axi_pkg::CUT_ALL_AX,
    PipelineStages:     Pipeline,
    AxiIdWidthSlvPorts: AxiIdWidth,
    AxiIdUsedSlvPorts:  AxiIdUsed,
    UniqueIds:          UniqueIds,
    AxiAddrWidth:       AxiAddrWidth,
    AxiDataWidth:       AxiDataWidth,
    NoAddrRules:        NumSlaves,
    AxiIdWidth:         0,
    AxiSrcIdWidth:      0,
    AxiTxIdWidth:       0,
    AxiUserWidth:       0,
    NoIdRules:          0
  };



typedef axi_pkg::xbar_rule_64_t         rule_t; // Has to be the same width as axi addr

localparam rule_t [xbar_cfg.NoAddrRules-1:0] AddrMap = addr_map_gen();

//assumed only one slave here
function rule_t [xbar_cfg.NoAddrRules-1:0] addr_map_gen ();
    for (int unsigned i = 0; i < xbar_cfg.NoAddrRules; i++) begin
      addr_map_gen[i] = rule_t'{
        idx:        unsigned'(i),
        start_addr:  i    * 64'h0000_0000_0000_0000,
        end_addr:   ({32'b0,i}+64'd1) * 64'hffff_ffff_ffff_ffff,
        default:    '0
      };
    end
endfunction

typedef logic [AxiIdWidth-1:0] axi_id_t;
typedef logic [AxiIdWidthMst-1:0] axi_mst_id_t;
typedef logic [AxiAddrWidth-1:0] axi_addr_t;
typedef logic [AxiAddrWidth-1:0] axi_mst_addr_t;
typedef logic [5:0] axi_atop_t;     
typedef logic [1:0] axi_burst_t;
typedef logic [1:0] axi_resp_t;
typedef logic [7:0] axi_len_t;
typedef logic [2:0] axi_size_t;
typedef logic [AxiDataWidth-1:0] axi_data_t;
typedef logic [AxiDataWidth-1:0] axi_mst_data_t;
typedef logic [AxiStrbWidth-1:0] axi_mst_strb_t;

//for LLC 
//Address ranges
localparam axi_addr_t SpmRegionStart     = axi_addr_t'(0);
localparam axi_addr_t SpmRegionLength    = axi_addr_t'(SetAssociativity_LLC * NumLines_LLC * NumBlocks_LLC * AxiDataWidth / 64'd8);
localparam axi_addr_t CachedRegionStart  = axi_addr_t'(32'h8000_0000);
localparam axi_addr_t CachedRegionLength = axi_addr_t'(2*SpmRegionLength);
axi_llc_pkg::events_t llc_events;

//////////////////////////////////////////

/////////////axi_interconnect/////////////

`AXI_TYPEDEF_AW_CHAN_T_rv(slv_aw_chan_t, axi_id_t, axi_addr_t, axi_len_t, axi_size_t, axi_burst_t, axi_atop_t)
`AXI_TYPEDEF_AW_CHAN_T_rv(mst_aw_chan_t, axi_mst_id_t, axi_mst_addr_t, axi_len_t, axi_size_t, axi_burst_t, axi_atop_t)
`AXI_TYPEDEF_W_CHAN_T_rv(w_chan_t, axi_mst_data_t, axi_mst_strb_t)
`AXI_TYPEDEF_B_CHAN_T_rv(mst_b_chan_t, axi_id_t, axi_resp_t)
`AXI_TYPEDEF_B_CHAN_T_rv(slv_b_chan_t, axi_id_t, axi_resp_t)
`AXI_TYPEDEF_AR_CHAN_T_rv(mst_ar_chan_t, axi_mst_addr_t, axi_mst_id_t, axi_len_t, axi_size_t, axi_burst_t)
`AXI_TYPEDEF_AR_CHAN_T_rv(slv_ar_chan_t, axi_addr_t, axi_id_t, axi_len_t, axi_size_t, axi_burst_t)
`AXI_TYPEDEF_R_CHAN_T_rv(mst_r_chan_t, axi_mst_data_t, axi_mst_id_t, axi_resp_t)
`AXI_TYPEDEF_R_CHAN_T_rv(slv_r_chan_t, axi_data_t, axi_id_t, axi_resp_t)
`AXI_TYPEDEF_REQ_T_rv(mst_req_t, mst_aw_chan_t, w_chan_t, mst_ar_chan_t)
`AXI_TYPEDEF_REQ_T_rv(slv_req_t, slv_aw_chan_t, w_chan_t, slv_ar_chan_t)
`AXI_TYPEDEF_RESP_T_rv(mst_resp_t, mst_b_chan_t, mst_r_chan_t)
`AXI_TYPEDEF_RESP_T_rv(slv_resp_t, slv_b_chan_t, slv_r_chan_t)

mst_req_t   mst_reqs;
mst_resp_t  mst_resps;
slv_req_t   [NumMasters-1:0]  slv_reqs;
slv_resp_t  [NumMasters-1:0]  slv_resps;

for (genvar i = 0; i < NumMasters; i++) begin : gen_assign_mst
    `ASSIGN_TO_REQ( slv_reqs[i], axi_req[i]  )
    `ASSIGN_TO_RESP( axi_rsp[i], slv_resps[i] )
end

axi_xbar #(
    .Cfg  (xbar_cfg),
    .slv_aw_chan_t  ( slv_aw_chan_t ),
    .mst_aw_chan_t  ( mst_aw_chan_t ),
    .w_chan_t       ( w_chan_t      ),
    .slv_b_chan_t   ( slv_b_chan_t  ),
    .mst_b_chan_t   ( mst_b_chan_t  ),
    .slv_ar_chan_t  ( slv_ar_chan_t ),
    .mst_ar_chan_t  ( mst_ar_chan_t ),
    .slv_r_chan_t   ( slv_r_chan_t  ),
    .mst_r_chan_t   ( mst_r_chan_t  ),
    .slv_req_t      ( slv_req_t     ),
    .slv_resp_t     ( slv_resp_t    ),
    .mst_req_t      ( mst_req_t     ),
    .mst_resp_t     ( mst_resp_t    ),
    .rule_t         ( rule_t        )
  ) i_xbar (
    .clk_i                  ( clk ),
    .rst_ni                 ( rst_n ),
    .test_i                 ( 1'b0 ),
    .slv_ports_req_i        ( slv_reqs ),
    .slv_ports_resp_o       ( slv_resps ),
    .mst_ports_req_o        ( mst_reqs ),
    .mst_ports_resp_i       ( mst_resps ),
    .addr_map_i             ( AddrMap ),
    .en_default_mst_port_i  ( '0 ),
    .default_mst_port_i     ( '0 )
  );


///////////////////////////////////////////


///////////////////LLC/////////////////////


mst_req_t  axi_mem_req_llc;
mst_resp_t axi_mem_res_llc;

`AXI_LLC_TYPEDEF_ALL(axi_llc, logic [63:0], logic[SetAssociativity_LLC-1:0])

axi_llc_cfg_regs_d_t     reg_cfg_hw_to_reg;
axi_llc_cfg_regs_q_t     reg_cfg_reg_to_hw;

assign reg_cfg_reg_to_hw.cfg_spm     = 0;
assign reg_cfg_reg_to_hw.cfg_flush   = 0;
assign reg_cfg_reg_to_hw.commit_cfg  = 0;
assign reg_cfg_reg_to_hw.flushed     = 0;

axi_llc_top #(
.SetAssociativity         ( SetAssociativity_LLC ),
.NumLines                 ( NumLines_LLC ),
.NumBlocks                ( NumBlocks_LLC ),
.AxiIdWidth               ( AxiIdWidth ),
.AxiAddrWidth             ( AxiAddrWidth ),
.AxiDataWidth             ( AxiDataWidth ),
.AxiUserWidth             ( AxiUserWidth ),
.RegWidth                 ( RegWidth_LLC ),
.conf_regs_d_t            ( axi_llc_cfg_regs_d_t ),
.conf_regs_q_t            ( axi_llc_cfg_regs_q_t ),
.slv_req_t                ( slv_req_t ),
.slv_resp_t               ( slv_resp_t ),
.mst_req_t                ( mst_req_t ),
.mst_resp_t               ( mst_resp_t ),
.rule_full_t              ( axi_pkg::xbar_rule_64_t ),
.PrintSramCfg             ( 0 ),
.PrintLlcCfg              ( 0 ),
.axi_addr_t               ( logic[AxiAddrWidth-1:0] ),
.way_ind_t                ( logic[SetAssociativity_LLC-1:0] )
) llc(
.clk_i                ( clk ),
.rst_ni               ( rst_n ),
.test_i               ( 1'b0 ),
.slv_req_i            ( mst_reqs ), 
.slv_resp_o           ( mst_resps ),
.mst_req_o            ( axi_mem_req_llc ),
.mst_resp_i           ( axi_mem_res_llc ),
.conf_regs_i          ( reg_cfg_reg_to_hw ),
.conf_regs_o          ( reg_cfg_hw_to_reg ), 
.cached_start_addr_i  ( CachedRegionStart ),
.cached_end_addr_i    ( CachedRegionStart + CachedRegionLength ),
.spm_start_addr_i     ( SpmRegionStart ), 
.axi_llc_events_o     ( llc_events )
);

`ASSIGN_FROM_REQ( axi_req_mst, axi_mem_req_llc )
`ASSIGN_FROM_RESP( axi_mem_res_llc, axi_rsp_mst )


//////////////////////////////////////


endmodule
