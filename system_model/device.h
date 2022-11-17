#pragma once

#include <string>
#include <vector>
#include <functional>

class device {

  private:
    const std::string tag_;
    const uint64_t addr_;
    const size_t size_;

  public:

    typedef std::vector<uint8_t> data_t;
    typedef std::vector<bool> strb_t;

    enum class Callback : uint32_t {
      NONE = 0,
      TIMER_INT = 1,
      SW_INT = 2,
      TERMINATE = 3,
      MAX = NONE
    };

    typedef struct {
      Callback cb;
      unsigned hart;
      unsigned val;
    } cb_t;

    typedef std::vector<cb_t> cbs_t;

    virtual void write(uint64_t addr, size_t length,
                       const data_t& data,
                       const strb_t& strb,
                       cbs_t& cbs) = 0;

    virtual void read(uint64_t addr, size_t length,
                      data_t& data,
                      cbs_t& cbs) = 0;

    virtual void tick(uint64_t advance, cbs_t& cbs) { };

    device(std::string tag, uint64_t addr, size_t size)
      : tag_(tag), addr_(addr), size_(size)
    { };

    virtual ~device() { };

    std::string tag()             const { return tag_          ; }
    uint64_t addr()               const { return addr_         ; }
    size_t size()                 const { return size_         ; }

    bool has_addr(uint64_t val)   const { return val >= addr_ && (val < (addr_ + size_)); }
};
