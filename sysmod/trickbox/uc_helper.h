// -*- c++ -*-

#pragma once

#include <mutex>
#include <atomic>
#include <thread>
#include <unistd.h>
#include "subdevice.h"
#include <iostream>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <cmath>
#include "pcg_random.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include <mem_manager.h>
DECLARE_int32(seed);
// Define a core local interruptor (uc_helper) at the given address
// and for the given hart count. The size will be 48k bytes.
class uc_helper : public subdevice
{
public:

  /// Define a uc_helper device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  uc_helper(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc, mem_manager &m_);
  // Destructor.
  virtual ~uc_helper();

  // Copy n bytes from the given integer, x, to the data iterator
  // following little endian convention. If n is larger than the size
  // of x, then copy zero bytes after copying the bytes of x.
  template <typename INT>
  void serializeInt(INT x, size_t n, data_t& data)
  {
    for (unsigned i = 0; i < n; ++i, x >>= 8)
      data[i] = x & 0xff;
  }

  // Copy bytes from data iterator into the given integer following
  // lilttle endian convention.
  template <typename INT>
  void deserializeInt(const data_t& data, INT& x)
  {
    x = 0;
    for (unsigned i = 0; i < sizeof(x); ++i)
      x |= INT(data[i]) << i*8;
  }

  /// Read length bytes from the given address to the data iterator.
  /// No-op if address is outside the range of this uc_helper or if
  /// address is not properly aligned.
  cvm::messenger::task<void> read(uint64_t addr, size_t length, data_t& data);


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
      std::cout<<"[TRICKBOX]: Reset uc_helper\n";
  }

protected:

  //Check plusarg usage
  void checkUsage();

private:
  uint64_t uc_helper_base = 0x9000000;
  uint64_t tx_status = 0;
  uint64_t tx_addr = 0x9000000;
  uint64_t tx_size = 30;
  uint64_t tx_trigger = 0;

  mem_manager m_;
  std::atomic<bool> terminate_ = false;
  
  pcg_extras::seed_seq_from<std::random_device> seed_source;
  pcg32 rng;
  //unsigned hartCount;
};

