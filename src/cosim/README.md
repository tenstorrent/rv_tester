# COSIM

Co-simulation (lockstep) infrastructure that compares a RISC-V DUT (Design Under Test) against the [Whisper](https://github.com/tenstorrent/whisper) RISC-V instruction set simulator (ISS) on every instruction retire.

## What it does

As the DUT executes, it streams architectural events (retired instructions, register/CSR writes, memory accesses, interrupts, exceptions, debug entry/exit) over the RISC-V Formal Interface (RVFI). `cosim` feeds those same instructions to Whisper, collects the ISS architectural state, and checks that the DUT and ISS agree. Any divergence is flagged as a mismatch, which makes this the primary correctness check for the core.

```
        RVFI events                  step / peek / poke
 DUT  ───────────────►  cosim  ◄──────────────────────►  Whisper ISS
                          │
                          ▼
                Core Arch Checker (CAC)
                 DUT state == ISS state ?
```

## Files

- **`cosim.sv`** — SystemVerilog top that wires the DUT/RVFI signals into the DPI layer.
- **`dut_if/`** — DUT-facing event capture. 
        - `rvfi/` — Receives RVFI transactions (retire, regs, CSRs, interrupts, traps, debug) and forwards them into the bridge. 
        - `mcmi/` — Memory Consistency Model interface: load/store/fetch ordering events (read, insert, bypass, write, evict) used for memory checks.
- **`whisper_if/`** — Wrapper around the Whisper ISS (`whisper_client`). Provides step, peek, poke, translate, page-table-walk, interrupt/NMI injection and MCM hooks, exposed as remote procedure calls so other components can drive the ISS.
- **`bridge/`** — The orchestration core. Translates DUT events into ISS steps, keeps DUT and ISS state aligned (including interrupt/NMI timing, debug mode, address translation and resynchronization for hardware-specific behavior), and drives the Core Arch Checker comparisons.
- **`utils/`** — Shared helpers, including start-of-test (`sot/`) and end-of-test (`eot/`) handling and general utilities.

## Data flow

1. The DUT retires an instruction and emits RVFI/MCM events.
2. `dut_if` packages those events and signals the `bridge`.
3. The `bridge` steps the Whisper ISS (`whisper_if`) for the same instruction and gathers the resulting ISS architectural state.
4. The bridge submits DUT vs ISS state to the Core Arch Checker, which reports any mismatch.
