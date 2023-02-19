#pragma once

#include <string>
#include <vector>
#include <functional>
#include "cvm/topology.hpp"

class device {

  private:
    const std::string tag_;
    const std::string type_;
    const uint64_t addr_;
    const size_t size_;
    const cvm::topology::loc_t loc_;

  public:

    typedef std::vector<uint8_t> data_t;
    typedef std::vector<bool> strb_t;

    virtual void write(uint64_t addr, size_t length,
                       const data_t& data,
                       const strb_t& strb) = 0;

    virtual void read(uint64_t addr, size_t length,
                      data_t& data) = 0;

    virtual void tick(uint64_t advance) { };

    virtual void reset() { };

    device(std::string tag, std::string type, uint64_t addr, size_t size, cvm::topology::loc_t loc)
      : tag_(tag), addr_(addr), size_(size), loc_(loc)
    { };

    virtual ~device() { };

    std::string tag()             const { return tag_          ; }
    std::string type()            const { return type_          ; }
    uint64_t addr()               const { return addr_         ; }
    size_t size()                 const { return size_         ; }
    cvm::topology::loc_t loc()    const { return loc_          ; }

    bool has_addr(uint64_t val)   const { return val >= addr_ && (val < (addr_ + size_)); }
};
