# OpenC910

Run open-source [OpenC910](https://github.com/XUANTIE-RV/openc910) (XuanTie C910) RISC-V core under `rv_tester` on Verilator, **lockstep instruction-by-instruction against Whisper ISS**. Both core and `rv_tester` are Bazel dependencies; RVFI additions applied to the upstream RTL by `rtl/apply_rvfi.py` at fetch time.

## What This Contains

Integration glue; C910 and `rv_tester` are fetched by Bazel.

```
bazel/
  openc910_ext.bzl           fetch C910, overlay RVFI srcs, run apply_rvfi.py
  openc910.BUILD             BUILD overlay (upstream has no Bazel)
rtl/
  apply_rvfi.py              RVFI plumbing edits to upstream RTL (`ifdef RVFI)
  rvfi/ct_rvfi_gen.v         RVFI reconstruction (iid-keyed retire record)
dv/
  verilator_opts.bzl         C910 lint waivers on top of the shared VOPTS
dv/harness/
  openc910_test_harness.sv   C910 ↔ rv_tester shim
  top.sv                     rv_tester + harness, wired by name (.*)
dv/config/
  *.yml                      topology/hart/platform/AXI config
dv/verilator/                Verilator build
dv/testlists/                smoke tests (run under //test/examples/common/dv:sim.sh)
```

C910 config: **single-hart RV64GC**. (C910 module is dual-core; core1 held in reset; only core0 checked.)

## Connection

`top.sv` instantiates `rv_tester` and `openc910_test_harness` side-by-side, wired by name (`.*`):

```
                     openc910_test_harness.sv
 OpenC910 core0 ────► core0_rvfi_* export ──► rv_tester rvfi[] ──┐
        │  biu plain-AXI4 (40-bit addr, 128-bit data)           │
        ▼                                                       ▼
   rv_tester axi_req[0]/axi_rsp[0]            Whisper lockstep check
```

- **RVFI**: C910 exposes retire-valid + retire-PC only; `ct_rvfi_gen.v` + `apply_rvfi.py` edits reconstruct full per-instruction record (iid-keyed table: dispatch captures metadata, writeback fills result, retire reads out). Flattened `core0_rvfi_*` bus packed into `rvfi[]`.
- **AXI**: `biu` master (40-bit addr, 128-bit data, 8-bit id) → `axi_req[0]`/`axi_rsp[0]`.
- **Order tag**: Harness generates monotonic counter (C910 exports no retire order).

## RVFI Stages

C910 is 3-wide OoO with register renaming; RVFI reconstructed in `ct_rvfi_gen.v` (instantiated in `ct_core`, patched in via `ifdef RVFI`, non-RVFI builds byte-identical to upstream):

- **Stage A**: `valid`, `pc_rdata`, `pc_wdata`, `iid`, `mode`, `trap`, `cause`, `intr` from retirement signals + ROB retire exports.
- **Stage B**: `rd_addr`, `rd_we`, `rd_wdata` from dispatch (captures by ROB iid) and writeback (IU/LSU pipes). Instruction word checked in lockstep.
- **Stage C**: `mem_addr`, `mem_rmask`, `mem_wmask`, `mem_rdata` from LSU taps (load/store data). Store `mem_wdata` captured from `sd_ex1_data` (iid-keyed).
- **Stage D**: FP `frd_*` from VFPU pipe6/7 writeback (smoke ELFs built no-F, so untested in smoke suite).

## Build & Run

```bash
# Build Verilator model
bazel-7 build --config=bzlmod //test/examples/openc910/dv/verilator:openc910_tb_verilator

# Run smoke
bazel-7 test  --config=bzlmod //test/examples/openc910/dv/testlists:all_smoke --test_output=errors
```

Requirements: Bazel 7, cvm podman image, network access. See cva6 README for dependency list.

## Tests

**`//test/examples/openc910/dv/testlists:all_smoke`**:
- `infinite_openc910_verilator`: infinite loop, 8 instructions (`+max_instr=8`).
- `hello_world_openc910_verilator`: runs to HTIF `tohost` completion (~1.3M retirements in lockstep). **Passes** with RVFI Stages A–D wired and `insn_check` enabled; store `mem_wdata` approximate only.

## Status & Known Limitations

- **Smoke tests PASS** in Whisper lockstep. RVFI Stages A–D wired; `insn_check` enabled. Store `mem_wdata` approximate (not exercised by smoke suite).
- **Cold-boot edits** (both in `rtl/apply_rvfi.py`):
  - `mmu/rtl/sysmap.h`: remap PMA so `0x8000_0000` is cacheable-executable (fetch lookup on `PA[39:12]`; `BASE0=0x02000` boot, `BASE1=0x80000` MMIO, `BASE2=0x100000` DRAM+).
  - `cp0/rtl/ct_cp0_regs.v`: reset `mhcr.IE/DE` to 1 (I/D cache enabled at reset); C910 cannot fetch cacheable memory with icache off.
- **Interrupts** (PLIC/CLINT): tied off for smoke bring-up. C910 has internal CLINT fed by harness `sys_cnt`.
- **Runtime**: Verilated C910 slow (~40 min for `hello_world`); smoke tests default to `timeout = "eternal"`. Waveform dump opt-in via `sim.sh`'s `+dbg` (or `+dbg=<on>:<off>` for window); full-run VCD is multi-GB.

## Cracked `jal`/`jalr` Micro-ops

C910 cracks `jal`/`jalr` into two ROB entries at the same PC (two consecutive records in `h0_dut_rvfi.log`):

```
#69 ... 0x80001750 0040009f r ...1 ...80001754 CUSTOM_MICRO_OP  (link, last_uop=0)
#69 ... 0x80001750 efdff0ef r ...0 ...0        jal x1, .-0x104  (redirect, last_uop=1)
```

- **Link micro-op** (`0x0040009f`): computes `ra = pc + 4`. Word is C910-internal encoding (not decodable RISC-V; `inst[6:0]=0x1f` is reserved column, `inst[11:7]=x1`); not a bug.
- **Redirect micro-op**: carries real jump word, performs control transfer.

rv_tester coalesces on `last_uop`: link supplies `ra` write, redirect supplies opcode/jump. Registered as known custom op via `C910_PLUSARGS` in `dv/openc910/testlists/BUILD.bazel` (`+rvfi_custom_uop_opcodes=0x0040009f:CUSTOM_MICRO_OP`); skips `insn` byte-check for coalesced op; all other instructions checked normally.
