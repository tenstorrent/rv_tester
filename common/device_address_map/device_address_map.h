#pragma once

#include <cstdint>

// Getters (read from plusargs / device address map)
uint64_t device_address_map_mmr_base_addr();
uint64_t device_address_map_sp_base_addr();
uint64_t device_address_map_sp_size();
uint32_t device_address_map_die_id_width();
uint32_t device_address_map_cluster_id_width();
uint32_t device_address_map_core_id_width();
uint32_t device_address_map_die_id_start_bit();
uint32_t device_address_map_priv_level_width();
uint32_t device_address_map_priv_level_start_bit();
uint32_t device_address_map_mmr_cluster_id_start_bit();
uint32_t device_address_map_mmr_device_id_width();
uint32_t device_address_map_mmr_device_id_start_bit();
uint32_t device_address_map_imsic_cluster_id_start_bit();
uint32_t device_address_map_imsic_core_id_start_bit();
uint32_t device_address_map_cpl_offset_end_bit();
uint32_t device_address_map_cpl_sram_offset_end_bit();

// Generic device address
// For IMSIC, device_id is used as core index in the address encoding
uint64_t generate_device_addr(uint32_t device_id, uint32_t cluster_id, uint32_t priv_level);

// IMSIC base addresses
uint64_t generate_imsic_m_addr(uint32_t cluster_id, uint32_t core_id);
uint64_t generate_imsic_s_addr(uint32_t cluster_id, uint32_t core_id);

// MMR base addresses
uint64_t generate_cr_device_addr(uint32_t cluster_id, uint32_t core_id=0); // Default core_id is 0
uint64_t generate_tr_device_addr(uint32_t cluster_id);
uint64_t generate_cpl_device_addr(uint32_t cluster_id);
uint64_t generate_cpl_sram_device_addr(uint32_t cluster_id);
uint64_t generate_acl_device_addr(uint32_t cluster_id);
uint64_t generate_dm_device_addr(uint32_t cluster_id);
uint64_t generate_sc_device_addr(uint32_t cluster_id);
uint64_t generate_axisw_device_addr(uint32_t cluster_id);

// Helper functions
uint64_t extract_mmr_offset(uint64_t addr);