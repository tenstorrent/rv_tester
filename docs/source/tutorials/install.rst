Installation & Build
====================

RV_TESTER builds with `Bazel <https://bazel.build/>`_ 7 in bzlmod mode.

Build and test
--------------

Every invocation must pass ``--config=bzlmod`` (see ``.bazelrc``). Without it Bazel falls back
to the legacy WORKSPACE path, which does not wire up all dependencies (e.g. ``@rules_verilator``)
and fails to load.

.. code-block:: sh

   # Build everything
   bazel-7 build //... --config=bzlmod

   # Build and run the tests
   bazel-7 test //test/... --config=bzlmod

These mirror CI: the ``smoke`` job in ``.github/workflows/ci.yml`` runs
``bazel-7 test //test/... --config=bzlmod --build_tests_only``, so a green
``bazel-7 test //test/...`` locally reproduces the CI smoke result. CI also runs the same
recipe under ``asan+ubsan`` and ``tsan``.

Downstream consumers still on Bazel 6 + WORKSPACE build a narrower target set without
``--config=bzlmod``:

.. code-block:: sh

   bazel build //src:all \
     //test/sw_testbench:sw_1c_rv_tester_sv \
     //test/sw_testbench:sw_2c_rv_tester_sv

Nested ``README`` files under ``src/`` (e.g. ``src/cosim/``, ``src/sysmod/``) document
individual subsystems in more detail; those are summarized in the :doc:`/user_guides/index`.

Running the SW testbench
------------------------

The software testbench validates rv_tester without a full-chip simulation. The smoke suite
builds the Verilator harness and runs it against a checked-in ELF:

.. code-block:: sh

   bazel-7 test //test/sw_testbench/testlists:all_smoke --config=bzlmod

To build a harness binary on its own — ``sw_1c`` (single hart) or ``sw_2c`` (dual hart):

.. code-block:: sh

   bazel-7 build //test/sw_testbench/verilator:sw_2c_tb_verilator --config=bzlmod

See :doc:`/user_guides/sw_testbench` for details.

Contributing
------------

Contributions are welcome. Bug reports and feature requests are handled via
`GitHub Issues <https://github.com/tenstorrent/rv_tester/issues>`_, and changes are submitted via
pull requests (reviewed weekly). See ``CONTRIBUTING.md`` for the full build/test and contribution
standards, including ``clang-format``/``clang-tidy`` formatting and SPDX/REUSE header requirements.

License
-------

- ``LICENSE`` (Apache-2.0) — Overall license for the project, except where specified.
- ``LICENSE-DOCS`` (CC-BY-4.0) — License for all documentation and images only.
- ``LICENSE_understanding.txt`` — Tenstorrent's clarification of how the Apache-2.0 license
  applies to this repository.

This repository is `REUSE <https://reuse.software>`_ compliant; per-file license and copyright
information is provided via inline SPDX headers and ``REUSE.toml``.
