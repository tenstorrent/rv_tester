module top #(
  parameter int EXTERNAL_CLOCK =
  `ifdef TB_EXTERNAL_CLOCK
      1
  `else
      0
  `endif
) (
    input clk_ext
);

    `RV_TESTER_VARS(topology_pkg::mods)

    rv_tester #(
        .EXTERNAL_CLOCK(EXTERNAL_CLOCK),
        .TOPOLOGY(topology_pkg::topology_t),
        .topology(topology_pkg::mods)
    ) tester (
        .*
    );

    function void write_rvfi(byte unsigned valid, int unsigned core, int unsigned insn, longint unsigned pc);
        rvfi_instr[core].insn = insn;
        rvfi_instr[core].valid = (valid != '0);
        rvfi_instr[core].pc_rdata = pc;
    endfunction

    export "DPI-C" function write_rvfi;

    import "DPI-C" context function void get_stimulus(logic reset, longint unsigned clocks);

    longint unsigned clocks = '0;
    assign quiesced = '1;
    assign debug_mode = '0;

    for (genvar i = 0; i < topology_pkg::mods.TOP.CLUSTER.CORE.AXI.TOTAL; i++) begin
      assign axi_req[i].ar_valid = '0;
      assign axi_req[i].aw_valid = '0;
      assign axi_req[i].w_valid = '0;
    end

    always @(posedge clk) begin
        if (!reset) begin
            clocks <= clocks + 1;
        end
        get_stimulus(reset, clocks);
    end

endmodule
