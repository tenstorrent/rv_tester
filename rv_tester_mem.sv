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
    parameter int unsigned RegWidth_LLC         = 32'd64,
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
) (

    input   logic                            clk,
    input   logic                            rst_n,
    //from core
    input   slv_req_t   axi_req_up [NumMasters-1:0]     ,        
    output  slv_resp_t  axi_resp_up [NumMasters-1:0]    ,     
    //to main memory
    output  mst_req_t   axi_req_mst_up [NumMasters-1:0]     ,    
    input   mst_resp_t  axi_resp_mst_up [NumMasters-1:0]     ,  
    input   logic 	bypass_cache
);

///////////////unpacked to packed////////////////////

    slv_req_t   [NumMasters-1:0] axi_req;
    slv_resp_t  [NumMasters-1:0] axi_resp;    
    mst_req_t   [NumMasters-1:0] axi_req_mst;
    mst_resp_t  [NumMasters-1:0] axi_resp_mst;


    always_comb begin
       for(int i=0;i<NumMasters;i++) begin	
           axi_req[i] = axi_req_up[i];
	   axi_resp_up[i] = axi_resp[i];
	   axi_req_mst_up[i] = axi_req_mst[i];
           axi_resp_mst[i] = axi_resp_mst_up[i];
       end
    end

////////////////////////////////////////////////////


////////////////local parameters/////////////////////


    slv_req_t [NumMasters-1:0] axi_req_xbar;
    slv_resp_t [NumMasters-1:0] axi_resp_xbar;

    mst_req_t axi_req_mst_imm;
    mst_resp_t axi_resp_mst_imm;

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




    typedef struct packed {
        int unsigned idx;
        logic [AxiAddrWidth-1:0] start_addr;
        logic [AxiAddrWidth-1:0] end_addr;
      } xbar_rule_t;
    

    localparam xbar_rule_t [xbar_cfg.NoAddrRules-1:0] AddrMap = addr_map_gen();
    localparam int unsigned PADDING1 = AxiAddrWidth-1;
    localparam int unsigned PADDING2 = AxiAddrWidth-32;
    function xbar_rule_t [xbar_cfg.NoAddrRules-1:0] addr_map_gen ();
        for (int unsigned i = 0; i < xbar_cfg.NoAddrRules; i++) begin
          addr_map_gen[i] = xbar_rule_t'{
            idx:        unsigned'(i),
            start_addr:  i    * {AxiAddrWidth{1'b1}},
            end_addr:   ({{PADDING2{1'b0}},i}+{{PADDING1{1'h0}},1'b1}) * {AxiAddrWidth{1'b1}},
            default:    '0
          };
        end
    endfunction

    //for LLC 
    //Address ranges
    typedef logic [AxiAddrWidth-1:0] axi_addr_t;
    localparam axi_addr_t SpmRegionStart     = {AxiAddrWidth{1'b0}};
    localparam axi_addr_t SpmRegionLength    = axi_addr_t'(SetAssociativity_LLC * NumLines_LLC * NumBlocks_LLC * AxiDataWidth / 64'd8);
    localparam axi_addr_t CachedRegionStart  = SpmRegionLength + 1;
    localparam axi_addr_t CachedRegionEnd  = {AxiAddrWidth{1'b1}};

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
        .rule_t         ( xbar_rule_t        )
    ) i_xbar (
        .clk_i                  ( clk ),
        .rst_ni                 ( rst_n ),
        .test_i                 ( 1'b0 ),
        .slv_ports_req_i        ( axi_req_xbar ),
        .slv_ports_resp_o       ( axi_resp_xbar ),
        .mst_ports_req_o        ( llc_req_t ),
        .mst_ports_resp_i       ( llc_resp_t ),
        .addr_map_i             ( AddrMap ),
        .en_default_mst_port_i  ( '0 ),
        .default_mst_port_i     ( '0 )
    );
///////////////////////////////////////////


///////////////////LLC/////////////////////

    `AXI_LLC_TYPEDEF_ALL(axi_llc, logic [63:0], logic[SetAssociativity_LLC-1:0])

    axi_llc_cfg_regs_d_t     reg_cfg_hw_to_reg;
    axi_llc_cfg_regs_q_t     reg_cfg_reg_to_hw;
    reg commit;
    reg cnt;

    assign reg_cfg_reg_to_hw.cfg_spm     = {SetAssociativity_LLC{1'b0}};
    assign reg_cfg_reg_to_hw.cfg_flush   = {SetAssociativity_LLC{1'b0}};
    assign reg_cfg_reg_to_hw.commit_cfg  = commit;
    assign reg_cfg_reg_to_hw.flushed     = {SetAssociativity_LLC{1'b0}};


    always@(posedge clk or negedge rst_n) begin
        if(!rst_n) begin
            commit <= 0;
            cnt <= 0;
    	end else begin
	    if(cnt == 0) begin
                commit <= 1;
                cnt <= 1;
	    end else begin
                commit <= 0;
                cnt <= 1;	
	    end 
        end
    end		


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
        .rule_full_t              ( xbar_rule_t ),
        .PrintSramCfg             ( 0 ),
        .PrintLlcCfg              ( 0 )
    ) llc(
        .clk_i                ( clk ),
        .rst_ni               ( rst_n ),
        .test_i               ( 1'b0 ),
        .slv_req_i            ( llc_req_t ), 
        .slv_resp_o           ( llc_resp_t ),
        .mst_req_o            ( axi_req_mst_imm  ),
        .mst_resp_i           ( axi_resp_mst_imm ),
        .conf_regs_i          ( reg_cfg_reg_to_hw ),
        .conf_regs_o          ( reg_cfg_hw_to_reg ), 
        .cached_start_addr_i  ( CachedRegionStart ),
        .cached_end_addr_i    ( CachedRegionEnd ),
        .spm_start_addr_i     ( SpmRegionStart ), 
        .axi_llc_events_o     ( )
    );


////////////////////////////////////////////////////


///////////////bypassing cache//////////////////////

    localparam ID_WIDTH_DIFF = $clog2(NumMasters) + 1;
    localparam ID_WIDTH_AXI_SW = AxiIdWidthMst + 1;
    slv_req_t temp_1;
    mst_req_t temp_2;
    slv_resp_t temp_3;
    mst_resp_t temp_4;    
    always_comb begin
	if(bypass_cache) begin
	    for(int i=0;i<NumMasters;i++) begin
		temp_1 = axi_req[i];
		/* verilator lint_off WIDTH */
		`AXI_SET_REQ_STRUCT(temp_2, temp_1)
		/* verilator lint_on WIDTH */
		temp_2.aw.id = {{ID_WIDTH_DIFF{1'b0}}, axi_req[i].aw.id};
		temp_2.ar.id = {{ID_WIDTH_DIFF{1'b0}}, axi_req[i].ar.id};
		axi_req_mst[i] = temp_2;
		temp_4 = axi_resp_mst[i];
		/* verilator lint_off WIDTH */
		`AXI_SET_RESP_STRUCT(temp_3, temp_4)
		/* verilator lint_on WIDTH */
		temp_3.b.id = axi_resp_mst[i].b.id[AxiIdWidth-1:0];
		temp_3.r.id = axi_resp_mst[i].r.id[AxiIdWidth-1:0];
		axi_resp[i] = temp_3;
	    end 
		axi_req_xbar = '0;			
		axi_resp_mst_imm = '0;
	end else begin
	    axi_req_xbar = axi_req;
	    axi_resp = axi_resp_xbar;
            axi_req_mst[0] = axi_req_mst_imm;	
	    axi_resp_mst_imm = axi_resp_mst[0];
	    for(int i=1;i<NumMasters;i++) begin
			axi_req_mst[i] = '0;
            end	
	end
end


////////////////////////////////////////////////////



endmodule
