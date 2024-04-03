module cosim
import rv_tester_params::*;
#(
    parameter int NUM = -1,
    parameter int NRET = 1,
    parameter int NREAD = 1,
    parameter int NINSERT = 1,
    parameter int NWRITE = 1,
    parameter int NBYPASS = 1,
    parameter int NIFETCH = 1,
    parameter int NIEVICT = 1,
    parameter int RESET_CLOCKS = 10,
    `TOPOLOGY,
    `RV_TESTER_TRANSACTIONS_COSIM_OUTPUT_PARAMS
)(
    input tb_clk,
    input clk,
    input reset,
    input dut_reset,
    input longint unsigned clocks,
    input rvfi_t [NRET-1:0] rvfi,
    input csri_t csri,
    input mcmi_t [NREAD-1:0] mcmi_read,
    input mcmi_t [NINSERT-1:0] mcmi_insert,
    input mcmi_t [NWRITE-1:0] mcmi_write,
    input mcmi_t [NBYPASS-1:0] mcmi_bypass,
    input mcmi_t [NIFETCH-1:0] mcmi_ifetch_req,
    input mcmi_t [NIFETCH-1:0] mcmi_ifetch_resp,
    input mcmi_t [NIEVICT-1:0] mcmi_ievict,
    input rv_tester_pkg::interrupt_t wired_interrupt,
    input rv_tester_params::mst_req_top imsic_interrupt,
    input debug_mode,
    output rv_tester_pkg::terminate_t terminate,
    `RV_TESTER_TRANSACTIONS_COSIM_OUTPUT_PORTS
);

    import "DPI-C" context function void cosim_set_scope(int unsigned location);

    typedef longint unsigned LU;
    int unsigned location = cvm_topology::nil;
    bit rvfi_enabled;

    always @(posedge tb_clk) begin
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
        $display("[cosim]: attempting to terminate");
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
        assign m_rvfis[n].data.vec_cracked = rvfi[n].vec_cracked;
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
        assign m_rvfis[n].data.mem_attr = rvfi[n].mem_attr;
    end

    // m_csri
    logic [CSR_COUNT-1:0] valid_d1;
    logic [CSR_COUNT-1:0][63:0] data_d1;
    logic [CSR_COUNT-1:0][63:0] mask_d1;
    always @(posedge clk) begin
      for (int n = 0; n < CSR_COUNT; n++) begin
        valid_d1[n] <= csri[n].valid;
        data_d1[n] <= csri[n].data;
        mask_d1[n] <= csri[n].mask;
      end
    end
    for (genvar n = 0; n < CSR_COUNT; n++) begin
        assign m_csris[n].valid = rvfi_enabled & ~dut_reset & ((csri[n].valid & ~valid_d1[n]) | (csri[n].valid & ((csri[n].data !== data_d1[n]) | (csri[n].mask !== mask_d1[n]))));
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
        assign m_mcmi_reads[n].data.cycle = mcmi_read[n].valid ? clocks : '0;
        /* verilator lint_on WIDTH */
        assign m_mcmi_reads[n].data.hart = NUM;
        assign m_mcmi_reads[n].data.order = mcmi_read[n].order;
        assign m_mcmi_reads[n].data.addr = mcmi_read[n].addr;
        assign m_mcmi_reads[n].data.mask = mcmi_read[n].mask;
        assign m_mcmi_reads[n].data.data = mcmi_read[n].data[63:0];
        assign m_mcmi_reads[n].data.amo = mcmi_read[n].amo;
        assign m_mcmi_reads[n].data.amo_op = mcmi_read[n].amo_op;
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

    // m_mcmi_bypass
    for (genvar n = 0; n < NBYPASS; n++) begin
        assign m_mcmi_bypasss[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_bypass[n].valid;
        assign m_mcmi_bypasss[n].data.location = location;
        assign m_mcmi_bypasss[n].data.cycle = mcmi_bypass[n].valid ? clocks : '0;
        assign m_mcmi_bypasss[n].data.hart = NUM;
        assign m_mcmi_bypasss[n].data.order = mcmi_bypass[n].order;
        assign m_mcmi_bypasss[n].data.addr = mcmi_bypass[n].addr;
        assign m_mcmi_bypasss[n].data.mask = mcmi_bypass[n].mask;
        assign m_mcmi_bypasss[n].data.data = mcmi_bypass[n].data[63:0];
        assign m_mcmi_bypasss[n].data.amo = mcmi_bypass[n].amo;
        assign m_mcmi_bypasss[n].data.amo_op = mcmi_bypass[n].amo_op;
    end

    // m_mcmi_ifetch
    for (genvar n = 0; n < NIFETCH; n++) begin
        assign m_mcmi_ifetch_reqs[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_ifetch_req[n].valid;
        assign m_mcmi_ifetch_reqs[n].data.location = location;
        assign m_mcmi_ifetch_reqs[n].data.cycle = mcmi_ifetch_req[n].valid ? clocks : '0;
        assign m_mcmi_ifetch_reqs[n].data.hart = NUM;
        assign m_mcmi_ifetch_reqs[n].data.order = mcmi_ifetch_req[n].order;
        assign m_mcmi_ifetch_reqs[n].data.addr = mcmi_ifetch_req[n].addr;
    end

    for (genvar n = 0; n < NIFETCH; n++) begin
        assign m_mcmi_ifetch_resps[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_ifetch_resp[n].valid;
        assign m_mcmi_ifetch_resps[n].data.location = location;
        assign m_mcmi_ifetch_resps[n].data.cycle = mcmi_ifetch_resp[n].valid ? clocks : '0;
        assign m_mcmi_ifetch_resps[n].data.hart = NUM;
        assign m_mcmi_ifetch_resps[n].data.order = mcmi_ifetch_resp[n].order;
    end

    // m_mcmi_ievict
    for (genvar n = 0; n < NIEVICT; n++) begin
        assign m_mcmi_ievicts[n].valid = MCMI_EN & rvfi_enabled & ~dut_reset & mcmi_ievict[n].valid;
        assign m_mcmi_ievicts[n].data.location = location;
        assign m_mcmi_ievicts[n].data.cycle = mcmi_ievict[n].valid ? clocks : '0;
        assign m_mcmi_ievicts[n].data.hart = NUM;
        assign m_mcmi_ievicts[n].data.addr = mcmi_ievict[n].addr;
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

    // m_core_intr
    rv_tester_pkg::interrupt_t wired_interrupt_d1;
    always @(posedge clk) begin
      if (reset) begin
        wired_interrupt_d1 <= 0;
      end else begin
        wired_interrupt_d1 <= wired_interrupt;
      end
    end
    assign m_core_intrs[0].valid = ~dut_reset & (|(wired_interrupt & ~wired_interrupt_d1) | |(~wired_interrupt & wired_interrupt_d1)) & rvfi_enabled;
    assign m_core_intrs[0].data.location = location;
    assign m_core_intrs[0].data.cycle = clocks;
    assign m_core_intrs[0].data.mip = get_mip(wired_interrupt);
    assign m_core_intrs[0].data.mip_mask = get_mip_mask(wired_interrupt, wired_interrupt_d1);
    assign m_core_intrs[0].data.mip_assert = get_mip_assert(wired_interrupt, wired_interrupt_d1);

    function automatic bit [63:0] get_mip(rv_tester_pkg::interrupt_t intr);
      bit [63:0] mip = 'h0;
      mip[12] = intr.sgei;
      mip[11] = intr.mei;
      mip[10]  = intr.vsei;
      mip[9]  = intr.sei;
      mip[7]  = intr.mti;
      mip[6]  = intr.vsti;
      mip[5]  = intr.sti;
      mip[3]  = intr.msi;
      mip[1]  = intr.ssi;
      return mip;
    endfunction


    localparam imsic_whisper_delays = 5;
    rv_tester_params::mst_req_top imsic_interrupt_delays[imsic_whisper_delays:0];
    rv_tester_params::mst_req_top imsic_interrupt_delayed;
    assign imsic_interrupt_delays[0]=imsic_interrupt;
    genvar i;
   generate
    for (i=1; i <= imsic_whisper_delays; i=i+1) begin
      always @(posedge clk)
      imsic_interrupt_delays[i] <= imsic_interrupt_delays[i-1];
    end
   endgenerate
   assign imsic_interrupt_delayed = imsic_interrupt_delays[imsic_whisper_delays];

    // m_imsic_msi
    enum logic {idle, aw} msi_slave_state,msi_slave_state_d;
    logic msi_addr_in_imsic_range;
    always @(posedge clk) begin
       if (reset) begin
        msi_slave_state <= idle;
       end else begin
        msi_slave_state <= msi_slave_state_d;
       end
    end
    assign msi_slave_state_d = imsic_interrupt_delayed.w_valid ? idle : imsic_interrupt_delayed.aw_valid ? aw : msi_slave_state;
    assign msi_addr_in_imsic_range = (imsic_interrupt_delayed.aw.addr[31:0] >= 32'h40000000 &&  imsic_interrupt_delayed.aw.addr[31:0] < 32'h42000000) || (imsic_interrupt_delayed.aw.addr[31:0] >= 32'h44000000 &&  imsic_interrupt_delayed.aw.addr[31:0] < 32'h46000000);
    assign m_imsic_msis[0].valid = ~dut_reset & ( (msi_slave_state==aw | imsic_interrupt_delayed.aw_valid) & imsic_interrupt_delayed.w_valid & imsic_interrupt_delayed.w.strb=='hf & msi_addr_in_imsic_range) & rvfi_enabled;
    assign m_imsic_msis[0].data.location = location;
    assign m_imsic_msis[0].data.cycle = clocks;
    /* verilator lint_off WIDTH */
    assign m_imsic_msis[0].data.addr = imsic_interrupt_delayed.aw.addr;
    assign m_imsic_msis[0].data.data = imsic_interrupt_delayed.w.data;
    /* verilator lint_on WIDTH */

    function automatic bit [63:0] get_mip_mask(rv_tester_pkg::interrupt_t intr, rv_tester_pkg::interrupt_t intr_d1);
      bit [63:0] mask = 'h0;
      mask[12]  = (intr.sgei & ~intr_d1.sgei) | (~intr.sgei & intr_d1.sgei);
      mask[11] = (intr.mei & ~intr_d1.mei) | (~intr.mei & intr_d1.mei);
      mask[10]  = (intr.vsei & ~intr_d1.vsei) | (~intr.vsei & intr_d1.vsei);
      mask[9]  = (intr.sei & ~intr_d1.sei) | (~intr.sei & intr_d1.sei);
      mask[7]  = (intr.mti & ~intr_d1.mti) | (~intr.mti & intr_d1.mti);
      mask[6]  = (intr.vsti & ~intr_d1.vsti) | (~intr.vsti & intr_d1.vsti);
      mask[5]  = (intr.sti & ~intr_d1.sti) | (~intr.sti & intr_d1.sti);
      mask[3]  = (intr.msi & ~intr_d1.msi) | (~intr.msi & intr_d1.msi);
      mask[1]  = (intr.ssi & ~intr_d1.ssi) | (~intr.ssi & intr_d1.ssi);
      return mask;
    endfunction

    function automatic bit [63:0] get_mip_assert(rv_tester_pkg::interrupt_t intr, rv_tester_pkg::interrupt_t intr_d1);
      bit [63:0] mask = 'h0;
      mask[12]  = (intr.sgei & ~intr_d1.sgei);
      mask[11] = (intr.mei & ~intr_d1.mei);
      mask[10]  = (intr.sgei & ~intr_d1.sgei);
      mask[9]  = (intr.sei & ~intr_d1.sei);
      mask[7]  = (intr.mti & ~intr_d1.mti);
      mask[6]  = (intr.vsti & ~intr_d1.vsti);
      mask[5]  = (intr.sti & ~intr_d1.sti);
      mask[3]  = (intr.msi & ~intr_d1.msi);
      mask[1]  = (intr.ssi & ~intr_d1.ssi);
      return mask;
    endfunction

    // Timeout checks
    int max_stall_cycle = 50000;
    longint unsigned max_cycle;
    int cycles_since_retire;
    longint unsigned hart_enable_mask;
    bit boot_wfi;

    always @(posedge tb_clk) begin
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
            if (NUM != 0 && hart_enable_mask[NUM] == 0 && rvfi[0].valid !== 0 && rvfi[0].insn[6:0] == 7'h73 && rvfi[0].pc_rdata < 'h20000) begin // WFI
              boot_wfi <= '1;
            end
            if (max_stall_cycle > 0 && cycles_since_retire > max_stall_cycle && !boot_wfi) begin
              $display("Error: Hart %0d: No instruction retired for max_stall_cycle (%0d) cycles", NUM, max_stall_cycle);
              cosim_terminate();
            end
            if (max_cycle > 0 && clocks > max_cycle) begin
              $display("Error: Hart %0d:  Test running for max_cycle (%0d) cycles - stuck in a loop, or too long", NUM, max_cycle);
              cosim_terminate();
            end
        end
    end

endmodule
