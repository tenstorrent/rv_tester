// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// -*- c++ -*-

#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <gflags/gflags.h>

// ACLINT model configuration shared between the sysmod device (aclint.cpp),
// the SV model (through DPI) and the cosim bridge. Every consumer derives the
// same values from the gflags and the simulation seed, so no run-time state
// has to be exchanged between the C++ libraries.
DECLARE_uint64(seed);
DECLARE_string(aclint_local_sync);
DECLARE_int64(aclint_local_sync_latency);
DECLARE_int64(aclint_local_sync_latency_max);
DECLARE_int64(aclint_timer_update_latency);
DECLARE_int64(aclint_timer_update_latency_max);
DECLARE_uint64(aclint_local_sync_min_lead);
DECLARE_uint64(aclint_ref_time_incr);
DECLARE_uint64(aclint_c_mscfg1_addr);

struct aclint_cfg_t {
  bool local_sync = false;          // two-phase local sync (spec 7.5.2) vs legacy single-step broadcast
  uint16_t local_sync_latency = 0;  // Y, mtime units
  uint8_t timer_update_latency = 0; // Z, mtime units
  uint8_t ref_time_incr = 10;       // mtime increment per reference tick
};

// Default mtime increment per reference tick (100 MHz reference, +10 ns).
inline constexpr uint8_t ACLINT_DEFAULT_REF_TIME_INCR = 10;

// Babylon core c_mscfg1 fields that must agree with the model configuration:
// DisAclintTimeSync [9] selects the legacy load on CTIME write, and
// AclintProgRefTimeIncr [17:10] is the CoreTime increment per tick.
inline constexpr unsigned ACLINT_C_MSCFG1_DIS_TIME_SYNC_BIT = 9;
inline constexpr unsigned ACLINT_C_MSCFG1_REF_TIME_INCR_LSB = 10;
inline constexpr uint64_t ACLINT_C_MSCFG1_REF_TIME_INCR_MASK = 0xffULL << ACLINT_C_MSCFG1_REF_TIME_INCR_LSB;

// Pure function of the gflags and the seed: safe to call from any library once
// the plusargs have been parsed, and repeatable within one simulation.
inline aclint_cfg_t aclint_resolve_cfg() {
  aclint_cfg_t cfg;
  std::mt19937_64 rng(FLAGS_seed ^ 0xac11a7ULL);

  if (FLAGS_aclint_local_sync == "random")
    cfg.local_sync = (rng() & 1) != 0;
  else
    cfg.local_sync = (FLAGS_aclint_local_sync != "0");

  const int64_t z_max = FLAGS_aclint_timer_update_latency_max < 0 ? 0 : (FLAGS_aclint_timer_update_latency_max > 255 ? 255 : FLAGS_aclint_timer_update_latency_max);
  int64_t z = FLAGS_aclint_timer_update_latency;
  if (z < 0)
    z = int64_t(rng() % uint64_t(z_max + 1));
  cfg.timer_update_latency = uint8_t(z > 255 ? 255 : z);

  // Y must leave the staged CTIME write enough lead over the commit strobe to
  // land in every core; the random range starts at Z plus that lead.
  const int64_t y_max = FLAGS_aclint_local_sync_latency_max < 0 ? 0 : (FLAGS_aclint_local_sync_latency_max > 65535 ? 65535 : FLAGS_aclint_local_sync_latency_max);
  int64_t y = FLAGS_aclint_local_sync_latency;
  if (y < 0) {
    int64_t y_min = int64_t(cfg.timer_update_latency) + int64_t(FLAGS_aclint_local_sync_min_lead);
    if (y_min > y_max)
      y_min = y_max;
    y = y_min + int64_t(rng() % uint64_t(y_max - y_min + 1));
  }
  cfg.local_sync_latency = uint16_t(y > 65535 ? 65535 : y);

  uint64_t incr = FLAGS_aclint_ref_time_incr;
  if (incr == 0 || incr > 255)
    incr = ACLINT_DEFAULT_REF_TIME_INCR;
  cfg.ref_time_incr = uint8_t(incr);
  return cfg;
}

// True when the configuration differs from the core reset values and the
// c_mscfg1 CSR has to be programmed to match the model.
inline bool aclint_cfg_overrides_c_mscfg1(const aclint_cfg_t& cfg) {
  return cfg.local_sync || cfg.ref_time_incr != ACLINT_DEFAULT_REF_TIME_INCR;
}

inline uint64_t aclint_cfg_apply_c_mscfg1(const aclint_cfg_t& cfg, uint64_t value) {
  value &= ~(uint64_t(1) << ACLINT_C_MSCFG1_DIS_TIME_SYNC_BIT);
  value |= uint64_t(!cfg.local_sync) << ACLINT_C_MSCFG1_DIS_TIME_SYNC_BIT;
  value &= ~ACLINT_C_MSCFG1_REF_TIME_INCR_MASK;
  value |= (uint64_t(cfg.ref_time_incr) << ACLINT_C_MSCFG1_REF_TIME_INCR_LSB) & ACLINT_C_MSCFG1_REF_TIME_INCR_MASK;
  return value;
}
