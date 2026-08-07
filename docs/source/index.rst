.. Public documentation main file

RV_TESTER Documentation
===================================================


.. toctree::
   :maxdepth: 2
   :caption: Contents:
   :hidden:

   /tutorials/index
   /user_guides/index
   /reference/index


**RV_TESTER** is a RISC-V CPU testing component written in SystemVerilog and C++.

It provides the verification collateral needed to interface with a RISC-V CPU core and
perform lockstep architectural checks against the `Whisper <https://github.com/tenstorrent/whisper>`_
RISC-V instruction set simulator (ISS). Along with the co-simulation flow, it provides soft
device models, an AXI transactor/master, performance-counter (PMU) modeling, interrupt and
trigger generation, and the platform glue that ties it all together. RV_TESTER is
RVA23-compatible.

The DUT (the RISC-V core under test) is surrounded with everything needed to boot, stimulate,
observe, and check the core. As the core executes, its retired instructions and memory-ordering
events are checked instruction-by-instruction against Whisper, while its bus traffic is serviced
by a software system model of the surrounding platform.


Main components
---------------

- :doc:`Co-simulation (cosim) </user_guides/cosim>` — Lockstep architectural checking of the DUT against the Whisper ISS.
- :doc:`System model (sysmod) </user_guides/sysmod>` — Software model of the platform: address map and device models.
- :doc:`Performance monitoring (pmu) </user_guides/pmu>` — Samples and checks the DUT's hardware performance counters.
- :doc:`SW testbench </user_guides/sw_testbench>` — Validates rv_tester without a full-chip simulation.


Getting Started
---------------

New to RV_TESTER? Start with:

- :doc:`Overview </tutorials/overview>` — How the pieces fit together and what happens on every instruction retire.
- :doc:`Installation & build </tutorials/install>` — Build and test rv_tester with Bazel.

Reference Documentation
------------------------

The :doc:`reference documentation </reference/index>` covers the repository layout and a
component-by-component map of the source tree.
