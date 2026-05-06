#pragma once

#include "cvm/plusargs.hpp"

// MMR/IMSIC device address map plusargs (defaults match typical cluster config).
// Plusarg names: +mmr_base_addr=0x40000000, +die_id_width=0, etc.

DECLARE_uint64(mmr_base_addr);
DECLARE_uint64(sp_base_addr);
DECLARE_uint64(sp_size);
DECLARE_uint32(die_id_width);
DECLARE_uint32(cluster_id_width);
DECLARE_uint32(core_id_width);
DECLARE_uint32(die_id_start_bit);
DECLARE_uint32(priv_level_width);
DECLARE_uint32(priv_level_start_bit);
DECLARE_uint32(mmr_cluster_id_start_bit);
DECLARE_uint32(mmr_device_id_width);
DECLARE_uint32(mmr_device_id_start_bit);
DECLARE_uint32(imsic_cluster_id_start_bit);
DECLARE_uint32(imsic_core_id_start_bit);
DECLARE_uint32(cpl_offset_end_bit);
DECLARE_uint32(cpl_sram_offset_end_bit);

// Device ID plusargs
DECLARE_uint32(cr_start_device_id);
DECLARE_uint32(tr_device_id);
DECLARE_uint32(cpl_device_id_start);
DECLARE_uint32(cpl_sram_device_id);
DECLARE_uint32(cpl_device_id_end);
DECLARE_uint32(acl_device_id);
DECLARE_uint32(dm_device_id);
DECLARE_uint32(sc_device_id);
DECLARE_uint32(axisw_device_id);

// Device privilege level (IMSIC/MMR M and S)
DECLARE_uint32(imsic_m);
DECLARE_uint32(mmr_m);
DECLARE_uint32(imsic_s);
DECLARE_uint32(mmr_s);

// Patch RAM plusargs
DECLARE_uint32(patch_ram_start_offset);
DECLARE_uint32(patch_ram_size);