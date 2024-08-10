module top
    import rv_tester_params::*;
#(
  parameter int EXTERNAL_CLOCK =
  `ifdef TB_EXTERNAL_CLOCK
      1
  `else
      0
  `endif
) (
    input clk_ext [NCLKS-1:0]
);

    typedef enum int {
      SW_1C,
      SW_2C
    } harness_id;

    localparam harness_id HARNESS = `HARNESS;

    `RV_TESTER_VARS(cvm_topology_gen::mods)

    rv_tester #(
        .EXTERNAL_CLOCK(EXTERNAL_CLOCK),
        .TOPOLOGY(cvm_topology_gen::topology_t),
        .topology(cvm_topology_gen::mods)
    ) tester (
        .*
    );

    assign dut_clk = clk;

    function automatic void write_rvfi(byte unsigned valid, int unsigned order, int unsigned hartid, int unsigned nretid, int unsigned insn, longint unsigned pc);
        int unsigned idx = hartid * cvm_topology_gen::mods.TOP.PLATFORM.COSIM.RVFI.NRETS_CUMSUM[hartid] + nretid;
        rvfi[idx].valid = (valid != '0);
        rvfi[idx].order = {32'h0, order};
        rvfi[idx].hart = hartid[HARTLEN-1:0];
        rvfi[idx].pc_rdata = pc;
        rvfi[idx].insn = insn;
        rvfi[idx].uop = {32'h0, insn};
        rvfi[idx].mode = 4'h3;
        rvfi[idx].last_uop = '1;
    endfunction

    export "DPI-C" function write_rvfi;

    import "DPI-C" context function void get_1c_stimulus(logic reset, int unsigned order);
    import "DPI-C" context function void get_2c_stimulus(logic reset, int unsigned order);

    longint unsigned clocks = '0;
    int unsigned order = '0;
    assign quiesced = '1;
    assign dmi_req_ready = '0;
    assign dmi_resp_valid = '0;

    for (genvar i = 0; i < cvm_topology_gen::mods.TOP.PLATFORM.NHARTS; i++) begin
      assign debug_mode[i] = '0;
    end

    for (genvar i = 0; i < cvm_topology_gen::mods.TOP.PLATFORM.AXI.TOTAL; i++) begin
      assign axi_req[i].ar_valid = '0;
      assign axi_req[i].aw_valid = '0;
      assign axi_req[i].w_valid = '0;
    end

    always @(posedge clk[CORE_CLK_IDX]) begin
        if (!reset[COLD_RESET_IDX]) begin
            clocks <= clocks + 1;
        end
        order <= order + 1;
        case(HARNESS)
        SW_1C:
          get_1c_stimulus(reset[COLD_RESET_IDX], order);
        SW_2C:
          get_2c_stimulus(reset[COLD_RESET_IDX], order);
        default:
            $error("No harness specified");
        endcase
    end

endmodule
