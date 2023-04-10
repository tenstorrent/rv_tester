module top #(
  `RV_TESTER_PARAMETERS(topology_pkg::mods),
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

    always @(posedge clk) begin
        if (!reset) begin
            clocks <= clocks + 1;
        end
        get_stimulus(reset, clocks);
    end

endmodule