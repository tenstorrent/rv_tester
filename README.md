# RV_TESTER

RISCV CPU testing component written in SV and C++
Bla 
## Overview

This repository contains the verification collateral needed to interface with a RISC-V CPU core and perform lockstep architectural checks against the [Whisper](https://github.com/tenstorrent/whisper) RISC-V CPU ISS. Along with the co-simulation flow, it provides soft device models, an AXI transactor/master, performance-counter (PMU) modeling, interrupt and trigger generation, and the platform glue that ties it all together. RV_TESTER is RVA23-compatible.

The DUT (the RISC-V core under test) is surrounded with everything needed to boot, stimulate, observe, and check the core. As the core executes, its retired instructions and memory ordering events are checked instruction-by-instruction against Whisper, while its bus traffic is serviced by a software system model of the surrounding platform.



The main components involved here are:

- **RVFI Monitor** — Samples signals from the RISC-V Formal Interface and passes them to the bridge.
- **Whisper API** — Steps the ISS on each instruction retire and reports CPU architectural state changes relative to the previous step.
- **Bridge** — Orchestrates collection of DUT vs ISS architectural state and forwards it to the CAC. The co-simulation flow (RVFI/MCM capture, the Whisper ISS wrapper, the bridge, and the CAC comparison) is detailed in `[src/cosim/](src/cosim/README.md)`, with nested READMEs for `[bridge/](src/cosim/bridge/README.md)` and `[dut_if/](src/cosim/dut_if/README.md)`.
- **CAC (Core Arch Checker)** — Compares DUT vs ISS architectural state and flags mismatches.
- **AXI SW (transactor)** — Receives requests from the RISC-V CPU AXI bus and creates C++ transactions.
- **AXI MST SW** — Converts C++ transactions into SystemVerilog bus-level activity.
- **Sysmod** — System model that divides the address space per the memmap and routes requests to device models. See `[src/sysmod/](src/sysmod/README.md)`.
- **Devices (CLINT, TRICKBOX, HTIF, DM, ...)** — Model device-specific functionality.
- **Clocking & reset** — Per-domain clock generation, optional glitch-free clock-profile muxing for dynamic frequency switching, external-clock support, and cold/warm reset sequencing (including DUT-requested warm resets) across clock domains.
- **Lifecycle & DPI bring-up** — Parses plusargs, seeds randomization, builds and tears down the `C++` object registry, and orders DPI initialization against reset so the `C++` side is ready before the core leaves reset.
- **Termination & rerun** — Aggregates termination sources (DUT, cosim, sysmod/HTIF, DMI timeout, errors), drives a graceful quiesce/drain handshake, prints PASS metrics, and supports rerunning or warm-resetting a test.
- **Performance & monitoring** — Periodic and end-of-test performance calculation, instruction counting, and clock monitoring.
- **Interrupts, triggers & PMU** — Generate interrupts and event triggers to the core and model its performance counters.

## Getting Started

rv_tester builds with [Bazel](https://bazel.build/) (bzlmod).

```sh
# Build the exported targets
bazel build //...

# Run the tests
bazel test //test/...
```

Nested READMEs under `src/` (e.g. `src/cosim/`, `src/sysmod/`) document
individual subsystems in more detail.

## Contributing

Contributions are welcome! Bug reports and feature requests are handled via
[GitHub Issues](https://github.com/tenstorrent/rv_tester/issues), and changes
are submitted via pull requests (reviewed weekly). See
[CONTRIBUTING.md](CONTRIBUTING.md) for build/test instructions and contribution
standards, and note that this project follows the
[Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md). To report a security
vulnerability, follow the process in [SECURITY.md](SECURITY.md).

## License

- [LICENSE](LICENSE) (Apache-2.0) — Overall license for this project, except where specified.
- [LICENSE-DOCS](LICENSE-DOCS) (CC-BY-4.0) — License for all documentation and images only.
- [LICENSE_understanding.txt](LICENSE_understanding.txt) — Tenstorrent's clarification of how the Apache-2.0 license applies to this repository.

This repository is [REUSE](https://reuse.software) compliant; per-file license
and copyright information is provided via inline SPDX headers and
[REUSE.toml](REUSE.toml). Third-party components (e.g. the PCG random library
and the IEEE 1800-2017 DPI header) retain their own licenses as recorded in
[NOTICE](NOTICE).
