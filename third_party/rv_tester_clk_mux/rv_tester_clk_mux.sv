// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Tenstorrent AI ULC
//
// Simulation-only clock mux. Replaces pulp-platform/common_cells'
// `clk_mux_glitch_free` for rv_tester's simulation testbenches, which
// don't need glitch-free behavior. Combinational; do NOT synthesize.
module rv_tester_clk_mux #(
    parameter int unsigned NUM_INPUTS = 2,
    localparam int unsigned SelWidth  = $clog2(NUM_INPUTS)
) (
    input  logic [NUM_INPUTS-1:0] clks_i,
    input  logic [SelWidth-1:0]   async_sel_i,
    output logic                  clk_o
);
    assign clk_o = clks_i[async_sel_i];
endmodule
