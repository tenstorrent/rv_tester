// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

module tb(
`ifdef VERILATOR
          input vclk
`endif
	  );

  localparam cvm_topology_gen::topology_t topology = cvm_topology_gen::mods;

  localparam int unsigned ADDR_WIDTH = topology.TOP.PLATFORM.AXI_SW[0].ADDR_WIDTH;
  localparam int unsigned DATA_WIDTH = topology.TOP.PLATFORM.AXI_SW[0].DATA_WIDTH;
  localparam int unsigned ID_WIDTH   = topology.TOP.PLATFORM.AXI_SW[0].ID_WIDTH;
  localparam int unsigned STRB_WIDTH = topology.TOP.PLATFORM.AXI_SW[0].STRB_WIDTH;

  typedef logic [ADDR_WIDTH-1:0] addr_t;
  typedef logic [DATA_WIDTH-1:0] data_t;
  typedef logic [ID_WIDTH  -1:0] id_t  ;
  typedef logic [STRB_WIDTH-1:0] strb_t;

  typedef logic [1:0] burst_t ;
  typedef logic [1:0] resp_t  ;
  typedef logic [7:0] len_t   ;
  typedef logic [2:0] size_t  ;
  typedef logic [5:0] atop_t  ;
  typedef logic [3:0] cache_t ;
  typedef logic [2:0] prot_t  ;
  typedef logic [3:0] qos_t   ;
  typedef logic [3:0] region_t;
  typedef logic  user_t  ;

  logic             axi_mst_ar_valid;
  id_t              axi_mst_ar_id;
  addr_t            axi_mst_ar_addr;
  len_t             axi_mst_ar_len;
  size_t            axi_mst_ar_size;
  burst_t           axi_mst_ar_burst;
  logic             axi_mst_ar_lock;
  cache_t           axi_mst_ar_cache;
  prot_t            axi_mst_ar_prot;
  qos_t             axi_mst_ar_qos;
  region_t          axi_mst_ar_region;
  user_t            axi_mst_ar_user;
  atop_t            axi_mst_ar_atop;

  logic             axi_mst_aw_valid;
  id_t              axi_mst_aw_id;
  addr_t            axi_mst_aw_addr;
  len_t             axi_mst_aw_len;
  size_t            axi_mst_aw_size;
  burst_t           axi_mst_aw_burst;
  logic             axi_mst_aw_lock;
  cache_t           axi_mst_aw_cache;
  prot_t            axi_mst_aw_prot;
  qos_t             axi_mst_aw_qos;
  region_t          axi_mst_aw_region;
  atop_t            axi_mst_aw_atop;
  user_t            axi_mst_aw_user;

  logic             axi_mst_w_valid;
  data_t            axi_mst_w_data;
  strb_t            axi_mst_w_strb;
  logic             axi_mst_w_last;

  logic             axi_mst_b_ready;
  logic             axi_mst_r_ready;

  logic             axi_slv_b_valid;
  id_t              axi_slv_b_id;
  resp_t            axi_slv_b_resp;

  logic             axi_slv_r_valid;
  id_t              axi_slv_r_id;
  data_t            axi_slv_r_data;
  resp_t            axi_slv_r_resp;
  logic             axi_slv_r_last;

  logic             axi_slv_aw_ready;
  logic             axi_slv_ar_ready;
  logic             axi_slv_w_ready;

  logic clk, reset_n;

`ifdef VERILATOR
  assign clk = vclk;
`endif
`ifdef VCS
  initial begin
    clk = '0;
    forever #5 clk = ~clk;
  end
`endif

  `RV_TESTER_TRANSACTIONS_DOMAIN(2, clk);

  import "DPI-C"         function bit axi_sw_tb_init();
  import "DPI-C"         function bit axi_sw_tb_shutdown();
  import "DPI-C" context function bit axi_sw_tb_flush_callbacks();

  bit cb_poll = '0;
  bit terminate = '0;
  bit tb_reset = '0;
  bit sys_reset = '0;
  always @(posedge clk) begin
    automatic bit _;
    if (tb_reset) begin
      _ = axi_sw_tb_init();
      terminate  <= '0;
      cb_poll    <= cvm_plusargs::get_bool("cb_async") == '0;
    end
    sys_reset <= tb_reset;
  end

  always@(posedge clk) begin
    automatic bit _;
    if (cb_poll) begin
      _ = axi_sw_tb_flush_callbacks();
    end
  end

  always @(posedge clk) begin
    automatic logic shutdowned = '0;
    if (terminate) begin
      shutdowned = axi_sw_tb_shutdown();
      if (shutdowned) begin
        $finish();
      end
    end
  end

  if (topology.TOP.PLATFORM.AXI_SW[0].TOTAL != 1) $error("Only 1 axi supported");
  axi_sw #(
           .ADDR_WIDTH(ADDR_WIDTH),
           .DATA_WIDTH(DATA_WIDTH),
           .ID_WIDTH(ID_WIDTH),
           .STRB_WIDTH(STRB_WIDTH),
           .R_Q_MAX(topology.TOP.PLATFORM.AXI_SW[0].R_Q_MAX),
           .B_Q_MAX(topology.TOP.PLATFORM.AXI_SW[0].B_Q_MAX),
           .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.AXI_SW[0].ID, 0)),
           `RV_TESTER_TRANSACTIONS_AXI_SOURCE_PARAMS(0)
	   ) mem (
		  .*,
		  `RV_TESTER_TRANSACTIONS_AXI_SOURCE_PORTS(2, 0, 0)
		  );

  if (topology.TOP.PLATFORM.AXI_SW_MST[0].TOTAL != 2) $error("Only 2 axi supported");
  axi_sw_mst #(
               .ADDR_WIDTH(topology.TOP.PLATFORM.AXI_SW_MST[0].ADDR_WIDTH),
               .DATA_WIDTH(topology.TOP.PLATFORM.AXI_SW_MST[0].DATA_WIDTH),
               .ID_WIDTH(topology.TOP.PLATFORM.AXI_SW_MST[0].ID_WIDTH  ),
               .STRB_WIDTH(topology.TOP.PLATFORM.AXI_SW_MST[0].STRB_WIDTH),
               .USER_WIDTH(topology.TOP.PLATFORM.AXI_SW_MST[0].USER_WIDTH),
               .AR_Q_MAX(topology.TOP.PLATFORM.AXI_SW_MST[0].AR_Q_MAX),
               .AW_Q_MAX(topology.TOP.PLATFORM.AXI_SW_MST[0].AW_Q_MAX),
               .W_Q_MAX(topology.TOP.PLATFORM.AXI_SW_MST[0].W_Q_MAX),
               .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.AXI_SW_MST[0].ID, 0)),
               `RV_TESTER_TRANSACTIONS_AXI_SW_MST_SOURCE_PARAMS(0)
	       ) mst0 (
		       .reset_n('0),
		       .sys_reset(sys_reset)
		       );

  axi_sw_mst #(
               .ADDR_WIDTH(topology.TOP.PLATFORM.AXI_SW_MST[1].ADDR_WIDTH),
               .DATA_WIDTH(topology.TOP.PLATFORM.AXI_SW_MST[1].DATA_WIDTH),
               .ID_WIDTH(topology.TOP.PLATFORM.AXI_SW_MST[1].ID_WIDTH  ),
               .STRB_WIDTH(topology.TOP.PLATFORM.AXI_SW_MST[1].STRB_WIDTH),
               .USER_WIDTH(topology.TOP.PLATFORM.AXI_SW_MST[1].USER_WIDTH),
               .AR_Q_MAX(topology.TOP.PLATFORM.AXI_SW_MST[1].AR_Q_MAX),
               .AW_Q_MAX(topology.TOP.PLATFORM.AXI_SW_MST[1].AW_Q_MAX),
               .W_Q_MAX(topology.TOP.PLATFORM.AXI_SW_MST[1].W_Q_MAX),
               .LOCATION(cvm_topology_gen::get_location(topology.TOP.PLATFORM.AXI_SW_MST[1].ID, 0)),
               `RV_TESTER_TRANSACTIONS_SMC_AXI_MST_SOURCE_PARAMS(0)
	       ) smc_mst (
			  .reset_n('0),
			  .sys_reset(sys_reset)
			  );

  import "DPI-C" function void get_stim(
					input  int unsigned clock,
					output byte finish,
					output byte vreset_n,
					output byte vtb_reset,
					output byte vaw_valid,
					output longint unsigned vaw_addr,
					output byte vaw_len,
					output byte vaw_size,
					output byte vaw_burst,
					output byte vaw_atop,
					output byte vaw_lock,
					output byte var_valid,
					output byte var_id,
					output longint unsigned var_addr,
					output byte var_len,
					output byte var_size,
					output byte var_burst,
					output byte var_lock,
					output byte vw_valid,
					output byte vw_data[64],
					output byte vw_strb[8],
					output byte vw_last,
					input  byte r_valid,
					input  byte r_id,
					input  byte r_data[64],
					input  byte r_last
					);

  int unsigned clock = 0;
  always @(posedge clk) begin
    automatic byte vreset_n;
    automatic byte vtb_reset;
    automatic byte finish;
    automatic byte vaw_valid;
    automatic longint unsigned vaw_addr;
    automatic byte vaw_len;
    automatic byte vaw_size;
    automatic byte vaw_burst;
    automatic byte vaw_atop;
    automatic byte vaw_lock;
    automatic byte var_valid;
    automatic byte var_id;
    automatic longint unsigned var_addr;
    automatic byte var_len;
    automatic byte var_size;
    automatic byte var_burst;
    automatic byte var_lock;
    automatic byte vw_valid;
    automatic byte vw_data[64];
    automatic byte vw_strb[8];
    automatic byte vw_last;
    automatic byte vr_data[64];
    clock <= clock + 1;
    if (axi_slv_r_valid) begin
      for (int i = 0; i < $size(axi_slv_r_data); i++) begin
        vr_data[i] = axi_slv_r_data[8*i +: 8];
      end
    end
    get_stim(
             clock,
             finish,
             vreset_n,
             vtb_reset,
             vaw_valid,
             vaw_addr,
             vaw_len,
             vaw_size,
             vaw_burst,
             vaw_atop,
             vaw_lock,
             var_valid,
             var_id,
             var_addr,
             var_len,
             var_size,
             var_burst,
             var_lock,
             vw_valid,
             vw_data,
             vw_strb,
             vw_last,
             8'(axi_slv_r_valid),
             8'(axi_slv_r_id),
             vr_data,
             8'(axi_slv_r_last)
             );
    /* verilator lint_off WIDTH */
    reset_n   <= vreset_n ;
    tb_reset <= vtb_reset;
    axi_mst_aw_valid <= vaw_valid;
    axi_mst_aw_addr  <= vaw_addr ;
    axi_mst_aw_len   <= vaw_len  ;
    axi_mst_aw_size  <= vaw_size ;
    axi_mst_aw_burst <= vaw_burst;
    axi_mst_aw_atop  <= vaw_atop;
    axi_mst_aw_lock  <= vaw_lock;
    axi_mst_ar_valid <= var_valid;
    axi_mst_ar_id    <= var_id   ;
    axi_mst_ar_addr  <= var_addr ;
    axi_mst_ar_len   <= var_len  ;
    axi_mst_ar_size  <= var_size ;
    axi_mst_ar_burst <= var_burst;
    axi_mst_ar_lock  <= var_lock;
    axi_mst_w_valid  <= vw_valid ;
    axi_mst_w_last   <= vw_last  ;
    /* verilator lint_on WIDTH */
    for (int i = 0; i < $size(axi_mst_w_data); i++) begin
      axi_mst_w_data[8*i +: 8] <= vw_data[i];
      axi_mst_w_strb[i]        <= vw_strb[i/8][i%8];
    end
    axi_mst_b_ready  <= 1'b1;
    axi_mst_r_ready  <= 1'b1;
    if (finish != '0) begin
      terminate <= 1'b1;
    end
  end

endmodule
