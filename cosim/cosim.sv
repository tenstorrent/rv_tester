module cosim
import rv_tester_params::*;
#(
    parameter int NUM = -1,
    parameter int NRET = 1,
    parameter int NREAD = 1,
    parameter int NINSERT = 1,
    parameter int NWRITE = 1,
    parameter int NBYPWRITE = 1,
    parameter int RESET_CLOCKS = 10,
    `TOPOLOGY,
    `RV_TESTER_TRANSACTIONS_COSIM_OUTPUT_PARAMS
)(
    input clk,
    input reset,
    input dut_reset,
    input longint unsigned clocks,
    input rvfi_t [NRET-1:0] rvfi,
    input csri_t csri,
    input mcmi_t [NREAD-1:0] mcmi_read,
    input mcmi_t [NINSERT-1:0] mcmi_insert,
    input mcmi_t [NWRITE-1:0] mcmi_write,
    input mcmi_t [NBYPWRITE-1:0] mcmi_bypass_write,
    input rv_tester_pkg::interrupt_t interrupt,
    input debug_mode,
    output rv_tester_pkg::terminate_t terminate,
    `RV_TESTER_TRANSACTIONS_COSIM_OUTPUT_PORTS
);

    import "DPI-C" context function void cosim_set_scope(int unsigned location);

    typedef longint unsigned LU;
    int unsigned location = cvm_topology::nil;
    bit rvfi_enabled;

    always @(posedge clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            location = cvm_topology::get_location(topology.TOP.PLATFORM.COSIM.ID, NUM);
            rvfi_enabled = (cvm_plusargs::get_bool("rvfi") != '0) & (location != cvm_topology::nil);
            if (rvfi_enabled) begin
              cosim_set_scope(location);
            end
            terminate.terminate = '0;
            /* verilator lint_on BLKSEQ */
        end
    end

    function void cosim_terminate ();
        $display("attempting to terminate");
        /* verilator lint_off BLKSEQ */
        terminate.terminate = '1;
        /* verilator lint_on BLKSEQ */
    endfunction

    // m_rvfi
    for (genvar n = 0; n < NRET; n++) begin
        assign m_rvfis[n].valid = RVFI_EN & rvfi_enabled & ~dut_reset & rvfi[n].valid;
        assign m_rvfis[n].data.location = location;
        assign m_rvfis[n].data.cycle = clocks;
        assign m_rvfis[n].data.hart = NUM;
        assign m_rvfis[n].data.last_uop = rvfi[n].last_uop;
        assign m_rvfis[n].data.last_insn = rvfi[n].last_insn;
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
        assign m_rvfis[n].data.vrd_valid = rvfi[n].vrd_valid;
        assign m_rvfis[n].data.vrd_addr = rvfi[n].vrd_addr;
        assign m_rvfis[n].data.vrd_wdata = rvfi[n].vrd_wdata;
        assign m_rvfis[n].data.csr_valid = rvfi[n].csr_valid;
        assign m_rvfis[n].data.csr_addr = rvfi[n].csr_addr;
        assign m_rvfis[n].data.csr_wdata = rvfi[n].csr_wdata;
        assign m_rvfis[n].data.csr_wmask = rvfi[n].csr_wmask;
        assign m_rvfis[n].data.pc_rdata = rvfi[n].pc_rdata;
        assign m_rvfis[n].data.pc_wdata = rvfi[n].pc_wdata;
        assign m_rvfis[n].data.mem_addr = rvfi[n].mem_addr;
        assign m_rvfis[n].data.mem_paddr = rvfi[n].mem_paddr;
        assign m_rvfis[n].data.mem_rmask = rvfi[n].mem_rmask;
        assign m_rvfis[n].data.mem_rdata = rvfi[n].mem_rdata;
        assign m_rvfis[n].data.mem_wmask = rvfi[n].mem_wmask;
        assign m_rvfis[n].data.mem_wdata = rvfi[n].mem_wdata;
    end

    // m_csri
    for (genvar n = 0; n < CSR_COUNT; n++) begin
        assign m_csris[n].valid = rvfi_enabled & ~reset & csri[n].valid & (csri[n].addr != 'h300 && csri[n].addr != 'h344); //FIXME Remove qualifiers for mstatus/mip after MC bug fix
        assign m_csris[n].data.location = location;
        assign m_csris[n].data.cycle = csri[n].valid ? clocks : '0;
        assign m_csris[n].data.hart = NUM;
        assign m_csris[n].data.addr = csri[n].addr;
        assign m_csris[n].data.mask = csri[n].mask;
        assign m_csris[n].data.data = csri[n].data;
    end

    // m_mcmi_read
    for (genvar n = 0; n < NREAD; n++) begin
        assign m_mcmi_reads[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_read[n].valid;
        assign m_mcmi_reads[n].data.location = location;
        /* verilator lint_off WIDTH */
        assign m_mcmi_reads[n].data.cycle = mcmi_read[n].valid ? (mcmi_read[n].cycle + RESET_CLOCKS + 1) : '0;
        /* verilator lint_on WIDTH */
        assign m_mcmi_reads[n].data.hart = NUM;
        assign m_mcmi_reads[n].data.order = mcmi_read[n].order;
        assign m_mcmi_reads[n].data.addr = mcmi_read[n].addr;
        assign m_mcmi_reads[n].data.mask = mcmi_read[n].mask;
        assign m_mcmi_reads[n].data.data = mcmi_read[n].data[63:0];
    end

    // m_mcmi_insert
    for (genvar n = 0; n < NINSERT; n++) begin
        assign m_mcmi_inserts[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_insert[n].valid;
        assign m_mcmi_inserts[n].data.location = location;
        assign m_mcmi_inserts[n].data.cycle = mcmi_insert[n].valid ? clocks : '0;
        assign m_mcmi_inserts[n].data.hart = NUM;
        assign m_mcmi_inserts[n].data.order = mcmi_insert[n].order;
        assign m_mcmi_inserts[n].data.addr = mcmi_insert[n].addr;
        assign m_mcmi_inserts[n].data.mask = mcmi_insert[n].mask;
        assign m_mcmi_inserts[n].data.data = mcmi_insert[n].data[63:0];
    end

    // m_mcmi_write
    for (genvar n = 0; n < NWRITE; n++) begin
        assign m_mcmi_writes[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_write[n].valid;
        assign m_mcmi_writes[n].data.location = location;
        assign m_mcmi_writes[n].data.cycle = mcmi_write[n].valid ? clocks : '0;
        assign m_mcmi_writes[n].data.hart = NUM;
        assign m_mcmi_writes[n].data.addr = mcmi_write[n].addr;
        assign m_mcmi_writes[n].data.mask = mcmi_write[n].mask;
        assign m_mcmi_writes[n].data.data = mcmi_write[n].data;
    end

    // m_mcmi_bypass_write
    for (genvar n = 0; n < NBYPWRITE; n++) begin
        assign m_mcmi_bypass_writes[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_bypass_write[n].valid;
        assign m_mcmi_bypass_writes[n].data.location = location;
        assign m_mcmi_bypass_writes[n].data.cycle = mcmi_bypass_write[n].valid ? clocks : '0;
        assign m_mcmi_bypass_writes[n].data.hart = NUM;
        assign m_mcmi_bypass_writes[n].data.order = mcmi_bypass_write[n].order;
        assign m_mcmi_bypass_writes[n].data.addr = mcmi_bypass_write[n].addr;
        assign m_mcmi_bypass_writes[n].data.mask = mcmi_bypass_write[n].mask;
        assign m_mcmi_bypass_writes[n].data.data = mcmi_bypass_write[n].data[63:0];
    end

    // m_trap
    for (genvar n = 0; n < NRET; n++) begin
        assign m_traps[n].valid = RVFI_EN & rvfi_enabled & ~dut_reset & (rvfi[n].cause != 0);
        assign m_traps[n].data.location = location;
        assign m_traps[n].data.cycle = clocks;
        assign m_traps[n].data.cause = rvfi[n].cause;
    end

    // m_debug
    logic debug_mode_d1;
    always @(posedge clk) begin
      debug_mode_d1 <= debug_mode;
    end
    assign m_debugs[0].valid = ~dut_reset & ((debug_mode & ~debug_mode_d1) | (~debug_mode & debug_mode_d1)) & rvfi_enabled;
    assign m_debugs[0].data.location = location;
    assign m_debugs[0].data.cycle = clocks;
    assign m_debugs[0].data.enter = debug_mode;
    assign m_debugs[0].data.exit = ~debug_mode;

    // m_intr
    rv_tester_pkg::interrupt_t interrupt_d1;
    always @(posedge clk) begin
      interrupt_d1 <= interrupt;
    end
    assign m_intrs[0].valid = ~dut_reset & (|(interrupt & ~interrupt_d1) | |(~interrupt & interrupt_d1)
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
    longint unsigned max_cycle;
    int cycles_since_retire;
    longint unsigned hart_enable_mask;
    bit boot_wfi;

    always @(posedge clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            max_cycle = cvm_plusargs::get_ulongint("max_cycle");
            max_stall_cycle = cvm_plusargs::get_int("max_stall_cycle");
            hart_enable_mask = cvm_plusargs::get_ulongint("hart_enable_mask");
            /* verilator lint_on BLKSEQ */
            cycles_since_retire <= 0;
            boot_wfi <= '0;
        end else if(!dut_reset) begin
            cycles_since_retire <= cycles_since_retire + 1;
            if (rvfi[0].valid !== 0) begin
              cycles_since_retire <= 0;
            end
            if (NUM != 0 && hart_enable_mask[NUM] == 1 && rvfi[0].valid !== 0 && rvfi[0].insn[6:0] == 7'h73 && rvfi[0].pc_rdata < 'h20000) begin // WFI
              boot_wfi <= '1;
            end
            if (max_stall_cycle > 0 && cycles_since_retire > max_stall_cycle && !boot_wfi) begin
              $display("Error: Hart%0d: No instruction retired for max_stall_cycle (%0d) cycles", NUM, max_stall_cycle);
              cosim_terminate();
            end
            if (max_cycle > 0 && clocks > max_cycle) begin
              $display("Error:Hart%0d:  Test running for max_cycle (%0d) cycles - stuck in a loop, or too long", NUM, max_cycle);
              cosim_terminate();
            end
        end
    end

endmodule
