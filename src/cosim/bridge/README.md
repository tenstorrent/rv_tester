# BRIDGE

The orchestration core of co-simulation. The bridge sits between the DUT-facing interfaces (`dut_if/`) and the Whisper ISS (`whisper_if/`), keeps the two in lockstep, and submits their architectural state to the Core Arch Checker (CAC) for comparison.

## Responsibilities

- **Step the ISS in lockstep with the DUT.** For each DUT-retired instruction it steps Whisper, collects the resulting state changes, and converts both sides into a common representation for comparison.
- **Compare architectural state.** PC, GPR/FPR/vector registers, CSRs, privilege level, and memory accesses are diffed via two CAC cores (`cac_` for general state, `csr_cac_` for CSRs). Any divergence is reported as a mismatch.
- **Drive the ISS.** Wraps Whisper peek/poke/step/translate/page-table-walk and interrupt/NMI injection so DUT-observed events can be reflected into the ISS.
- **Model timing-sensitive behavior.** Handles interrupt and NMI delivery and deferral, MTIP/timer interrupts, IMSIC MSIs, debug-mode entry/exit, and address translation so the ISS matches the DUT's actual timing.
- **Check the memory consistency model (multi-core).** Forwards the DUT's memory-ordering events (read/insert/bypass/write/ifetch/ievict) into Whisper's MCM so loads/stores are validated against the RISC-V memory model across all harts, not just per-instruction architectural state.
- **Resynchronize on legitimate divergence.** Some hardware behavior (custom CSRs, MMIO reads from CLINT/HTIF/trickbox/UART, LR/SC, hypervisor-masked fields, opcode rewrites) cannot be predicted by the ISS. The bridge detects these cases and re-syncs the ISS to the DUT instead of flagging an error.

## Pre-step / post-step hooks

A key pattern in `bridge.cpp` is the pre-step / post-step sequencing around each Whisper step. Before stepping, the bridge may poke pending exceptions, LR/SC state, debug entry/exit and interrupts into the ISS; after stepping it checks NMI/interrupt/exception outcomes and handles SATP writes. This ordering is what keeps interrupt and trap timing aligned between DUT and ISS.

## Hypervisor CSRs save and restore special considerations:
In the DUT, when `misa.H` is cleared to zero the hypervisor-related CSRs are saved and then restored when `misa.H` goes back to one. To keep the ISS consistent with this behavior, `rv_tester` saves those hypervisor CSRs in the CoreArchChecker when `misa.H` becomes zero and restores them when `misa.H` is set again. The bridge maintains maps for special CSR treatment — hypervisor-masked CSRs and their masked bit-fields (gated by `misa.H`), the full hypervisor CSR set, and CSRs that may be peeked/resynced for interrupts. These tables let it correctly mask, compare, or resync CSR state depending on enabled extensions.


