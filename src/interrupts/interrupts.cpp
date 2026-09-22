// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "interrupts.hpp"

REGISTRY_register(interrupts, INTERRUPTS, cvm::registry::all);

bool validate_interrupt_injection_rand_delay_min(const char* flagname, const int value) {
  if (value <= 0) {
    cvm::log(cvm::ERROR, "Error: Invalid value for +{}={}, must be >= 1, as we currently don't support injecting multiple interrupts in a single cycle\n", flagname, value);
    return false;
  }
  return true;
}

DEFINE_bool(interrupt_injection_enable, false, "Enable event based interrupt injection sequences in the sim");
DEFINE_int32(interrupt_injection_count, 1, "Number of interrupt injections per trigger event");
DEFINE_int32(interrupt_injection_rand_delay_min, 1, "min TB cycle interval between injected interrupts");
DEFINE_validator(interrupt_injection_rand_delay_min, &validate_interrupt_injection_rand_delay_min);
DEFINE_int32(interrupt_injection_rand_delay_max, 16, "max TB cycle interval between injected interrupts");
DEFINE_string(interrupt_injection_initial_delay, "0:0", "Initial delay range (min:max) after which interrupt trigger starts");
DEFINE_int32(interrupt_injection_event_mask, 0, "Bitmask to enable specific uarch event triggers");
DEFINE_string(interrupt_injection_event_names, "", "Comma-separated list of uarch event names to trigger interrupts");
DEFINE_string(interrupt_injection_label, "", "Label to trigger interrupt");
DEFINE_string(interrupt_injection_pc, "", "Comma-separated list of PC addresses to trigger interrupts");
