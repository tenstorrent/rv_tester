#include "common/device_address_map/device_address_map_plusargs.h"

// Define plusargs for device address map attributes.
// Defaults: MMR_BASE_ADDR=0x40000000, DIE_ID_WIDTH=0, CLUSTER_ID_WIDTH=4, etc.

DEFINE_uint64(mmr_base_addr, 0x40000000ULL, "MMR base address");
DEFINE_uint32(die_id_width, 0, "DIE_ID width in bits");
DEFINE_uint32(cluster_id_width, 4, "CLUSTER_ID width in bits");
DEFINE_uint32(core_id_width, 3, "CORE_ID width in bits");
DEFINE_uint32(die_id_start_bit, 45, "DIE_ID start bit in address");
DEFINE_uint32(priv_level_width, 2, "Privilege level width in bits");
DEFINE_uint32(priv_level_start_bit, 25, "Privilege level start bit in address");
DEFINE_uint32(mmr_cluster_id_start_bit, 21, "MMR cluster ID start bit");
DEFINE_uint32(mmr_device_id_width, 5, "MMR device ID width in bits");
DEFINE_uint32(mmr_device_id_start_bit, 16, "MMR device ID start bit");
DEFINE_uint32(imsic_cluster_id_start_bit, 21, "IMSIC cluster ID start bit");
DEFINE_uint32(imsic_core_id_start_bit, 18, "IMSIC core ID start bit");
DEFINE_uint32(cpl_offset_end_bit, 19, "CPL offset end bit");

DEFINE_uint32(cr_start_device_id, 0x0, "CR start device ID");
DEFINE_uint32(tr_device_id, 0x8, "TR device ID");
DEFINE_uint32(cpl_device_id, 0x10, "CPL device ID");
DEFINE_uint32(acl_device_id, 0x18, "ACL device ID");
DEFINE_uint32(dm_device_id, 0x19, "DM device ID");
DEFINE_uint32(sc_device_id, 0x1A, "SC device ID");
DEFINE_uint32(axisw_device_id, 0x1B, "AXISW device ID");

DEFINE_uint32(imsic_m, 0x0, "IMSIC machine privilege level");
DEFINE_uint32(mmr_m, 0x1, "MMR machine privilege level");
DEFINE_uint32(imsic_s, 0x2, "IMSIC supervisor privilege level");
DEFINE_uint32(mmr_s, 0x3, "MMR supervisor privilege level");
