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

## Project-defined devices

A project can add its own device model without modifying `sysmod`. The device derives from `device` (see `device.h`), and is handed to `sysmod` through the `sysmod_add_device` RPC declared in `sysmod_rpc.h`. The RPC is registered in the `sysmod` constructor, so it may be called from any registry component's `configure()` regardless of registration order. Calling it from a constructor is not supported.

1. Reserve the address range in `memmap.json` with the type `external`. The crossbar address rules are generated from the memmap, so an unreserved range never reaches `sysmod`.

```json
{ "base": "0x91000000", "size": "0x1000", "type": "external", "tag": "scratch0" }
```

2. Construct the device and register it from the project component's `configure()`:

```cpp
#include "sysmod/sysmod_rpc.h"

void my_agent::configure() {
  auto sysmod_loc = cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0);
  auto dev = std::make_shared<scratch_dev>("scratch0", 0x91000000, 0x1000, sysmod_loc);
  cvm::registry::messenger.call<sysmod_add_device>(sysmod_loc, dev);
}
```

`sysmod` calls `configure()` on the device, then includes it in address decode, tag lookup, and all `tick` variants alongside the memmap-composed devices. A device with `size` 0 is tick-only and needs no memmap entry. Tags must be unique across all devices. Construct a fresh device object on every `configure()` call; re-registering an object that survived a registry rebuild would connect its handlers twice. The project library depends on `@rv_tester//src/sysmod:device`, `@rv_tester//src/sysmod:sysmod_params`, and `@rv_tester//src/common:common`.
