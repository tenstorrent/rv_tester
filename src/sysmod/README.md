# SYSMOD

The **system model**: a software model of the platform surrounding the RISC-V core. It owns the global address map, routes bus transactions to the right device model, and advances time-based devices (timers, interrupts, debug).

## What it does

The DUT's AXI bus traffic arrives (via the transactors) as C++ read/write transactions. `sysmod` looks up the target address in the memory map and dispatches the request to the device that owns that range. Devices model the behavior of real platform peripherals — memory, timers, interrupt controllers, the debug module, host interface, etc. — so the core sees a realistic system without needing the full RTL of those blocks.

```
 DUT AXI ──► transactor ──► sysmod (addr decode) ──► device model
                                                       (mem, clint, htif, ...)
```

## Core files

- `sysmod.sv` — SystemVerilog top: generates periodic "tick" events, exports DPI functions for timer/software interrupts, DMI writes and termination, and drives interrupt lines back to the DUT.
- `sysmod.{h,cpp}` — C++ engine. Builds the device list from the memory map, routes `read`/`write`/backdoor accesses to `dev(addr)`, ticks devices, loads boot/firmware/program images, and handles reset and termination.
- `device.{h,cpp}` — Base class for all device models. Defines the address range a device owns (`has_addr`), the read/write callbacks, backdoor access, tick/reset hooks and snapshot save/restore.
- `io_device.h`, `pm_common.h`, `sysmod_params.hpp`, `sysmod_plusargs.h`, `sysmod_rpc.h` — Shared types, parameters, plusargs and RPC definitions.

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

## Build

`sysmod.bzl` defines `sysmod_gen`, which builds the SystemVerilog library and the DPI C++ library, pulling in each device model as a dependency.
