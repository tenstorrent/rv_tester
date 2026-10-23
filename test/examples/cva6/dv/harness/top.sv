// SPDX-FileCopyrightText: © 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// Simulation top: rv_tester platform + CVA6 DUT harness
module top
    import rv_tester_params::*;
(
    input clk_ext [NCLKS-1:0]
);

    `RV_TESTER_VARS(cvm_topology_gen::mods)

    rv_tester #(
        .TOPOLOGY(cvm_topology_gen::topology_t),
        .topology(cvm_topology_gen::mods)
    ) tester (
        .*
    );

    cva6_test_harness #(
        .TOPOLOGY(cvm_topology_gen::topology_t),
        .topology(cvm_topology_gen::mods)
    ) dut_harness (
        .*
    );

    // TB-model reset per clock domain; same policy as the sw_testbench harness.
    assign dut_reset[TB_CLK_IDX]   = reset[COLD_RESET_IDX] || reset[WARM_RESET_IDX];
    assign dut_reset[CORE_CLK_IDX] = &core_no_fetch || reset[WARM_RESET_IDX] || warm_reset_pullup;
    assign dut_reset[AXI_CLK_IDX]  = reset_window || reset[WARM_RESET_IDX] || warm_reset_pullup;
    assign dut_reset[SOC_CLK_IDX]  = reset[COLD_RESET_IDX];
    assign dut_reset[REF_CLK_IDX]  = reset_window;

endmodule
