module pmu
import rv_tester_params::*;
import rv_tester_pkg::*;
#(
  parameter int NUM = -1,
  parameter int NRET = 1,
  parameter logic SC_PMCI_ENABLED =  0,
  `TOPOLOGY,
  `RV_TESTER_TRANSACTIONS_PMU_CORE_OUTPUT_PARAMS,
  `RV_TESTER_TRANSACTIONS_PMU_SC_OUTPUT_PARAMS
)(
  input clk,
  input reset,
  input logic [63:0] clocks,
  input sys_reset,
  input pmci_t pmci,
  input hpmi_t hpmi,
  input sc_pmci_t sc_pmci,
  input rvfi_t [NRET-1:0] rvfi,
  input bit terminate,
  `RV_TESTER_TRANSACTIONS_PMU_CORE_OUTPUT_PORTS,
  `RV_TESTER_TRANSACTIONS_PMU_SC_OUTPUT_PORTS
);

    parameter int unsigned location = cvm_topology_gen::get_location (topology.TOP.PLATFORM.PMCI.ID, NUM);
    longint unsigned period = 0;
    longint unsigned instructions = 0;
    longint unsigned tb_cycles_offset = 0;
    bit cycle_sync_en, instruction_sync_en;
    assign cycle_sync_en = (period != '0);
    assign instruction_sync_en = (instructions != '0);
    bit perf_enabled = '0;
    bit perf_start = '0;
    bit perf_end = '0;
    logic terminate_1T;

    always @(posedge clk) begin
        if (sys_reset) begin
            /* verilator lint_off BLKSEQ */
            perf_enabled = (cvm_plusargs::get_bool("perf") != '0) & (location != cvm_topology::nil);
            period = cvm_plusargs::get_ulongint("sync_pmcounters_period");
            instructions = cvm_plusargs::get_ulongint("sync_pmcounters_instructions");
            tb_cycles_offset = cvm_plusargs::get_ulongint("perf_tb_cycles_rvfi_offset");
            /* verilator lint_on BLKSEQ */
        end
    end

    longint unsigned cpu_cycles = 0;
    longint unsigned sync_cycles = 0;
    longint unsigned sync_instructions = 0;
    longint unsigned prev_sync_instructions = 0;
    longint unsigned nret = {32'h0, NRET};
    longint unsigned pmcounter [EVENT_COUNT] = '{default:0};
    longint unsigned branch_instructions;

    always @(posedge clk) begin
      if (reset) begin
        pmcounter[0] <= 0;
        cpu_cycles <= 0;
        sync_cycles <= 0;
        sync_instructions <= 1;
      end else begin
        if (perf_start) begin
          sync_cycles <= 1;
          sync_instructions <= nret;
        end else if (perf_end) begin
          sync_cycles <= cpu_cycles;
          sync_instructions <= pmcounter[INSTRUCTIONS];
        end else begin
          sync_cycles <= sync_cycles + 1;
          sync_instructions <= sync_instructions + {60'h0, pmci[INSTRUCTIONS]};
        end
        cpu_cycles <= cpu_cycles + 1;
      end
      prev_sync_instructions <= sync_instructions;
      terminate_1T <= terminate;
    end

    generate
      for (genvar i=1; i < EVENT_COUNT; i++) begin : pmci_regs
        always @(posedge clk) begin
            if (reset) begin
                pmcounter[i] <= 0;
            end else begin
                pmcounter[i] <= pmcounter[i] + {60'h0, pmci[i]};
            end
        end
      end
    endgenerate

    always_comb begin
      branch_instructions = pmcounter[OP_RETIRED_DIRECT_BRANCH] + pmcounter[OP_RETIRED_RET_BRANCH] +
                            pmcounter[OP_RETIRED_INDIRECT_BRANCH] + pmcounter[OP_RETIRED_COND_BRANCH];
    end

    bit [NRET-1:0] perf_match_array_start;
    bit [NRET-1:0] perf_match_array_end;
    always @(*) begin
      for (integer n = 0; n < NRET; n++) begin
        perf_match_array_start[n] = (rvfi[n].insn == 32'h00058013) && rvfi[n].valid;
        perf_match_array_end[n] = (rvfi[n].insn == 32'h00060013) && rvfi[n].valid;
      end

      perf_start = | perf_match_array_start;
      perf_end = | perf_match_array_end;
    end

    logic [3:0]  instr_avg;

    //generate
    //   for(genvar i=0; i<EVENT_COUNT; i++) begin : evt_logic
    logic [3:0]  mod_pmci;

    assign mod_pmci = (pmci[INSTRUCTIONS][0]===1'bx) ? 4'b0 : pmci[INSTRUCTIONS];

    // 128-cycle moving average
    logic        ema_busy;
    logic        ema_en;

    assign ema_en   = |mod_pmci | ema_busy;

    pmu_ema #(.WIDTH(4), .DECAY(7))
    u_evt (.i_clk(clk), .i_reset_n(!reset), .i_en(ema_en),
            .i_activity(mod_pmci), .i_decayadj('0),
            .o_activity(instr_avg), .o_busy(ema_busy));
    //   end
    //endgenerate

    logic [NRET-1:0] mhpm_write;
    always_comb begin
      for (integer n = 0; n < NRET; n++) begin
        mhpm_write[n] = rvfi[n].valid && (
                        (rvfi[n].csr_addr == 12'h323)| (rvfi[n].csr_addr == 12'h324)|(rvfi[n].csr_addr == 12'h325)|(rvfi[n].csr_addr == 12'h326)|
                        (rvfi[n].csr_addr == 12'h327)|(rvfi[n].csr_addr == 12'h328)| (rvfi[n].csr_addr == 12'h329)|(rvfi[n].csr_addr == 12'h32A));
      end
    end

    always_comb begin
      for (integer i = 0; i < NRET; i++) begin
        pmc_checkers[i].valid = !reset  && (mhpm_write[i] || (terminate^terminate_1T));
        pmc_checkers[i].data.location   = location;
        pmc_checkers[i].data.terminate  = terminate;
        pmc_checkers[i].data.event_csr  = rvfi[i].csr_addr[3:0] - 4'h3;
        pmc_checkers[i].data.event_id   = rvfi[i].csr_wdata[55:0] & rvfi[i].csr_wmask[55:0];
      end
    end

    //AUTOGENERATED -- NO TOUCH
    localparam MAX_COUNTER_VALUE_CHANGE_IN_ONE_CYCLE = 32;
    parameter OVERFLOW_BIT = 24 - 1;
    if (OVERFLOW_BIT < ($clog2(MAX_COUNTER_VALUE_CHANGE_IN_ONE_CYCLE) + 1)) $error("The selected overflow bit cannot cover the maximum value change of a counter within a single clock cycle.");
    parameter OVERFLOW_BIT_EXTRA = 2;
    logic overflow;
    logic [EVENT_COUNT + SC_EVENT_COUNT + OVERFLOW_BIT_EXTRA -1 : 0] pmcounter_overflow_bit;
    assign pmcounters_cores[0].valid = !reset && perf_enabled && (overflow || (|mhpm_write) || terminate || (cycle_sync_en && (sync_cycles % period) == 0) || (instruction_sync_en && (((prev_sync_instructions % instructions) > nret) && ((sync_instructions % instructions) < nret))) || perf_start || perf_end);
    assign pmcounters_cores[0].data.location = location;
    assign pmcounters_cores[0].data.tb_cycles = 24'(clocks - tb_cycles_offset);
    assign pmcounters_cores[0].data.cpu_cycles = 24'(cpu_cycles);
    assign pmcounters_cores[0].data.instructions = 24'(pmcounter[INSTRUCTIONS]);
    assign pmcounters_cores[0].data.branch_instructions = 24'(branch_instructions);
    assign pmcounters_cores[0].data.perf_start = perf_start;
    assign pmcounters_cores[0].data.perf_end = perf_end;
    assign pmcounters_cores[0].data.terminate = terminate;
    assign pmcounters_cores[0].data.overflow = overflow;
    assign pmcounters_cores[0].data.sync = (cycle_sync_en && (sync_cycles % period) == 0) || (instruction_sync_en && (((prev_sync_instructions % instructions) > nret) && ((sync_instructions % instructions) < nret)));
    assign pmcounters_cores[0].data.m_mode_cycles = 24'(pmcounter[M_MODE_CYCLES]);
    assign pmcounters_cores[0].data.m_mode_instret = 24'(pmcounter[M_MODE_INSTRET]);
    assign pmcounters_cores[0].data.s_mode_cycles = 24'(pmcounter[S_MODE_CYCLES]);
    assign pmcounters_cores[0].data.s_mode_instret = 24'(pmcounter[S_MODE_INSTRET]);
    assign pmcounters_cores[0].data.u_mode_cycles = 24'(pmcounter[U_MODE_CYCLES]);
    assign pmcounters_cores[0].data.u_mode_instret = 24'(pmcounter[U_MODE_INSTRET]);
    assign pmcounters_cores[0].data.ref_cpu_cycles = 24'(pmcounter[REF_CPU_CYCLES]);
    assign pmcounters_cores[0].data.stalls_bst_full = 24'(pmcounter[STALLS_BST_FULL]);
    assign pmcounters_cores[0].data.stalls_pfx_full = 24'(pmcounter[STALLS_PFX_FULL]);
    assign pmcounters_cores[0].data.nfp_early_redirect = 24'(pmcounter[NFP_EARLY_REDIRECT]);
    assign pmcounters_cores[0].data.nfp_late_redirect = 24'(pmcounter[NFP_LATE_REDIRECT]);
    assign pmcounters_cores[0].data.stalls_indirect_miss = 24'(pmcounter[STALLS_INDIRECT_MISS]);
    assign pmcounters_cores[0].data.stalls_icache_miss = 24'(pmcounter[STALLS_ICACHE_MISS]);
    assign pmcounters_cores[0].data.stalls_itlb_miss = 24'(pmcounter[STALLS_ITLB_MISS]);
    assign pmcounters_cores[0].data.stalls_exception = 24'(pmcounter[STALLS_EXCEPTION]);
    assign pmcounters_cores[0].data.stalls_irb_full = 24'(pmcounter[STALLS_IRB_FULL]);
    assign pmcounters_cores[0].data.stalls_ifbuf_full = 24'(pmcounter[STALLS_IFBUF_FULL]);
    assign pmcounters_cores[0].data.page_crossing_fetchblocks = 24'(pmcounter[PAGE_CROSSING_FETCHBLOCKS]);
    assign pmcounters_cores[0].data.ifbuf_full_redirect = 24'(pmcounter[IFBUF_FULL_REDIRECT]);
    assign pmcounters_cores[0].data.fault_resync = 24'(pmcounter[FAULT_RESYNC]);
    assign pmcounters_cores[0].data.fault_refetch = 24'(pmcounter[FAULT_REFETCH]);
    assign pmcounters_cores[0].data.cmode_entry = 24'(pmcounter[CMODE_ENTRY]);
    assign pmcounters_cores[0].data.branch_misses = 24'(pmcounter[BRANCH_MISSES]);
    assign pmcounters_cores[0].data.br_ret_misses = 24'(pmcounter[BR_RET_MISSES]);
    assign pmcounters_cores[0].data.ind_br_misses = 24'(pmcounter[IND_BR_MISSES]);
    assign pmcounters_cores[0].data.rel_br_misses = 24'(pmcounter[REL_BR_MISSES]);
    assign pmcounters_cores[0].data.spec_branch_redirect = 24'(pmcounter[SPEC_BRANCH_REDIRECT]);
    assign pmcounters_cores[0].data.spec_lsu_resyncs = 24'(pmcounter[SPEC_LSU_RESYNCS]);
    assign pmcounters_cores[0].data.total_flushes = 24'(pmcounter[TOTAL_FLUSHES]);
    assign pmcounters_cores[0].data.total_traps = 24'(pmcounter[TOTAL_TRAPS]);
    assign pmcounters_cores[0].data.bst_full_on_ex_redirect = 24'(pmcounter[BST_FULL_ON_EX_REDIRECT]);
    assign pmcounters_cores[0].data.pfx_full_on_ex_redirect = 24'(pmcounter[PFX_FULL_ON_EX_REDIRECT]);
    assign pmcounters_cores[0].data.l1i_read_access = 24'(pmcounter[L1I_READ_ACCESS]);
    assign pmcounters_cores[0].data.l1i_read_miss = 24'(pmcounter[L1I_READ_MISS]);
    assign pmcounters_cores[0].data.l1i_prefetch_access = 24'(pmcounter[L1I_PREFETCH_ACCESS]);
    assign pmcounters_cores[0].data.l1i_prefetch_miss = 24'(pmcounter[L1I_PREFETCH_MISS]);
    assign pmcounters_cores[0].data.itlb_read_access = 24'(pmcounter[ITLB_READ_ACCESS]);
    assign pmcounters_cores[0].data.itlb_read_miss = 24'(pmcounter[ITLB_READ_MISS]);
    assign pmcounters_cores[0].data.itlb_prefetch_access = 24'(pmcounter[ITLB_PREFETCH_ACCESS]);
    assign pmcounters_cores[0].data.itlb_prefetch_miss = 24'(pmcounter[ITLB_PREFETCH_MISS]);
    assign pmcounters_cores[0].data.ic_way_mispred = 24'(pmcounter[IC_WAY_MISPRED]);
    assign pmcounters_cores[0].data.ras_underflow = 24'(pmcounter[RAS_UNDERFLOW]);
    assign pmcounters_cores[0].data.ras_overflow = 24'(pmcounter[RAS_OVERFLOW]);
    assign pmcounters_cores[0].data.num_fetchgroups = 24'(pmcounter[NUM_FETCHGROUPS]);
    assign pmcounters_cores[0].data.bdp_bank_conflicts = 24'(pmcounter[BDP_BANK_CONFLICTS]);
    assign pmcounters_cores[0].data.btp_bank_conflicts = 24'(pmcounter[BTP_BANK_CONFLICTS]);
    assign pmcounters_cores[0].data.bpu_writes = 24'(pmcounter[BPU_WRITES]);
    assign pmcounters_cores[0].data.uops_decoded = 24'(pmcounter[UOPS_DECODED]);
    assign pmcounters_cores[0].data.decode_serialize_cycles = 24'(pmcounter[DECODE_SERIALIZE_CYCLES]);
    assign pmcounters_cores[0].data.decode_idle_serialize_cycles = 24'(pmcounter[DECODE_IDLE_SERIALIZE_CYCLES]);
    assign pmcounters_cores[0].data.nonspec_resync = 24'(pmcounter[NONSPEC_RESYNC]);
    assign pmcounters_cores[0].data.patch_match_m_mode_exception = 24'(pmcounter[PATCH_MATCH_M_MODE_EXCEPTION]);
    assign pmcounters_cores[0].data.patch_match_s_mode_exception = 24'(pmcounter[PATCH_MATCH_S_MODE_EXCEPTION]);
    assign pmcounters_cores[0].data.patch_match_u_mode_exception = 24'(pmcounter[PATCH_MATCH_U_MODE_EXCEPTION]);
    assign pmcounters_cores[0].data.patch_match_vs_mode_exception = 24'(pmcounter[PATCH_MATCH_VS_MODE_EXCEPTION]);
    assign pmcounters_cores[0].data.patch_match_vu_mode_exception = 24'(pmcounter[PATCH_MATCH_VU_MODE_EXCEPTION]);
    assign pmcounters_cores[0].data.patch_match_ucode = 24'(pmcounter[PATCH_MATCH_UCODE]);
    assign pmcounters_cores[0].data.patch_match_m_mode_exception_cycles = 24'(pmcounter[PATCH_MATCH_M_MODE_EXCEPTION_CYCLES]);
    assign pmcounters_cores[0].data.patch_match_s_mode_exception_cycles = 24'(pmcounter[PATCH_MATCH_S_MODE_EXCEPTION_CYCLES]);
    assign pmcounters_cores[0].data.patch_match_u_mode_exception_cycles = 24'(pmcounter[PATCH_MATCH_U_MODE_EXCEPTION_CYCLES]);
    assign pmcounters_cores[0].data.patch_match_vs_mode_exception_cycles = 24'(pmcounter[PATCH_MATCH_VS_MODE_EXCEPTION_CYCLES]);
    assign pmcounters_cores[0].data.patch_match_vu_mode_exception_cycles = 24'(pmcounter[PATCH_MATCH_VU_MODE_EXCEPTION_CYCLES]);
    assign pmcounters_cores[0].data.patch_match_ucode_cycles = 24'(pmcounter[PATCH_MATCH_UCODE_CYCLES]);
    assign pmcounters_cores[0].data.stalled_cycles_frontend = 24'(pmcounter[STALLED_CYCLES_FRONTEND]);
    assign pmcounters_cores[0].data.stalled_cycles_backend = 24'(pmcounter[STALLED_CYCLES_BACKEND]);
    assign pmcounters_cores[0].data.cycles_no_int_prn = 24'(pmcounter[CYCLES_NO_INT_PRN]);
    assign pmcounters_cores[0].data.cycles_no_fp_prn = 24'(pmcounter[CYCLES_NO_FP_PRN]);
    assign pmcounters_cores[0].data.cycles_no_vec_prn = 24'(pmcounter[CYCLES_NO_VEC_PRN]);
    assign pmcounters_cores[0].data.cycles_no_vl_prn = 24'(pmcounter[CYCLES_NO_VL_PRN]);
    assign pmcounters_cores[0].data.cycles_no_vm_prn = 24'(pmcounter[CYCLES_NO_VM_PRN]);
    assign pmcounters_cores[0].data.cycles_no_rob = 24'(pmcounter[CYCLES_NO_ROB]);
    assign pmcounters_cores[0].data.dispatched_nops = 24'(pmcounter[DISPATCHED_NOPS]);
    assign pmcounters_cores[0].data.op_retired_direct_branch = 24'(pmcounter[OP_RETIRED_DIRECT_BRANCH]);
    assign pmcounters_cores[0].data.op_retired_ret_branch = 24'(pmcounter[OP_RETIRED_RET_BRANCH]);
    assign pmcounters_cores[0].data.op_retired_indirect_branch = 24'(pmcounter[OP_RETIRED_INDIRECT_BRANCH]);
    assign pmcounters_cores[0].data.op_retired_cond_branch = 24'(pmcounter[OP_RETIRED_COND_BRANCH]);
    assign pmcounters_cores[0].data.op_retired_ld = 24'(pmcounter[OP_RETIRED_LD]);
    assign pmcounters_cores[0].data.op_retired_st = 24'(pmcounter[OP_RETIRED_ST]);
    assign pmcounters_cores[0].data.op_retired_int = 24'(pmcounter[OP_RETIRED_INT]);
    assign pmcounters_cores[0].data.op_retired_csr = 24'(pmcounter[OP_RETIRED_CSR]);
    assign pmcounters_cores[0].data.op_retired_fp = 24'(pmcounter[OP_RETIRED_FP]);
    assign pmcounters_cores[0].data.op_retired_vec = 24'(pmcounter[OP_RETIRED_VEC]);
    assign pmcounters_cores[0].data.op_complete_ld = 24'(pmcounter[OP_COMPLETE_LD]);
    assign pmcounters_cores[0].data.op_complete_st = 24'(pmcounter[OP_COMPLETE_ST]);
    assign pmcounters_cores[0].data.op_complete_int = 24'(pmcounter[OP_COMPLETE_INT]);
    assign pmcounters_cores[0].data.op_complete_fp = 24'(pmcounter[OP_COMPLETE_FP]);
    assign pmcounters_cores[0].data.op_complete_vec = 24'(pmcounter[OP_COMPLETE_VEC]);
    assign pmcounters_cores[0].data.op_issued_pipe0 = 24'(pmcounter[OP_ISSUED_PIPE0]);
    assign pmcounters_cores[0].data.op_issued_pipe1 = 24'(pmcounter[OP_ISSUED_PIPE1]);
    assign pmcounters_cores[0].data.op_issued_pipe2 = 24'(pmcounter[OP_ISSUED_PIPE2]);
    assign pmcounters_cores[0].data.op_issued_pipe3 = 24'(pmcounter[OP_ISSUED_PIPE3]);
    assign pmcounters_cores[0].data.op_issued_pipe4 = 24'(pmcounter[OP_ISSUED_PIPE4]);
    assign pmcounters_cores[0].data.op_issued_pipe5 = 24'(pmcounter[OP_ISSUED_PIPE5]);
    assign pmcounters_cores[0].data.op_issued_pipe6 = 24'(pmcounter[OP_ISSUED_PIPE6]);
    assign pmcounters_cores[0].data.op_issued_pipe7 = 24'(pmcounter[OP_ISSUED_PIPE7]);
    assign pmcounters_cores[0].data.op_issued_pipe8 = 24'(pmcounter[OP_ISSUED_PIPE8]);
    assign pmcounters_cores[0].data.op_issued_pipe9 = 24'(pmcounter[OP_ISSUED_PIPE9]);
    assign pmcounters_cores[0].data.op_issued_pipe10 = 24'(pmcounter[OP_ISSUED_PIPE10]);
    assign pmcounters_cores[0].data.op_issued_pipe11 = 24'(pmcounter[OP_ISSUED_PIPE11]);
    assign pmcounters_cores[0].data.op_issued_pipe12 = 24'(pmcounter[OP_ISSUED_PIPE12]);
    assign pmcounters_cores[0].data.op_issued_pipe13 = 24'(pmcounter[OP_ISSUED_PIPE13]);
    assign pmcounters_cores[0].data.op_issued_pipe14 = 24'(pmcounter[OP_ISSUED_PIPE14]);
    assign pmcounters_cores[0].data.op_issued_pipe15 = 24'(pmcounter[OP_ISSUED_PIPE15]);
    assign pmcounters_cores[0].data.wasted_issue_slots_via_throttling = 24'(pmcounter[WASTED_ISSUE_SLOTS_VIA_THROTTLING]);
    assign pmcounters_cores[0].data.store_uops_rejected_via_stq_advance = 24'(pmcounter[STORE_UOPS_REJECTED_VIA_STQ_ADVANCE]);
    assign pmcounters_cores[0].data.op_issued_fp64 = 24'(pmcounter[OP_ISSUED_FP64]);
    assign pmcounters_cores[0].data.fp64_export_overflow = 24'(pmcounter[FP64_EXPORT_OVERFLOW]);
    assign pmcounters_cores[0].data.cache_references = 24'(pmcounter[CACHE_REFERENCES]);
    assign pmcounters_cores[0].data.cache_misses = 24'(pmcounter[CACHE_MISSES]);
    assign pmcounters_cores[0].data.l1d_read_access_non_clc = 24'(pmcounter[L1D_READ_ACCESS_NON_CLC]);
    assign pmcounters_cores[0].data.l1d_read_access_clc = 24'(pmcounter[L1D_READ_ACCESS_CLC]);
    assign pmcounters_cores[0].data.l1d_read_access_4kx = 24'(pmcounter[L1D_READ_ACCESS_4KX]);
    assign pmcounters_cores[0].data.l1d_read_access_all = 24'(pmcounter[L1D_READ_ACCESS_ALL]);
    assign pmcounters_cores[0].data.l1d_write_access_non_clc = 24'(pmcounter[L1D_WRITE_ACCESS_NON_CLC]);
    assign pmcounters_cores[0].data.l1d_write_access_clc = 24'(pmcounter[L1D_WRITE_ACCESS_CLC]);
    assign pmcounters_cores[0].data.l1d_write_access_4kx = 24'(pmcounter[L1D_WRITE_ACCESS_4KX]);
    assign pmcounters_cores[0].data.l1d_write_access_all = 24'(pmcounter[L1D_WRITE_ACCESS_ALL]);
    assign pmcounters_cores[0].data.l1d_prefetch_access_non_clc = 24'(pmcounter[L1D_PREFETCH_ACCESS_NON_CLC]);
    assign pmcounters_cores[0].data.l1d_prefetch_access_clc = 24'(pmcounter[L1D_PREFETCH_ACCESS_CLC]);
    assign pmcounters_cores[0].data.l1d_prefetch_access_all = 24'(pmcounter[L1D_PREFETCH_ACCESS_ALL]);
    assign pmcounters_cores[0].data.l1d_mmu_access = 24'(pmcounter[L1D_MMU_ACCESS]);
    assign pmcounters_cores[0].data.l1d_snoop_access = 24'(pmcounter[L1D_SNOOP_ACCESS]);
    assign pmcounters_cores[0].data.l1d_access_all = 24'(pmcounter[L1D_ACCESS_ALL]);
    assign pmcounters_cores[0].data.l1d_read_miss = 24'(pmcounter[L1D_READ_MISS]);
    assign pmcounters_cores[0].data.l1d_write_miss = 24'(pmcounter[L1D_WRITE_MISS]);
    assign pmcounters_cores[0].data.l1d_prefetch_miss = 24'(pmcounter[L1D_PREFETCH_MISS]);
    assign pmcounters_cores[0].data.l1d_mmu_miss = 24'(pmcounter[L1D_MMU_MISS]);
    assign pmcounters_cores[0].data.l1d_miss_all = 24'(pmcounter[L1D_MISS_ALL]);
    assign pmcounters_cores[0].data.transbuf_or_reqbuf_cannot_alloc_load = 24'(pmcounter[TRANSBUF_OR_REQBUF_CANNOT_ALLOC_LOAD]);
    assign pmcounters_cores[0].data.transbuf_or_reqbuf_cannot_alloc_store = 24'(pmcounter[TRANSBUF_OR_REQBUF_CANNOT_ALLOC_STORE]);
    assign pmcounters_cores[0].data.transbuf_or_reqbuf_cannot_alloc_prefetch = 24'(pmcounter[TRANSBUF_OR_REQBUF_CANNOT_ALLOC_PREFETCH]);
    assign pmcounters_cores[0].data.transbuf_or_reqbuf_cannot_alloc_mmu = 24'(pmcounter[TRANSBUF_OR_REQBUF_CANNOT_ALLOC_MMU]);
    assign pmcounters_cores[0].data.transbuf_cannot_alloc_all = 24'(pmcounter[TRANSBUF_CANNOT_ALLOC_ALL]);
    assign pmcounters_cores[0].data.l1d_write_upgrade_req = 24'(pmcounter[L1D_WRITE_UPGRADE_REQ]);
    assign pmcounters_cores[0].data.dtlb_read_access = 24'(pmcounter[DTLB_READ_ACCESS]);
    assign pmcounters_cores[0].data.dtlb_write_access = 24'(pmcounter[DTLB_WRITE_ACCESS]);
    assign pmcounters_cores[0].data.dtlb_prefetch_access = 24'(pmcounter[DTLB_PREFETCH_ACCESS]);
    assign pmcounters_cores[0].data.dtlb_read_access_cacheable = 24'(pmcounter[DTLB_READ_ACCESS_CACHEABLE]);
    assign pmcounters_cores[0].data.dtlb_read_access_noncacheable = 24'(pmcounter[DTLB_READ_ACCESS_NONCACHEABLE]);
    assign pmcounters_cores[0].data.dtlb_write_access_cacheable = 24'(pmcounter[DTLB_WRITE_ACCESS_CACHEABLE]);
    assign pmcounters_cores[0].data.dtlb_write_access_noncacheable = 24'(pmcounter[DTLB_WRITE_ACCESS_NONCACHEABLE]);
    assign pmcounters_cores[0].data.dtlb_access_all = 24'(pmcounter[DTLB_ACCESS_ALL]);
    assign pmcounters_cores[0].data.dtlb_read_miss = 24'(pmcounter[DTLB_READ_MISS]);
    assign pmcounters_cores[0].data.dtlb_write_miss = 24'(pmcounter[DTLB_WRITE_MISS]);
    assign pmcounters_cores[0].data.dtlb_prefetch_miss = 24'(pmcounter[DTLB_PREFETCH_MISS]);
    assign pmcounters_cores[0].data.dtlb_miss_4k = 24'(pmcounter[DTLB_MISS_4K]);
    assign pmcounters_cores[0].data.dtlb_miss_hugepage = 24'(pmcounter[DTLB_MISS_HUGEPAGE]);
    assign pmcounters_cores[0].data.dtlb_miss_all = 24'(pmcounter[DTLB_MISS_ALL]);
    assign pmcounters_cores[0].data.leaf_tlb_access_ls = 24'(pmcounter[LEAF_TLB_ACCESS_LS]);
    assign pmcounters_cores[0].data.leaf_tlb_access_fe = 24'(pmcounter[LEAF_TLB_ACCESS_FE]);
    assign pmcounters_cores[0].data.leaf_tlb_access_mmu_prefetch = 24'(pmcounter[LEAF_TLB_ACCESS_MMU_PREFETCH]);
    assign pmcounters_cores[0].data.leaf_tlb_access_all = 24'(pmcounter[LEAF_TLB_ACCESS_ALL]);
    assign pmcounters_cores[0].data.leaf_tlb_miss_ls = 24'(pmcounter[LEAF_TLB_MISS_LS]);
    assign pmcounters_cores[0].data.leaf_tlb_miss_fe = 24'(pmcounter[LEAF_TLB_MISS_FE]);
    assign pmcounters_cores[0].data.leaf_tlb_miss_mmu_prefetch = 24'(pmcounter[LEAF_TLB_MISS_MMU_PREFETCH]);
    assign pmcounters_cores[0].data.leaf_tlb_miss_all = 24'(pmcounter[LEAF_TLB_MISS_ALL]);
    assign pmcounters_cores[0].data.nonleaf_tlb_access_ls = 24'(pmcounter[NONLEAF_TLB_ACCESS_LS]);
    assign pmcounters_cores[0].data.nonleaf_tlb_access_fe = 24'(pmcounter[NONLEAF_TLB_ACCESS_FE]);
    assign pmcounters_cores[0].data.nonleaf_tlb_access_mmu_prefetch = 24'(pmcounter[NONLEAF_TLB_ACCESS_MMU_PREFETCH]);
    assign pmcounters_cores[0].data.nonleaf_tlb_access_all = 24'(pmcounter[NONLEAF_TLB_ACCESS_ALL]);
    assign pmcounters_cores[0].data.nonleaf_tlb_miss_ls = 24'(pmcounter[NONLEAF_TLB_MISS_LS]);
    assign pmcounters_cores[0].data.nonleaf_tlb_miss_fe = 24'(pmcounter[NONLEAF_TLB_MISS_FE]);
    assign pmcounters_cores[0].data.nonleaf_tlb_miss_mmu_prefetch = 24'(pmcounter[NONLEAF_TLB_MISS_MMU_PREFETCH]);
    assign pmcounters_cores[0].data.nonleaf_tlb_miss_all = 24'(pmcounter[NONLEAF_TLB_MISS_ALL]);
    assign pmcounters_cores[0].data.page_table_walks_ls = 24'(pmcounter[PAGE_TABLE_WALKS_LS]);
    assign pmcounters_cores[0].data.page_table_walks_fe = 24'(pmcounter[PAGE_TABLE_WALKS_FE]);
    assign pmcounters_cores[0].data.page_table_walks_mmu_prefetch = 24'(pmcounter[PAGE_TABLE_WALKS_MMU_PREFETCH]);
    assign pmcounters_cores[0].data.page_table_walks_all = 24'(pmcounter[PAGE_TABLE_WALKS_ALL]);
    assign pmcounters_cores[0].data.stlf_replay_load = 24'(pmcounter[STLF_REPLAY_LOAD]);
    assign pmcounters_cores[0].data.stlf_replay_mmu = 24'(pmcounter[STLF_REPLAY_MMU]);
    assign pmcounters_cores[0].data.stlf_replay_all = 24'(pmcounter[STLF_REPLAY_ALL]);
    assign pmcounters_cores[0].data.data_bank_conflict_replay_load = 24'(pmcounter[DATA_BANK_CONFLICT_REPLAY_LOAD]);
    assign pmcounters_cores[0].data.data_bank_conflict_replay_store = 24'(pmcounter[DATA_BANK_CONFLICT_REPLAY_STORE]);
    assign pmcounters_cores[0].data.data_bank_conflict_replay_mmu = 24'(pmcounter[DATA_BANK_CONFLICT_REPLAY_MMU]);
    assign pmcounters_cores[0].data.data_bank_conflict_replay_all = 24'(pmcounter[DATA_BANK_CONFLICT_REPLAY_ALL]);
    assign pmcounters_cores[0].data.ls_way_predictor_replay_load = 24'(pmcounter[LS_WAY_PREDICTOR_REPLAY_LOAD]);
    assign pmcounters_cores[0].data.ls_way_predictor_replay_store = 24'(pmcounter[LS_WAY_PREDICTOR_REPLAY_STORE]);
    assign pmcounters_cores[0].data.ls_way_predictor_replay_prefetch = 24'(pmcounter[LS_WAY_PREDICTOR_REPLAY_PREFETCH]);
    assign pmcounters_cores[0].data.ls_way_predictor_replay_mmu = 24'(pmcounter[LS_WAY_PREDICTOR_REPLAY_MMU]);
    assign pmcounters_cores[0].data.ls_way_predictor_replay_all = 24'(pmcounter[LS_WAY_PREDICTOR_REPLAY_ALL]);
    assign pmcounters_cores[0].data.ls_micro_way_predictor_replay_load = 24'(pmcounter[LS_MICRO_WAY_PREDICTOR_REPLAY_LOAD]);
    assign pmcounters_cores[0].data.ls_micro_way_predictor_replay_prefetch = 24'(pmcounter[LS_MICRO_WAY_PREDICTOR_REPLAY_PREFETCH]);
    assign pmcounters_cores[0].data.ls_micro_way_predictor_replay_all = 24'(pmcounter[LS_MICRO_WAY_PREDICTOR_REPLAY_ALL]);
    assign pmcounters_cores[0].data.tag_bank_conflict_replay_load = 24'(pmcounter[TAG_BANK_CONFLICT_REPLAY_LOAD]);
    assign pmcounters_cores[0].data.tag_bank_conflict_replay_store = 24'(pmcounter[TAG_BANK_CONFLICT_REPLAY_STORE]);
    assign pmcounters_cores[0].data.tag_bank_conflict_replay_prefetch = 24'(pmcounter[TAG_BANK_CONFLICT_REPLAY_PREFETCH]);
    assign pmcounters_cores[0].data.tag_bank_conflict_replay_mmu = 24'(pmcounter[TAG_BANK_CONFLICT_REPLAY_MMU]);
    assign pmcounters_cores[0].data.tag_bank_conflict_replay_all = 24'(pmcounter[TAG_BANK_CONFLICT_REPLAY_ALL]);
    assign pmcounters_cores[0].data.dtlb_replay_load = 24'(pmcounter[DTLB_REPLAY_LOAD]);
    assign pmcounters_cores[0].data.dtlb_replay_store = 24'(pmcounter[DTLB_REPLAY_STORE]);
    assign pmcounters_cores[0].data.dtlb_replay_prefetch = 24'(pmcounter[DTLB_REPLAY_PREFETCH]);
    assign pmcounters_cores[0].data.dtlb_replay_all = 24'(pmcounter[DTLB_REPLAY_ALL]);
    assign pmcounters_cores[0].data.sipt_replay_load = 24'(pmcounter[SIPT_REPLAY_LOAD]);
    assign pmcounters_cores[0].data.sipt_replay_store = 24'(pmcounter[SIPT_REPLAY_STORE]);
    assign pmcounters_cores[0].data.sipt_replay_all = 24'(pmcounter[SIPT_REPLAY_ALL]);
    assign pmcounters_cores[0].data.reqbuf_hit_replay_load = 24'(pmcounter[REQBUF_HIT_REPLAY_LOAD]);
    assign pmcounters_cores[0].data.reqbuf_hit_replay_store = 24'(pmcounter[REQBUF_HIT_REPLAY_STORE]);
    assign pmcounters_cores[0].data.reqbuf_hit_replay_mmu = 24'(pmcounter[REQBUF_HIT_REPLAY_MMU]);
    assign pmcounters_cores[0].data.reqbuf_hit_replay_all = 24'(pmcounter[REQBUF_HIT_REPLAY_ALL]);
    assign pmcounters_cores[0].data.fillbuf_hit_replay_load = 24'(pmcounter[FILLBUF_HIT_REPLAY_LOAD]);
    assign pmcounters_cores[0].data.fillbuf_hit_replay_store = 24'(pmcounter[FILLBUF_HIT_REPLAY_STORE]);
    assign pmcounters_cores[0].data.fillbuf_hit_replay_mmu = 24'(pmcounter[FILLBUF_HIT_REPLAY_MMU]);
    assign pmcounters_cores[0].data.fillbuf_hit_replay_all = 24'(pmcounter[FILLBUF_HIT_REPLAY_ALL]);
    assign pmcounters_cores[0].data.l1d_miss_reqbuf_link_load = 24'(pmcounter[L1D_MISS_REQBUF_LINK_LOAD]);
    assign pmcounters_cores[0].data.l1d_miss_reqbuf_link_store = 24'(pmcounter[L1D_MISS_REQBUF_LINK_STORE]);
    assign pmcounters_cores[0].data.l1d_miss_reqbuf_link_mmu = 24'(pmcounter[L1D_MISS_REQBUF_LINK_MMU]);
    assign pmcounters_cores[0].data.l1d_miss_reqbuf_link_all = 24'(pmcounter[L1D_MISS_REQBUF_LINK_ALL]);
    assign pmcounters_cores[0].data.l1d_miss_misc_replay_load = 24'(pmcounter[L1D_MISS_MISC_REPLAY_LOAD]);
    assign pmcounters_cores[0].data.l1d_miss_misc_replay_store = 24'(pmcounter[L1D_MISS_MISC_REPLAY_STORE]);
    assign pmcounters_cores[0].data.l1d_miss_misc_replay_prefetch = 24'(pmcounter[L1D_MISS_MISC_REPLAY_PREFETCH]);
    assign pmcounters_cores[0].data.l1d_miss_misc_replay_mmu = 24'(pmcounter[L1D_MISS_MISC_REPLAY_MMU]);
    assign pmcounters_cores[0].data.l1d_miss_misc_replay_all = 24'(pmcounter[L1D_MISS_MISC_REPLAY_ALL]);
    assign pmcounters_cores[0].data.l1d_victim_fill_evict = 24'(pmcounter[L1D_VICTIM_FILL_EVICT]);
    assign pmcounters_cores[0].data.l1d_victim_early_evict = 24'(pmcounter[L1D_VICTIM_EARLY_EVICT]);
    assign pmcounters_cores[0].data.l1d_victim_demand_req = 24'(pmcounter[L1D_VICTIM_DEMAND_REQ]);
    assign pmcounters_cores[0].data.l1d_victim_prefetch_req = 24'(pmcounter[L1D_VICTIM_PREFETCH_REQ]);
    assign pmcounters_cores[0].data.l1d_victim_mru_alloc = 24'(pmcounter[L1D_VICTIM_MRU_ALLOC]);
    assign pmcounters_cores[0].data.l1d_victim_lru_alloc = 24'(pmcounter[L1D_VICTIM_LRU_ALLOC]);
    assign pmcounters_cores[0].data.l1d_victim_all = 24'(pmcounter[L1D_VICTIM_ALL]);
    assign pmcounters_cores[0].data.l1d_cache_invalidate_snoop = 24'(pmcounter[L1D_CACHE_INVALIDATE_SNOOP]);
    assign pmcounters_cores[0].data.l1d_cache_invalidate_cmo = 24'(pmcounter[L1D_CACHE_INVALIDATE_CMO]);
    assign pmcounters_cores[0].data.l1d_cache_invalidate_ras = 24'(pmcounter[L1D_CACHE_INVALIDATE_RAS]);
    assign pmcounters_cores[0].data.l1d_cache_invalidate_all = 24'(pmcounter[L1D_CACHE_INVALIDATE_ALL]);
    assign pmcounters_cores[0].data.lsu_resyncs_rar_stpipe = 24'(pmcounter[LSU_RESYNCS_RAR_STPIPE]);
    assign pmcounters_cores[0].data.lsu_resyncs_rar_ldpipe = 24'(pmcounter[LSU_RESYNCS_RAR_LDPIPE]);
    assign pmcounters_cores[0].data.lsu_resyncs_rar_all = 24'(pmcounter[LSU_RESYNCS_RAR_ALL]);
    assign pmcounters_cores[0].data.pfc_prefetches_late_l1pend = 24'(pmcounter[PFC_PREFETCHES_LATE_L1PEND]);
    assign pmcounters_cores[0].data.pfc_prefetches_late_reqbuf = 24'(pmcounter[PFC_PREFETCHES_LATE_REQBUF]);
    assign pmcounters_cores[0].data.pfc_prefetches_late_wasted = 24'(pmcounter[PFC_PREFETCHES_LATE_WASTED]);
    assign pmcounters_cores[0].data.pfc_prefetches_late_all = 24'(pmcounter[PFC_PREFETCHES_LATE_ALL]);
    assign pmcounters_cores[0].data.pfc_agt_evict_chaining = 24'(pmcounter[PFC_AGT_EVICT_CHAINING]);
    assign pmcounters_cores[0].data.pfc_pht_chain_lookup = 24'(pmcounter[PFC_PHT_CHAIN_LOOKUP]);
    assign pmcounters_cores[0].data.pfc_pht_chain_hit = 24'(pmcounter[PFC_PHT_CHAIN_HIT]);
    assign pmcounters_cores[0].data.pfc_pht_rtf_tap_filtered = 24'(pmcounter[PFC_PHT_RTF_TAP_FILTERED]);
    assign pmcounters_cores[0].data.pfc_pht_rtf_chain_filtered = 24'(pmcounter[PFC_PHT_RTF_CHAIN_FILTERED]);
    assign pmcounters_cores[0].data.ls_chillout_cycles_relaxed = 24'(pmcounter[LS_CHILLOUT_CYCLES_RELAXED]);
    assign pmcounters_cores[0].data.ls_chillout_cycles_medium = 24'(pmcounter[LS_CHILLOUT_CYCLES_MEDIUM]);
    assign pmcounters_cores[0].data.ls_chillout_cycles_heavy = 24'(pmcounter[LS_CHILLOUT_CYCLES_HEAVY]);
    assign pmcounters_cores[0].data.ls_chillout_cycles_all = 24'(pmcounter[LS_CHILLOUT_CYCLES_ALL]);
    assign pmcounters_cores[0].data.ls_chillout_cycles_ldc = 24'(pmcounter[LS_CHILLOUT_CYCLES_LDC]);
    assign pmcounters_cores[0].data.ls_chillout_cycles_stc = 24'(pmcounter[LS_CHILLOUT_CYCLES_STC]);
    assign pmcounters_cores[0].data.ls_chillout_cycles_mmu = 24'(pmcounter[LS_CHILLOUT_CYCLES_MMU]);
    assign pmcounters_cores[0].data.ls_chillout_cycles_cif = 24'(pmcounter[LS_CHILLOUT_CYCLES_CIF]);
    assign pmcounters_cores[0].data.ls_chillout_requests_ldc = 24'(pmcounter[LS_CHILLOUT_REQUESTS_LDC]);
    assign pmcounters_cores[0].data.ls_chillout_requests_stc = 24'(pmcounter[LS_CHILLOUT_REQUESTS_STC]);
    assign pmcounters_cores[0].data.ls_chillout_requests_mmu = 24'(pmcounter[LS_CHILLOUT_REQUESTS_MMU]);
    assign pmcounters_cores[0].data.ls_chillout_requests_cif = 24'(pmcounter[LS_CHILLOUT_REQUESTS_CIF]);
    assign pmcounters_cores[0].data.ls_chillout_requests_all = 24'(pmcounter[LS_CHILLOUT_REQUESTS_ALL]);
    assign pmcounters_cores[0].data.ls_chillout_entrances_ldc = 24'(pmcounter[LS_CHILLOUT_ENTRANCES_LDC]);
    assign pmcounters_cores[0].data.ls_chillout_entrances_stc = 24'(pmcounter[LS_CHILLOUT_ENTRANCES_STC]);
    assign pmcounters_cores[0].data.ls_chillout_entrances_mmu = 24'(pmcounter[LS_CHILLOUT_ENTRANCES_MMU]);
    assign pmcounters_cores[0].data.ls_chillout_entrances_cif = 24'(pmcounter[LS_CHILLOUT_ENTRANCES_CIF]);
    assign pmcounters_cores[0].data.ls_chillout_entrances_all = 24'(pmcounter[LS_CHILLOUT_ENTRANCES_ALL]);
    assign pmcounters_cores[0].data.utlb_hit_load = 24'(pmcounter[UTLB_HIT_LOAD]);
    assign pmcounters_cores[0].data.utlb_hit_store = 24'(pmcounter[UTLB_HIT_STORE]);
    assign pmcounters_cores[0].data.utlb_hit_all = 24'(pmcounter[UTLB_HIT_ALL]);
    assign pmcounters_cores[0].data.utlb_miss_load = 24'(pmcounter[UTLB_MISS_LOAD]);
    assign pmcounters_cores[0].data.utlb_miss_store = 24'(pmcounter[UTLB_MISS_STORE]);
    assign pmcounters_cores[0].data.utlb_miss_all = 24'(pmcounter[UTLB_MISS_ALL]);
    assign pmcounters_cores[0].data.ldq_cannot_alloc = 24'(pmcounter[LDQ_CANNOT_ALLOC]);
    assign pmcounters_cores[0].data.mdp_correct_prediction = 24'(pmcounter[MDP_CORRECT_PREDICTION]);
    assign pmcounters_cores[0].data.mdp_false_hit = 24'(pmcounter[MDP_FALSE_HIT]);
    assign pmcounters_cores[0].data.mdp_total_prediction = 24'(pmcounter[MDP_TOTAL_PREDICTION]);
    assign pmcounters_cores[0].data.stalls_mem_l1d_miss = 24'(pmcounter[STALLS_MEM_L1D_MISS]);
    assign pmcounters_cores[0].data.stalls_mem_l1dtlb_miss = 24'(pmcounter[STALLS_MEM_L1DTLB_MISS]);
    assign pmcounters_cores[0].data.rar_cannot_alloc = 24'(pmcounter[RAR_CANNOT_ALLOC]);
    assign pmcounters_cores[0].data.raw_cannot_alloc = 24'(pmcounter[RAW_CANNOT_ALLOC]);
    assign pmcounters_cores[0].data.pcb_cannot_alloc = 24'(pmcounter[PCB_CANNOT_ALLOC]);
    assign pmcounters_cores[0].data.udb_cannot_alloc = 24'(pmcounter[UDB_CANNOT_ALLOC]);
    assign pmcounters_cores[0].data.udb_data_return = 24'(pmcounter[UDB_DATA_RETURN]);
    assign pmcounters_cores[0].data.udb_lost = 24'(pmcounter[UDB_LOST]);
    assign pmcounters_cores[0].data.atomics_retired_lr = 24'(pmcounter[ATOMICS_RETIRED_LR]);
    assign pmcounters_cores[0].data.lr_stall = 24'(pmcounter[LR_STALL]);
    assign pmcounters_cores[0].data.ld_executed_vec_nano = 24'(pmcounter[LD_EXECUTED_VEC_NANO]);
    assign pmcounters_cores[0].data.ld_masked_vec_nano = 24'(pmcounter[LD_MASKED_VEC_NANO]);
    assign pmcounters_cores[0].data.stlf_hits = 24'(pmcounter[STLF_HITS]);
    assign pmcounters_cores[0].data.dfp_access_load = 24'(pmcounter[DFP_ACCESS_LOAD]);
    assign pmcounters_cores[0].data.dfp_access_store = 24'(pmcounter[DFP_ACCESS_STORE]);
    assign pmcounters_cores[0].data.dfp_access_mmu = 24'(pmcounter[DFP_ACCESS_MMU]);
    assign pmcounters_cores[0].data.dfp_access_evict = 24'(pmcounter[DFP_ACCESS_EVICT]);
    assign pmcounters_cores[0].data.dfp_access_fill = 24'(pmcounter[DFP_ACCESS_FILL]);
    assign pmcounters_cores[0].data.dfp_access_snoop = 24'(pmcounter[DFP_ACCESS_SNOOP]);
    assign pmcounters_cores[0].data.dfp_access_all = 24'(pmcounter[DFP_ACCESS_ALL]);
    assign pmcounters_cores[0].data.tlb_invalidates = 24'(pmcounter[TLB_INVALIDATES]);
    assign pmcounters_cores[0].data.stalls_mem_stores = 24'(pmcounter[STALLS_MEM_STORES]);
    assign pmcounters_cores[0].data.lsu_resyncs_raw = 24'(pmcounter[LSU_RESYNCS_RAW]);
    assign pmcounters_cores[0].data.smb_wants_to_alloc = 24'(pmcounter[SMB_WANTS_TO_ALLOC]);
    assign pmcounters_cores[0].data.smb_cannot_alloc = 24'(pmcounter[SMB_CANNOT_ALLOC]);
    assign pmcounters_cores[0].data.atomics_retired_sc = 24'(pmcounter[ATOMICS_RETIRED_SC]);
    assign pmcounters_cores[0].data.atomics_retired_sc_fail = 24'(pmcounter[ATOMICS_RETIRED_SC_FAIL]);
    assign pmcounters_cores[0].data.atomics_retired_sc_success = 24'(pmcounter[ATOMICS_RETIRED_SC_SUCCESS]);
    assign pmcounters_cores[0].data.atomics_retired_amo = 24'(pmcounter[ATOMICS_RETIRED_AMO]);
    assign pmcounters_cores[0].data.st_executed_vec_nano = 24'(pmcounter[ST_EXECUTED_VEC_NANO]);
    assign pmcounters_cores[0].data.st_masked_vec_nano = 24'(pmcounter[ST_MASKED_VEC_NANO]);
    assign pmcounters_cores[0].data.tap_access_load = 24'(pmcounter[TAP_ACCESS_LOAD]);
    assign pmcounters_cores[0].data.tap_access_store = 24'(pmcounter[TAP_ACCESS_STORE]);
    assign pmcounters_cores[0].data.tap_access_prefetch = 24'(pmcounter[TAP_ACCESS_PREFETCH]);
    assign pmcounters_cores[0].data.tap_access_mmu = 24'(pmcounter[TAP_ACCESS_MMU]);
    assign pmcounters_cores[0].data.tap_access_evict = 24'(pmcounter[TAP_ACCESS_EVICT]);
    assign pmcounters_cores[0].data.tap_access_fill = 24'(pmcounter[TAP_ACCESS_FILL]);
    assign pmcounters_cores[0].data.tap_access_snoop = 24'(pmcounter[TAP_ACCESS_SNOOP]);
    assign pmcounters_cores[0].data.tap_access_all = 24'(pmcounter[TAP_ACCESS_ALL]);
    assign pmcounters_cores[0].data.uwp_access_agp = 24'(pmcounter[UWP_ACCESS_AGP]);
    assign pmcounters_cores[0].data.uwp_access_arb = 24'(pmcounter[UWP_ACCESS_ARB]);
    assign pmcounters_cores[0].data.uwp_access_all = 24'(pmcounter[UWP_ACCESS_ALL]);
    assign pmcounters_cores[0].data.uwp_miss_agp = 24'(pmcounter[UWP_MISS_AGP]);
    assign pmcounters_cores[0].data.uwp_miss_tap_dfp = 24'(pmcounter[UWP_MISS_TAP_DFP]);
    assign pmcounters_cores[0].data.uwp_miss_all = 24'(pmcounter[UWP_MISS_ALL]);
    assign pmcounters_cores[0].data.uwp_true_hit_agp = 24'(pmcounter[UWP_TRUE_HIT_AGP]);
    assign pmcounters_cores[0].data.uwp_true_hit_arb = 24'(pmcounter[UWP_TRUE_HIT_ARB]);
    assign pmcounters_cores[0].data.uwp_true_hit_all = 24'(pmcounter[UWP_TRUE_HIT_ALL]);
    assign pmcounters_cores[0].data.uwp_invalidate_agp = 24'(pmcounter[UWP_INVALIDATE_AGP]);
    assign pmcounters_cores[0].data.uwp_invalidate_tap_dfp = 24'(pmcounter[UWP_INVALIDATE_TAP_DFP]);
    assign pmcounters_cores[0].data.uwp_invalidate_all = 24'(pmcounter[UWP_INVALIDATE_ALL]);
    assign pmcounters_cores[0].data.wp_access = 24'(pmcounter[WP_ACCESS]);
    assign pmcounters_cores[0].data.wp_miss = 24'(pmcounter[WP_MISS]);
    assign pmcounters_cores[0].data.wp_true_hit = 24'(pmcounter[WP_TRUE_HIT]);
    assign pmcounters_cores[0].data.pfc_prefetches_hit = 24'(pmcounter[PFC_PREFETCHES_HIT]);
    assign pmcounters_cores[0].data.pfc_useless_prefetches = 24'(pmcounter[PFC_USELESS_PREFETCHES]);
    assign pmcounters_cores[0].data.tlp_access_load = 24'(pmcounter[TLP_ACCESS_LOAD]);
    assign pmcounters_cores[0].data.tlp_access_store = 24'(pmcounter[TLP_ACCESS_STORE]);
    assign pmcounters_cores[0].data.tlp_access_prefetch = 24'(pmcounter[TLP_ACCESS_PREFETCH]);
    assign pmcounters_cores[0].data.tlp_access_agp = 24'(pmcounter[TLP_ACCESS_AGP]);
    assign pmcounters_cores[0].data.tlp_access_arb = 24'(pmcounter[TLP_ACCESS_ARB]);
    assign pmcounters_cores[0].data.tlp_access_all = 24'(pmcounter[TLP_ACCESS_ALL]);
    assign pmcounters_cores[0].data.fillbuf_cannot_alloc = 24'(pmcounter[FILLBUF_CANNOT_ALLOC]);
    assign pmcounters_cores[0].data.pfc_agt_cannot_alloc = 24'(pmcounter[PFC_AGT_CANNOT_ALLOC]);
    assign pmcounters_cores[0].data.pfc_agt_training_alloc = 24'(pmcounter[PFC_AGT_TRAINING_ALLOC]);
    assign pmcounters_cores[0].data.pfc_agt_training_update = 24'(pmcounter[PFC_AGT_TRAINING_UPDATE]);
    assign pmcounters_cores[0].data.pfc_agt_training_tag_miss = 24'(pmcounter[PFC_AGT_TRAINING_TAG_MISS]);
    assign pmcounters_cores[0].data.pfc_agt_training_pf_hit = 24'(pmcounter[PFC_AGT_TRAINING_PF_HIT]);
    assign pmcounters_cores[0].data.pfc_agt_training_load = 24'(pmcounter[PFC_AGT_TRAINING_LOAD]);
    assign pmcounters_cores[0].data.pfc_agt_training_store = 24'(pmcounter[PFC_AGT_TRAINING_STORE]);
    assign pmcounters_cores[0].data.pfc_agt_training_all = 24'(pmcounter[PFC_AGT_TRAINING_ALL]);
    assign pmcounters_cores[0].data.pfc_agt_evict = 24'(pmcounter[PFC_AGT_EVICT]);
    assign pmcounters_cores[0].data.pfc_pht_tap_lookup = 24'(pmcounter[PFC_PHT_TAP_LOOKUP]);
    assign pmcounters_cores[0].data.pfc_pht_tap_hit = 24'(pmcounter[PFC_PHT_TAP_HIT]);
    assign pmcounters_cores[0].data.pfc_pht_agt_alloc = 24'(pmcounter[PFC_PHT_AGT_ALLOC]);
    assign pmcounters_cores[0].data.pfc_pht_agt_update = 24'(pmcounter[PFC_PHT_AGT_UPDATE]);
    assign pmcounters_cores[0].data.pfc_prt_alloc = 24'(pmcounter[PFC_PRT_ALLOC]);
    assign pmcounters_cores[0].data.pfc_prt_update = 24'(pmcounter[PFC_PRT_UPDATE]);
    assign pmcounters_cores[0].data.pfc_prt_cannot_alloc = 24'(pmcounter[PFC_PRT_CANNOT_ALLOC]);
    assign pmcounters_cores[0].data.pfc_no_tlb_credit_stalls = 24'(pmcounter[PFC_NO_TLB_CREDIT_STALLS]);
    assign pmcounters_cores[0].data.pfc_no_tag_credit_stalls = 24'(pmcounter[PFC_NO_TAG_CREDIT_STALLS]);
    assign pmcounters_cores[0].data.pfc_prefetches_sent = 24'(pmcounter[PFC_PREFETCHES_SENT]);
    assign pmcounters_cores[0].data.pfc_prt_l1d_evict_hit = 24'(pmcounter[PFC_PRT_L1D_EVICT_HIT]);
    assign pmcounters_cores[0].data.pfc_prt_reqbuf_alloc_hit = 24'(pmcounter[PFC_PRT_REQBUF_ALLOC_HIT]);
    assign pmcounters_cores[0].data.sc_hit_read = 24'(pmcounter[SC_HIT_READ]);
    assign pmcounters_cores[0].data.sc_hit_write = 24'(pmcounter[SC_HIT_WRITE]);
    assign pmcounters_cores[0].data.sc_hit_prefetch = 24'(pmcounter[SC_HIT_PREFETCH]);
    assign pmcounters_cores[0].data.sc_miss_read = 24'(pmcounter[SC_MISS_READ]);
    assign pmcounters_cores[0].data.sc_miss_write = 24'(pmcounter[SC_MISS_WRITE]);
    assign pmcounters_cores[0].data.sc_miss_prefetch = 24'(pmcounter[SC_MISS_PREFETCH]);
    assign pmcounters_cores[0].data.ldq_missq_full_delay = 24'(pmcounter[LDQ_MISSQ_FULL_DELAY]);
    assign pmcounters_cores[0].data.stq_missq_full_delay = 24'(pmcounter[STQ_MISSQ_FULL_DELAY]);
    generate
        if (SC_PMCI_ENABLED == 1) begin
             assign  pmcounters_scs[0].valid = pmcounters_cores[0].valid;
             assign pmcounters_scs[0].data.location = pmcounters_cores[0].data.location;
             assign pmcounters_scs[0].data.sc_tb_cycles = 24'(clocks - tb_cycles_offset);
             assign pmcounters_scs[0].data.perf_start_sc = pmcounters_cores[0].data.perf_start;
             assign pmcounters_scs[0].data.perf_end_sc = pmcounters_cores[0].data.perf_end;
             assign pmcounters_scs[0].data.terminate_sc = terminate;
             assign pmcounters_scs[0].data.overflow_sc = pmcounters_cores[0].data.overflow;
             assign pmcounters_scs[0].data.sync_sc = pmcounters_cores[0].data.sync;
             assign pmcounters_scs[0].data.sc_cache_access = 24'(sc_pmci[SC_CACHE_ACCESS]);
             assign pmcounters_scs[0].data.sc_cache_rd = 24'(sc_pmci[SC_CACHE_RD]);
             assign pmcounters_scs[0].data.sc_cache_miss = 24'(sc_pmci[SC_CACHE_MISS]);
             assign pmcounters_scs[0].data.sc_cache_miss_rd = 24'(sc_pmci[SC_CACHE_MISS_RD]);
             assign pmcounters_scs[0].data.sc_cache_refill = 24'(sc_pmci[SC_CACHE_REFILL]);
             assign pmcounters_scs[0].data.sc_cache_allocate = 24'(sc_pmci[SC_CACHE_ALLOCATE]);
             assign pmcounters_scs[0].data.sc_cache_wb_dirty = 24'(sc_pmci[SC_CACHE_WB_DIRTY]);
             assign pmcounters_scs[0].data.sc_cache_wb_clean = 24'(sc_pmci[SC_CACHE_WB_CLEAN]);
             assign pmcounters_scs[0].data.sc_cache_inval = 24'(sc_pmci[SC_CACHE_INVAL]);
             assign pmcounters_scs[0].data.sc_snoop = 24'(sc_pmci[SC_SNOOP]);
             assign pmcounters_scs[0].data.sc_scratchpad_rd = 24'(sc_pmci[SC_SCRATCHPAD_RD]);
             assign pmcounters_scs[0].data.sc_scratchpad_wr = 24'(sc_pmci[SC_SCRATCHPAD_WR]);
             assign pmcounters_scs[0].data.f2sc_rd = 24'(sc_pmci[F2SC_RD]);
             assign pmcounters_scs[0].data.f2sc_wr = 24'(sc_pmci[F2SC_WR]);
             assign pmcounters_scs[0].data.mshr_lifetime = 24'(sc_pmci[MSHR_LIFETIME]);
             assign pmcounters_scs[0].data.mshr_allocations = 24'(sc_pmci[MSHR_ALLOCATIONS]);
             assign pmcounters_scs[0].data.sc2f_rd_u = 24'(sc_pmci[SC2F_RD_U]);
             assign pmcounters_scs[0].data.sc2f_rd_c = 24'(sc_pmci[SC2F_RD_C]);
             assign pmcounters_scs[0].data.sc2f_rd_o = 24'(sc_pmci[SC2F_RD_O]);
             assign pmcounters_scs[0].data.sc2f_wr = 24'(sc_pmci[SC2F_WR]);
             assign pmcounters_scs[0].data.c2sc_rd_i = 24'(sc_pmci[C2SC_RD_I]);
             assign pmcounters_scs[0].data.c2sc_rd_d = 24'(sc_pmci[C2SC_RD_D]);
             assign pmcounters_scs[0].data.c2sc_wb_full = 24'(sc_pmci[C2SC_WB_FULL]);
             assign pmcounters_scs[0].data.c2sc_evict = 24'(sc_pmci[C2SC_EVICT]);
             assign pmcounters_scs[0].data.c2sc_snp_wb_full = 24'(sc_pmci[C2SC_SNP_WB_FULL]);
             assign pmcounters_scs[0].data.c2sc_wrnosnpptl = 24'(sc_pmci[C2SC_WRNOSNPPTL]);
             assign pmcounters_scs[0].data.c2sc_snp = 24'(sc_pmci[C2SC_SNP]);
             assign pmcounters_scs[0].data.c2sc_req_stall = 24'(sc_pmci[C2SC_REQ_STALL]);
             assign pmcounters_scs[0].data.c2sc_wdat_stall = 24'(sc_pmci[C2SC_WDAT_STALL]);
             assign pmcounters_scs[0].data.c2sc_srsp_stall = 24'(sc_pmci[C2SC_SRSP_STALL]);
             assign pmcounters_scs[0].data.c2sc_rdat_stall = 24'(sc_pmci[C2SC_RDAT_STALL]);
             assign pmcounters_scs[0].data.c2sc_snp_stall = 24'(sc_pmci[C2SC_SNP_STALL]);
             assign pmcounters_scs[0].data.c2sc_crsp_stall = 24'(sc_pmci[C2SC_CRSP_STALL]);
             assign pmcounters_scs[0].data.sc2f_evict = 24'(sc_pmci[SC2F_EVICT]);
             assign pmcounters_scs[0].data.sc2f_wrbackfull = 24'(sc_pmci[SC2F_WRBACKFULL]);
             assign pmcounters_scs[0].data.sc2f_wdat = 24'(sc_pmci[SC2F_WDAT]);
             assign pmcounters_scs[0].data.sc2f_snoop = 24'(sc_pmci[SC2F_SNOOP]);
             assign pmcounters_scs[0].data.sc2f_rdat = 24'(sc_pmci[SC2F_RDAT]);
             assign pmcounters_scs[0].data.sc2f_req_stall = 24'(sc_pmci[SC2F_REQ_STALL]);
             assign pmcounters_scs[0].data.sc2f_wdat_stall = 24'(sc_pmci[SC2F_WDAT_STALL]);
             assign pmcounters_scs[0].data.sc2f_srsp_stall = 24'(sc_pmci[SC2F_SRSP_STALL]);
             assign pmcounters_scs[0].data.sc2f_rdat_stall = 24'(sc_pmci[SC2F_RDAT_STALL]);
             assign pmcounters_scs[0].data.sc2f_snoop_stall = 24'(sc_pmci[SC2F_SNOOP_STALL]);
             assign pmcounters_scs[0].data.sc2f_crsp_stall = 24'(sc_pmci[SC2F_CRSP_STALL]);
             assign pmcounters_scs[0].data.f2sc_wdat = 24'(sc_pmci[F2SC_WDAT]);
             assign pmcounters_scs[0].data.f2sc_rdat = 24'(sc_pmci[F2SC_RDAT]);
             assign pmcounters_scs[0].data.f2sc_wrnosnpptl = 24'(sc_pmci[F2SC_WRNOSNPPTL]);
             assign pmcounters_scs[0].data.f2sc_req_stall = 24'(sc_pmci[F2SC_REQ_STALL]);
             assign pmcounters_scs[0].data.f2sc_wdat_stall = 24'(sc_pmci[F2SC_WDAT_STALL]);
             assign pmcounters_scs[0].data.f2sc_rdat_stall = 24'(sc_pmci[F2SC_RDAT_STALL]);
             assign pmcounters_scs[0].data.f2sc_crsp_stall = 24'(sc_pmci[F2SC_CRSP_STALL]);
             assign pmcounters_scs[0].data.sc_tag_lookup = 24'(sc_pmci[SC_TAG_LOOKUP]);
             assign pmcounters_scs[0].data.sc_tag_write = 24'(sc_pmci[SC_TAG_WRITE]);
             assign pmcounters_scs[0].data.sc_state_write = 24'(sc_pmci[SC_STATE_WRITE]);
             assign pmcounters_scs[0].data.sc_repl_write = 24'(sc_pmci[SC_REPL_WRITE]);
             assign pmcounters_scs[0].data.sc_tag_hit = 24'(sc_pmci[SC_TAG_HIT]);
             assign pmcounters_scs[0].data.sc_data_read = 24'(sc_pmci[SC_DATA_READ]);
             assign pmcounters_scs[0].data.sc_data_write = 24'(sc_pmci[SC_DATA_WRITE]);
             assign pmcounters_scs[0].data.sc_state_i2e = 24'(sc_pmci[SC_STATE_I2E]);
             assign pmcounters_scs[0].data.sc_state_i2m = 24'(sc_pmci[SC_STATE_I2M]);
             assign pmcounters_scs[0].data.sc_state_i2s = 24'(sc_pmci[SC_STATE_I2S]);
             assign pmcounters_scs[0].data.sc_state_s2i = 24'(sc_pmci[SC_STATE_S2I]);
             assign pmcounters_scs[0].data.sc_state_s2m = 24'(sc_pmci[SC_STATE_S2M]);
             assign pmcounters_scs[0].data.sc_state_s2e = 24'(sc_pmci[SC_STATE_S2E]);
             assign pmcounters_scs[0].data.sc_state_m2i = 24'(sc_pmci[SC_STATE_M2I]);
             assign pmcounters_scs[0].data.sc_state_m2s = 24'(sc_pmci[SC_STATE_M2S]);
             assign pmcounters_scs[0].data.sc_state_m2e = 24'(sc_pmci[SC_STATE_M2E]);
             assign pmcounters_scs[0].data.sc_state_e2i = 24'(sc_pmci[SC_STATE_E2I]);
             assign pmcounters_scs[0].data.sc_state_e2s = 24'(sc_pmci[SC_STATE_E2S]);
             assign pmcounters_scs[0].data.sc_state_e2m = 24'(sc_pmci[SC_STATE_E2M]);
             assign pmcounters_scs[0].data.sft_lookup = 24'(sc_pmci[SFT_LOOKUP]);
             assign pmcounters_scs[0].data.sft_hit = 24'(sc_pmci[SFT_HIT]);
             assign pmcounters_scs[0].data.sft_write = 24'(sc_pmci[SFT_WRITE]);
             assign pmcounters_scs[0].data.sft_eviction = 24'(sc_pmci[SFT_EVICTION]);
             assign pmcounters_scs[0].data.sft_snp_single_icache = 24'(sc_pmci[SFT_SNP_SINGLE_ICACHE]);
             assign pmcounters_scs[0].data.sft_snp_single_dcache = 24'(sc_pmci[SFT_SNP_SINGLE_DCACHE]);
             assign pmcounters_scs[0].data.sft_snp_multi_cores = 24'(sc_pmci[SFT_SNP_MULTI_CORES]);
             assign pmcounters_scs[0].data.sft_eviction_replay = 24'(sc_pmci[SFT_EVICTION_REPLAY]);
             assign pmcounters_scs[0].data.mshr_occupancy = 24'(sc_pmci[MSHR_OCCUPANCY]);
             assign pmcounters_scs[0].data.mshr_full = 24'(sc_pmci[MSHR_FULL]);
             assign pmcounters_scs[0].data.mshr_saq_alloc = 24'(sc_pmci[MSHR_SAQ_ALLOC]);
             assign pmcounters_scs[0].data.mshr_saq_full = 24'(sc_pmci[MSHR_SAQ_FULL]);
             assign pmcounters_scs[0].data.no_alloc_no_mshr = 24'(sc_pmci[NO_ALLOC_NO_MSHR]);
             assign pmcounters_scs[0].data.no_alloc_hint_not_set = 24'(sc_pmci[NO_ALLOC_HINT_NOT_SET]);
             assign pmcounters_scs[0].data.sc_replay_ecc = 24'(sc_pmci[SC_REPLAY_ECC]);
             assign pmcounters_scs[0].data.sc_victim = 24'(sc_pmci[SC_VICTIM]);
             assign pmcounters_scs[0].data.no_alloc_srrip = 24'(sc_pmci[NO_ALLOC_SRRIP]);
             assign pmcounters_scs[0].data.sc_cancel_piperesult = 24'(sc_pmci[SC_CANCEL_PIPERESULT]);

        end
    endgenerate

    // AUTOGENERATED -- off

    assign hpmcounters_cores[0].valid            = !reset  && ((|mhpm_write) || (terminate^terminate_1T));
    assign hpmcounters_cores[0].data.location    = location;
    assign hpmcounters_cores[0].data.hpmcounter3 = hpmi[HPMCOUNTER3];
    assign hpmcounters_cores[0].data.hpmcounter4 = hpmi[HPMCOUNTER4];
    assign hpmcounters_cores[0].data.hpmcounter5 = hpmi[HPMCOUNTER5];
    assign hpmcounters_cores[0].data.hpmcounter6 = hpmi[HPMCOUNTER6];
    assign hpmcounters_cores[0].data.hpmcounter7 = hpmi[HPMCOUNTER7];
    assign hpmcounters_cores[0].data.hpmcounter8 = hpmi[HPMCOUNTER8];
    assign hpmcounters_cores[0].data.hpmcounter9 = hpmi[HPMCOUNTER9];
    assign hpmcounters_cores[0].data.hpmcounter10 = hpmi[HPMCOUNTER10];

    always_ff @(posedge clk) begin : overflow_logic
        automatic logic overflow_nxt = '0;
        if (!reset) begin
            if (SC_PMCI_ENABLED == 1) begin
                for (int i = 0; i < EVENT_COUNT; i++) begin
                    overflow_nxt |= pmcounter[i][OVERFLOW_BIT] ^ pmcounter_overflow_bit[i];
                end
                overflow_nxt |= (cpu_cycles[OVERFLOW_BIT] ^ pmcounter_overflow_bit[EVENT_COUNT]) | (branch_instructions[OVERFLOW_BIT] ^ pmcounter_overflow_bit[EVENT_COUNT + OVERFLOW_BIT_EXTRA - 1]);
                for (int i = EVENT_COUNT + OVERFLOW_BIT_EXTRA; i < EVENT_COUNT + OVERFLOW_BIT_EXTRA + SC_EVENT_COUNT; i++) begin
                    overflow_nxt |= sc_pmci[i - (EVENT_COUNT + OVERFLOW_BIT_EXTRA)][OVERFLOW_BIT] ^ pmcounter_overflow_bit[i];
                end
            end else begin
                for (int i = 0; i < EVENT_COUNT; i++) begin
                    overflow_nxt |= pmcounter[i][OVERFLOW_BIT] ^ pmcounter_overflow_bit[i];
                end
                overflow_nxt |= (cpu_cycles[OVERFLOW_BIT] ^ pmcounter_overflow_bit[EVENT_COUNT]) | (branch_instructions[OVERFLOW_BIT] ^ pmcounter_overflow_bit[EVENT_COUNT + OVERFLOW_BIT_EXTRA - 1]);
            end
        end
        overflow <= overflow_nxt;
    end

    always_ff @(posedge clk) begin : pmcounters_overflow_bit
        if (reset) begin
            pmcounter_overflow_bit <= '0;
        end else if (pmcounters_cores[0].valid) begin
            if (SC_PMCI_ENABLED == 1) begin
                for (int i = 0; i < EVENT_COUNT; i++) begin
                    pmcounter_overflow_bit[i] <= pmcounter[i][OVERFLOW_BIT];
                end
                pmcounter_overflow_bit[EVENT_COUNT]        <= cpu_cycles[OVERFLOW_BIT];
                pmcounter_overflow_bit[EVENT_COUNT + OVERFLOW_BIT_EXTRA - 1]    <= branch_instructions[OVERFLOW_BIT];
                for (int i = EVENT_COUNT + OVERFLOW_BIT_EXTRA; i < EVENT_COUNT + OVERFLOW_BIT_EXTRA + SC_EVENT_COUNT; i++) begin
                    pmcounter_overflow_bit[i] <= sc_pmci[i - (EVENT_COUNT + OVERFLOW_BIT_EXTRA)][OVERFLOW_BIT];
                end
            end else begin
                for (int i = 0; i < EVENT_COUNT; i++) begin
                    pmcounter_overflow_bit[i] <= pmcounter[i][OVERFLOW_BIT];
                end
                pmcounter_overflow_bit[EVENT_COUNT]        <= cpu_cycles[OVERFLOW_BIT];
                pmcounter_overflow_bit[EVENT_COUNT + OVERFLOW_BIT_EXTRA - 1]    <= branch_instructions[OVERFLOW_BIT];
            end
        end
    end

endmodule

module pmu_ema #(parameter WIDTH=8, // Width of Reported Value / Activity
                 parameter DECAY=8) // Width of extra decay bits, total size = WIDTH+DECAY
(
  input              i_clk,
  input              i_reset_n,
  input              i_en,
  input [WIDTH-1:0]  i_activity, // new activity to add in
  input [WIDTH-1:0]  i_decayadj, // decay rate adjustment in powers of 2, tie to 0 if unneeded
  output [WIDTH-1:0] o_activity, // Ema Current Value
  output             o_busy // Ema is non-zero
);

   localparam EMA_SIZE = WIDTH+DECAY;
   logic [EMA_SIZE-1:0] NxtEma;
   logic [EMA_SIZE-1:0] Ema;

   // Old Value + New Activity - OldValue Decayed
   assign NxtEma[EMA_SIZE-1:0]
     = Ema[EMA_SIZE-1:0] +
       ({{DECAY{1'b0}},i_activity[WIDTH-1:0]} << i_decayadj[WIDTH-1:0]) -
       ({{DECAY{1'b0}},Ema[WIDTH+DECAY-1:DECAY+1],        // upper bits
         Ema[DECAY] |                                     // bit 0 of upper
         ~(|Ema[WIDTH+DECAY-1:DECAY]) & (|Ema[DECAY-1:0]) // no upper bits, decay lower bits rounded up
         } << i_decayadj[WIDTH-1:0]);

   always_ff @(posedge i_clk)
     if (~i_reset_n)
       Ema[EMA_SIZE-1:0] <= {EMA_SIZE{1'b0}};
     else if (i_en)
       Ema[EMA_SIZE-1:0] <= NxtEma[EMA_SIZE-1:0];

   assign o_activity[WIDTH-1:0] = Ema[WIDTH+DECAY-1:DECAY];
   assign o_busy = |Ema;

endmodule
