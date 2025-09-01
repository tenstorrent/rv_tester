module dm_model #(
  parameter int NUM = -1,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_DM_MODEL_OUTPUT_PARAMS
)(
  input clk,
  input reset,
  input rv_tester_pkg::dmi_req_t dmi_req,
  input dmi_req_valid,
  input dmi_resp_valid,
  input rv_tester_pkg::dmi_resp_t dmi_resp,
  input bit terminate,
  input logic                                            dm_mem_tx_vld,
  input logic                                            dm_mem_tx_we,
  input logic [rv_tester_params::DM_AXI_ADDR_WIDTH-1:0]  dm_mem_tx_addr,
  input logic [rv_tester_params::DM_AXI_DATA_WIDTH-1:0]  dm_mem_tx_rd_data,
  input logic [rv_tester_params::DM_AXI_DATA_WIDTH-1:0]  dm_mem_tx_wr_data,
  input logic [rv_tester_params::DM_AXI_STRB_WIDTH-1:0]  dm_mem_tx_wr_data_be,
  input logic [7:0] misc_signals,
  input logic [7:0] DM_DebugReq_Valids,
  input logic                                            dmi_status,
  input logic [31:0]                                     dmi_commands_in_queue,
  input logic                                            dmi_warm_reset,
  `RV_TESTER_TRANSACTIONS_DM_MODEL_OUTPUT_PORTS
);

    parameter int unsigned location = cvm_topology_gen::get_location (topology.TOP.PLATFORM.DM_MODEL.ID, 0);

    logic dm_mem_tx_rd_data_resp_vld;
    logic [7:0] DM_DebugReq_Valids_q;
    logic dmi_warm_reset_q;
    
    assign dmi_statuss[0].valid = (dmi_warm_reset_q != dmi_warm_reset);
    assign dmi_statuss[0].data.location = location;
    assign dmi_statuss[0].data.status = dmi_status;
    assign dmi_statuss[0].data.commands_in_queue = dmi_commands_in_queue;
    assign dmi_statuss[0].data.warm_reset = dmi_warm_reset;

    assign dmi_reqs[0].valid = !reset && dmi_req_valid;
    assign dmi_reqs[0].data.location = location;
    assign dmi_reqs[0].data.op = dmi_req.op;
    assign dmi_reqs[0].data.addr = dmi_req.addr;
    assign dmi_reqs[0].data.data = dmi_req.data;
    assign dmi_reqs[0].data.misc_signals = misc_signals;

    assign dmi_resps[0].valid = !reset && dmi_resp_valid;
    assign dmi_resps[0].data.location = location;
    assign dmi_resps[0].data.resp = dmi_resp.resp;
    assign dmi_resps[0].data.data = dmi_resp.data;

    assign dm_load_cmds[0].valid = !reset && dm_mem_tx_vld && ~dm_mem_tx_we;
    assign dm_load_cmds[0].data.location = location;
    assign dm_load_cmds[0].data.addr = dm_mem_tx_addr[11:0];
    assign dm_load_cmds[0].data.size = $bits( dm_load_cmds[0].data.size)'('h8); // Always 8-byte loads
    assign dm_load_cmds[0].data.id = $bits(dm_load_cmds[0].data.id)'('h0);

    assign dm_load_datas[0].valid = !reset && dm_mem_tx_rd_data_resp_vld;
    assign dm_load_datas[0].data.location = location;
    assign dm_load_datas[0].data.data = dm_mem_tx_rd_data;
    assign dm_load_datas[0].data.id = $bits(dm_load_datas[0].data.id)'('h0);

    assign dm_stores[0].valid = !reset && dm_mem_tx_vld && dm_mem_tx_we;
    assign dm_stores[0].data.location = location;
    assign dm_stores[0].data.data = $bits(dm_stores[0].data.data)'(dm_mem_tx_wr_data);
    assign dm_stores[0].data.addr = dm_mem_tx_addr[11:0];
    assign dm_stores[0].data.len = $bits(dm_stores[0].data.len)'($clog2(dm_mem_tx_wr_data_be + 1'b1));

    assign dm_reqs[0].valid = !reset && (DM_DebugReq_Valids_q != DM_DebugReq_Valids);
    assign dm_reqs[0].data.location = location;
    assign dm_reqs[0].data.dm_ms_req =  DM_DebugReq_Valids;

    always @(posedge clk) begin
      if (reset)
        dm_mem_tx_rd_data_resp_vld <= 0;
      else
        dm_mem_tx_rd_data_resp_vld <= (dm_mem_tx_vld && ~dm_mem_tx_we);
    end
    
    always @(posedge clk) begin
      if (reset)
        DM_DebugReq_Valids_q <= 0;
      else
        DM_DebugReq_Valids_q <= DM_DebugReq_Valids;
    end
    
    always @(posedge clk) begin
      dmi_warm_reset_q <= dmi_warm_reset;
    end
endmodule
