#pragma once

#include <vector>
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"

class transactor {

  public:

    struct write_t {
        uint64_t addr;
        size_t length;
        std::vector<uint8_t> data;
        std::vector<bool> strb;
    };

    struct read_t {
        uint32_t id;
        uint64_t addr;
        size_t length;
    };

    struct read_response_t {
        uint32_t id;
        std::vector<uint8_t> data;
    };

    struct read_request_t {
        uint64_t addr;
        size_t length;
    };

    struct write_request_t {
        uint64_t addr;
        size_t length;
        std::vector<uint8_t> data;
    };

    transactor(cvm::topology::loc_t loc, const std::string& tag)
      : loc_(loc), tag_(tag), resp_channel_(cvm::registry::messenger.channel<read_response_t>(loc_))
    { };

    virtual ~transactor() = default;

    void write(uint64_t addr, size_t length, const std::vector<uint8_t>& data, const std::vector<bool>& strb)
    {
      cvm::registry::messenger.signal<write_t>(
        loc_,
        write_t{addr, length, data, strb});
    }

    cvm::messenger::task<std::vector<uint8_t>> read(uint32_t id, uint64_t addr, size_t length /*, bool block = false */)
    {
      // for resolving requests out of order
      cvm::registry::messenger.channel_filter<read_response_t>(resp_channel_, [&id] (const read_response_t& r) { return r.id == id; });

      cvm::registry::messenger.signal<read_t>(
          loc_,
          read_t{id, addr, length});

      // TODO: let complete out of order
      auto response = co_await cvm::registry::messenger.wait<read_response_t>(resp_channel_);
      co_return response.data;
    }

    std::string tag() { return tag_; }

  private:

    const cvm::topology::loc_t loc_;
    const std::string tag_;
    cvm::messenger::pool<read_response_t>::channel_info resp_channel_;
};
