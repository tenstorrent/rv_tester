// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "interrupts.hpp"
#include "cvm/topology_defs.hpp"

REGISTRY_register(interrupts, INTERRUPTS, cvm::registry::all);
