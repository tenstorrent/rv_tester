// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "device_address_map/device_address_map.h"
#include "device_address_map/device_address_map_plusargs.h"

// --- Getters (from plusargs) ---

uint64_t device_address_map_mmr_base_addr() { return FLAGS_mmr_base_addr; }
uint64_t device_address_map_sp_base_addr() { return FLAGS_sp_base_addr; }
uint64_t device_address_map_sp_size() { return FLAGS_sp_size; }
uint32_t device_address_map_die_id_width() { return FLAGS_die_id_width; }
uint32_t device_address_map_cluster_id_width() { return FLAGS_cluster_id_width; }
uint32_t device_address_map_core_id_width() { return FLAGS_core_id_width; }
uint32_t device_address_map_die_id_start_bit() { return FLAGS_die_id_start_bit; }
uint32_t device_address_map_priv_level_width() { return FLAGS_priv_level_width; }
uint32_t device_address_map_priv_level_start_bit() { return FLAGS_priv_level_start_bit; }
uint32_t device_address_map_mmr_cluster_id_start_bit() { return FLAGS_mmr_cluster_id_start_bit; }
uint32_t device_address_map_mmr_device_id_width() { return FLAGS_mmr_device_id_width; }
uint32_t device_address_map_mmr_device_id_start_bit() { return FLAGS_mmr_device_id_start_bit; }
uint32_t device_address_map_imsic_cluster_id_start_bit() { return FLAGS_imsic_cluster_id_start_bit; }
uint32_t device_address_map_imsic_core_id_start_bit() { return FLAGS_imsic_core_id_start_bit; }
uint32_t device_address_map_cpl_offset_end_bit() { return FLAGS_cpl_offset_end_bit; }
uint32_t device_address_map_cpl_sram_offset_end_bit() { return FLAGS_cpl_sram_offset_end_bit; }
uint32_t device_address_map_patch_ram_start_offset() { return FLAGS_patch_ram_start_offset; }
uint32_t device_address_map_patch_ram_size() { return FLAGS_patch_ram_size; }
// --- Generic device address ---
// is_imsic = (priv_level == IMSIC_M || priv_level == IMSIC_S)
// IMSIC: res |= device_id << IMSIC_CORE_ID_START_BIT, res |= cluster_id << IMSIC_CLUSTER_ID_START_BIT
// MMR:   res |= device_id << MMR_DEVICE_ID_START_BIT, res |= cluster_id << MMR_CLUSTER_ID_START_BIT
// res |= priv_level << PRIV_LEVEL_START_BIT

uint64_t generate_device_addr(uint32_t device_id, uint32_t cluster_id, uint32_t priv_level) {
  const uint64_t mmr_base = FLAGS_mmr_base_addr;
  const uint32_t pl_start = FLAGS_priv_level_start_bit;
  const uint32_t imsic_m = FLAGS_imsic_m;
  const uint32_t imsic_s = FLAGS_imsic_s;
  const uint32_t die_id_width = FLAGS_die_id_width;
  const uint32_t die_id_start_bit = FLAGS_die_id_start_bit;

  bool is_imsic = (priv_level == imsic_m || priv_level == imsic_s);
  uint64_t res = mmr_base;

  uint32_t cluster_id_lower = cluster_id & ((1 << FLAGS_cluster_id_width) - 1);
  uint32_t cluster_id_upper = cluster_id >> FLAGS_cluster_id_width;

  if (die_id_width > 0)
    res |= (uint64_t)cluster_id_upper << die_id_start_bit;

  if (is_imsic) {
    res |= (uint64_t)device_id << FLAGS_imsic_core_id_start_bit;
    res |= (uint64_t)cluster_id_lower << FLAGS_imsic_cluster_id_start_bit;
  } else {
    res |= (uint64_t)device_id << FLAGS_mmr_device_id_start_bit;
    res |= (uint64_t)cluster_id_lower << FLAGS_mmr_cluster_id_start_bit;
  }
  res |= (uint64_t)priv_level << pl_start;
  return res;
}

// --- IMSIC base addresses ---

uint64_t generate_imsic_m_addr(uint32_t cluster_id, uint32_t core_id) {
  return generate_device_addr(core_id, cluster_id, FLAGS_imsic_m);
}

uint64_t generate_imsic_s_addr(uint32_t cluster_id, uint32_t core_id) {
  return generate_device_addr(core_id, cluster_id, FLAGS_imsic_s);
}

// --- MMR Base Addresses ---

uint64_t generate_cr_device_addr(uint32_t cluster_id, uint32_t core_id) {
  return generate_device_addr(core_id, cluster_id, FLAGS_mmr_m);
}

uint64_t generate_tr_device_addr(uint32_t cluster_id) {
  return generate_device_addr(FLAGS_tr_device_id, cluster_id, FLAGS_mmr_m);
}

uint64_t generate_cpl_device_addr(uint32_t cluster_id) {
  return generate_device_addr(FLAGS_cpl_device_id_start, cluster_id, FLAGS_mmr_m);
}

uint64_t generate_cpl_sram_device_addr(uint32_t cluster_id) {
  return generate_device_addr(FLAGS_cpl_sram_device_id, cluster_id, FLAGS_mmr_m);
}

uint64_t generate_acl_device_addr(uint32_t cluster_id) {
  return generate_device_addr(FLAGS_acl_device_id, cluster_id, FLAGS_mmr_m);
}

uint64_t generate_dm_device_addr(uint32_t cluster_id) {
  return generate_device_addr(FLAGS_dm_device_id, cluster_id, FLAGS_mmr_m);
}

uint64_t generate_sc_device_addr(uint32_t cluster_id) {
  return generate_device_addr(FLAGS_sc_device_id, cluster_id, FLAGS_mmr_m);
}

uint64_t generate_axisw_device_addr(uint32_t cluster_id) {
  return generate_device_addr(FLAGS_axisw_device_id, cluster_id, FLAGS_mmr_m);
}

// --- Helper functions ---

uint64_t extract_mmr_offset(uint64_t addr) {
  return addr & ((1 << FLAGS_mmr_device_id_width) - 1);
}

static uint32_t privilege_level_from_addr(uint64_t addr) {
  const uint32_t w = FLAGS_priv_level_width;
  if (w == 0)
    return 0;
  const uint32_t mask = w >= 32 ? ~0u : ((1u << w) - 1u);
  return (uint32_t)((addr >> FLAGS_priv_level_start_bit) & mask);
}

static bool pl_is_imsic_space(uint32_t pl) {
  return pl == FLAGS_imsic_m || pl == FLAGS_imsic_s;
}

uint32_t extract_cluster_id_from_address(uint64_t addr) {
  const uint32_t pl = privilege_level_from_addr(addr);
  const uint32_t cw = FLAGS_cluster_id_width;
  const uint32_t cluster_mask = cw >= 32 ? ~0u : ((1u << cw) - 1u);
  uint32_t cluster_lower;
  if (pl_is_imsic_space(pl)) {
    cluster_lower =
        (uint32_t)((addr >> FLAGS_imsic_cluster_id_start_bit) & cluster_mask);
  } else {
    cluster_lower =
        (uint32_t)((addr >> FLAGS_mmr_cluster_id_start_bit) & cluster_mask);
  }
  uint32_t cluster_upper = 0;
  if (FLAGS_die_id_width > 0) {
    const uint32_t dw = FLAGS_die_id_width;
    const uint32_t die_mask = dw >= 32 ? ~0u : ((1u << dw) - 1u);
    cluster_upper =
        (uint32_t)((addr >> FLAGS_die_id_start_bit) & die_mask);
  }
  return (cluster_upper << cw) | cluster_lower;
}

bool is_internal_device(uint64_t addr, uint32_t cluster_id) {
  const uint32_t low_bits =
      FLAGS_priv_level_start_bit + FLAGS_priv_level_width;
  uint64_t mmr_base_mask;
  if (low_bits == 0) {
    mmr_base_mask = ~0ULL;
  } else if (low_bits >= 64) {
    mmr_base_mask = 0;
  } else {
    mmr_base_mask = ~((1ULL << low_bits) - 1ULL);
  }
  const uint64_t mmr_base = FLAGS_mmr_base_addr;
  const bool base_match = (addr & mmr_base_mask) == (mmr_base & mmr_base_mask);
  return base_match && (cluster_id == extract_cluster_id_from_address(addr));
}
