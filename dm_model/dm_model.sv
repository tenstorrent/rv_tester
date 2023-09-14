module dm_model #(
  parameter int NUM = -1,
  `TOPOLOGY
)(
  input clk,
  input reset,
  input rv_tester_pkg::dmi_req_t dmi_req,
  input dmi_req_valid,
  input dmi_resp_valid,
  input rv_tester_pkg::dmi_resp_t dmi_resp,
  input bit terminate,
  input rv_tester_params::mst_req_top axi_req_mst,
  input rv_tester_params::mst_resp_top axi_resp_mst,
  input logic [7:0] misc_signals,
  `RV_TESTER_TRANSACTIONS_OUTPUT_DM_MODEL
);

    int unsigned location = cvm_topology::nil;
    
    always @(posedge clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            location = cvm_topology::get_location(topology.TOP.PLATFORM.DM_MODEL.ID, 0);
            // perf_enabled = (cvm_plusargs::get_bool("perf") != '0) & (location != cvm_topology::nil);
            /* verilator lint_on BLKSEQ */
        end
    end

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

    assign dm_load_cmds[0].valid = !reset && axi_req_mst.ar_valid;
    assign dm_load_cmds[0].data.location = location;
    assign dm_load_cmds[0].data.addr = axi_req_mst.ar.addr;
    assign dm_load_cmds[0].data.size = axi_req_mst.ar.size; //(2**axi_req_mst.ar.size)/8;
    assign dm_load_cmds[0].data.id = axi_req_mst.ar.id;

    assign dm_load_datas[0].valid = !reset && axi_resp_mst.r_valid;
    assign dm_load_datas[0].data.location = location;
    assign dm_load_datas[0].data.data = axi_resp_mst.r.data[63:0];
    assign dm_load_datas[0].data.id = axi_resp_mst.r.id;

    assign dm_stores[0].valid = !reset && axi_req_mst.w_valid && axi_req_mst.aw_valid;
    assign dm_stores[0].data.location = location;
    assign dm_stores[0].data.data = axi_req_mst.w.data[31:0];
    assign dm_stores[0].data.addr = axi_req_mst.aw.addr;
    assign dm_stores[0].data.len = axi_req_mst.aw.len[3:0];

endmodule
