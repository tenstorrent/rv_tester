Repository Layout
=================

A component-by-component map of the ``rv_tester`` source tree. Most subsystems carry their own
``README`` in the repository; the :doc:`/user_guides/index` expand on the major ones.

``src/`` — Source
-----------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Path
     - Contents
   * - ``src/cosim/``
     - Co-simulation (lockstep) infrastructure: ``bridge/``, ``dut_if/`` (``rvfi/``, ``mcmi/``),
       ``whisper_if/``, ``utils/``. See :doc:`/user_guides/cosim`.
   * - ``src/sysmod/``
     - System model: address map plus device models (``mem/``, ``clint/``, ``aclint/``,
       ``aplic/``, ``dm/``, ``htif/``, ``trickbox/``, ``io_dev/``, ``mmr_txn_router/``,
       ``sep_entropy_fifo/``, ``heartbeat/``, ``null_dev/``). See :doc:`/user_guides/sysmod`.
   * - ``src/transactors/``
     - AXI transactor / master (``axi_sw/``): converts DUT AXI bus traffic to and from C++
       transactions.
   * - ``src/pmu/``
     - Performance Monitoring Unit: samples and checks the DUT's hardware performance counters.
       See :doc:`/user_guides/pmu`.
   * - ``src/interrupts/``
     - Interrupt generation to the core.
   * - ``src/triggers/``
     - Event trigger generation to the core.
   * - ``src/memdump/``
     - Memory dump utilities.
   * - ``src/common/``
     - Shared utilities, including the device address map (``device_address_map/``).

``test/`` — Tests
-----------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Path
     - Contents
   * - ``test/sw_testbench/``
     - Software testbench that validates rv_tester without a full-chip simulation.
       See :doc:`/user_guides/sw_testbench`.
   * - ``test/transactors/``
     - Transactor tests.
   * - ``test/rv_tester_delay_resp_tb/``
     - Delayed-response testbench.

Build & infrastructure
----------------------

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Path
     - Contents
   * - ``MODULE.bazel`` / ``BUILD.bazel`` / ``WORKSPACE``
     - Bazel (bzlmod) build configuration.
   * - ``infra/``
     - Shared Bazel dependency declarations (``infra/bazel/``).
   * - ``bazel/``
     - Bazel helper rules, external dependency declarations, and toolchain configuration.
   * - ``scripts/``
     - Support scripts (e.g. ``csr/``, ``preload_axi_llc/``).
   * - ``emu/``
     - Emulation support.
   * - ``docs/``
     - This documentation (Sphinx sources under ``docs/source/``, build wrapper
       ``docs/build.py``).
