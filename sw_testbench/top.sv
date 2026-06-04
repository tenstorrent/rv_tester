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
    /* verilator lint_off WIDTHEXPAND */
    assign core_no_fetch[cvm_topology_gen::mods.TOP.PLATFORM.NHARTS-1:0] = {cvm_topology_gen::mods.TOP.PLATFORM.NHARTS{reset[COLD_RESET_IDX] || reset[WARM_RESET_IDX]}};
    /* verilator lint_on WIDTHEXPAND */

    rv_tester_params::rvfi_t [rv_tester_params::TOTAL_NRETS-1:0] rvfi_next;

    function automatic void write_rvfi(byte unsigned valid, int unsigned order, int unsigned hartid, int unsigned nretid, int unsigned insn, longint unsigned pc);
        int unsigned idx = hartid * cvm_topology_gen::mods.TOP.PLATFORM.COSIM.RVFI.NRETS_CUMSUM[hartid] + nretid;
        rvfi_next[idx].valid = (valid != '0);
        rvfi_next[idx].order = {32'h0, order};
        rvfi_next[idx].hart = hartid[HARTLEN-1:0];
        rvfi_next[idx].pc_rdata = pc;
        rvfi_next[idx].insn = insn;
        rvfi_next[idx].uop = {32'h0, insn};
        rvfi_next[idx].mode = 4'h3;
        rvfi_next[idx].last_uop = '1;
    endfunction

    export "DPI-C" function write_rvfi;

    import "DPI-C" context function void get_1c_stimulus(logic reset, int unsigned order);
    import "DPI-C" context function void get_2c_stimulus(logic reset, int unsigned order);

    int unsigned order = '0;
    assign quiesced = '1;
    assign dmi_poll_timeout_terminate = '0;
    int unsigned reset_deassert_cycle = 100;
    assign warm_reset_en = '0;
    assign num_resets = -1;
    assign target_num_resets = 0;
  `ifdef UVM_MACROS_SVH
    assign uvm_done = '1;
  `endif 

    for (genvar i = 0; i < cvm_topology_gen::mods.TOP.PLATFORM.NHARTS; i++) begin
      assign debug_mode[i] = '0;
    end

    always @(posedge clk[CORE_CLK_IDX]) begin
        order <= order + 1;
        if (order <= reset_deassert_cycle) begin
            cold_reset <= '1;
        end else begin
            cold_reset <= '0;
        end
        case(HARNESS)
        SW_1C:
          get_1c_stimulus(reset[COLD_RESET_IDX], order);
        SW_2C:
          get_2c_stimulus(reset[COLD_RESET_IDX], order);
        default:
            $error("No harness specified");
        endcase
        rvfi <= rvfi_next;
    end

endmodule
