// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "src/sysmod/device.h"
#include "cvm/topology.hpp"
#include <string>

class null_dev : public device {

private:
public:
  void write(const transactor::write_t& w);

  void read(const transactor::read_t& r, data_t& data);

  // add max mem size
  null_dev(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc) : device(tag, addr, size, loc, &null_dev::write, &null_dev::read, this) {}
};
