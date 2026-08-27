Co-simulation (cosim)
=====================

Co-simulation (lockstep) infrastructure that compares a RISC-V DUT against the
`Whisper <https://github.com/tenstorrent/whisper>`_ RISC-V ISS on every instruction retire.
This is the **primary correctness check** for the core.

What it does
------------

As the DUT executes, it streams architectural events (retired instructions, register/CSR writes,
memory accesses, interrupts, exceptions, debug entry/exit) over the RISC-V Formal Interface
(RVFI). ``cosim`` feeds those same instructions to Whisper, collects the ISS architectural state,
and checks that the DUT and ISS agree. Any divergence is flagged as a mismatch.

.. code-block:: text

                  RVFI / MCM events                                       step / peek / poke
 DUT  ─────────►  cosim.sv  ─────────►  dut_if  ─────────►  bridge  ◄──────────────────────►  Whisper ISS
                  (SV -> C++ DPI)      (rvfi, mcmi)            │
                                                              ▼
                                                    Core Arch Checker (CAC)
                                                    DUT state == ISS state ?

Data flow
---------

1. The DUT retires an instruction and emits RVFI/MCM events.
2. ``dut_if`` packages those events and signals the ``bridge``.
3. The ``bridge`` steps the Whisper ISS (``whisper_if``) for the same instruction and gathers the
   resulting ISS architectural state.
4. The bridge submits DUT vs ISS state to the Core Arch Checker, which reports any mismatch.

Components
----------

``cosim.sv``
   SystemVerilog top that wires the DUT/RVFI signals into the DPI layer.

``dut_if/`` — DUT interface
   DUT-facing event capture. Both interfaces connect to the messenger by transaction type and
   forward into the same ``bridge`` instance.

   - ``rvfi/`` — Receives RVFI transactions (retire, regs, CSRs, interrupts, traps, debug) and
     forwards them into the bridge. The ``rvfi`` class assembles a complete retired instruction:
     ``make_instr()`` builds an ``rv_instr_t`` from an incoming transaction,
     ``append_uop_changes_to_instr()`` merges micro-op / cracked-instruction pieces so a single
     architectural instruction is presented as one unit, and ``send_instr()`` /
     ``send_instr_group()`` / ``send_csr()`` forward the assembled state to the bridge. Debug
     entry/exit, NCIO (non-cacheable I/O) fetch tracking, and vector conservative-mode bookkeeping
     are handled here before dispatch.
   - ``mcmi/`` — Memory Consistency Model interface: load/store/fetch ordering events (read,
     insert, bypass, write, evict) used for memory checks. The ``mcmi`` class decodes events into
     ``mem_t`` / ``mem_cl_t`` records, reconstructs split / non-consecutive accesses from a byte
     mask so vector and masked accesses map back to contiguous ranges, models atomics (computing
     the expected AMO result and tracking store-conditional success/failure), and tracks in-flight
     ifetch requests.

``whisper_if/``
   Wrapper around the Whisper ISS (``whisper_client``). Provides step, peek, poke, translate,
   page-table-walk, interrupt/NMI injection, and MCM hooks, exposed as remote procedure calls so
   other components can drive the ISS.

``bridge/``
   The orchestration core. See :ref:`bridge-orchestration` below.

``utils/``
   Shared helpers, including start-of-test (``sot/``) and end-of-test (``eot/``) handling and
   general utilities.

.. _bridge-orchestration:

The bridge
----------

The bridge sits between the DUT-facing interfaces (``dut_if/``) and the Whisper ISS
(``whisper_if/``), keeps the two in lockstep, and submits their architectural state to the Core
Arch Checker (CAC) for comparison.

Responsibilities
~~~~~~~~~~~~~~~~~

- **Step the ISS in lockstep with the DUT.** For each DUT-retired instruction it steps Whisper,
  collects the resulting state changes, and converts both sides into a common representation for
  comparison.
- **Compare architectural state.** PC, GPR/FPR/vector registers, privilege level, and memory
  accesses are diffed via a CAC core (``cac_``); CSRs are checked by **CSRAL**, the generated CSR
  model (see the CSRAL guide). Any divergence is reported as a mismatch.
- **Drive the ISS.** Wraps Whisper peek/poke/step/translate/page-table-walk and interrupt/NMI
  injection so DUT-observed events can be reflected into the ISS.
- **Model timing-sensitive behavior.** Handles interrupt and NMI delivery and deferral,
  MTIP/timer interrupts, IMSIC MSIs, debug-mode entry/exit, and address translation so the ISS
  matches the DUT's actual timing.
- **Check the memory consistency model (multi-core).** Forwards the DUT's memory-ordering events
  (read/insert/bypass/write/ifetch/ievict) into Whisper's MCM so loads/stores are validated
  against the RISC-V memory model across all harts, not just per-instruction architectural state.
- **Resynchronize on legitimate divergence.** Some hardware behavior (custom CSRs, MMIO reads from
  CLINT/HTIF/trickbox/UART, LR/SC, hypervisor-masked fields, opcode rewrites) cannot be predicted
  by the ISS. The bridge detects these cases and re-syncs the ISS to the DUT instead of flagging an
  error.

Pre-step / post-step hooks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A key pattern in ``bridge.cpp`` is the pre-step / post-step sequencing around each Whisper step.
Before stepping, the bridge may poke pending exceptions, LR/SC state, debug entry/exit, and
interrupts into the ISS; after stepping it checks NMI/interrupt/exception outcomes and handles
SATP writes. This ordering is what keeps interrupt and trap timing aligned between DUT and ISS.

CSRAL: the CSR model
~~~~~~~~~~~~~~~~~~~~~

CSR state, checking, masking, and save/restore live in CSRAL (``src/cosim/csral/``), a CSR model
whose tables (``csral_tables.hpp``) are generated by ``csr_param_gen.py`` from the project's CSR
spec plus a policy YAML (``scripts/csr/csral_defaults.yaml`` merged with the ``csral:`` section
of the project override; the project wins per key). Generation validates everything — a bad spec
or a typo'd policy fails the build, never a simulation.

**Mirrors and whisper access.** The model keeps a per-hart DUT mirror (what the DUT last wrote)
and ISS mirror (what whisper holds). Every whisper CSR poke/peek goes through the model: pokes
write through to the ISS mirror, peeks refresh it, so the two can never silently drift. Aliases
fan out automatically — ``ALIAS_OF`` for same-position views, ``field_aliases`` in the YAML for
shifted views like ``fcsr[7:5]`` ↔ ``frm[2:0]``.

**DUT-side updates.** Three entry points with distinct semantics:

- ``sw_write`` — retire-path writes: whisper's write mask plus registered legalize hooks
  (cross-CSR WARL the spec cannot express: PMP granularity read-back keyed on the NAPOT bit,
  Smstateen hierarchy ANDing, hgatp/PMM/srmcfg legal values), then a queued check.
- ``hw_update`` — csri implicit hardware updates: masked by whisper's poke mask, queued check.
- ``dut_force`` — hardware-forced side effects (e.g. the mideleg VS bits on a ``misa.H`` edge):
  raw mask, **no check queued** — the ISS is not expected to mirror these until its own change
  report arrives.

**Checking.** Updates queue their CSR; the bridge drains the queue each instruction group and
compares the mirrors under the effective mask (write mask minus fields whose masking condition
is inactive). Volatile CSRs are re-peeked live before a mismatch is reported; writes to
addresses the spec doesn't know are logged and never compared; after a bridge resynch the next
drain is silent. Per-CSR policy keys: ``check``, ``on_mismatch`` (``error``/``skip``/
``resynch_rd``), ``volatile`` (ISS side reads live), ``class: interrupt``, ``exists_if``
(whole-CSR existence gate — hypervisor CSRs behind ``misa.H`` have no updates, checks, or mask
peeks while the gate is off), ``may_not_exist``, and ``check_reset``. Policy names are exact or
fnmatch globs, never substrings; unknown names error in a project's ``csral:`` section and warn
in the shared defaults.

**Masking conditions.** Field masking comes from the spec's two-level ``MASKED_BY`` (CSR-level
names the masking register, field-level the gate field), giving conditions named
``<gate_csr>.<gate_field>``. Masked fields compare only while their condition is active; gates
may chain (``menvcfg`` → ``henvcfg``) and the graph must be a DAG.

**Save/restore.** For every enabled condition, the model snapshots the masked field bits at the
gate's on→off edge — the values the DUT hardware retains — and pokes them back into whisper at
the next off→on edge. The snapshot is sticky while the gate is off (masked bits are inaccessible
to software then), and a condition that has never gone inactive restores nothing. The enabled
set is the plusarg ``+csral_save_restore`` (comma list of condition names, default ``misa.H``;
empty disables all); ``+hyp_save_restore_en=0`` remains as an alias for removing ``misa.H``.
Delegation-view conditions (e.g. ``mideleg.* → sip``) are mask-only and cannot be enabled.

**Reset.** ``csr_init`` seeds both mirrors from the spec resets, then checks whisper against
them and adopts whisper's values — whisper is the authority for the run; the check exists to
catch spec-vs-whisper drift at time zero. Drift is an error by default
(``+csral_reset_check=warn|off`` to downgrade); CSRs whose reset encodes per-core configuration
opt out with ``check_reset: false`` (the shared defaults exempt ``misa``).

**Plusargs.** ``+csr_rd_check``/``+csr_wr_check`` gate the retire-write and hw-update check
classes; ``+cosim_resynch_csr`` adds runtime skip entries as exact spec names (unknown names are
startup errors); ``+csral_save_restore`` and ``+csral_reset_check`` as above.
