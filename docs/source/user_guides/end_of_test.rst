End of Test
===========

``rv_tester`` ends a test in a fixed sequence. Each step waits for a signal from the side that
knows when it is done. There is one counter, ``quiesce_counter``, which counts TB clocks since
``terminate`` went high, and two timeouts as safety nets.

Sequence
--------

1. **An end condition fires.** Sources are the HTIF ``tohost`` write, the cosim end-of-test check,
   the DMI poll timeout, an ``rv_tester`` error, or ``dut_terminate`` from the harness.
   ``terminate`` goes high and stays high. In a UVM build it also requires ``uvm_done``, so the
   UVM test controls when termination may start.
2. **The harness drains the DUT.** ``terminate`` is an output. The harness stops feeding the DUT
   and asserts ``quiesced`` when its queues are empty. If ``quiesced`` does not come within
   ``+quiesce_timeout`` clocks, ``rv_tester`` prints an error and continues anyway.
3. **``cvm_done`` goes high** once ``quiesced`` is seen and at least ``CVM_DONE_DELAY_CYCLES``
   clocks have passed since ``terminate``. It stays high until the next ``rv_tester_reset``.
   In a UVM build the test waits on it, then drops its run-phase objection so the extract,
   check, report and final phases can run.
4. **UVM acknowledges.** The UVM test sets ``uvm_final_done`` in its ``final_phase``. If the
   acknowledge does not come within ``+uvm_final_timeout`` clocks of ``cvm_done``, ``rv_tester``
   prints an error and continues anyway. A build without UVM skips this step.
5. **``terminate_now`` fires.** ``rv_tester`` shuts the C++ registry down, retrying every clock
   until every component reports ``shutdown_ready()``, puts the DUT in reset, prints the
   ``INFO_PASS`` metrics, and calls ``$finish`` unless ``+terminate_call_finish=0``.

``dut_terminate``, ``warm_reset_now`` and ``unconditional_terminate`` bypass steps 2 to 4.

Signals
-------

.. list-table::
   :header-rows: 1
   :widths: 22 12 66

   * - Signal
     - Direction
     - Meaning
   * - ``terminate``
     - out
     - End condition seen. Latched until ``rv_tester_reset``.
   * - ``quiesced``
     - in
     - Harness reports the DUT idle. Tie to ``1`` if the harness has nothing to drain.
   * - ``cvm_done``
     - out
     - DUT idle and margin elapsed. Request to the UVM side.
   * - ``uvm_done``
     - in
     - UVM run phase complete. Gates ``terminate``. UVM builds only.
   * - ``uvm_final_done``
     - in
     - UVM final phase complete. Acknowledge of ``cvm_done``. UVM builds only.
   * - ``terminate_now``
     - out
     - Shutdown in progress.
   * - ``terminated``
     - out
     - Shutdown complete.

Knobs
-----

.. list-table::
   :header-rows: 1
   :widths: 32 12 56

   * - Knob
     - Default
     - Effect
   * - ``CVM_DONE_DELAY_CYCLES`` (parameter)
     - 500
     - Minimum clocks between ``terminate`` and ``cvm_done``. Set to ``0`` to follow ``quiesced`` alone.
   * - ``+quiesce_timeout``
     - 600
     - Clocks to wait for ``quiesced``. ``0`` waits without limit.
   * - ``+uvm_final_timeout``
     - 1000
     - Clocks to wait for ``uvm_final_done`` after ``cvm_done``. ``0`` waits without limit.
   * - ``+terminate_call_finish``
     - 1
     - Call ``$finish`` at the end of the sequence.

UVM integration
---------------

The UVM side needs three connections. The pattern below is what the core and cluster testbenches
in ``riscv_cluster`` use through their ``msg_intf``.

.. code-block:: systemverilog

   // top.sv
   assign uvm_done       = i_msg_intf.uvm_run_phase_completed;
   assign uvm_final_done = i_msg_intf.uvm_final_phase_completed;
   assign i_msg_intf.cvm_run_phase_completed = cvm_done;

   // base test
   virtual task run_phase(uvm_phase phase);
     i_msg_intf.uvm_run_phase_completed   = '0;
     i_msg_intf.uvm_final_phase_completed = '0;
     phase.raise_objection(this);
     i_msg_intf.uvm_run_phase_completed = '1;
     wait (i_msg_intf.cvm_run_phase_completed === '1);
     phase.drop_objection(this);
   endtask

   virtual function void final_phase(uvm_phase phase);
     super.final_phase(phase);
     i_msg_intf.uvm_final_phase_completed = '1;
   endfunction

Set ``uvm_top.finish_on_completion = 0`` so that ``rv_tester`` owns ``$finish``. The UVM report
summary prints in the same time step as ``final_phase``, one TB clock before ``rv_tester`` samples
the acknowledge, so it is always in the log.

A harness built without ``UVM_MACROS_SVH`` has neither ``uvm_done`` nor ``uvm_final_done``, and
``terminate_now`` follows ``quiesced`` directly.
