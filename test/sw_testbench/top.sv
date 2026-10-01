// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

module top
    import rv_tester_params::*;
(
    input clk_ext [NCLKS-1:0]
);

    typedef enum int {
      SW_1C,
      SW_2C,
      SW_8C
    } harness_id;
 
    localparam harness_id HARNESS = `HARNESS;
    
    `RV_TESTER_VARS(cvm_topology_gen::mods)

    rv_tester #(
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

    function automatic void write_rvfi(byte unsigned valid, int unsigned order, int unsigned hartid, int unsigned nretid, int unsigned insn, longint unsigned pc,
                                       byte unsigned rd_addr, longint unsigned rd_wdata);
        int unsigned idx = cvm_topology_gen::mods.TOP.PLATFORM.COSIM.RVFI.NRETS_CUMSUM[hartid] + nretid;
        rvfi_next[idx].valid = (valid != '0);
        rvfi_next[idx].order = {32'h0, order};
        rvfi_next[idx].hart = hartid[HARTLEN-1:0];
        rvfi_next[idx].pc_rdata = pc;
        rvfi_next[idx].insn = insn;
        rvfi_next[idx].uop = {32'h0, insn};
        rvfi_next[idx].mode = 4'h3;
        rvfi_next[idx].last_uop = '1;
        rvfi_next[idx].rd_addr = rd_addr[5:0];
        rvfi_next[idx].rd_wdata = rd_wdata;
        rvfi_next[idx].trap = '0;
        rvfi_next[idx].cause = '0;
        rvfi_next[idx].mem_rmask = '0;
        rvfi_next[idx].mem_wmask = '0;
    endfunction

    // Adds a store to the record that write_rvfi just wrote for the hart.
    function automatic void write_rvfi_store(int unsigned hartid, longint unsigned paddr, byte unsigned wmask,
                                             longint unsigned wdata, int unsigned attr);
        int unsigned idx = cvm_topology_gen::mods.TOP.PLATFORM.COSIM.RVFI.NRETS_CUMSUM[hartid];
        rvfi_next[idx].mem_addr = paddr;
        rvfi_next[idx].mem_paddr = paddr[PALEN-1:0];
        rvfi_next[idx].mem_wmask = wmask;
        rvfi_next[idx].mem_wdata = wdata;
        rvfi_next[idx].mem_attr = attr;
    endfunction

    // A trap record: cosim.sv turns a non-zero cause into an m_trap, and the
    // next retired instruction counts as the first one of the handler.
    function automatic void write_rvfi_trap(int unsigned order, int unsigned hartid, longint unsigned pc, longint unsigned cause);
        int unsigned idx = cvm_topology_gen::mods.TOP.PLATFORM.COSIM.RVFI.NRETS_CUMSUM[hartid];
        write_rvfi(1, order, hartid, 0, 0, pc, 0, 0);
        rvfi_next[idx].trap = '1;
        rvfi_next[idx].cause = cause;
    endfunction

    rv_tester_params::msi_t imsic_msi_next [cvm_topology_gen::mods.TOP.PLATFORM.NHARTS-1:0];

    // An MSI the hart's IMSIC file received, as rv_tester sees it from the DUT.
    function automatic void write_msi(int unsigned hartid, byte unsigned valid, longint unsigned addr, int unsigned data);
        imsic_msi_next[hartid] = '0;
        imsic_msi_next[hartid].valid = (valid != '0);
        imsic_msi_next[hartid].addr = addr;
        imsic_msi_next[hartid].data = data;
    endfunction

    export "DPI-C" function write_rvfi;
    export "DPI-C" function write_rvfi_trap;
    export "DPI-C" function write_rvfi_store;
    export "DPI-C" function write_msi;

    import "DPI-C" context function void get_1c_stimulus(logic reset, int unsigned order);
    import "DPI-C" context function void get_2c_stimulus(logic reset, int unsigned order);
    import "DPI-C" context function void get_8c_stimulus(logic reset, int unsigned order);

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
        SW_8C:
          get_8c_stimulus(reset[COLD_RESET_IDX], order);
        default:
            $error("No harness specified");
        endcase
        rvfi <= rvfi_next;
        imsic_msi <= imsic_msi_next;
    end

endmodule
