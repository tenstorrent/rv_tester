// -*- c++ -*-

#pragma once

#include <mutex>
#include <atomic>
#include <thread>
#include <unistd.h>
#include "device.h"
#include <iostream>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <cmath>
#include "pcg_random.hpp"
#include "cvm/plusargs.hpp"
#include "debugger.h"

// Define a core local  (debugger) at the given address
// and for the given hart count. The size will be 48k bytes.
class debugger : public device
{
public:

  /// Define a debugger device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  debugger(const std::string& tag, uint64_t addr, unsigned hartCount);

  // Destructor.
  virtual ~debugger();

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
  /// No-op if address is outside the range of this debugger or if
  /// address is not properly aligned.
  virtual void read(uint64_t addr, size_t length, data_t& data, cbs_t& cbs) override;

  // Write to this debugger.
  virtual void write(uint64_t addr, size_t length, const data_t& data,
                      const strb_t& strb, cbs_t& cbs) override;

  virtual void tick(uint64_t advance, cbs_t& cbs) override
  {
    //std::cout<<"[debugger]: tick\n";
    std::lock_guard<std::mutex> lock(mutex_);
    timer_ += advance;
    timer_advance = advance;
  }

  void reset(){
      std::cout<<"[TRICKBOX]: Reset debugger\n";
  }


private:

  unsigned hartCount_ = 1;
  unsigned numInterrupts_ = 6;
  unsigned debugger_en = 0;

  std::vector<uint32_t> soft_;  // Software interrupt: one per hart.
  std::vector<uint64_t> timeCompare_;  // One per interrupt type.
  std::vector<uint32_t> IntrHart_;  // Hart to be interrupted.
  std::vector<bool> delayedRandomIntValid_; // Valid bit for interrupt
  std::vector<bool> IntrValue_; // Value of interrupt pin
  std::vector<bool> timerIntPrev_; // Value of interrupt pin
  uint64_t timer_ = 0;
  uint64_t timer_advance = 200;
  uint64_t timer_rand_intr = 500;
  uint64_t debugger_base = 0x9004000;
  uint64_t debugger_size = 0x4000;
  
  std::atomic<bool> terminate_ = false;
  std::mutex mutex_;

};

