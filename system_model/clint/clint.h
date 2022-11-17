// -*- c++ -*-

#pragma once

#include <mutex>
#include <atomic>
#include <thread>
#include <unistd.h>
#include "device.h"

// Define a core local interruptor (Clint) at the given address
// and for the given hart count. The size will be 48k bytes.
class clint : public device
{
public:

  /// Define a CLINT device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  clint(const std::string& tag, uint64_t addr, unsigned hartCount);

  // Destructor.
  virtual ~clint();

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
  /// No-op if address is outside the range of this clint or if
  /// address is not properly aligned.
  virtual void read(uint64_t addr, size_t length, data_t& data, cbs_t& cbs) override;

  // Write to this clint. Call softwareInterrupt with flag set to 0/1
  // if a hart software interrupt entry is written. Update time
  // compare and call timerInterrupt if a hart time compare entry is
  // written. Call timerInterrupt on every hart if timer is written.
  //
  // This is a no-op if address is not aligned, if length is not 4 for
  // software interrupt entries, if length is not 8 for
  // timer/time-compare entries.
  virtual void write(uint64_t addr, size_t length, const data_t& data,
                      const strb_t& strb, cbs_t& cbs) override;

  virtual void tick(uint64_t advance, cbs_t& cbs) override
  {
    std::lock_guard<std::mutex> lock(mutex_);
    timer_ += advance;
    processTimerInterrupts(cbs);
  }

protected:

  /// Assert/deassert the timer interrupt for each hart where the
  /// time-compare value is greater-than-or-equal/less-than the timer
  /// value.
  void processTimerInterrupts(cbs_t& cbs)
  {
    for (unsigned i = 0; i < hartCount_; ++i)
      {
        bool flag = timer_ >= timeCompare_.at(i);
	timerInterrupt(i, flag, cbs);
      }
  }

  // Used to assert/deassert a software interrupt (PIPI) for given hart.
  virtual void softwareInterrupt(unsigned hart, bool flag, cbs_t& cbs)
  {
    cbs.push_back(cb_t{Callback::SW_INT, hart, flag});
  }

  // Used to assert/deassert a timer interrupt for given hart.
  virtual void timerInterrupt(unsigned hart, bool flag, cbs_t& cbs)
  {
    cbs.push_back(cb_t{Callback::TIMER_INT, hart, flag});
  }

  // Start a thread to increment timer after n microseconds.
  void selfTick(useconds_t n);

private:

  unsigned hartCount_ = 1;

  std::vector<uint32_t> soft_;  // Software interrupt: one per hart.
  std::vector<uint64_t> timeCompare_;  // One per hart.
  uint64_t timer_ = 0;

  std::atomic<bool> terminate_ = false;
  std::mutex mutex_;

  std::thread timerThread_;
};

