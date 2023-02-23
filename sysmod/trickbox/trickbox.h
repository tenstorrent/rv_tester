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
#include "interrupter.h"

// Define a core local  (trickbox) at the given address
// and for the given hart count. The size will be 48k bytes.
class trickbox : public device
{
public:

  /// Define a trickbox device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  trickbox(const std::string& tag, const std::string& type, uint64_t addr, unsigned hartCount);

  // Destructor.
  virtual ~trickbox();

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
  /// No-op if address is outside the range of this trickbox or if
  /// address is not properly aligned.
  virtual void read(uint64_t addr, size_t length, data_t& data, cbs_t& cbs) override;

  // Write to this trickbox. 
  virtual void write(uint64_t addr, size_t length, const data_t& data,
                      const strb_t& strb, cbs_t& cbs) override;

  virtual void tick(uint64_t advance, cbs_t& cbs) override
  {
    for (auto& d : subdevices_) {
      d->tick(advance,cbs);
    }
  }

  void reset(){
      std::cout<<"[TRICKBOX]: Reset\n";
      for (auto& d : subdevices_) {
        d->reset();
      }
  }

private:

  unsigned hartCount_ = 1;
  unsigned numInterrupts_ = 6;

  uint64_t interrupter_base = 0x9000000;
  uint64_t interrupter_size =    0x4000;
  uint64_t debugger_base    = 0x9004000;
  uint64_t debugger_size    =    0x4000;
  uint64_t scratch_base     = 0x9008000;
  uint64_t scratch_size     =    0x4000;
  
  std::atomic<bool> terminate_ = false;
  std::mutex mutex_;

  std::vector<std::unique_ptr<device> > subdevices_;
  pcg32 rng;
};

