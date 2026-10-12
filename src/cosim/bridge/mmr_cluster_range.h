// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

namespace mmr_cluster {

template <typename BaseFn>
bool any_cluster_half_open(uint64_t pa, uint32_t nclusters, BaseFn base_of, uint64_t size) {
  for (uint32_t cluster_id = 0; cluster_id < nclusters; ++cluster_id) {
    const uint64_t base = base_of(cluster_id);
    if (pa >= base && pa < base + size)
      return true;
  }
  return false;
}

template <typename BaseFn>
bool any_cluster_sc_slice(uint64_t pa, uint32_t nclusters, BaseFn base_of) {
  for (uint32_t cluster_id = 0; cluster_id < nclusters; ++cluster_id) {
    if ((pa & 0xffffffffffff0fffULL) == base_of(cluster_id))
      return true;
  }
  return false;
}

} // namespace mmr_cluster
