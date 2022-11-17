#pragma once

#include <vector>

class endpoint {

  public:

    endpoint()
    { };

    virtual ~endpoint() = default;

    virtual void write(uint64_t addr, size_t length, const std::vector<uint8_t>& data, const std::vector<bool>& strb) = 0;
    virtual void read(uint64_t addr, size_t length, std::vector<uint8_t>& data) = 0;
};
