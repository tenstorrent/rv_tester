// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "src/sysmod/heartbeat/heartbeat.h"

DEFINE_int32(heartbeat_period, 5000, "Cycles between heartbeat messages. Set to 0 to turn off");

void heartbeat::tick(std::uint64_t advance) {

  if (!count_ && FLAGS_heartbeat_period % advance) {
    cvm::log(cvm::MEDIUM, "[HEARTBEAT] Warning: clock period {} is not a multiple of heartbeat period {}, you will not see heartbeat prints as specified.\n", advance, FLAGS_heartbeat_period);
  }

  std::uint64_t next = count_ + advance;
  bool print = FLAGS_heartbeat_period && count_ / FLAGS_heartbeat_period != next / FLAGS_heartbeat_period;

  count_ = next;

  if (print) {
    log_(cvm::LOW, "[HEARTBEAT] cycle {}\n", count_);
    log_.flush();
  }
}
