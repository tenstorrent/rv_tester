module pmu
import rv_tester_params::*;
#(
  parameter int NUM = -1,
  parameter int NRET = 1,
  `TOPOLOGY
)(
  input clk,
  input reset,
  input longint unsigned clocks,
  input pmci_t pmci,
  input rvfi_t rvfi[NRET-1:0],
  input bit terminate,
  `RV_TESTER_TRANSACTIONS_OUTPUT_PMU
);

    int unsigned location = cvm_topology::nil;
    longint unsigned period = 0;
    bit sync;
    assign sync = period != '0;
    bit perf_enabled = '0;

    always @(posedge clk) begin
        if (reset) begin
            /* verilator lint_off BLKSEQ */
            location = cvm_topology::get_location(topology.TOP.PLATFORM.PMCI.ID, 0);
            perf_enabled = (cvm_plusargs::get_bool("perf") != '0) & (location != cvm_topology::nil);
            period = cvm_plusargs::get_ulongint("sync_pmcounters_period");
            /* verilator lint_on BLKSEQ */
        end
    end

    longint unsigned cpu_cycles = 0;
    longint unsigned instructions = 0;
    longint unsigned pmcounter [EVENT_COUNT] = '{default:0};

    always @(posedge clk) begin
        if (!reset) begin
            automatic longint unsigned total = 0;
            // fold
            for (integer n = 0; n < NRET; n++) begin
                if (rvfi[n].valid) begin
                    total++;
                end
            end
            cpu_cycles <= clocks;
            instructions <= instructions + total;
            // Count supported events
            for (integer n = 0; n < EVENT_COUNT; n++) begin
              pmcounter[n] <= pmcounter[n] + {60'h0, pmci[n].counter};
            end
        end
    end

    assign pmcounterss[0].valid = !reset && perf_enabled && (terminate || (sync & (cpu_cycles % period) == 0));
    assign pmcounterss[0].data.location = location;
    assign pmcounterss[0].data.cpu_cycles = cpu_cycles;
    assign pmcounterss[0].data.instructions = instructions;
    assign pmcounterss[0].data.itlb_read_access = pmcounter[ITLB_READ_ACCESS];
    assign pmcounterss[0].data.itlb_read_miss = pmcounter[ITLB_READ_MISS];
    assign pmcounterss[0].data.l1i_read_access = pmcounter[L1I_READ_ACCESS];
    assign pmcounterss[0].data.l1i_read_miss = pmcounter[L1I_READ_MISS];
    assign pmcounterss[0].data.l1d_read_access = pmcounter[L1D_READ_ACCESS];
    assign pmcounterss[0].data.l1d_write_access = pmcounter[L1D_WRITE_ACCESS];

endmodule
