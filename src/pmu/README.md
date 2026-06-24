# PMU

Performance Monitoring Unit infrastructure that samples the RISC-V DUT's hardware performance counters (PMCs), aggregates them on the C++ side, and reports per-hart and shared-cache (SC) metrics, optionally checking them against expected values.

## What it does

The DUT exposes per-cycle event activity over the PMC interface (PMCI). `pmu.sv` accumulates each event into wide counters and emits a transaction whenever a sync point is hit (terminate, overflow, periodic cycle/instruction sync, `perf_start`/`perf_end` markers, or an mhpmevent CSR write). The C++ `pmu` class receives those transactions, folds the (truncated, `pmcounter_dpi_width`-bit) deltas back into full 64-bit counters, logs metrics, and runs optional pass/fail checks (IPC, L1D read-miss rate, and sideband-vs-hpmcounter cross-checks).

```
                                                                                    transactions
      PMCI / HPMI / SC-PMCI                                                   (sync/terminate/overflow)
 DUT  ──────────────────────►  pmu.sv  ──────────►  pmcounters_core / _sc  ─────────────────────────────►  pmu (C++)
                        (accumulate + sync)       hpmcounters / pmc_checker                                   │
                                                                                                              ▼
                                                                                                      report + checks (IPC,
                                                                                                     L1D miss, sideband PMC)
```

The PMU is gated by the topology `PMCI` attributes: `PMU_ENABLE` instantiates the module and `SC_PMCI_ENABLED` enables the shared-cache path. The event set is data-driven — events are defined in the `*_pmc_spec.csv` specs and generated into the C++/SV sources at build time.

## Setup

Enable the topology attributes (`rv_tester_pmci_attrs` in the `*_rv_tester_hart.yml`):

```yaml
rv_tester_pmci_attrs: &rv_tester_pmci_attrs
  PMU_ENABLE: 1
  SC_PMCI_ENABLED: 1
```

At build time `pmu_gen.py` consumes the `*_pmc_spec.csv` specs and generates the matching C++ enums/maps and SV fragments — to add an event, edit the CSV only.

Then run with `+perf` to collect metrics. Optional plusargs:

```
+perf                                   # master enable (required)
+pmcounters_log                         # dump per-trigger counter rows to log
+sync_pmcounters_period=<N>             # periodic sync every N cycles (0 = only at terminate)
+ipc_check +ipc_expected=<f>            # pass/fail IPC check within +ipc_tolerance_perc
+l1d_read_miss_check ...                # pass/fail L1D read-miss-rate check
+pmc_sideband_check                     # cross-check hpmcounters vs sideband counters
```
