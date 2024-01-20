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

    `RV_TESTER_VARS(topology_pkg::mods)

    rv_tester #(
        .EXTERNAL_CLOCK(EXTERNAL_CLOCK),
        .TOPOLOGY(topology_pkg::topology_t),
        .topology(topology_pkg::mods)
    ) tester (
        .*
    );

    function void write_rvfi(byte unsigned valid, int unsigned hartid, int unsigned nretid, int unsigned insn, longint unsigned pc);
        int unsigned idx = hartid * topology_pkg::mods.TOP.PLATFORM.COSIM.RVFI.NRETS_CUMSUM[hartid] + nretid;
        rvfi[idx].valid = (valid != '0);
        rvfi[idx].hart = hartid[HARTLEN-1:0];
        rvfi[idx].pc_rdata = pc;
        rvfi[idx].insn = insn;
        rvfi[idx].uop = {32'h0, insn};
        rvfi[idx].mode = 4'h3;
        rvfi[idx].last_uop = '1;
    endfunction

    export "DPI-C" function write_rvfi;

    import "DPI-C" context function void get_1c_stimulus(logic reset);
    import "DPI-C" context function void get_2c_stimulus(logic reset);

    longint unsigned clocks = '0;
    assign quiesced = '1;
    assign dmi_req_ready = '0;
    assign dmi_resp_valid = '0;

    for (genvar i = 0; i < topology_pkg::mods.TOP.PLATFORM.NHARTS; i++) begin
      assign debug_mode[i] = '0;
    end

    for (genvar i = 0; i < topology_pkg::mods.TOP.PLATFORM.AXI.TOTAL; i++) begin
      assign axi_req[i].ar_valid = '0;
      assign axi_req[i].aw_valid = '0;
      assign axi_req[i].w_valid = '0;
    end

    always @(posedge clk[CORE_CLK_IDX]) begin
        if (!reset) begin
            clocks <= clocks + 1;
        end
        case(HARNESS)
        SW_1C:
          get_1c_stimulus(reset);
        SW_2C:
          get_2c_stimulus(reset);
        default:
            $error("No harness specified");
        endcase
    end

endmodule
