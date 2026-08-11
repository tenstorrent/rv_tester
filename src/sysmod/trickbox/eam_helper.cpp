// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "src/sysmod/trickbox/eam_helper.h"
#include "src/transactors/axi_sw/eam.hpp"
#include "cvm/logger.hpp"

eam_helper::eam_helper(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc)
    : subdevice(tag, addr, SIZE, loc) {
  eam_helper_base = addr;
  reset();
}

eam_helper::~eam_helper() {
}

void eam_helper::configure() {
  subdevice::configure();
  cvm::registry::messenger.procedure<eam_helper_set_fail_addr_RPC>(loc(), [this](std::uint64_t value) { return this->set_fail_addr(value); });
  cvm::registry::messenger.procedure<eam_helper_set_fail_cnt_RPC>(loc(), [this](std::uint64_t value) { return this->set_fail_cnt(value); });
  cvm::registry::messenger.procedure<eam_helper_set_fail_en_RPC>(loc(), [this](bool value) { return this->set_fail_en(value); });
}

bool eam_helper::set_fail_addr(uint64_t value) {
  fail_addr_ = value;
  eam::instance().set_tb_fail_addr(value);
  cvm::log(cvm::HIGH, "[eam_helper] fail_addr_ = {:#x}\n", value);
  return true;
}

bool eam_helper::set_fail_cnt(uint64_t value) {
  fail_cnt_ = value;
  eam::instance().set_tb_fail_cnt(value);
  cvm::log(cvm::HIGH, "[eam_helper] fail_cnt_ = {}\n", value);
  return true;
}

bool eam_helper::set_fail_en(bool value) {
  fail_en_ = value;
  eam::instance().set_tb_fail_en(value);
  cvm::log(cvm::HIGH, "[eam_helper] fail_en_ = {}\n", value);
  return true;
}

void eam_helper::read_dev(uint64_t addr, size_t length, data_t& data) {
  if (not has_addr(addr))
    return;

  const uint64_t offset = addr - eam_helper_base;
  if (offset == FAIL_ADDR_OFFSET) {
    serializeInt(fail_addr_, length, data);
  } else if (offset == FAIL_CNT_OFFSET) {
    serializeInt(fail_cnt_, length, data);
  } else if (offset == FAIL_EN_OFFSET) {
    serializeInt(uint64_t(fail_en_), length, data);
  } else {
    cvm::log(cvm::FULL, "[eam_helper] read of unmapped offset {:#x}\n", offset);
    return;
  }

  cvm::log(cvm::FULL, "[eam_helper] read addr {:#x} offset {:#x} length {}\n", addr, offset, length);
}

void eam_helper::write(uint64_t addr, size_t, const data_t& data,
                       const strb_t&) {
  if (not has_addr(addr))
    return;

  uint64_t t_data = 0;
  deserializeInt(data, t_data);

  const uint64_t offset = addr - eam_helper_base;
  if (offset == FAIL_ADDR_OFFSET) {
    set_fail_addr(t_data);
  } else if (offset == FAIL_CNT_OFFSET) {
    set_fail_cnt(t_data);
  } else if (offset == FAIL_EN_OFFSET) {
    set_fail_en((t_data & 0x1) != 0);
  } else {
    cvm::log(cvm::FULL, "[eam_helper] write to unmapped offset {:#x} data {:#x}\n", offset, t_data);
  }
}
