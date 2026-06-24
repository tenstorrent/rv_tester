module pmu
  import rv_tester_params::*;
  import rv_tester_pkg::*;
  import pmu_pkg::*;
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
  longint unsigned cycle_period_counter = 0;
  longint unsigned instruction_period_counter = 0;
  longint unsigned nret = {32'h0, NRET};
  longint unsigned pmcounter [EVENT_COUNT]  = '{default:0};
  bit [EVENT_COUNT-1:0][63:0] pmcounter_vec;
  bit [63:0] pmcounter_instr;
  bit [63:0] tb_cycles      ; 
  longint unsigned branch_instructions;

  // Create shared sync condition signals
  logic cycle_sync_condition;
  logic instruction_sync_condition;
  assign cycle_sync_condition = cycle_sync_en && cycle_period_counter == 0;

`ifdef IXCOM_COMPILE
  // Palladium creates memories instead of flip flops for large
  // arrays. This created a memory with 1955 ports for pmcounter,
  // which caused a timing slowdown. We are using $ixc_control to 
  // force ixcom to synthesize flip flops to improve timing
  initial $ixc_ctrl("map_to_reg", pmcounter);
`endif

  always @(posedge clk) begin

    instruction_sync_condition <= '0;

    if (reset) begin
      pmcounter[0] <= 0;
      cpu_cycles <= 0;
      cycle_period_counter <= 0;
      instruction_period_counter <= instructions;
    end else begin
      if(perf_enabled)begin
        if (perf_start) begin
          cycle_period_counter <= 1;  // Start counting from 1 to get full period before first sync
          instruction_period_counter <= instructions;
        end else begin
          // Update period counters
          if (cycle_sync_en) begin
            if (cycle_period_counter >= (period - 1)) begin
              cycle_period_counter <= 0;
            end else begin
              cycle_period_counter <= cycle_period_counter + 1;
            end
          end

          if (instruction_sync_en) begin
            if (instruction_period_counter <= {60'h0, pmci[INSTRUCTIONS]}) begin
              // When syncing, start next period but account for extra instructions executed
              // If instructions is smaller than the extra instructions executed, start from instructions
              instruction_period_counter <= instructions - (
                                                            ({60'h0, pmci[INSTRUCTIONS]} - instruction_period_counter) > instructions ?
                                                            '0 :
                                                            ({60'h0, pmci[INSTRUCTIONS]} - instruction_period_counter)
                                                            );
              instruction_sync_condition <= '1;
            end else begin
              instruction_period_counter <= instruction_period_counter - {60'h0, pmci[INSTRUCTIONS]};
            end
          end
        end
        cpu_cycles <= cpu_cycles + 1;
      end
    end
    terminate_1T <= terminate;
  end

  assign pmcounter_vec[0] = pmcounter[0];
  generate
    for (genvar i=1; i < EVENT_COUNT; i++) begin : pmci_regs
      always @(posedge clk) begin
        if (reset) begin
          pmcounter[i] <= 0;
        end else begin
          if(perf_enabled)begin
            pmcounter[i] <= pmcounter[i] + {60'h0, pmci[i]}; 
          end
        end
      end
      assign pmcounter_vec[i] = pmcounter[i];
    end
  endgenerate
  assign tb_cycles       = clocks - tb_cycles_offset; 

  `RV_TESTER_KEEPER_INIT(clk,3)
  `RV_TESTER_KEEPER_DATA(clk,1,pmcounter_vec)
  `RV_TESTER_KEEPER_DATA(clk,2,tb_cycles)
  `RV_TESTER_KEEPER_DATA(clk,3,cpu_cycles)

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
      pmc_checkers[i].valid = !reset  && (mhpm_write[i] || (terminate^terminate_1T)) && perf_enabled;
      pmc_checkers[i].data.location   = location;
      pmc_checkers[i].data.terminate  = terminate;
      pmc_checkers[i].data.event_csr  = rvfi[i].csr_addr[3:0] - 4'h3;
      pmc_checkers[i].data.event_id   = rvfi[i].csr_wdata[55:0] & rvfi[i].csr_wmask[55:0];
    end
  end


  localparam MAX_COUNTER_VALUE_CHANGE_IN_ONE_CYCLE = 32;
  parameter OVERFLOW_BIT = 24 - 1;
  parameter OVERFLOW_BIT_EXTRA = 2;
  logic overflow;
  logic [EVENT_COUNT + SC_EVENT_COUNT + OVERFLOW_BIT_EXTRA -1 : 0] pmcounter_overflow_bit;

  if (OVERFLOW_BIT < ($clog2(MAX_COUNTER_VALUE_CHANGE_IN_ONE_CYCLE) + 1))
    $fatal(1, "The selected overflow bit cannot cover the maximum value change of a counter within a single clock cycle.");

  assign pmcounters_cores[0].valid = !reset && perf_enabled && (overflow || (|mhpm_write) || terminate || cycle_sync_condition || instruction_sync_condition || perf_start || perf_end);
  assign pmcounters_cores[0].data.location = location;
  assign pmcounters_cores[0].data.tb_cycles = 24'(clocks - tb_cycles_offset);
  assign pmcounters_cores[0].data.cpu_cycles = 24'(cpu_cycles);
  assign pmcounters_cores[0].data.instructions = 24'(pmcounter[INSTRUCTIONS]);
  assign pmcounters_cores[0].data.branch_instructions = 24'(branch_instructions);
  assign pmcounters_cores[0].data.perf_start = perf_start;
  assign pmcounters_cores[0].data.perf_end = perf_end;
  assign pmcounters_cores[0].data.terminate = terminate;
  assign pmcounters_cores[0].data.overflow = overflow;
  assign pmcounters_cores[0].data.sync = cycle_sync_condition || instruction_sync_condition;

  generate
    if (SC_PMCI_ENABLED == 1) begin
      assign pmcounters_scs[0].valid = pmcounters_cores[0].valid;
      assign pmcounters_scs[0].data.location = pmcounters_cores[0].data.location;
      assign pmcounters_scs[0].data.sc_tb_cycles = 24'(clocks - tb_cycles_offset);
      assign pmcounters_scs[0].data.perf_start_sc = pmcounters_cores[0].data.perf_start;
      assign pmcounters_scs[0].data.perf_end_sc = pmcounters_cores[0].data.perf_end;
      assign pmcounters_scs[0].data.terminate_sc = terminate;
      assign pmcounters_scs[0].data.overflow_sc = pmcounters_cores[0].data.overflow;
      assign pmcounters_scs[0].data.sync_sc = pmcounters_cores[0].data.sync;
    end
  endgenerate

`include "gen_events.svh"


  assign hpmcounters_cores[0].valid            = !reset  && ((|mhpm_write) || (terminate^terminate_1T)) && perf_enabled;
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
    if (!reset && perf_enabled) begin
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
    end else if (pmcounters_cores[0].valid && perf_enabled) begin
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
