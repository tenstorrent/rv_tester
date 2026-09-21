# rv_tester SW Testbench

This testbench serves to validate the behavior and performance of `rv_tester` without requiring a full-chip simulation. It is implemented as a simple wrapper around `rv_tester.sv` with utilities for mocking the chip's `rvfi` messages using a C++ DPI (see [dpi.cpp](dpi.cpp)).

Two harnesses are built, differing only in hart count: `sw_1c` (single hart) and `sw_2c` (dual hart).

## Functional Testing

The smoke suite runs each harness against a checked-in ELF (`testlists/infinite.elf`):

```
bazel-7 test //test/sw_testbench/testlists:all_smoke --config=bzlmod
```

Individual tests are `//test/sw_testbench/testlists:infinite_1c_verilator` and `:infinite_2c_verilator`. To build a harness binary without running it:

```
bazel-7 build //test/sw_testbench/verilator:sw_2c_tb_verilator --config=bzlmod
```

Each smoke entry is an `sh_test` that invokes [sim.sh](testlists/sim.sh) with the pre-built harness binary and a set of plusargs, and checks the simulator's stdout for error patterns.

To add new regressions, create an assembly/C++ source application under `testlists/`, add an `sh_test` entry for it in [testlists/BUILD.bazel](testlists/BUILD.bazel), and add that test to the `all_smoke` `test_suite` in the same file.

> **Note:** `testlists/smoke.py` is vestigial. It emits `cc_test_generator` / `risc_p_cores_test_executor` directives for the internal `bzsim` testgen flow, which is not part of this repository; the rules in `testlists/BUILD.bazel` are hand-written replacements. Adding entries to `smoke.py` has no effect.
