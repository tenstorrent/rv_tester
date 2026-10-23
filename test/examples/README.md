# rv_tester Examples

Integration examples: open-source RISC-V cores under the `rv_tester` testbench
on Verilator, checked **instruction by instruction against the Whisper ISS**.
Each example is one package tree under `test/examples/`.


## Examples

| Example | Core | Details |
|---|---|---|
| [`cva6/`](cva6/) | [CVA6](https://github.com/openhwgroup/cva6) `cv64a6_imafdc_sv39` | See [`cva6/docs/README.md`](cva6/docs/README.md) |
| [`openc910/`](openc910/) | [OpenC910](https://github.com/XUANTIE-RV/openc910) RV64GC | See [`openc910/docs/README.md`](openc910/docs/README.md) |

Test stimulus beyond the prebuilt testbins comes from
[`riescue_test_gen/`](riescue_test_gen/): [RiescueD](https://github.com/tenstorrent/riescue)
generates test ELFs at build time that run on either core, see
[`riescue_test_gen/docs/README.md`](riescue_test_gen/docs/README.md).

## Layout

Everything shared lives in [`common/`](common/), referenced as
`//test/examples/common/...`: the test runner (`dv/sim.sh`), the (elf, tb)
parameterized test/run wrappers (`dv/sim_test.bzl`, `dv/sim_run.sh`),
`dv/gflags.cpp`, the example-specific Verilator defines
(`dv/verilator_opts.bzl`, on top of the repo defaults in `//test:verilator.bzl`),
testbins, `dv/memmap.json` and `dv/whisper.json`.

```
BUILD.bazel                   # examples_smoke suite + ad-hoc run_<core> runners
common/                       # shared code & assets: sim.sh, gflags, verilator opts, testbins, configs
riescue_test_gen/             # RiescueD-generated test ELFs + run targets for both cores
<example>/
├── bazel/                    # core fetch extension + BUILD overlay
├── rtl/                      # RTL modifications
├── dv/
│   ├── BUILD.bazel           # codegen targets
│   ├── harness/              # top.sv, test_harness.sv, defines
│   ├── config/               # *.yml (topology, hart, platform, AXI)
│   ├── verilator/            # Verilator build
│   └── testlists/            # smoke tests
└── docs/                     # README
```

## Adding a New Example

1. Copy an existing example as `newcore/`.
2. Write `newcore/bazel/newcore_ext.bzl` to fetch the upstream core, and
   register it in the repo-root `MODULE.bazel` next to `cva6_ext`.
3. Add RTL patches in `newcore/bazel/` and `newcore/rtl/`.
4. Fill `newcore/dv/{harness,config}/` with harness and YAML config.
5. Reuse shared assets via `//test/examples/common/...`.
6. Add the smoke suite to `examples_smoke` in [`BUILD.bazel`](BUILD.bazel);
   the CI smoke job runs it as part of `//test/...`.

## Quick Start

All commands run inside the cvm container, like the rest of the repo (see the
top-level [README](../../README.md)).

```bash
bazel-7 build --config=bzlmod //test/examples/cva6/dv/verilator:cva6_tb_verilator
bazel-7 test  --config=bzlmod //test/examples/cva6/dv/testlists:all_smoke --test_output=errors

# Every example smoke suite, as CI runs it:
bazel-7 test  --config=bzlmod //test/examples:examples_smoke --test_output=errors
```

### Running an arbitrary ELF

[`BUILD.bazel`](BUILD.bazel) declares one ad-hoc runner per core,
`//test/examples:run_cva6` and `//test/examples:run_openc910`. The ELF goes
after `--`, as a bare path or `+load=path/to/any.elf`, followed by any extra
`+plusargs`. Paths are resolved against the invocation directory.

```bash
bazel-7 run --config=bzlmod //test/examples:run_cva6     -- path/to/any.elf +max_instr=50000
bazel-7 run --config=bzlmod //test/examples:run_openc910 -- +load=path/to/any.elf +nometrics
```

### Extra simulator arguments

Pass `+plusargs` to a test target with `--test_arg`:

```bash
bazel-7 test --config=bzlmod //test/examples/cva6/dv/testlists:hello_world_cva6_verilator \
  --test_arg=+dbg=1000:5000 --test_arg=+save_all_files
```

`sim.sh` consumes `+save_all_files` (archive the run on a clean pass),
`+dbg[=start:end]` (VCD dump, optional cycle window) and
`+run_path=<dir>` (run the simulation in `<dir>`); everything else is
forwarded to the simulator.

### Where the output goes

The simulator writes its logs (`h0_dut_rvfi.log`, `iss_cosim.log`, ...), dumps
and `run.log` into its working directory. For a test that is the sandbox; on
failure or with `+save_all_files` the files are copied to
`bazel-testlogs/test/examples/<path>/test.outputs/`. For a `run` target pass
`+run_path=<dir>` to collect them elsewhere:

```bash
bazel-7 run --config=bzlmod //test/examples:run_cva6 -- path/to/foo.elf +dbg +run_path=results/foo
```

See the example's `docs/README.md` for full details.
