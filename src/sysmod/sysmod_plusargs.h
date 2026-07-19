// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once
// declares plusargs for any packages that depends on them
#include "cvm/plusargs.hpp"

DECLARE_string(load);
DECLARE_string(hex);
DECLARE_string(load_lz4);
DECLARE_string(load_bin);
DECLARE_bool(bootrom);
DECLARE_string(bootrom_path);
DECLARE_bool(debugrom);
DECLARE_string(debugrom_path);
DECLARE_bool(cplfw);
DECLARE_string(cplfw_path);
DECLARE_uint64(seed);
DECLARE_uint32(num_harts);
DECLARE_uint32(hart_enable_mask);
DECLARE_bool(dcls_en);
DECLARE_string(hart_enable_id);
DECLARE_int32(sc_dis_ways_mask);
DECLARE_uint32(debug_enable);
DECLARE_bool(ntrace_enable);
DECLARE_bool(dst_enable);
DECLARE_bool(cla_enable);
DECLARE_bool(io_coherency_disable);
DECLARE_int32(imsic_intr_delay_min);       //, 4, "Minimum Delay between 2 consecutive interrupts");
DECLARE_int32(imsic_intr_delay_max);       //, 7, "Maximum Delay between 2 consecutive interrupts");
DECLARE_bool(random_imsic_intr);           //, false, "Drive random interrups");
DECLARE_bool(trickbox_write_enables_intr); //, false, "Require software write to 0x9004040 before random interrupts start");
DECLARE_bool(disable_m_imsic_intr);
DECLARE_bool(disable_s_imsic_intr);
DECLARE_bool(disable_vs_imsic_intr);
DECLARE_bool(disable_random_hart_imsic_intr);
DECLARE_uint64(imsic_intr_mask);
DECLARE_uint64(imsic_vs_intr_mask);
DECLARE_int32(imsic_vs_id_threshold);
DECLARE_int32(imsic_hart_threshold);
DECLARE_int32(imsic_intr_start_delay);
DECLARE_bool(export_control_en);
DECLARE_int32(clk_profile);
DECLARE_string(warm_reset);
DECLARE_string(warm_reset_debug_hold);
DECLARE_bool(rand_core_harvest);
DECLARE_int32(max_intr_count);
DECLARE_bool(dm_model_check_bypass);
DECLARE_bool(dbg_rand_core);
DECLARE_int32(dbg_rand_core_idx);
DECLARE_bool(sysmod_terminate);
DECLARE_string(stee_secure_region);
DECLARE_bool(overlay_mmr_check);
DECLARE_bool(jtag_en);
DECLARE_bool(enable_ntrace_in_boot);
DECLARE_bool(time_mtime_sync_enable);
