# PMU

Performance Monitoring Unit infrastructure that samples the RISC-V DUT's hardware performance counters (PMCs), aggregates them on the C++ side, and reports per-hart and shared-cache (SC) metrics, optionally checking them against expected values.

## What it does

The DUT exposes per-cycle event activity over the PMC interface (PMCI). `pmu.sv` accumulates each event into wide counters and emits a transaction whenever a sync point is hit (terminate, overflow, periodic cycle/instruction sync, `perf_start`/`perf_end` markers, or an mhpmevent CSR write). The C++ `pmu` class receives those transactions, folds the (truncated, `pmcounter_dpi_width`-bit) deltas back into full 64-bit counters, logs metrics, and runs optional pass/fail checks (IPC, L1D read-miss rate, and sideband-vs-hpmcounter cross-checks). The event set is data-driven: it is generated at build time from CSV specs, so adding an event only requires a CSV edit.

```
                                                                                    transactions
       PMCI / HPMI /SC-PMCI                                                   (sync/terminate/overflow)
 DUT  ──────────────────────►  pmu.sv  ──────────►  pmcounters_core / _sc  ─────────────────────────────►  pmu (C++)
                        (accumulate + sync)       hpmcounters / pmc_checker                                   │
                                                                                                              ▼
                                                                                                      report + checks (IPC,
                                                                                                     L1D miss, sideband PMC)
```

## Files

- **`pmu.sv`** — SystemVerilog module (template). Accumulates per-event counters from `pmci`/`hpmi`/`sc_pmci`, generates sync/terminate/overflow conditions, derives `perf_start`/`perf_end` from RVFI marker instructions, and drives the `pmcounters_core`, `pmcounters_sc`, `hpmcounters_core`, and `pmc_checker` transactions. Includes the `pmu_ema` exponential moving-average helper. The `\`include "gen_events.svh"` placeholder is replaced at build time with the per-event counter assigns.
- **`pmu.hpp` / `pmu.cpp`** — C++ `pmu` class. Connects to the counter transactions via the registry, reconstructs full counters (`core_to_vector` / `sc_to_vector`), tracks a per-source state machine (SYNCING → SYNC_UNTIL_TERMINATE → READY_TO_TERMINATE), records perf-region snapshots, logs metrics, and implements `ipc_check`, `l1d_read_miss_check`, and the sideband `pmc_checker` comparison.
- **`core_pmc_spec.csv` / `sharedcache_pmc_spec.csv`** — The event specifications: `Event ID`, `Name`, `Description`, and (core only) `Multi-hot Encoding` / `Multi-dimensional Encoding` columns used for filtered/multi-hot event decoding.
- **`pmu_gen.py`** — Code generator. Parses the CSVs (plus synthetic events such as `branch_instructions` and `tb_cycles`) and emits the generated fragments: `gen_events.hpp` (enums, `*_to_vector`, reflection maps, event/filtered-event maps), `gen_events.svh`, `gen_pmu_pkg.sv`, `gen_pmu_defines.sv` (`RV_TESTER_PMCI_PORTS/VARS`), `gen_monitor.svh`, `gen_events.yaml`, and the inlined `pmu.sv`.
- **`pmu_fragment_gen.bzl`** — Bazel rule that runs `pmu_gen` to produce the generated headers/SV and wraps them in `cc_library` / `verilog_library` targets.
- **`pmu.bzl`** — `pmu_gen` macro that ties the generated fragments together into the DPI `cc_library` and the `verilog_library` for the module.
- **`pmu_plusargs.h`** — Declares the shared `perf` plusarg for dependents.
- **`BUILD.bazel`** — `pmu_gen` py_binary, `pmu_spec` filegroup (the two CSVs), and `pmu_plusargs`.

## Data flow

1. The DUT drives per-cycle event activity on `pmci` (core), `hpmi` (HPM counters), and `sc_pmci` (shared cache).
2. `pmu.sv` accumulates each event and, at a sync point (terminate, overflow, periodic cycle/instruction sync, `perf_start`/`perf_end`, or an mhpmevent write), emits the counter transactions.
3. The C++ `pmu` reconstructs full 64-bit counters from the truncated transaction fields and updates its state machine.
4. On `perf_start`/`perf_end` it snapshots a perf region; on shutdown it reports all metrics and runs the configured IPC / L1D-miss / sideband-PMC checks.

## Plusargs

- **`perf`** — master enable for PMU collection.
- **`pmcounters_log`** — dump per-trigger counter rows to `h<id>_pmcounters_core.log` / `pmcounters_sc.log`.
- **`sync_pmcounters_period` / `sync_pmcounters_instructions`** — periodic sync cadence (0 = only sync on terminate).
- **`perf_tb_cycles_rvfi_offset`** — offset between RVFI retire cycle and PMU retire cycle for `tb_cycles`.
- **`ipc_check` / `ipc_expected` / `ipc_tolerance_perc`** — enable and bound the IPC check.
- **`l1d_read_miss_check` / `l1d_read_miss_expected` / `l1d_read_miss_tolerance_perc`** — enable and bound the L1D read-miss-rate check.
- **`pmc_sideband_check` / `pmc_check_threshold` / `ignore_pmc_reprogram`** — enable the sideband-vs-hpmcounter cross-check and control its threshold and illegal-reprogram handling.
