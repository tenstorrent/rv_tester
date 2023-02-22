module cosim #(
    rv_tester_pkg::cfg_t CFG              =    '0,
    parameter int NUM                     =    -1,
    `RV_TESTER_PARAMETERS(CFG),
    `TOPOLOGY
)(
    input clk,
    input reset,
    input longint unsigned clocks,
    input rvfi_t rvfi[CFG.NRET],
    input mcmi_t mcmi_store[CFG.STQ_PORTS],
    input rv_tester_pkg::interrupt_t interrupt,
    input debug_mode
);

    typedef longint unsigned LU;

    int unsigned loc = cvm_topology::nil;
    bit rvfi_enabled;

    always @(posedge clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            loc = cvm_topology::get_location(topology.PLATFORM, 0);
            rvfi_enabled = cvm_plusargs::get_bool("rvfi") != '0;
            /* verilator lint_on BLKSEQ */
        end
    end

    // CVM transactions
    `COSIM_TRANSACTIONS_DOMAIN(1, clk)

    // m_rvfi
    for (genvar n = 0; n < CFG.NRET; n++) begin
        assign tx_dom_1.m_rvfis[n].valid = ~reset & rvfi[n].valid & rvfi_enabled;
        assign tx_dom_1.m_rvfis[n].data.location = loc;
        assign tx_dom_1.m_rvfis[n].data.cycle = clocks;
        assign tx_dom_1.m_rvfis[n].data.order = rvfi[n].order;
        assign tx_dom_1.m_rvfis[n].data.insn = rvfi[n].insn;
        assign tx_dom_1.m_rvfis[n].data.trap = rvfi[n].trap;
        assign tx_dom_1.m_rvfis[n].data.cause = rvfi[n].cause;
        assign tx_dom_1.m_rvfis[n].data.intr = rvfi[n].intr;
        assign tx_dom_1.m_rvfis[n].data.mode = rvfi[n].mode;
        assign tx_dom_1.m_rvfis[n].data.ixl = rvfi[n].ixl;
        assign tx_dom_1.m_rvfis[n].data.rd_addr = rvfi[n].rd_addr;
        assign tx_dom_1.m_rvfis[n].data.rd_wdata = rvfi[n].rd_wdata;
        assign tx_dom_1.m_rvfis[n].data.pc_rdata = rvfi[n].pc_rdata;
        assign tx_dom_1.m_rvfis[n].data.pc_wdata = rvfi[n].pc_wdata;
        assign tx_dom_1.m_rvfis[n].data.mem_addr = rvfi[n].mem_addr;
        assign tx_dom_1.m_rvfis[n].data.mem_paddr = rvfi[n].mem_paddr;
        assign tx_dom_1.m_rvfis[n].data.mem_rmask = rvfi[n].mem_rmask;
        assign tx_dom_1.m_rvfis[n].data.mem_rdata = rvfi[n].mem_rdata;
        assign tx_dom_1.m_rvfis[n].data.mem_wmask = rvfi[n].mem_wmask;
        assign tx_dom_1.m_rvfis[n].data.mem_wdata = rvfi[n].mem_wdata;
    end

    // m_mcmi_store
    for (genvar n = 0; n < CFG.STQ_PORTS; n++) begin
        assign tx_dom_1.m_mcmi_stores[n].valid = ~reset & mcmi_store[n].valid;
        assign tx_dom_1.m_mcmi_stores[n].data.location = loc;
        assign tx_dom_1.m_mcmi_stores[n].data.cycle = clocks;
        assign tx_dom_1.m_mcmi_stores[n].data.order = mcmi_store[n].order;
        assign tx_dom_1.m_mcmi_stores[n].data.addr = mcmi_store[n].addr;
        assign tx_dom_1.m_mcmi_stores[n].data.size = mcmi_store[n].size;
        assign tx_dom_1.m_mcmi_stores[n].data.data = mcmi_store[n].data;
    end
    
    // m_trap
    for (genvar n = 0; n < CFG.NRET; n++) begin
        assign tx_dom_1.m_traps[n].valid = ~reset & (rvfi[n].cause != 0);
        assign tx_dom_1.m_traps[n].data.location = loc;
        assign tx_dom_1.m_traps[n].data.cycle = clocks;
        assign tx_dom_1.m_traps[n].data.cause = rvfi[n].cause;
    end

    // m_debug
    logic debug_mode_d1;
    always @(posedge clk) begin
      debug_mode_d1 <= debug_mode;
    end
    assign tx_dom_1.m_debugs[0].valid = ~reset & ((debug_mode & ~debug_mode_d1) | (~debug_mode & debug_mode_d1));
    assign tx_dom_1.m_debugs[0].data.location = loc;
    assign tx_dom_1.m_debugs[0].data.cycle = clocks;
    assign tx_dom_1.m_debugs[0].data.enter = debug_mode;
    assign tx_dom_1.m_debugs[0].data.exit = ~debug_mode;

    // FIXME derive transactions depth from NRET
    for (genvar n = CFG.NRET; n < 8; n++) begin
        assign tx_dom_1.m_rvfis[n].valid = '0;
        assign tx_dom_1.m_traps[n].valid = '0;
    end

    if (CFG.NRET > 8) begin
        $error("NRET %d exceeds transactions.yml depth 8", CFG.NRET);
    end

    // m_intr
    assign tx_dom_1.m_intrs[0].valid = ~reset & 1'b0;
    assign tx_dom_1.m_intrs[0].data.location = loc;
    assign tx_dom_1.m_intrs[0].data.cycle = clocks;

    // Timeout checks
    int max_stall_cycle = 50000;
    int max_cycle;
    int cycles_since_retire;

    always @(posedge clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            max_cycle = cvm_plusargs::get_int("max_cycle");
            max_stall_cycle = cvm_plusargs::get_int("max_stall_cycle");
            /* verilator lint_on BLKSEQ */
            cycles_since_retire <= 0;
        end else begin
            cycles_since_retire <= cycles_since_retire + 1;
            if (rvfi[0].valid !== 0) begin
              cycles_since_retire <= 0;
            end
            if (max_stall_cycle > 0 && cycles_since_retire > max_stall_cycle) begin
              $display("Error: No instruction retired for max_stall_cycle (%0d) cycles", max_stall_cycle); 
              $finish;
            end
            if (max_cycle > 0 && clocks > LU'(max_cycle)) begin
              $display("Error: Test running for max_cycle (%0d) cycles - stuck in a loop, or too long", max_cycle);
              $finish;
            end
        end
    end

endmodule
