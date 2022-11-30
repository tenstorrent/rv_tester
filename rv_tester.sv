module rv_tester #(
    parameter int RESET_CLOCKS              =      10,
    parameter bit EXTERNAL_CLOCK            =       0,
    parameter int CLOCK_PERIOD_PS           =     500,
    parameter int SW_CLOCK_UPDATE_PERIOD_PS = 100_000,
    rv_tester_pkg::cfg_t CFG                =      '0,
    `RV_TESTER_PARAMETERS(CFG)
) (
    input clk_ext,
    `_RV_TESTER_PORTS(output,input)
);

    if (EXTERNAL_CLOCK) begin
        assign clk = clk_ext;
    end else begin
        logic clkgen = '0;
        initial forever #(CLOCK_PERIOD_PS*1ps/2) clkgen = !clkgen;
        assign clk = clkgen;
    end

    sysmod#(
        .RESET_CLOCKS(RESET_CLOCKS),
        .CLOCK_PERIOD_PS(CLOCK_PERIOD_PS),
        .SW_CLOCK_UPDATE_PERIOD_PS(SW_CLOCK_UPDATE_PERIOD_PS),
        .CFG(CFG),
        .NUM(0)
    ) sysmod (
        .clk,
        .reset,
        .bootstrap,
        .interrupt
    );

`ifndef NO_COSIM
    cosim #(
        .CFG(CFG)
    ) cosim (
        .clk,
        .reset,
        .rvfi(rvfi_instr)
    );
`endif

    for (genvar p = 0; p < CFG.AXI_PORTS; p++) begin
        axi_sw #(
            .ADDR_WIDTH(CFG.AXI_ADDR_WIDTH),
            .DATA_WIDTH(CFG.AXI_DATA_WIDTH),
            .ID_WIDTH  (CFG.AXI_ID_WIDTH  ),
            .POLL_DEFAULT(1), // Use 0 or runtime +axi_sw_r_poll for Zebu
            .SYSMOD_NUM(0)
        ) axi_sw(
            .clk,
            .reset_n(~reset),
            .axi_mst_ar_valid(axi_req[p].ar_valid),
            .axi_mst_ar_id   (axi_req[p].ar_id),
            .axi_mst_ar_addr (axi_req[p].ar_addr),
            .axi_mst_ar_len  (axi_req[p].ar_len),
            .axi_mst_ar_size (axi_req[p].ar_size),
            .axi_mst_ar_burst(axi_req[p].ar_burst),
         
            .axi_mst_aw_valid(axi_req[p].aw_valid),
            .axi_mst_aw_id   (axi_req[p].aw_id),
            .axi_mst_aw_addr (axi_req[p].aw_addr),
            .axi_mst_aw_len  (axi_req[p].aw_len),
            .axi_mst_aw_size (axi_req[p].aw_size),
            .axi_mst_aw_burst(axi_req[p].aw_burst),
            .axi_mst_aw_atop (axi_req[p].aw_atop),
         
            .axi_mst_w_valid(axi_req[p].w_valid),
            .axi_mst_w_data (axi_req[p].w_data),
            .axi_mst_w_strb (axi_req[p].w_strb),
            .axi_mst_w_last (axi_req[p].w_last),
         
            .axi_mst_b_ready(axi_req[p].b_ready),
            .axi_mst_r_ready(axi_req[p].r_ready),

            .axi_slv_b_valid(axi_rsp[p].b_valid),
            .axi_slv_b_id   (axi_rsp[p].b_id),
            .axi_slv_b_resp (axi_rsp[p].b_resp),
         
            .axi_slv_r_valid(axi_rsp[p].r_valid),
            .axi_slv_r_id   (axi_rsp[p].r_id),
            .axi_slv_r_data (axi_rsp[p].r_data),
            .axi_slv_r_resp (axi_rsp[p].r_resp),
            .axi_slv_r_last (axi_rsp[p].r_last),
         
            .axi_slv_aw_ready(axi_rsp[p].aw_ready),
            .axi_slv_ar_ready(axi_rsp[p].ar_ready),
            .axi_slv_w_ready (axi_rsp[p].w_ready)
        );
    end

endmodule
