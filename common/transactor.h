#pragma once

#include <vector>
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"

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
      cvm::registry::messenger.signal<write_t>(
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
      cvm::registry::messenger.signal<read_t>(
          loc_,
          read_t{addr, length, data});
    }

    struct read_request_t {
        uint64_t addr;
        size_t length;
    };
    struct read_response_t {
        std::vector<uint8_t> data;
    };
    void read_request(uint64_t addr, size_t length, std::function<void(read_response_t)> cb);

    struct write_request_t {
        uint64_t addr;
        size_t length;
        std::vector<uint8_t> data;
    };

    std::string tag() { return tag_; }

  private:

    const cvm::topology::loc_t loc_;
    const std::string tag_;
};
