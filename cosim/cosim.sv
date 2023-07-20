module cosim #(
    parameter int NUM                     =    -1,
    `TOPOLOGY
)(
    input clk,
    input reset,
    input longint unsigned clocks,
    input rv_tester_params::rvfi_t rvfi[topology.TOP.CLUSTER.CORE.NRET],
    input rv_tester_params::mcmi_t mcmi_store[topology.TOP.CLUSTER.CORE.STQ_PORTS],
    input rv_tester_pkg::interrupt_t interrupt,
    input debug_mode,
    `RV_TESTER_TRANSACTIONS_OUTPUT_COSIM
);

    import "DPI-C" context function void cosim_set_scope(int unsigned location);

    typedef longint unsigned LU;
    int unsigned location = cvm_topology::nil;
    bit rvfi_enabled;

    always @(posedge clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            location = cvm_topology::get_location(topology.TOP.PLATFORM.COSIM.ID, 0);
            rvfi_enabled = (cvm_plusargs::get_bool("rvfi") != '0) & (location != cvm_topology::nil);
            if (rvfi_enabled) begin
              cosim_set_scope(location);
            end
            /* verilator lint_on BLKSEQ */
        end
    end

    // m_rvfi
    for (genvar n = 0; n < topology.TOP.CLUSTER.CORE.NRET; n++) begin
        assign m_rvfis[n].valid = ~reset & rvfi[n].valid & rvfi_enabled;
        assign m_rvfis[n].data.location = location;
        assign m_rvfis[n].data.cycle = clocks;
        assign m_rvfis[n].data.last_uop = rvfi[n].last_uop;
        assign m_rvfis[n].data.comp = rvfi[n].comp;
        assign m_rvfis[n].data.order = rvfi[n].order;
        assign m_rvfis[n].data.insn = rvfi[n].insn;
        assign m_rvfis[n].data.uop = rvfi[n].uop;
        assign m_rvfis[n].data.trap = rvfi[n].trap;
        assign m_rvfis[n].data.cause = rvfi[n].cause;
        assign m_rvfis[n].data.intr = rvfi[n].intr;
        assign m_rvfis[n].data.mode = rvfi[n].mode;
        assign m_rvfis[n].data.ixl = rvfi[n].ixl;
        assign m_rvfis[n].data.rd_addr = rvfi[n].rd_addr;
        assign m_rvfis[n].data.rd_wdata = rvfi[n].rd_wdata;
        assign m_rvfis[n].data.frd_valid = rvfi[n].frd_valid;
        assign m_rvfis[n].data.frd_addr = rvfi[n].frd_addr;
        assign m_rvfis[n].data.frd_wdata = rvfi[n].frd_wdata;
        assign m_rvfis[n].data.pc_rdata = rvfi[n].pc_rdata;
        assign m_rvfis[n].data.pc_wdata = rvfi[n].pc_wdata;
        assign m_rvfis[n].data.mem_addr = rvfi[n].mem_addr;
        assign m_rvfis[n].data.mem_paddr = rvfi[n].mem_paddr;
        assign m_rvfis[n].data.mem_rmask = rvfi[n].mem_rmask;
        assign m_rvfis[n].data.mem_rdata = rvfi[n].mem_rdata;
        assign m_rvfis[n].data.mem_wmask = rvfi[n].mem_wmask;
        assign m_rvfis[n].data.mem_wdata = rvfi[n].mem_wdata;
    end

    // m_mcmi_store
    for (genvar n = 0; n < topology.TOP.CLUSTER.CORE.STQ_PORTS; n++) begin
        assign m_mcmi_stores[n].valid = ~reset & mcmi_store[n].valid & rvfi_enabled;
        assign m_mcmi_stores[n].data.location = location;
        assign m_mcmi_stores[n].data.cycle = clocks;
        assign m_mcmi_stores[n].data.order = mcmi_store[n].order;
        assign m_mcmi_stores[n].data.addr = mcmi_store[n].addr;
        assign m_mcmi_stores[n].data.size = mcmi_store[n].size;
        assign m_mcmi_stores[n].data.data = mcmi_store[n].data;
    end

    // m_trap
    for (genvar n = 0; n < topology.TOP.CLUSTER.CORE.NRET; n++) begin
        assign m_traps[n].valid = ~reset & (rvfi[n].cause != 0) & rvfi_enabled;
        assign m_traps[n].data.location = location;
        assign m_traps[n].data.cycle = clocks;
        assign m_traps[n].data.cause = rvfi[n].cause;
    end

    // m_debug
    logic debug_mode_d1;
    always @(posedge clk) begin
      debug_mode_d1 <= debug_mode;
    end
    assign m_debugs[0].valid = ~reset & ((debug_mode & ~debug_mode_d1) | (~debug_mode & debug_mode_d1)) & rvfi_enabled;
    assign m_debugs[0].data.location = location;
    assign m_debugs[0].data.cycle = clocks;
    assign m_debugs[0].data.enter = debug_mode;
    assign m_debugs[0].data.exit = ~debug_mode;

    // m_intr
    rv_tester_pkg::interrupt_t interrupt_d1;
    always @(posedge clk) begin
      interrupt_d1 <= interrupt;
    end
    assign m_intrs[0].valid = ~reset & (|(interrupt & ~interrupt_d1) | |(~interrupt & interrupt_d1)
      | (interrupt.sei & ~interrupt_d1.sei) | (~interrupt.sei & interrupt_d1.sei)) & rvfi_enabled;
    assign m_intrs[0].data.location = location;
    assign m_intrs[0].data.cycle = clocks;
    assign m_intrs[0].data.mip_posedge = |(interrupt & ~interrupt_d1);
    assign m_intrs[0].data.mip = get_mip(interrupt);
    assign m_intrs[0].data.seip_posedge = (interrupt.sei & ~interrupt_d1.sei);
    assign m_intrs[0].data.seip_negedge = (~interrupt.sei & interrupt_d1.sei);
    assign m_intrs[0].data.seip = interrupt.sei;
    assign m_intrs[0].data.stip_negedge = (~interrupt.sti & interrupt_d1.sti);

    function automatic bit [63:0] get_mip(rv_tester_pkg::interrupt_t intr);
      bit [63:0] mip = 'h0;
      mip[11] = intr.mei;
      mip[7]  = intr.mti;
      mip[3]  = intr.msi;
      mip[1]  = intr.ssi;
      return mip;
    endfunction

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
