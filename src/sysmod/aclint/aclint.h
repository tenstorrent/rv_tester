// -*- c++ -*-

#pragma once

#include <mutex>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <functional>
#include "src/sysmod/device.h"
#include "src/sysmod/clint/clint.h"
#include "cvm/registry.hpp"

// Define a core local interruptor (aClint) at the given address
// and for the given hart count. The size will be 48k bytes.
class aclint : public device
{
public:

  /// Define a aCLINT device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  aclint(const std::string& tag, uint64_t addr, unsigned hartCount,
        cvm::topology::loc_t loc);

  // Destructor.
  virtual ~aclint();

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
  /// No-op if address is outside the range of this aclint or if
  /// address is not properly aligned.
  void read(const transactor::read_t& r, data_t& data);

  // Write to this aclint. Call softwareInterrupt with flag set to 0/1
  // if a hart software interrupt entry is written. Update time
  // compare and call timerInterrupt if a hart time compare entry is
  // written. Call timerInterrupt on every hart if timer is written.
  //
  // This is a no-op if address is not aligned, if length is not 4 for
  // software interrupt entries, if length is not 8 for
  // timer/time-compare entries.
  void write(const transactor::write_t& w);

  virtual void tick(uint64_t advance) override;



protected:

  /// Assert/deassert the timer interrupt for each hart where the
  /// time-compare value is greater-than-or-equal/less-than the timer
  /// value.
  void processTimerInterrupts()
  {
    for (unsigned i = 0; i < hartCount_; ++i) {
      bool flag = timer_ >= timeCompare_.at(i);
      if (timerIntPrev_.at(i) != flag)
        timerInterrupt(i, flag);
      timerIntPrev_.at(i) = flag;
    }
  }

  // Used to assert/deassert a timer interrupt for given hart.
  virtual void timerInterrupt(unsigned hart, bool flag)
  {
    cvm::registry::messenger.signal<clint::timer_t>(loc(), clint::timer_t{hart, flag, timer_});
  }


private:

  unsigned hartCount_ = 1;

  std::vector<uint32_t> soft_;  // Software interrupt: one per hart.
  std::vector<uint64_t> timeCompare_;  // One per hart.
  std::vector<bool> timerIntPrev_; // Previous value of timer interrupt
  uint64_t timer_ = 0;

  std::mutex mutex_;

  std::uint64_t tickDivisor_;
};

