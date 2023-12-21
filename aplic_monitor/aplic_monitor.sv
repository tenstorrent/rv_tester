module aplic_monitor #(
  parameter int NUM = -1,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_APLIC_MONITOR_OUTPUT_PARAMS
)(
  input clk,
  input reset,
  input bit terminate,
  input rv_tester_params::mst_req_top axi_req_mst, //MMR REQ
  input rv_tester_params::mst_resp_top axi_resp_mst, //MMR REQ
  input rv_tester_params::msi_slv_req_top msi_axi_req, //MMR READ 
  input logic[1023:0] aplic_pin_input,
  input logic [7:0] misc_signals,
  `RV_TESTER_TRANSACTIONS_APLIC_MONITOR_OUTPUT_PORTS
);

    int unsigned location = cvm_topology::nil;

    always @(posedge clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            location = cvm_topology::get_location(topology.TOP.PLATFORM.APLIC_MON.ID, 0);
            // perf_enabled = (cvm_plusargs::get_bool("perf") != '0) & (location != cvm_topology::nil);
            /* verilator lint_on BLKSEQ */
        end
    end
    
    //APLIC INPUT PINS MONITOR
    assign aplic_intr_reqs[0].valid = !reset;
    assign aplic_intr_reqs[0].data.location = location;
    assign aplic_intr_reqs[0].data.pin_value = aplic_pin_input;
    
    //APLIC MMR WRITE MONITOR
    assign aplic_mmr_stores[0].valid = !reset && axi_req_mst.w_valid && axi_req_mst.aw_valid;
    assign aplic_mmr_stores[0].data.location = location;
    assign aplic_mmr_stores[0].data.data = axi_req_mst.w.data[31:0];
    assign aplic_mmr_stores[0].data.addr = axi_req_mst.aw.addr[11:0];
    assign aplic_mmr_stores[0].data.len = axi_req_mst.aw.len[3:0];

    //APLIC MMR READ CMD MONITOR
    assign aplic_mmr_load_cmds[0].valid = !reset && axi_req_mst.ar_valid;
    assign aplic_mmr_load_cmds[0].data.location = location;
    assign aplic_mmr_load_cmds[0].data.addr = axi_req_mst.ar.addr[11:0];
    assign aplic_mmr_load_cmds[0].data.size = axi_req_mst.ar.size; //(2**axi_req_mst.ar.size)/8;
    assign aplic_mmr_load_cmds[0].data.id = axi_req_mst.ar.id;

    //APLIC MMR READ DATA MONITOR
    assign aplic_mmr_load_datas[0].valid = !reset && axi_resp_mst.r_valid;
    assign aplic_mmr_load_datas[0].data.location = location;
    assign aplic_mmr_load_datas[0].data.data = axi_resp_mst.r.data;
    assign aplic_mmr_load_datas[0].data.id = axi_resp_mst.r.id;

    //APLIC MSI MESSAGE MONITOR
    assign msi_reqs[0].valid = !reset && msi_axi_req.w_valid && msi_axi_req.aw_valid;
    assign msi_reqs[0].data.location = location;
    assign msi_reqs[0].data.data = msi_axi_req.w.data[31:0];
    assign msi_reqs[0].data.addr = msi_axi_req.aw.addr[11:0];
    //assign msi_reqs[0].data.len = axi_req_mst.aw.len[3:0];

endmodule
