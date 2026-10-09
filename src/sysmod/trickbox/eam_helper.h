// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// -*- c++ -*-

#pragma once

#include <unistd.h>
#include "src/sysmod/trickbox/subdevice.h"
#include <cstdint>
#include <string>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/messenger.hpp"

// Trickbox subdevice used by software to program the AXI Exclusive Access
// Monitor (eam) failure-injection parameters.
//
// Register map, relative to the device base (0x9092000, size 0x1000):
//   +0x100 : fail_addr_ (64-bit) - address the eam should target
//   +0x120 : fail_cnt_  (64-bit) - number of times to apply the injection
//   +0x140 : fail_en_   (bit 0)  - arms EAM fail counting
//
// A test programs the LR/SC loop address and the randomised fail count first,
// then writes fail_en_ to start counting, so the eam never observes a
// half-programmed configuration.
//
// A write to either register updates the local shadow copy and pushes the new
// value into the process-wide eam singleton. Reads return the shadow copy.
class eam_helper : public subdevice {
public:
  static constexpr uint64_t FAIL_ADDR_OFFSET = 0x100;
  static constexpr uint64_t FAIL_CNT_OFFSET = 0x120;
  static constexpr uint64_t FAIL_EN_OFFSET = 0x140;
  static constexpr uint64_t SIZE = 0x1000;

  eam_helper(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc);
  virtual ~eam_helper();

  void configure() override;

  // Copy n bytes from the given integer, x, to the data iterator following
  // little endian convention.
  template <typename INT>
  void serializeInt(INT x, size_t n, data_t& data) {
    for (unsigned i = 0; i < n; ++i, x >>= 8)
      data[i] = x & 0xff;
  }

  // Copy bytes from the data iterator into the given integer following little
  // endian convention.
  template <typename INT>
  void deserializeInt(const data_t& data, INT& x) {
    x = 0;
    for (unsigned i = 0; i < sizeof(x) and i < data.size(); ++i)
      x |= INT(data[i]) << i * 8;
  }

  void read_dev(uint64_t addr, size_t length, data_t& data) override;

  virtual void write(uint64_t addr, size_t length, const data_t& data,
                     const strb_t& strb) override;

  void reset() override {
    fail_addr_ = 0;
    fail_cnt_ = 0;
    fail_en_ = false;
  }

  // Programming entry points. Both update the local shadow copy and forward
  // the value to the active eam instance.
  bool set_fail_addr(uint64_t value);
  bool set_fail_cnt(uint64_t value);
  bool set_fail_en(bool value);

  CVM_MESSENGER_procedure_call(eam_helper_set_fail_addr_RPC, bool(uint64_t));
  CVM_MESSENGER_procedure_call(eam_helper_set_fail_cnt_RPC, bool(uint64_t));
  CVM_MESSENGER_procedure_call(eam_helper_set_fail_en_RPC, bool(bool));

private:
  uint64_t eam_helper_base = 0x9092000;

  uint64_t fail_addr_ = 0;
  uint64_t fail_cnt_ = 0;
  bool fail_en_ = false;
};
