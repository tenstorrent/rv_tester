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
In the DUT, when `misa.H` is cleared to zero the hypervisor-related CSRs are saved and then restored when `misa.H` goes back to one. To keep the ISS consistent with this behavior, the bridge saves those hypervisor CSRs when `misa.H` becomes zero and restores them when `misa.H` is set again. This save/restore is controlled by the gflag `hyp_save_restore_en` (default `true`). Disable it with `+hyp_save_restore_en=0` if you want CSR values to reset when hypervisor mode is re-enabled instead of being restored from the saved snapshot. The bridge maintains maps for special CSR treatment — hypervisor-masked CSRs and their masked bit-fields (gated by `misa.H`), the full hypervisor CSR set, and CSRs that may be peeked/resynced for interrupts. These tables let it correctly mask, compare, or resync CSR state depending on enabled extensions.

The CSRs to save and restore are not listed in the bridge code: they are passed with the gflag `hyp_masked_csr` as a comma-separated list of `address:field_mask` pairs, where `field_mask` marks the fields masked by `misa.H` (`0x0` for a CSR that is saved and restored without masked fields). The flag is empty by default, so no CSR is saved or restored until a project passes its list, typically from its flagfile. `henvcfg`, `vsip` and `hstateen0` are not expected in the list because they are disabled, or retained by the ISS, while `misa.H` is zero. A CSR in a custom address range may be listed as well. The ISS has no `misa.H` gating for those, so the bridge additionally pokes the masked fields to zero in the ISS when `misa.H` is cleared, keeps them at zero across software writes made while `misa.H` is zero, and pokes the saved values back when `misa.H` is set. The ISS poke mask of such a CSR must cover the masked fields.


