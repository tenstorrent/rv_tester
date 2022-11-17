#pragma once

#include <vector>
#include "endpoint.h"

class transactor {

  public:

    transactor(endpoint* e, const std::string& tag)
      : e_(e), tag_(tag)
    { };

    virtual ~transactor() = default;

    void write(uint64_t addr, size_t length, const std::vector<uint8_t>& data, const std::vector<bool>& strb)
    { e_->write(addr, length, data, strb); }

    void read(uint64_t addr, size_t length, std::vector<uint8_t>& data)
    { e_->read(addr, length, data); }

    std::string tag() { return tag_; }

  private:

    endpoint* e_;
    const std::string tag_;
};
