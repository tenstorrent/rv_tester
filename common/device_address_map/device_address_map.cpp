#include "common/device_address_map/device_address_map.h"
#include "common/device_address_map/device_address_map_plusargs.h"

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

  if(die_id_width > 0)
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