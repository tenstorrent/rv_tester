# CVA6

Run open-source [CVA6](https://github.com/openhwgroup/cva6) RISC-V core under `rv_tester` on Verilator, **lockstep instruction-by-instruction against Whisper ISS**. The core is a Bazel dependency fetched by `cva6_ext`; no submodules.

## What This Contains

Integration glue only; CVA6 is a dependency, `rv_tester` is this tree.

```
bazel/
  cva6_ext.bzl               fetch CVA6 + submodules
  cva6.BUILD                 BUILD overlay (upstream has no Bazel)
dv/
  verilator_opts.bzl         CVA6 lint waivers on top of the shared VOPTS
dv/harness/
  cva6_test_harness.sv       CVA6 ↔ rv_tester shim
  top.sv                     rv_tester + harness, wired by name (.*)
dv/config/
  *.yml                      topology/hart/platform/AXI config
dv/verilator/                Verilator build
dv/testlists/                smoke tests (run under //test/examples/common/dv:sim.sh)
```

CVA6 config: **`cv64a6_imafdc_sv39`** (RV64IMAFDC, sv39 MMU, write-through cache).

## Connection

`top.sv` instantiates `rv_tester` and `cva6_test_harness` side-by-side, wired by name (`.*`):

```
                          cva6_test_harness.sv
 CVA6 ariane ────────────► rvfi_probes_o ──► [cva6_rvfi] ──► rvfi[] ──┐
        │  noc_req/noc_resp (ariane_axi)                              │
        ▼                                                             ▼
   rv_tester axi_req[0]/axi_rsp[0]            Whisper lockstep check
```

- **RVFI**: CVA6 emits `rvfi_probes_o`; `cva6_rvfi` expands to retired-instruction stream, harness remaps to `rvfi[]`.
- **AXI**: `ariane_axi` NoC master → `axi_req[0]`/`axi_rsp[0]`.
- **Order tag**: CVA6's RVFI has no `order` field; harness generates monotonic counter (same as rv_tester software testbench).

## Requirements

| Item | Detail |
|---|---|
| **Bazel 7** | `bazel-7` in cvm image; system `bazel` 6.5 will not work. |
| **Podman + cvm** | `aus-gitlab.local.tenstorrent.com:5005/riscv/dv/cvm:0.1.3` (or `ghcr.io/tenstorrent/cvm:0.1.3`). |
| **Network** | aus-gitlab, github.com, bcr.bazel.build. |
| **Disk** | Output on regression area; first build is large. |

Bazel pulls: `CVA6` (openhwgroup + submodules) plus the rv_tester dependencies (`whisper`, `cvm`, CoreArchChecker, mem_manager, rules_verilator, verilator 5.046, boost, zlib, etc.), all pinned in the repo-root `MODULE.bazel`.

## Build & Run

```bash
# Build Verilator model
bazel-7 build --config=bzlmod //test/examples/cva6/dv/verilator:cva6_tb_verilator

# Run smoke
bazel-7 test  --config=bzlmod //test/examples/cva6/dv/testlists:all_smoke --test_output=errors
```

## Tests

All tests run `cva6_tb_verilator` under the shared `common/dv/sim.sh`, checking each retired instruction against Whisper in lockstep.

**`//test/examples/cva6/dv/testlists:all_smoke`** (CI suite, passes):
- `infinite_cva6_verilator`: `infinite.elf`, runs 8 instructions (`+max_instr=8`).
- `hello_world_cva6_verilator`: `hello_world.elf`, runs to HTIF `tohost` completion. ELF built `rv64ima_zicsr` (no F) to avoid FP-write limitation below. **Passes** in lockstep.

**Artifacts** (on failure or with `--test_arg=+save_all_files`, under `test.outputs/`):
- `h0_dut_rvfi.log`, `trace_hart_0.dasm`, `h0_bridge.log`, `iss_cmd.log`, `iss_cosim.log`, `whisper_cosim.json`, `run.log`.

## Known Limitations

- **FP register writes unmapped**: Harness ties off `frd_valid`/`frd_addr`/`frd_wdata`. CVA6's RVFI has no FP-write flag, so FP programs fail CoreArchChecker check. Fix requires decoding FP opcodes in harness or exposing CVA6's `is_rd_fpr`. `hello_world` avoids this by building without F extension.
- **RVFI `order`/`pc_wdata` testbench-supplied**: CVA6's RVFI leaves these unpopulated (built for Spike-tandem).

## CI

`all_smoke` is part of `//test/examples:examples_smoke`, which the rv_tester
`smoke` job runs with the rest of `//test/...` (`.github/workflows/ci.yml`).
