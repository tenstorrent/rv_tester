# riescue_test_gen

[RiESCUE](https://github.com/tenstorrent/riescue) integration: RiescueD
(the directed-test framework) generates test ELFs **at Bazel build time**,
which then run on the example cores (cva6, openc910) under rv_tester
lockstep exactly like the prebuilt `//test/examples/common/testbins` ELFs.

## Quick Start

```bash
# Generate the ELF (runs riescued + RISC-V GCC + a whisper first pass):
bazel-7 build --config=bzlmod //test/examples/riescue_test_gen:riescue_smoke_elf

# Run it on the cores in lockstep vs Whisper:
bazel-7 test --config=bzlmod //test/examples/riescue_test_gen:riescue_smoke_cva6_verilator --test_output=errors
bazel-7 test --config=bzlmod //test/examples/riescue_test_gen:riescue_smoke_openc910_verilator --test_output=errors
bazel-7 test --config=bzlmod //test/examples/riescue_test_gen:all_smoke --test_output=errors
```

The generated artifacts land in `bazel-bin/test/examples/riescue_test_gen/`:
`riescue_smoke.elf`, `riescue_smoke.dis` (disassembly), and
`riescue_smoke_whisper.log` (the build-time ISS run).

To run an **arbitrary ELF** (riescue-generated or not) on a core without
declaring a test, use the ad-hoc runners in `//test/examples:BUILD.bazel`
(they are core utilities, independent of this package):

```bash
bazel-7 run --config=bzlmod //test/examples:run_openc910 -- path/to/any.elf
bazel-7 run --config=bzlmod //test/examples:run_cva6     -- path/to/any.elf +dbg
bazel-7 run --config=bzlmod //test/examples:run_cva6     -- +load=path/to/any.elf +dbg +max_instr=50000 +run_path=results/foo
```

The declared tests here and the root runners are built on the shared
`rv_tester_sim_test` / `rv_tester_sim_run` macros in
`//test/examples/common/dv:sim_test.bzl`, parameterized over (elf, tb, whisper config,
plusargs) — any package can instantiate them against any core testbench.

## How it works

```
tests/riescue_smoke.s ──┐
config/rv_tester_cpu_config.json ──┤   riescued (@riescue, py_binary)
                                   ├─► + riscv-none-elf-gcc (@riscv_none_elf_gcc)
                                   │   + whisper first pass (@whisper//:whisper)
                                   └─► riescue_smoke.elf  ──►  sh_test: sim.sh + <core>_tb_verilator
```

- **`bazel/riescue_ext.bzl`** fetches two repos: `@riescue` (pinned GitHub
  commit, BUILD overlay in `bazel/riescue.BUILD`) and `@riscv_none_elf_gcc`
  (xPack prebuilt bare-metal GCC — relocatable, runs in the cvm container and
  on glibc >= 2.28 hosts). RiescueD's pypi deps (`pyyaml`, `sortedcontainers`,
  `intervaltree`) come from the `riescue_pypi` pip hub
  (`bazel/requirements.txt`).
- **`:riescued`** is the py_binary entry point (`riescued_main.py` wraps
  `riescue.riescued:main`; the pypi deps are attached here because they are
  not resolvable from inside the `@riescue` repo mapping).
- **`:riescue_smoke_elf`** is a genrule running the full RiescueD flow with a
  fixed `--seed` (reproducible output). `--run_iss` makes whisper execute the
  test at build time, so a broken ELF fails the build, not the cosim.
- The sh_tests mirror the hello_world tests in `<example>/dv/testlists`
  (same `sim.sh`, memmap, plusargs).

## Core / config constraints baked in

- `config/rv_tester_cpu_config.json` mirrors `common/dv/memmap.json`
  (dram 2 GiB @ `0x8000_0000`, htif @ `0x7000_0000`, clint @ `0x200_0000`);
  RiescueD links `tohost`/`fromhost` into the htif region and the ELF at the
  `0x8000_0000` reset vector, and ends the test through the standard HTIF
  `tohost` write (`+eot=tohost`).
- Features are the **RV64IMAC_zicsr** intersection of both cores. `f`/`d`
  must stay `supported: false` (not just disabled): RiescueD's loader
  initializes FP registers whenever FP is *supported*, and the cva6 harness
  ties off rv_tester `frd_*` (see the hello_world note in
  `cva6/dv/testlists/BUILD.bazel`).
- The genrule passes `--compiler_march rv64imac_zicsr_zicntr_zifencei`
  explicitly (riescued's default march includes extensions gcc 14 rejects)
  plus `--compiler_opts=-mabi=lp64` (xPack gcc defaults to rv32 ABI), and
  `--no_random_csr_reads` (the scheduler otherwise reads random
  `mhpmcounter*`/`mhpmevent*` CSRs that neither the cores nor the cosim
  whisper config implement).
- **Per-core whisper configs** (`:cva6_whisper_json`,
  `:openc910_whisper_json`): riescue's boot code reads `misa` and `mstatus`,
  which hello_world never does, and the shared `common/dv/whisper.json`
  diverges from the DUTs there: its `"isa"` string lacks `su` (whisper then
  disables U-mode and legalizes `mstatus.MPP` 0 -> 3); cva6's misa also sets
  B; openc910's misa also sets X and its mstatus resets with MPP=3, SPP=1.
  The BUILD genrules derive corrected per-core configs from the shared file
  so it stays the single source of truth.
- **`bazel/riescue_eot_sd_tohost.patch`**: riescue ends little-endian tests
  with a 32-bit `sw` to the 64-bit `tohost` HTIF register; the rv_tester htif
  device deserializes the full 64-bit write beat, so the mirrored upper AXI
  lanes corrupt the pass/fail payload. The patch makes riescue use `sd`
  (which its big-endian path already does).

## Bug-reproduction tests (tagged `manual`)

`tests/openc910_issue_69.s` targets
[openc910 issue #69](https://github.com/XUANTIE-RV/openc910/issues/69): C910
registers the retire increment (`cnt_adder_ff` in `ct_hpcp_cnt.v`) and applies
it one cycle *after* an explicit counter CSR write, so
`csrw minstret, zero; nop; csrr minstret` reads 2 where the priv spec (and
Whisper, and cva6) say 1. **Expected to FAIL on openc910** until the RTL is
fixed — it is tagged `manual` so suites and wildcards stay green:

```bash
bazel-7 test --config=bzlmod //test/examples/riescue_test_gen:openc910_issue_69_openc910_verilator --test_arg=+nometrics
bazel-7 test --config=bzlmod //test/examples/riescue_test_gen:openc910_issue_69_cva6_verilator      # control: PASSES
```

The failure fires twice: the lockstep compare flags the `csrr` rd
(`Core Arch Checker Mismatch - X5 - csr:minstret DUT: 0x2 ISS: 0x1`) and the
test's own `bne` self-check writes the HTIF fail code, so it also fails when
run standalone outside lockstep.

## Adding a test

1. Add `tests/<name>.s` (RiescueD format; see the [RiescueD user
   guide](https://docs.tenstorrent.com/riescue/user_guides/riescued_user_guide.html)
   and `@riescue//riescue/dtest_framework/tests/tutorials`). Keep
   `;#test.priv machine`, `;#test.paging disable`, and RV64IMAC instructions
   unless you extend the cpuconfig accordingly.
2. Instantiate `riescued_elf(name = "<name>", testfile = "tests/<name>.s")`
   (see `bazel/riescued_elf.bzl`) and clone the sh_tests for the new name.
3. Randomization: RiescueD randomizes from `--seed`; the fixed seed keeps
   Bazel outputs reproducible. For a seed sweep, wrap the genrule in a macro
   that stamps different seeds into different output names.
