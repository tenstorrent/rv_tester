// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "device_address_map/device_address_map_plusargs.h"

// Define plusargs for device address map attributes.
// Defaults are intentionally 0 so the real layout is not disclosed in source.
// The real values are supplied at runtime via the project flagfile

DEFINE_uint64(mmr_base_addr, 0x0ULL, "MMR base address");
DEFINE_uint64(sp_base_addr, 0x0ULL, "SP base address");
DEFINE_uint64(sp_size, 0x0ULL, "SP size");
DEFINE_uint32(die_id_width, 0, "DIE_ID width in bits");
DEFINE_uint32(cluster_id_width, 0, "CLUSTER_ID width in bits");
DEFINE_uint32(core_id_width, 0, "CORE_ID width in bits");
DEFINE_uint32(die_id_start_bit, 0, "DIE_ID start bit in address");
DEFINE_uint32(priv_level_width, 0, "Privilege level width in bits");
DEFINE_uint32(priv_level_start_bit, 0, "Privilege level start bit in address");
DEFINE_uint32(mmr_cluster_id_start_bit, 0, "MMR cluster ID start bit");
DEFINE_uint32(mmr_device_id_width, 0, "MMR device ID width in bits");
DEFINE_uint32(mmr_device_id_start_bit, 0, "MMR device ID start bit");
DEFINE_uint32(imsic_cluster_id_start_bit, 0, "IMSIC cluster ID start bit");
DEFINE_uint32(imsic_core_id_start_bit, 0, "IMSIC core ID start bit");
DEFINE_uint32(cpl_offset_end_bit, 0, "CPL offset end bit");
DEFINE_uint32(cpl_sram_offset_end_bit, 0, "CPL SRAM end offset bit");

DEFINE_uint32(cr_start_device_id, 0x0, "CR start device ID");
DEFINE_uint32(tr_device_id, 0x0, "TR device ID");
DEFINE_uint32(cpl_device_id_start, 0x0, "CPL device ID start");
DEFINE_uint32(cpl_sram_device_id, 0x0, "CPL SRAM device ID");
DEFINE_uint32(cpl_device_id_end, 0x0, "CPL device ID end");
DEFINE_uint32(acl_device_id, 0x0, "ACL device ID");
DEFINE_uint32(dm_device_id, 0x0, "DM device ID");
DEFINE_uint32(sc_device_id, 0x0, "SC device ID");
DEFINE_uint32(axisw_device_id, 0x0, "AXISW device ID");

DEFINE_uint32(imsic_m, 0x0, "IMSIC machine privilege level");
DEFINE_uint32(mmr_m, 0x0, "MMR machine privilege level");
DEFINE_uint32(imsic_s, 0x0, "IMSIC supervisor privilege level");
DEFINE_uint32(mmr_s, 0x0, "MMR supervisor privilege level");

DEFINE_uint32(patch_ram_start_offset, 0x0, "Patch RAM start offset");
DEFINE_uint32(patch_ram_size, 0x0, "Patch RAM size");
