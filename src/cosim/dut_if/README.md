# DUT INTERFACE

The DUT-facing capture layer. These components subscribe to transactions emitted by the DUT, repackage them into the bridge's data model, and hand them to the `bridge` for lockstep checking against the ISS. Both interfaces connect to the messenger by transaction type and forward into the same `bridge` instance.

```
 DUT RVFI/MCM transactions ──► dut_if (rvfi, mcmi) ──► bridge
```

## rvfi/ — RISC-V Formal Interface

Captures the per-instruction architectural trace. The `rvfi` class subscribes to `m_rvfi`, `m_steps`, `m_trap`, `m_gp_regs`, `m_fp_regs`, `m_vc_regs`, `m_core_nmi`, `m_interrupt_pend`, `m_mtip`, `m_mtime`, `m_imsic_msi`, `m_debug`, `m_csri`, `m_mhpm_counter_ovf`, and reset/disable-checks messages.

Its main job is to assemble a complete retired instruction:

- `make_instr()` builds an `rv_instr_t` from an incoming `m_rvfi` transaction.
- `append_uop_changes_to_instr()` merges micro-op / cracked-instruction pieces so a single architectural instruction is presented as one unit.
- `send_instr()` / `send_instr_group()` / `send_csr()` forward the assembled state to the bridge.
- Debug entry/exit, NCIO (non-cacheable I/O) fetch tracking, and vector conservative-mode bookkeeping are handled here before dispatch.

It owns an `eot` (end-of-test) helper and holds a reference to the shared `mcmi` instance so instruction and memory streams stay coordinated.

Files: `rvfi.h`, `rvfi.cpp`, `rvfi_plusargs.h`.

## mcmi/ — Memory Consistency Model Interface

Captures memory-ordering events used for memory checking. The `mcmi` class subscribes to the MCM transaction family: `m_mcmi_read`, `m_mcmi_insert`, `m_mcmi_bypass`, `m_mcmi_write`, `m_mcmi_ifetch_req/resp`, `m_mcmi_ievict`, `m_mcmi_devict`, `m_mcmi_flush`, `m_mcmi_writeback`, `m_mcmi_dfetch`.

Responsibilities:

- Decode read/insert/bypass/write/fetch/evict events into `mem_t`/`mem_cl_t` records and forward them to the bridge's `process_dut_mcm_*` handlers.
- Reconstruct split / non-consecutive accesses from a byte mask (`process_split_memory_accesses`, `process_memory_access`) so vector and masked accesses map back to contiguous ranges.
- Model atomics: `process_amo()` / `amo_modify_write_data()` / `amo_arithmetic()` compute the expected AMO result, and `sc_failed()` tracks store-conditional success/failure.
- Track in-flight ifetch requests, AMO writes, and SC results/bypasses, and coordinate NCIO fetch transitions with the RVFI stream.

Files: `mcmi.h`, `mcmi.cpp`.

## Shared data model

Both interfaces produce the structs defined in `cosim/bridge/bridge_if.h` (`rv_instr_t`, `mem_t`, `csr_t`, `rv_intr_t`, ...), which is the single contract the bridge consumes.
