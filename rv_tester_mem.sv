///////////////includes///////////////////////////////

`include "axi_llc/typedef.svh"

/////////////////////////////////////////////////////

module rv_tester_mem #(
//number of AXI masters
parameter int unsigned NumMasters           = 32'd1,
//AXI id width
parameter int unsigned AxiIdWidth           = 32'd8,
//AXI data width
parameter int unsigned AxiDataWidth         = 32'd64,
//AXI addr width
parameter int unsigned AxiAddrWidth         = 32'd64,
//AXI strobe width
parameter int unsigned AxiStrbWidth         = 32'd8,
//AXI user width
parameter int unsigned AxiUserWidth         = 32'd0,
//Num of lines in cache, need to be a power of 2
parameter int unsigned NumLines_LLC         = 32'd8,
//number of blocks in per line in cache, needs to power of 2
parameter int unsigned NumBlocks_LLC        = 32'd256,
//internal register width
parameter int unsigned RegWidth_LLC         = 64,
//set associativity LLC
parameter int unsigned SetAssociativity_LLC = 32'd1,
// AXI4+ATOP request struct type for the slave ports.
parameter type slv_req_t                    = logic,
// AXI4+ATOP response struct type for the slave ports.
parameter type slv_resp_t                   = logic,
// AXI4+ATOP request struct type for the master ports.
parameter type mst_req_t                    = logic,
// AXI4+ATOP response struct type for the master ports
parameter type mst_resp_t                   = logic
)
(

input   logic                            clk,
input   logic                            rst_n,
//from core
input   slv_req_t   [NumMasters-1:0]     axi_req,        
output  slv_resp_t  [NumMasters-1:0]     axi_resp,     
//to main memory
output  mst_req_t                        axi_req_mst,    
input   mst_resp_t                       axi_resp_mst  

);

////////////////local parameters/////////////////////

localparam int unsigned Pipeline              = 32'b1;
localparam int unsigned AxiIdUsed             = 32'd3;
localparam bit UniqueIds                      = 1'b0;
localparam int unsigned NumSlaves             = 32'd1;
localparam bit ATOPS                          = 1'b1;
localparam int unsigned AxiIdWidthMst         = AxiIdWidth + $clog2(NumMasters);

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



typedef axi_pkg::xbar_rule_64_t         rule_t_64; // Has to be the same width as axi addr

localparam rule_t_64 [xbar_cfg.NoAddrRules-1:0] AddrMap_64 = addr_map_gen_64();

//assumed only one slave here
function rule_t_64 [xbar_cfg.NoAddrRules-1:0] addr_map_gen_64 ();
    for (int unsigned i = 0; i < xbar_cfg.NoAddrRules; i++) begin
      addr_map_gen_64[i] = rule_t_64'{
        idx:        unsigned'(i),
        start_addr:  i    * 64'h0000_0000_0000_0000,
        end_addr:   ({32'b0,i}+64'd1) * 64'hffff_ffff_ffff_ffff,
        default:    '0
      };
    end
endfunction

typedef axi_pkg::xbar_rule_32_t         rule_t_32; // Has to be the same width as axi addr

localparam rule_t_32 [xbar_cfg.NoAddrRules-1:0] AddrMap_32 = addr_map_gen_32();

//assumed only one slave here
function rule_t_32 [xbar_cfg.NoAddrRules-1:0] addr_map_gen_32 ();
    for (int unsigned i = 0; i < xbar_cfg.NoAddrRules; i++) begin
      addr_map_gen_32[i] = rule_t_32'{
        idx:        unsigned'(i),
        start_addr:  i    * 32'h0000_0000,
        end_addr:   (i+32'd1) * 32'hffff_ffff,
        default:    '0
      };
    end
endfunction


//for LLC 
//Address ranges
typedef logic [AxiAddrWidth-1:0] axi_addr_t;
localparam axi_addr_t SpmRegionStart     = axi_addr_t'(0);
localparam axi_addr_t SpmRegionLength    = axi_addr_t'(SetAssociativity_LLC * NumLines_LLC * NumBlocks_LLC * AxiDataWidth / 64'd8);
localparam axi_addr_t CachedRegionStart  = axi_addr_t'(32'h8000_0000);
localparam axi_addr_t CachedRegionLength = axi_addr_t'(2*SpmRegionLength);
axi_llc_pkg::events_t llc_events;

//////////////////////////////////////////

/////////////axi_interconnect/////////////

typedef logic [AxiIdWidthMst-1:0] id_mst_xbar;
typedef logic [AxiIdWidth-1:0] id_slv_xbar;
typedef logic [AxiAddrWidth-1:0] addr_xbar;
typedef logic [AxiDataWidth-1:0] data_xbar;
typedef logic [AxiStrbWidth-1:0] strb_xbar;
typedef logic [AxiUserWidth-1:0] user_xbar;

`AXI_TYPEDEF_AW_CHAN_T(mst_aw_chan_xbar, addr_xbar, id_mst_xbar, user_xbar)
`AXI_TYPEDEF_AW_CHAN_T(slv_aw_chan_xbar, addr_xbar, id_slv_xbar, user_xbar)
`AXI_TYPEDEF_W_CHAN_T(w_chan_xbar, data_xbar, strb_xbar, user_xbar)
`AXI_TYPEDEF_B_CHAN_T(mst_b_chan_xbar, id_mst_xbar, user_xbar)
`AXI_TYPEDEF_B_CHAN_T(slv_b_chan_xbar, id_slv_xbar, user_xbar)
`AXI_TYPEDEF_AR_CHAN_T(mst_ar_chan_xbar, addr_xbar, id_mst_xbar, user_xbar)
`AXI_TYPEDEF_AR_CHAN_T(slv_ar_chan_xbar, addr_xbar, id_slv_xbar, user_xbar)
`AXI_TYPEDEF_R_CHAN_T(mst_r_chan_xbar, data_xbar, id_mst_xbar, user_xbar)
`AXI_TYPEDEF_R_CHAN_T(slv_r_chan_xbar, data_xbar, id_slv_xbar, user_xbar)
`AXI_TYPEDEF_REQ_T(mst_req_xbar, mst_aw_chan_xbar, w_chan_xbar, mst_ar_chan_xbar)
`AXI_TYPEDEF_RESP_T(mst_resp_xbar, mst_b_chan_xbar, mst_r_chan_xbar)


mst_req_xbar llc_req_t;
mst_resp_xbar llc_resp_t;

generate
if(AxiAddrWidth == 32) begin
  axi_xbar #(
    .Cfg  (xbar_cfg),
    .slv_aw_chan_t  ( slv_aw_chan_xbar ),
    .mst_aw_chan_t  ( mst_aw_chan_xbar ),
    .w_chan_t       ( w_chan_xbar      ),
    .slv_b_chan_t   ( slv_b_chan_xbar  ),
    .mst_b_chan_t   ( mst_b_chan_xbar  ),
    .slv_ar_chan_t  ( slv_ar_chan_xbar ),
    .mst_ar_chan_t  ( mst_ar_chan_xbar ),
    .slv_r_chan_t   ( slv_r_chan_xbar  ),
    .mst_r_chan_t   ( mst_r_chan_xbar  ),
    .slv_req_t      ( slv_req_t     ),
    .slv_resp_t     ( slv_resp_t    ),
    .mst_req_t      ( mst_req_xbar     ),
    .mst_resp_t     ( mst_resp_xbar    ),
    .rule_t         ( rule_t_32        )
  ) i_xbar (
    .clk_i                  ( clk ),
    .rst_ni                 ( rst_n ),
    .test_i                 ( 1'b0 ),
    .slv_ports_req_i        ( axi_req ),
    .slv_ports_resp_o       ( axi_resp ),
    .mst_ports_req_o        ( llc_req_t ),
    .mst_ports_resp_i       ( llc_resp_t ),
    .addr_map_i             ( AddrMap_32 ),
    .en_default_mst_port_i  ( '0 ),
    .default_mst_port_i     ( '0 )
  );
end else begin
  axi_xbar #(
    .Cfg  (xbar_cfg),
    .slv_aw_chan_t  ( slv_aw_chan_xbar ),
    .mst_aw_chan_t  ( mst_aw_chan_xbar ),
    .w_chan_t       ( w_chan_xbar      ),
    .slv_b_chan_t   ( slv_b_chan_xbar  ),
    .mst_b_chan_t   ( mst_b_chan_xbar  ),
    .slv_ar_chan_t  ( slv_ar_chan_xbar ),
    .mst_ar_chan_t  ( mst_ar_chan_xbar ),
    .slv_r_chan_t   ( slv_r_chan_xbar  ),
    .mst_r_chan_t   ( mst_r_chan_xbar  ),
    .slv_req_t      ( slv_req_t     ),
    .slv_resp_t     ( slv_resp_t    ),
    .mst_req_t      ( mst_req_xbar     ),
    .mst_resp_t     ( mst_resp_xbar    ),
    .rule_t         ( rule_t_64        )
  ) i_xbar (
    .clk_i                  ( clk ),
    .rst_ni                 ( rst_n ),
    .test_i                 ( 1'b0 ),
    .slv_ports_req_i        ( axi_req ),
    .slv_ports_resp_o       ( axi_resp ),
    .mst_ports_req_o        ( llc_req_t ),
    .mst_ports_resp_i       ( llc_resp_t ),
    .addr_map_i             ( AddrMap_64 ),
    .en_default_mst_port_i  ( '0 ),
    .default_mst_port_i     ( '0 )
  );
  end
endgenerate
///////////////////////////////////////////


///////////////////LLC/////////////////////





`AXI_LLC_TYPEDEF_ALL(axi_llc, logic [63:0], logic[SetAssociativity_LLC-1:0])

axi_llc_cfg_regs_d_t     reg_cfg_hw_to_reg;
axi_llc_cfg_regs_q_t     reg_cfg_reg_to_hw;

assign reg_cfg_reg_to_hw.cfg_spm     = {SetAssociativity_LLC{1'b0}};
assign reg_cfg_reg_to_hw.cfg_flush   = {SetAssociativity_LLC{1'b0}};
assign reg_cfg_reg_to_hw.commit_cfg  = 1'b0;
assign reg_cfg_reg_to_hw.flushed     = {SetAssociativity_LLC{1'b0}};

generate
  if(AxiAddrWidth == 32) begin
    axi_llc_top #(
    .SetAssociativity         ( SetAssociativity_LLC ),
    .NumLines                 ( NumLines_LLC ),
    .NumBlocks                ( NumBlocks_LLC ),
    .AxiIdWidth               ( AxiIdWidthMst ),
    .AxiAddrWidth             ( AxiAddrWidth ),
    .AxiDataWidth             ( AxiDataWidth ),
    .AxiUserWidth             ( AxiUserWidth ),
    .RegWidth                 ( RegWidth_LLC ),
    .conf_regs_d_t            ( axi_llc_cfg_regs_d_t ),
    .conf_regs_q_t            ( axi_llc_cfg_regs_q_t ),
    .slv_req_t                ( mst_req_xbar ),
    .slv_resp_t               ( mst_resp_xbar ),
    .mst_req_t                ( mst_req_t ),
    .mst_resp_t               ( mst_resp_t ),
    .rule_full_t              ( axi_pkg::xbar_rule_32_t ),
    .PrintSramCfg             ( 0 ),
    .PrintLlcCfg              ( 0 ),
    .axi_addr_t               ( logic[AxiAddrWidth-1:0] ),
    .way_ind_t                ( logic[SetAssociativity_LLC-1:0] )
  ) llc(
    .clk_i                ( clk ),
    .rst_ni               ( rst_n ),
    .test_i               ( 1'b0 ),
    .slv_req_i            ( llc_req_t ), 
    .slv_resp_o           ( llc_resp_t ),
    .mst_req_o            ( axi_req_mst ),
    .mst_resp_i           ( axi_resp_mst ),
    .conf_regs_i          ( reg_cfg_reg_to_hw ),
    .conf_regs_o          ( reg_cfg_hw_to_reg ), 
    .cached_start_addr_i  ( CachedRegionStart ),
    .cached_end_addr_i    ( CachedRegionStart + CachedRegionLength ),
    .spm_start_addr_i     ( SpmRegionStart ), 
    .axi_llc_events_o     ( llc_events )
  );
end else begin
  axi_llc_top #(
    .SetAssociativity         ( SetAssociativity_LLC ),
    .NumLines                 ( NumLines_LLC ),
    .NumBlocks                ( NumBlocks_LLC ),
    .AxiIdWidth               ( AxiIdWidthMst ),
    .AxiAddrWidth             ( AxiAddrWidth ),
    .AxiDataWidth             ( AxiDataWidth ),
    .AxiUserWidth             ( AxiUserWidth ),
    .RegWidth                 ( RegWidth_LLC ),
    .conf_regs_d_t            ( axi_llc_cfg_regs_d_t ),
    .conf_regs_q_t            ( axi_llc_cfg_regs_q_t ),
    .slv_req_t                ( mst_req_xbar ),
    .slv_resp_t               ( mst_resp_xbar ),
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
    .slv_req_i            ( llc_req_t ), 
    .slv_resp_o           ( llc_resp_t ),
    .mst_req_o            ( axi_req_mst ),
    .mst_resp_i           ( axi_resp_mst ),
    .conf_regs_i          ( reg_cfg_reg_to_hw ),
    .conf_regs_o          ( reg_cfg_hw_to_reg ), 
    .cached_start_addr_i  ( CachedRegionStart ),
    .cached_end_addr_i    ( CachedRegionStart + CachedRegionLength ),
    .spm_start_addr_i     ( SpmRegionStart ), 
    .axi_llc_events_o     ( llc_events )
  );
end
endgenerate



//////////////////////////////////////


endmodule
