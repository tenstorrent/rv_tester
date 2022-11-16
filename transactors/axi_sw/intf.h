#pragma once

#include "sysmod.h"
#include "safe_queue.h"

class intf {

  public:

    intf(sysmod* s, const std::string& tag)
      : sysmod_(s), tag_(tag)
    { };

    virtual ~intf() = default;

    void write(uint64_t addr, size_t length, const device::data_t& data, const device::strb_t& strb)
    { sysmod_->write(addr, length, data, strb); }

    void read(uint64_t addr, size_t length, device::data_t& data)
    { sysmod_->read(addr, length, data); }

    std::string tag() { return tag_; }

  private:

    sysmod* sysmod_;
    const std::string tag_;
};
