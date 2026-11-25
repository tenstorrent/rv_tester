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

    function automatic bit [NCLKS-1:0][31:0] clk_set(int unsigned tb, int unsigned core, int unsigned axi,
                                                     int unsigned soc, int unsigned ref_clk);
        bit [NCLKS-1:0][31:0] freq_mhz = '0;
        freq_mhz[TB_CLK_IDX]   = tb;
        freq_mhz[CORE_CLK_IDX] = core;
        freq_mhz[AXI_CLK_IDX]  = axi;
        freq_mhz[SOC_CLK_IDX]  = soc;
        freq_mhz[REF_CLK_IDX]  = ref_clk;
        return freq_mhz;
    endfunction

    localparam bit [NCLKS-1:0][31:0] CLOCK_FREQ_MHZ = clk_set(2000, 2000, 1000, 500, 100);

    localparam CLK_PROFILE_FREQ_MHZ = {
        clk_set(2100, 2100, 2100, 2100, 2100),
        clk_set(2100, 2100, 2100, 2100, 2100),
        clk_set(2100, 2100, 2100, 2100, 2100),
        clk_set(2100, 2100, 2100, 2100, 2100),
        clk_set(2100, 2100, 2100, 2100, 2100),
        clk_set(1500, 1500, 1500, 1500, 1500),
        CLOCK_FREQ_MHZ
    };

    rv_tester #(
        .CLK_PROFILE_FREQ_MHZ(CLK_PROFILE_FREQ_MHZ),
        .GLITCH_FREE_CLK_SWITCH(HARNESS != SW_2C),
        .TOPOLOGY(cvm_topology_gen::topology_t),
        .topology(cvm_topology_gen::mods)
    ) tester (
        .*
    );

    assign dut_clk = clk;

    // Every clock, TB_CLK and REF_CLK included, must run at the selected
    // profile's frequency once cold reset is released. Under +dyn_clk_switch a
    // clock may only be off for a few edges after a switch that changed its own
    // frequency.
    localparam int unsigned NUM_CLK_TABLE_ROWS = $bits(CLK_PROFILE_FREQ_MHZ) / (NCLKS * 32);
    localparam bit [NUM_CLK_TABLE_ROWS-1:0][NCLKS-1:0][31:0] CLK_FREQ_TABLE = CLK_PROFILE_FREQ_MHZ;
    localparam int unsigned CLK_SETTLE_EDGES = 4;
    int check_clk_profile = 0;
    bit check_dyn_clk_switch = '0;
    initial begin
        void'($value$plusargs("clk_profile=%d", check_clk_profile));
        if (check_clk_profile < 0 || check_clk_profile >= int'(NUM_CLK_TABLE_ROWS)) check_clk_profile = 0;
        check_dyn_clk_switch = $test$plusargs("dyn_clk_switch");
    end

    function automatic realtime expected_clk_period(int mode, int clk_idx);
        foreach (CLK_FREQ_TABLE[m, c]) begin
            if (m == mode && c == clk_idx) return 1000000.0 / real'(CLK_FREQ_TABLE[m][c]) * 1ps;
        end
        return 0;
    endfunction

    for (genvar c = 0; c < NCLKS; c++) begin : g_clk_freq_check
        realtime last_edge = 0;
        int last_mode = -1;
        int unsigned settle_edges = 0;
        always @(posedge clk[c]) begin
            automatic int mode = check_dyn_clk_switch ? int'(tester.clock_mode) : check_clk_profile;
            automatic realtime expected = expected_clk_period(mode, c);
            if (last_mode >= 0 && expected != expected_clk_period(last_mode, c)) begin
                settle_edges = CLK_SETTLE_EDGES;
            end
            if (last_edge != 0) begin
                automatic realtime period = $realtime - last_edge;
                if (period <= expected + 2ps && period >= expected - 2ps) begin
                    settle_edges = 0;
                end else if (settle_edges != 0) begin
                    settle_edges--;
                end else if (!reset[COLD_RESET_IDX]) begin
                    $error("clk[%0d] period %0.1f ps, expected %0.1f ps for clock mode %0d",
                           c, period / 1ps, expected / 1ps, mode);
                end
            end
            last_mode = mode;
            last_edge = $realtime;
        end
    end

    /* verilator lint_off WIDTHEXPAND */
    assign core_no_fetch[cvm_topology_gen::mods.TOP.PLATFORM.NHARTS-1:0] = {cvm_topology_gen::mods.TOP.PLATFORM.NHARTS{reset[COLD_RESET_IDX] || reset[WARM_RESET_IDX]}};
    /* verilator lint_on WIDTHEXPAND */

    // TB-model reset per clock domain; same policy as the cluster harness.
    assign dut_reset[TB_CLK_IDX]   = reset[COLD_RESET_IDX] || reset[WARM_RESET_IDX];
    assign dut_reset[CORE_CLK_IDX] = &core_no_fetch || reset[WARM_RESET_IDX] || warm_reset_pullup;
    assign dut_reset[AXI_CLK_IDX]  = reset_window || reset[WARM_RESET_IDX] || warm_reset_pullup;
    assign dut_reset[SOC_CLK_IDX]  = reset[COLD_RESET_IDX];
    assign dut_reset[REF_CLK_IDX]  = reset_window;

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

    function automatic void write_rvfi_store(int unsigned hartid, longint unsigned paddr, byte unsigned wmask,
                                             longint unsigned wdata, int unsigned attr);
        int unsigned idx = cvm_topology_gen::mods.TOP.PLATFORM.COSIM.RVFI.NRETS_CUMSUM[hartid];
        rvfi_next[idx].mem_addr = paddr;
        rvfi_next[idx].mem_paddr = paddr[PALEN-1:0];
        rvfi_next[idx].mem_wmask = wmask;
        rvfi_next[idx].mem_wdata = wdata;
        rvfi_next[idx].mem_attr = attr;
    endfunction

    function automatic void write_rvfi_trap(int unsigned order, int unsigned hartid, longint unsigned pc, longint unsigned cause);
        int unsigned idx = cvm_topology_gen::mods.TOP.PLATFORM.COSIM.RVFI.NRETS_CUMSUM[hartid];
        write_rvfi(1, order, hartid, 0, 0, pc, 0, 0);
        rvfi_next[idx].trap = '1;
        rvfi_next[idx].cause = cause;
    endfunction

    rv_tester_params::msi_t imsic_msi_next [cvm_topology_gen::mods.TOP.PLATFORM.NHARTS-1:0];

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
