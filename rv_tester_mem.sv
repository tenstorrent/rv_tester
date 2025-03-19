///////////////includes///////////////////////////////

`include "axi_llc/typedef.svh"

/////////////////////////////////////////////////////

module rv_tester_mem #(
    //number of AXI masters
    parameter int unsigned NumMasters           = 32'd1,
    //number of NCIO AXI masters
    parameter int unsigned NcioNumMasters           = 32'd1,
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
    parameter type mst_resp_t                   = logic,
    //Address Map data type
    parameter type rule_t                       = axi_pkg::xbar_rule_64_t,
    //Number of address rules
    parameter int unsigned NoAddrRules	        = 2,
    //number of master ports of rv_tester_mem
    parameter int unsigned NumMastersMem	= 2
) (

    input   logic                            clk,
    input   logic                            rst_n,
    //from core
    input   slv_req_t   axi_req_up [NumMasters-1:0]     ,        
    output  slv_resp_t  axi_resp_up [NumMasters-1:0]    ,     
    //to main memory
    output  mst_req_t   axi_req_mst_up [NumMastersMem-1:0]     ,    
    input   mst_resp_t  axi_resp_mst_up [NumMastersMem-1:0]     ,
    input   rule_t	[NoAddrRules-1:0] addr_map,	   
    input   logic 	bypass_mem	,
    input   logic       flush_cache	,
    output  logic	flush_complete  ,
    output  logic       bist_status_done,
    input   string      preload_file,
    input   string      preload_file_data,
    input   string preload_file_data_arr [0:4-1],
    input   string preload_file_tag_arr[0:4-1]
);

///////////////unpacked to packed////////////////////

    slv_req_t   [NumMasters-1:0] axi_req;
    slv_resp_t  [NumMasters-1:0] axi_resp;    
    mst_req_t   [NumMastersMem-1:0] axi_req_mst;
    mst_resp_t  [NumMastersMem-1:0] axi_resp_mst;

    always_comb begin
       for(int i=0;i<NumMasters;i++) begin	
           axi_req[i] = axi_req_up[i];
	   axi_resp_up[i] = axi_resp[i];
       end

       for(int i=0;i<NumMastersMem;i++) begin
           axi_req_mst_up[i] = axi_req_mst[i];
           axi_resp_mst[i] = axi_resp_mst_up[i];
       end

    end

////////////////////////////////////////////////////


////////////////local parameters/////////////////////

    logic clk_gated, flush_complete_delayed_2, flush_complete_delayed_1, flush_cache_delayed_3, flush_cache_delayed_1, flush_cache_delayed_2, flush_complete_reg;
    logic enable_flop /*verilator clock_enable*/;

    slv_req_t [NumMasters-1:0] axi_req_xbar;
    slv_resp_t [NumMasters-1:0] axi_resp_xbar;

    mst_req_t axi_req_mst_imm;
    mst_resp_t axi_resp_mst_imm;

    localparam int unsigned Pipeline              = 32'b1;
    localparam int unsigned AxiIdUsed             = 32'd3;
    localparam bit UniqueIds                      = 1'b0;
    localparam int unsigned NumSlaves             = 32'd2;
    localparam int unsigned AxiIdWidthMst         = AxiIdWidth + $clog2(NumMasters);
    localparam AxiAddrWidthCache		  = AxiAddrWidth + 1;
    localparam axi_pkg::xbar_cfg_t xbar_cfg = '{
        NoSlvPorts:         NumMasters,
        NoMstPorts:         NumSlaves,
        MaxMstTrans:        10,
        MaxSlvTrans:        6,
        FallThrough:        1'b0,
        LatencyMode:        axi_pkg::CUT_SLV_PORTS,
        PipelineStages:     Pipeline,
        AxiIdWidthSlvPorts: AxiIdWidth,
        AxiIdUsedSlvPorts:  AxiIdUsed,
        UniqueIds:          UniqueIds,
        AxiAddrWidth:       AxiAddrWidth,
        AxiDataWidth:       AxiDataWidth,
        NoAddrRules:        NoAddrRules,
        default:            0
      };

    //for LLC 
    //Address ranges

    localparam CachedRegionStart  = {AxiAddrWidthCache{1'b0}}; 
    localparam CachedRegionEnd    = {1'b0,{AxiAddrWidth{1'b1}}} + {{AxiAddrWidth{1'b0}}, 1'b1};
    localparam SpmRegionStart     = CachedRegionEnd;
    // string preload_data_file_arr [0:SetAssociativity_LLC-1];
    localparam int data_words = 8;
    localparam int tag_words  = 128;

    int preload_data_words = 32; 
    int preload_tag_words  = 1;   

    string preload_data_file;
    string preload_tag_file;
    // import "DPI-C" function void set_preload_data_file(int unsigned way, string file);

    // function void set_preload_data_file(string file);
    //     preload_data_file = file;
    //     $display("Preload data file set to: %s", preload_data_file);
    // endfunction
    // export "DPI-C" function set_preload_data_file;

    // function void set_preload_tag_file(string file);
    //     preload_tag_file = file;
    //     $display("Preload tag file set to: %s", preload_tag_file);
    // endfunction
    // export "DPI-C" function set_preload_tag_file;



    always@(negedge clk) begin
        enable_flop <= ~bypass_mem;
    end

    assign clk_gated = clk & enable_flop;

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
    

    mst_req_xbar [1:0] mem_req_t;
    mst_resp_xbar [1:0] mem_resp_t;

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
        .rule_t         ( rule_t        )
    ) i_xbar (
        .clk_i                  ( clk_gated ),
        .rst_ni                 ( rst_n ),
        .test_i                 ( 1'b0 ),
        .slv_ports_req_i        ( axi_req_xbar ),
        .slv_ports_resp_o       ( axi_resp_xbar ),
        .mst_ports_req_o        ( mem_req_t ),
        .mst_ports_resp_i       ( mem_resp_t ),
        .addr_map_i             ( addr_map ),
        .en_default_mst_port_i  ( '0 ),
        .default_mst_port_i     ( '0 )
    );

///////////////////////////////////////////


///////////////////LLC/////////////////////

    `AXI_LLC_TYPEDEF_ALL(axi_llc, logic [63:0], logic[SetAssociativity_LLC-1:0])

    axi_llc_cfg_regs_d_t     reg_cfg_hw_to_reg;
    axi_llc_cfg_regs_q_t     reg_cfg_reg_to_hw;

    logic rst_n_1T;
    // always@(posedge clk_gated) begin
	// rst_n_1T <= rst_n;
    //     if(rst_n && !rst_n_1T) begin
	//     if (preload_file != "") begin
	// 	    $readmem(preload_file, "some_array");
	// 	end
    //     end
    // end



    // ARRAY PRELOAD FILE
    for (genvar i = 0; i < SetAssociativity_LLC; i++) begin
        always @(posedge clk_gated) begin
            rst_n_1T <= rst_n;
            if (rst_n && !rst_n_1T) begin
                if (preload_file_data_arr[i] != "") begin
                    $display("Preloading data SRAM for LLC way %0d with file: %s", i, preload_file_data_arr[i]);
                    $readmemh(preload_file_data_arr[i],
                            llc.i_llc_ways.gen_data_ways[i].i_data_way.i_data_sram.sram,
                            0, data_words - 1);
                    $readmemh(preload_file_tag_arr[i], 
                            llc.i_hit_miss_unit.i_tag_store.gen_tag_macros[i].i_tag_store.sram,
                            0, tag_words - 1);
                end
            end
        end
    end


    // for (genvar i = 0; i < SetAssociativity_LLC; i++) begin
    //     always @(posedge clk_gated) begin
    //         rst_n_1T <= rst_n;
    //         if (rst_n && !rst_n_1T) begin
    //             if (preload_file != "" && preload_file_data != "") begin
    //                 $display("Preloading LLC way %0d with file: %s", i, preload_file);
    //                 $readmemh(preload_file, llc.i_hit_miss_unit.i_tag_store.gen_tag_macros[i].i_tag_store.sram, data_words, data_words + tag_words - 1);
    //                 $readmemh(preload_file_data, llc.i_llc_ways.gen_data_ways[i].i_data_way.i_data_sram.sram, i * data_words, (i+1)*data_words - 1);
    //             end
    //         end
    //     end
    // end

    always@(posedge clk_gated) begin
        if(!rst_n) begin
                bist_status_done                     <= 0;
        end else begin
                bist_status_done                     <= reg_cfg_hw_to_reg.bist_status_done;
        end
    end

    typedef logic [AxiIdWidthMst:0] id_mst_llc;
    typedef logic [AxiIdWidthMst-1:0] id_slv_llc;
    typedef logic [AxiAddrWidthCache-1:0] addr_llc;
    typedef logic [AxiDataWidth-1:0] data_llc;
    typedef logic [AxiStrbWidth-1:0] strb_llc;
    typedef logic [AxiUserWidth-1:0] user_llc;

    typedef struct packed {
        int unsigned idx;
        logic [AxiAddrWidthCache-1:0] start_addr;
        logic [AxiAddrWidthCache-1:0] end_addr;
    } rule_llc;


    `AXI_TYPEDEF_AW_CHAN_T(mst_aw_chan_llc, addr_llc , id_mst_llc, user_llc)
    `AXI_TYPEDEF_AW_CHAN_T(slv_aw_chan_llc, addr_llc, id_slv_llc, user_llc)
    `AXI_TYPEDEF_W_CHAN_T(mst_w_chan_llc, data_llc, strb_llc, user_llc)
    `AXI_TYPEDEF_W_CHAN_T(slv_w_chan_llc, data_llc, strb_llc, user_llc)
    `AXI_TYPEDEF_B_CHAN_T(mst_b_chan_llc, id_mst_llc, user_llc)
    `AXI_TYPEDEF_B_CHAN_T(slv_b_chan_llc, id_slv_llc, user_llc)
    `AXI_TYPEDEF_AR_CHAN_T(mst_ar_chan_llc, addr_llc, id_mst_llc, user_llc)
    `AXI_TYPEDEF_AR_CHAN_T(slv_ar_chan_llc, addr_llc, id_slv_llc, user_llc)
    `AXI_TYPEDEF_R_CHAN_T(mst_r_chan_llc, data_llc, id_mst_llc, user_llc)
    `AXI_TYPEDEF_R_CHAN_T(slv_r_chan_llc, data_llc, id_slv_llc, user_llc)
    `AXI_TYPEDEF_REQ_T(mst_req_llc, mst_aw_chan_llc, mst_w_chan_llc, mst_ar_chan_llc)
    `AXI_TYPEDEF_REQ_T(slv_req_llc, slv_aw_chan_llc, slv_w_chan_llc, slv_ar_chan_llc)
    `AXI_TYPEDEF_RESP_T(mst_resp_llc, mst_b_chan_llc, mst_r_chan_llc)
    `AXI_TYPEDEF_RESP_T(slv_resp_llc, slv_b_chan_llc, slv_r_chan_llc)

    slv_req_llc mem_req_t_1;
    mst_req_llc axi_req_mst_imm_1;

    always_comb begin
	/* verilator lint_off WIDTH */
        `AXI_SET_REQ_STRUCT(mem_req_t_1, mem_req_t[0]);
	/* verilator lint_on WIDTH */
        mem_req_t_1.aw.addr = {1'b0, mem_req_t[0].aw.addr};
        mem_req_t_1.ar.addr = {1'b0, mem_req_t[0].ar.addr};
    end

    axi_llc_top #(
        .SetAssociativity         ( SetAssociativity_LLC ),
        .NumLines                 ( NumLines_LLC ),
        .NumBlocks                ( NumBlocks_LLC ),
        .AxiIdWidth               ( AxiIdWidthMst ),
        .AxiAddrWidth             ( AxiAddrWidthCache ),
        .AxiDataWidth             ( AxiDataWidth ),
        .AxiUserWidth             ( AxiUserWidth ),
        .RegWidth                 ( RegWidth_LLC ),
        .conf_regs_d_t            ( axi_llc_cfg_regs_d_t ),
        .conf_regs_q_t            ( axi_llc_cfg_regs_q_t ),
        .slv_req_t                ( slv_req_llc ),
        .slv_resp_t               ( mst_resp_xbar ),
        .mst_req_t                ( mst_req_llc ),
        .mst_resp_t               ( mst_resp_t ),
        .rule_full_t              ( rule_llc ),
        .PrintSramCfg             ( 0 ),
        .PrintLlcCfg              ( 0 )
    ) llc(
        .clk_i                ( clk_gated ),
        .rst_ni               ( rst_n ),
        .test_i               ( 1'b0 ),
        .slv_req_i            ( mem_req_t_1 ), 
        .slv_resp_o           ( mem_resp_t[0] ),
        .mst_req_o            ( axi_req_mst_imm_1  ),
        .mst_resp_i           ( axi_resp_mst_imm ),
        .conf_regs_i          ( reg_cfg_reg_to_hw ),
        .conf_regs_o          ( reg_cfg_hw_to_reg ), 
        .cached_start_addr_i  ( CachedRegionStart ),
        .cached_end_addr_i    ( CachedRegionEnd ),
        .spm_start_addr_i     ( SpmRegionStart ), 
        .axi_llc_events_o     ( )
    );

    always_comb begin
	/* verilator lint_off WIDTH */
        `AXI_SET_REQ_STRUCT(axi_req_mst_imm, axi_req_mst_imm_1);
	/* verilator lint_on WIDTH */
        axi_req_mst_imm.aw.addr = axi_req_mst_imm_1.aw.addr[AxiAddrWidth-1:0];
        axi_req_mst_imm.ar.addr = axi_req_mst_imm_1.ar.addr[AxiAddrWidth-1:0];
    end


////////////////////////////////////////////////////


///////////////bypassing cache//////////////////////

    localparam ID_WIDTH_DIFF = $clog2(NumMasters) + 1;

    slv_req_t temp_1;
    mst_req_t temp_2;
    slv_resp_t temp_3;
    mst_resp_t temp_4;
    mst_req_xbar temp_5;
    mst_resp_xbar temp_6;  
    always_comb begin
	if(bypass_mem) begin
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

	    for(int i=NumMasters;i<NumMastersMem;i++) begin
		axi_req_mst[i] = '0;
	    end 
		mem_resp_t[1] = '0;			
		axi_req_xbar = '0;			
		axi_resp_mst_imm = '0;
            temp_5 = '0;
            temp_6 = '0;
	end else begin
	    axi_req_xbar = axi_req;
	    axi_resp = axi_resp_xbar;
            axi_req_mst[0] = axi_req_mst_imm;	
	    axi_resp_mst_imm = axi_resp_mst[0];
            temp_5 = mem_req_t[1];
	    /* verilator lint_off WIDTH */
            `AXI_SET_REQ_STRUCT(temp_2, temp_5)
            /* verilator lint_on WIDTH */
            temp_2.aw.id = {{1'b0}, mem_req_t[1].aw.id};
            temp_2.ar.id = {{1'b0}, mem_req_t[1].ar.id};
	    axi_req_mst[1] = temp_2;
            temp_4 = axi_resp_mst[1];
            /* verilator lint_off WIDTH */
            `AXI_SET_RESP_STRUCT(temp_6, temp_4)
            /* verilator lint_on WIDTH */
            temp_6.b.id = axi_resp_mst[1].b.id[AxiIdWidthMst-1:0];
            temp_6.r.id = axi_resp_mst[1].r.id[AxiIdWidthMst-1:0];
            mem_resp_t[1] = temp_6;
	    for(int i=2;i<NumMasters;i++) begin
			axi_req_mst[i] = '0;
            end	
	    temp_1 = '0;
            temp_3 = '0;
	end
    end


////////////////////////////////////////////////////

//////////////////Flushing//////////////////////////


    always@(posedge clk_gated) begin
        if(!rst_n) begin
                flush_cache_delayed_1                     <= '0;
		flush_cache_delayed_2			  <= '0;
		flush_cache_delayed_3  			  <= '0;
		flush_complete_delayed_1		  <= '0;
                flush_complete_delayed_2                  <= '0;
        end else begin
                flush_cache_delayed_1                     <= flush_cache & !bypass_mem;
		flush_cache_delayed_2			  <= flush_cache_delayed_1;
		flush_cache_delayed_3			  <= flush_cache_delayed_2;
		flush_complete_delayed_1	          <= |reg_cfg_hw_to_reg.cfg_flush;
		flush_complete_delayed_2		  <= flush_complete_delayed_1;	
        end
    end

    prim_subreg #(
    .DW      (SetAssociativity_LLC),
    .SWACCESS("RW"),
    .RESVAL  ('0)
    ) u_cfg_spm (
    .clk_i   (clk_gated    ),
    .rst_ni  (rst_n  ),

    // from register interface
    .we     (1'b0),
    .wd     ({SetAssociativity_LLC{1'b0}}),

    // from internal hardware
    .de     (reg_cfg_hw_to_reg.cfg_spm_en),
    .d      (reg_cfg_hw_to_reg.cfg_spm),

    // to internal hardware
    .qe     (),
    .q      (reg_cfg_reg_to_hw.cfg_spm),

    // to register interface (read)
    .qs     ()
    );


    prim_subreg #(
    .DW      (SetAssociativity_LLC),
    .SWACCESS("RW"),
    .RESVAL  ('0)
    ) u_cfg_flush (
    .clk_i   (clk_gated    ),
    .rst_ni  (rst_n  ),

    // from register interface
    .we     (flush_cache_delayed_2^flush_cache_delayed_1),
    .wd     ({SetAssociativity_LLC{flush_cache_delayed_1}}),

    // from internal hardware
    .de     (reg_cfg_hw_to_reg.cfg_flush_en),
    .d      (reg_cfg_hw_to_reg.cfg_flush),

    // to internal hardware
    .qe     (),
    .q      (reg_cfg_reg_to_hw.cfg_flush),

    // to register interface (read)
    .qs     ()
    );

    prim_subreg #(
    .DW      (1),
    .SWACCESS("RW"),
    .RESVAL  ('0)
    ) u_cfg_commit (
    .clk_i   (clk_gated    ),
    .rst_ni  (rst_n  ),

    // from register interface
    .we     (flush_cache_delayed_2^flush_cache_delayed_3),
    .wd     (flush_cache_delayed_2),

    // from internal hardware
    .de     (reg_cfg_hw_to_reg.commit_cfg_en),
    .d      (reg_cfg_hw_to_reg.commit_cfg),

    // to internal hardware
    .qe     (),
    .q      (reg_cfg_reg_to_hw.commit_cfg),

    // to register interface (read)
    .qs     ()
    );

    prim_subreg #(
    .DW      (SetAssociativity_LLC),
    .SWACCESS("RO"),
    .RESVAL  ('0)
    ) u_cfg_flushed (
    .clk_i   (clk_gated    ),
    .rst_ni  (rst_n  ),

    // from register interface
    .we     (1'b0),
    .wd     ('0),

    // from internal hardware
    .de     (reg_cfg_hw_to_reg.flushed_en),
    .d      (reg_cfg_hw_to_reg.flushed),

    // to internal hardware
    .qe     (),
    .q      (reg_cfg_reg_to_hw.flushed),

    // to register interface (read)
    .qs     ()
    );

    always@(posedge clk_gated) begin
        if(!rst_n) begin
                flush_complete_reg <= 0;
        end else if((flush_complete_delayed_2 == 1) && (flush_complete_delayed_1 == 0)) begin
                flush_complete_reg <= 1;
        end else begin
                flush_complete_reg <= flush_complete_reg;
        end
    end

    assign flush_complete = flush_complete_reg || bypass_mem;



////////////////////////////////////////////////////


endmodule
