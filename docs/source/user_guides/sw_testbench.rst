SW Testbench
============

The software testbench validates the behavior and performance of ``rv_tester`` without requiring a
full-chip simulation. It is implemented as a simple wrapper around ``rv_tester.sv`` with utilities
for mocking the chip's ``rvfi`` messages using a C++ DPI (see ``test/sw_testbench/dpi.cpp``).

Two harnesses are built, differing only in hart count: ``sw_1c`` (single hart) and ``sw_2c``
(dual hart).

Functional testing
------------------

The smoke suite runs each harness against a checked-in ELF (``testlists/infinite.elf``):

.. code-block:: sh

   bazel-7 test //test/sw_testbench/testlists:all_smoke --config=bzlmod

Individual tests are ``//test/sw_testbench/testlists:infinite_1c_verilator`` and
``:infinite_2c_verilator``. To build a harness binary without running it:

.. code-block:: sh

   bazel-7 build //test/sw_testbench/verilator:sw_2c_tb_verilator --config=bzlmod

Each smoke entry is an ``sh_test`` that invokes ``testlists/sim.sh`` with the pre-built harness
binary and a set of plusargs, and checks the simulator's stdout for error patterns.

To add new regressions:

1. Create an assembly / C++ source application under ``testlists/``.
2. Add an ``sh_test`` entry to ``testlists/BUILD.bazel``, passing the harness binary, the
   program to load, and the plusargs the test needs.
3. Add the new test to the ``all_smoke`` ``test_suite`` in the same file.
