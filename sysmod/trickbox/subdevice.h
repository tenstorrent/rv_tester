#pragma once

#include "sysmod/device.h"

class subdevice : public device {
  public:

    subdevice(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc)
      : device(tag, addr, hartCount, loc) {};

    virtual void write(uint64_t addr, size_t length, const data_t& data,
                        const strb_t& strb) = 0;
};
