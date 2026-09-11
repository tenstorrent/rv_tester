// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// Registration expands against cvm::static_topology, so it lives in its own
// per-topology translation unit (compiled by cosim_gen with the concrete
// topology in deps) rather than in the shared, topology-agnostic whisper_if
// library.
#include "cvm/registry.hpp"
#include "cvm/topology_defs.hpp"
#include "whisper_client.h"

REGISTRY_register(whisperClient<uint64_t>, TOP.PLATFORM.WHISPER_CLIENT, 0);
