Overview
========

RV_TESTER is the verification collateral that surrounds a RISC-V CPU core (the DUT — Design
Under Test) with everything needed to boot, stimulate, observe, and check it. As the core
executes, its retired instructions and memory-ordering events are checked
instruction-by-instruction against the `Whisper <https://github.com/tenstorrent/whisper>`_
RISC-V ISS, while its bus traffic is serviced by a software system model of the surrounding
platform. RV_TESTER is RVA23-compatible.

How the pieces fit together
---------------------------

.. figure:: /common/images/rv_tester.png
   :alt: RV_TESTER architecture
   :width: 100%

   RV_TESTER wraps the DUT in SystemVerilog interfaces (transactors, drivers, monitors) backed
   by C++ models: the system model and its device models service bus traffic, while the bridge
   drives the Whisper ISS and the Core Arch Checker.

The lockstep checking path on its own:

.. mermaid::

   flowchart LR
       DUT[RISC-V DUT]
       RVFI[RVFI Monitor]
       BR[Bridge]
       WH[Whisper ISS]
       CAC[Core Arch Checker]
       AXI[AXI transactor / master]
       SYS[Sysmod]
       DEV[Device models<br/>CLINT · HTIF · DM · TRICKBOX · ...]

       DUT -->|RVFI / MCM events| RVFI --> BR
       BR <-->|step / peek / poke| WH
       BR --> CAC
       DUT <-->|AXI| AXI --> SYS --> DEV

The main components are:

- **RVFI Monitor** — Samples signals from the RISC-V Formal Interface and passes them to the
  bridge.
- **Whisper ISS** — Steps the ISS on each instruction retire and reports CPU architectural
  state changes relative to the previous step.
- **Bridge** — Orchestrates collection of DUT vs ISS architectural state and forwards it to
  the Core Arch Checker.
- **CAC (Core Arch Checker)** — Compares DUT vs ISS architectural state and flags mismatches.
- **AXI SW (transactor)** — Receives requests from the RISC-V CPU AXI bus and creates C++
  transactions.
- **AXI MST SW** — Converts C++ transactions into SystemVerilog bus-level activity.
- **Sysmod** — System model that divides the address space per the memory map and routes
  requests to device models.
- **Devices (CLINT, TRICKBOX, HTIF, DM, ...)** — Model device-specific functionality.
- **Clocking & reset** — Per-domain clock generation, optional glitch-free clock-profile muxing
  for dynamic frequency switching, external-clock support, and cold/warm reset sequencing across
  clock domains.
- **Lifecycle & DPI bring-up** — Parses plusargs, seeds randomization, builds and tears down the
  C++ object registry, and orders DPI initialization against reset so the C++ side is ready
  before the core leaves reset.
- **Termination & rerun** — Aggregates termination sources (DUT, cosim, sysmod/HTIF, DMI timeout,
  errors), drives a graceful quiesce/drain handshake, prints PASS metrics, and supports rerunning
  or warm-resetting a test.
- **Performance & monitoring** — Periodic and end-of-test performance calculation, instruction
  counting, and clock monitoring.
- **Interrupts, triggers & PMU** — Generate interrupts and event triggers to the core and model
  its performance counters.

Dependencies
------------

RV_TESTER is not self-contained: several components shown above live in separate repositories
and are fetched by Bazel. They are declared in ``MODULE.bazel`` and
``bazel/external_deps.bzl``, with licenses recorded in ``NOTICE``.

.. list-table::
   :header-rows: 1
   :widths: 22 78

   * - Dependency
     - Role
   * - `Whisper <https://github.com/tenstorrent/whisper>`_
     - RISC-V ISS. The architectural reference model the DUT is checked against; stepped once
       per instruction retire by the bridge.
   * - `CoreArchChecker <https://github.com/tenstorrent/CoreArchChecker>`_
     - The **CAC**. Compares DUT and ISS architectural state and flags mismatches. Consumed as
       ``@CoreArchChecker//src:cac_core`` / ``:cac_lib`` by ``src/cosim/bridge/``.
   * - `cvm <https://github.com/tenstorrent/cvm>`_
     - Shared Tenstorrent verification-methodology library, used on both the C++ and
       SystemVerilog sides: plusarg parsing, logging, the object registry, randomization,
       bit-manipulation helpers, and the topology description. Also provides the
       ``topology_gen`` Bazel rule and the container image CI builds in.
   * - `mem-manager <https://github.com/tenstorrent/mem-manager>`_
     - Sparse memory backing store used by the sysmod device models (``mem/``, ``dm/``,
       ``trickbox/``, ``io_dev/``, ``mmr_txn_router/``).
   * - `pulp-platform AXI <https://github.com/pulp-platform/axi>`_
     - AXI SystemVerilog packages used by the AXI transactor and master.
   * - `nlohmann/json <https://github.com/nlohmann/json>`_
     - JSON parsing for the memory map, Whisper, and topology configuration files.
   * - ``rules_hdl`` / ``rules_verilator``
     - Bazel rule sets providing ``verilog_library`` and the Verilator build integration.

Where to go next
----------------

- :doc:`install` — Build and run rv_tester.
- :doc:`/user_guides/index` — In-depth guides to each subsystem.
- :doc:`/reference/index` — Repository layout and component map.
