// -*- c++ -*-

#pragma once

#include <unistd.h>
#include "src/sysmod/trickbox/subdevice.h"
#include <iostream>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <cmath>
#include "common/pcg_random.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include <mem_manager.h>

#include "whisper_client.h"

// Define a core local interruptor (uc_helper) at the given address
// and for the given hart count. The size will be 48k bytes.
class uc_helper : public subdevice {
public:
  /// Define a uc_helper device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  uc_helper(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc, mem_manager& m_);
  // Destructor.
  virtual ~uc_helper();

  void configure() override;

  // Copy n bytes from the given integer, x, to the data iterator
  // following little endian convention. If n is larger than the size
  // of x, then copy zero bytes after copying the bytes of x.
  template <typename INT>
  void serializeInt(INT x, size_t n, data_t& data) {
    for (unsigned i = 0; i < n; ++i, x >>= 8)
      data[i] = x & 0xff;
  }

  // Copy bytes from data iterator into the given integer following
  // lilttle endian convention.
  template <typename INT>
  void deserializeInt(const data_t& data, INT& x) {
    x = 0;
    for (unsigned i = 0; i < sizeof(x); ++i)
      x |= INT(data[i]) << i * 8;
  }

  /// Read length bytes from the given address to the data iterator.
  /// No-op if address is outside the range of this uc_helper or if
  /// address is not properly aligned.
  cvm::messenger::task<void> read(uint64_t addr, size_t length, data_t& data);
  void read_dev(uint64_t addr, size_t length, data_t& data) override;

  // Write to this uc_helper. Call softwareInterrupt with flag set to 0/1
  // if a hart software interrupt entry is written. Update time
  // compare and call timerInterrupt if a hart time compare entry is
  // written. Call timerInterrupt on every hart if timer is written.
  //
  // This is a no-op if address is not aligned, if length is not 4 for
  // software interrupt entries, if length is not 8 for
  // timer/time-compare entries.

  virtual void write(uint64_t addr, size_t length, const data_t& data,
                     const strb_t& strb) override;
  void reset() override {
  }

  uint64_t convertToUInt64(const std::vector<uint8_t>& vec) {
    if (vec.size() != 8) {
      throw std::invalid_argument("Vector must have exactly 8 elements.");
    }

    uint64_t result = 0;
    for (size_t i = 0; i < 8; ++i) {
      result |= static_cast<uint64_t>(vec[i]) << (8 * i);
    }

    return result;
  }
  struct uc_helper_write_t {
    uint64_t addr;
    size_t length;
    std::vector<uint8_t> data;
    std::vector<bool> strb;
  };
  struct uc_helper_read_req_t {
    uint64_t addr;
    size_t length;
    std::vector<uint8_t> data;
    std::vector<bool> strb;
  };
  struct trickbox_mem_req_t {
    uint64_t addr;
    size_t length;
    std::vector<uint8_t> data;
    std::vector<bool> strb;
  };

  void update_mem_model(trickbox_mem_req_t& i);

protected:
  //Check plusarg usage
  void checkUsage();

private:
  uint64_t uc_helper_base = 0x9000000;
  uint64_t tx_status = 0;
  uint64_t tx_addr = 0x90a0000;
  uint64_t tx_size = 2;
  uint64_t tx_trigger = 0;
  uint8_t read_flag = 0;
  mem_manager m_;

  pcg_extras::seed_seq_from<std::random_device> seed_source;
  pcg32 rng;
  //unsigned hartCount;
};
