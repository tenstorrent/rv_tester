# SYSMOD

The **system model**: a software model of the platform surrounding the RISC-V core. It owns the global address map, routes bus transactions to the right device model, and advances time-based devices (timers, interrupts, debug).

## What it does

The DUT's AXI bus traffic arrives (via the transactors) as C++ read/write transactions. `sysmod` looks up the target address in the memory map and dispatches the request to the device that owns that range. Devices model the behavior of real platform peripherals — memory, timers, interrupt controllers, the debug module, host interface, etc. — so the core sees a realistic system without needing the full RTL of those blocks.

```
                                                          ┌─────────────────── sysmod ───────────────────┐
                                                          │                     ┌──► mem                 │
  ____  _   _ _____                                       │                     ├──► clint / aclint      │
 |  _ \| | | |_   _|  ──AXI──►  transactor  ──C++ txn──►  │  address decode ────┼──► aplic               │
 | | | | | | | | |    ◄───────   (rd/wr)   ◄───────────   │     (dev(addr))     ├──► htif                │
 | |_| | |_| | | |                                        │                     ├──► dm                  │
 |____/ \___/  |_|    ◄─────── interrupt lines ◄───────   ┤                     ├──► trickbox            │
                                                          │                     └──► ...                 │
                                                          │                                              │
                                                          │  tick() ──► timers / interrupts              │
                                                          │                                              │
                                                          └──────────────────────────────────────────────┘
```

## Device models

| Directory | Models |
|-----------|--------|
| `mem/` | Main memory backing store |
| `clint/` | Core-Local Interruptor (mtime/mtimecmp timer + software interrupts) |
| `aclint/` | Advanced CLINT timer/IPI device |
| `aplic/` | Advanced Platform-Level Interrupt Controller |
| `dm/` | RISC-V Debug Module (halt/resume, DMI access) |
| `htif/` | Host Target Interface (console + test termination) |
| `trickbox/` | Test backdoor "magic" device: interrupt injection, DMA, event triggers, microcode/RAS/IO-coherency helpers |
| `io_dev/` | Generic memory-mapped I/O device |
| `mmr_txn_router/` | Routes memory-mapped register transactions |
| `sep_entropy_fifo/` | Entropy source FIFO model |
| `heartbeat/` | Periodic liveness/progress indicator |
| `null_dev/` | Fallback device for unmapped addresses |
