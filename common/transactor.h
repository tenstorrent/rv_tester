#pragma once

#include <vector>
#include "cvm/topology.hpp"
#include "cvm/messenger.hpp"

class transactor {

  public:

    transactor(cvm::topology::loc_t loc, const std::string& tag)
      : loc_(loc), tag_(tag)
    { };

    virtual ~transactor() = default;

    struct write_t {
        uint64_t addr;
        size_t length;
        const std::vector<uint8_t>& data;
        const std::vector<bool>& strb;
    };

    void write(uint64_t addr, size_t length, const std::vector<uint8_t>& data, const std::vector<bool>& strb)
    {
      cvm::messenger<write_t>::signal(
        loc_,
        write_t{addr, length, data, strb});
    }

    struct read_t {
        uint64_t addr;
        size_t length;
        std::vector<uint8_t>& data;
    };

    void read(uint64_t addr, size_t length, std::vector<uint8_t>& data)
    {
      cvm::messenger<read_t>::signal(
          loc_,
          read_t{addr, length, data});
    }

    std::string tag() { return tag_; }

  private:

    const cvm::topology::loc_t loc_;
    const std::string tag_;
};
