module pmu #(
  parameter int NUM = -1,
  `TOPOLOGY
)(
  input clk,
  input reset,
  input longint unsigned clocks,
  bit terminate,
  `RV_TESTER_TRANSACTIONS_OUTPUT_PMU
);

    int unsigned location = cvm_topology::nil;
    longint unsigned period = 0;
    bit perf_enabled = '0;

    always @(posedge clk) begin
        if (reset) begin
            location <= cvm_topology::get_location(topology.TOP.PLATFORM.PMU.ID, 0);
            perf_enabled <= (cvm_plusargs::get_bool("perf") != '0) & (location != cvm_topology::nil);
            period <= cvm_plusargs::get_ulongint("pmcounters_period");
        end
    end

    assign pmcounterss[0].valid = perf_enabled && (terminate || ((clocks % period) == 0));
    assign pmcounterss[0].data.location = location;
    assign pmcounterss[0].data.cpu_cycles = clocks;

    always@(pmcounterss[0].valid) begin
        if (pmcounterss[0].valid) begin
              $display("here");
        end
    end

endmodule
