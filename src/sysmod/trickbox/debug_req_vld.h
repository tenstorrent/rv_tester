// -*- c++ -*-

#pragma once

#include <string>
#include "src/sysmod/trickbox/subdevice.h"
#include "cvm/logger.hpp"
#include "cvm/registry.hpp"
#include "cvm/topology.hpp"

// Trickbox I/O handler for external debug request injection. The requesting
// hart is the value written; the resulting per-hart mask drives
// DebugReqVld_ANY through the sysmod DPI export.
//   - Base+0x0 : write hart id to assert the debug request for that hart
//   - Base+0x8 : write hart id to deassert the debug request for that hart
//   - Base+0x0 read : current per-hart request mask
class debug_req_vld : public subdevice {
public:
  struct request_t {
    uint64_t hart;
    bool set;
  };

  debug_req_vld(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc);

  virtual ~debug_req_vld();

  template <typename INT>
  void serializeInt(INT x, size_t n, data_t& data) {
    for (unsigned i = 0; i < n; ++i, x >>= 8)
      data[i] = x & 0xff;
  }

  template <typename INT>
  void deserializeInt(const data_t& data, INT& x) {
    x = 0;
    for (unsigned i = 0; i < sizeof(x); ++i)
      x |= INT(data[i]) << i * 8;
  }

  void read_dev(uint64_t addr, size_t length, data_t& data) override;

  virtual void write(uint64_t addr, size_t length, const data_t& data,
                     const strb_t& strb) override;

  void reset() override { req_mask_ = 0; }

private:
  uint64_t assert_addr_;
  uint64_t deassert_addr_;
  const unsigned hart_count_;
  uint64_t req_mask_ = 0;
  cvm::topology::loc_t loc_;
};
