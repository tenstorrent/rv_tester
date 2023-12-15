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
#include "aplic_driver.h"

DECLARE_int32(intr_delay_min);//, 4, "Minimum Delay between 2 consecutive interrupts");
DECLARE_int32(intr_delay_max);//, 7, "Maximum Delay between 2 consecutive interrupts");
DECLARE_bool(random_intr);//, false, "Drive random interrups");
DECLARE_int32(num_wires);
DECLARE_int32(toggle_prob);

// Define a core local interruptor (aplic_driver) at the given address
// and for the given hart count. The size will be 48k bytes.
class aplic_driver : public subdevice
{
public:

  /// Define a aplic_driver device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  aplic_driver(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc);

  // Destructor.
  virtual ~aplic_driver();

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
  /// No-op if address is outside the range of this aplic_driver or if
  /// address is not properly aligned.
  cvm::messenger::task<void> read(uint64_t addr, size_t length, data_t& data);
   void read_dev(uint64_t addr, size_t length,  data_t& data) override;
  // Write to this aplic_driver. Call softwareInterrupt with flag set to 0/1
  // if a hart software interrupt entry is written. Update time
  // compare and call timerInterrupt if a hart time compare entry is
  // written. Call timerInterrupt on every hart if timer is written.
  //
  // This is a no-op if address is not aligned, if length is not 4 for
  // software interrupt entries, if length is not 8 for
  // timer/time-compare entries.
  virtual void write(uint64_t addr, size_t length, const data_t& data,
                      const strb_t& strb) override;

  virtual void tick(uint64_t advance) override
  {
    std::lock_guard<std::mutex> lock(mutex_);
    timer_ += advance;
    timer_advance = advance;
    cvm::log(cvm::FULL, "[Trickbox] Timer tick {} advance interval {} \n", timer_, timer_advance);
    processDelayedRandomInterrupts();
  }

  void reset() override {
    if(FLAGS_random_intr){
      cvm::log(cvm::MEDIUM, "[Trickbox] Enable random interrupts. Mask: {:#x}\n", FLAGS_random_intr);
      uint32_t rand_num =  (rng() %  2)+1;  //default delay
      if(FLAGS_intr_delay_min){
         rand_num = (rng() % ( FLAGS_intr_delay_max - FLAGS_intr_delay_min + 1)) + FLAGS_intr_delay_min;
      }
      timer_ = 0;

      timer_rand_intr = timer_ + FLAGS_tbox_start_delay +(rand_num*timer_advance);

    }

  }
  typedef enum { APLIC_CFG,APLIC_EN,APLIC_T0,APLIC_T1 } aplic_tx_type_e;
  struct aplic_data_t {
    aplic_tx_type_e tx_type;
    unsigned toggle_cycles;
    unsigned num_toggles;
    uint64_t enables_offset;
    uint64_t enables_value;
    uint64_t toggle0_offset;
    uint64_t toggle0_value;
    uint64_t toggle1_offset;
    uint64_t toggle1_value;
  };

protected:

  /// Assert/deassert the timer interrupt for each hart where the
  /// time-compare value is greater-than-or-equal/less-than the timer
  /// value.
  void processDelayedRandomInterrupts()
  {
    //RANDOM INTR
    if(FLAGS_random_intr){
      if(timer_ >= timer_rand_intr){
         
      }
    }

  }
  // Used to assert/deassert a aplic_driver interrupt (PIPI) for given hart.
  virtual void driveInterrupt(unsigned hart, unsigned intr_select, unsigned intr_value)
  {
    cvm::registry::messenger.signal(loc(), aplic_data_t{hart, intr_select, intr_value});
  }

  // Start a thread to increment timer after n microseconds.
  void selfTick(useconds_t n);
  //Check plusarg usage
  void checkUsage();

private:
  unsigned numInterrupts_ = 6;
  
  
  std::vector<uint64_t> timeCompare_;  // One per interrupt type.
  std::vector<uint32_t> IntrHart_;  // Hart to be interrupted.
  std::vector<bool> delayedRandomIntValid_; // Valid bit for interrupt
  std::vector<bool> IntrValue_; // Value of interrupt pin
  std::vector<bool> timerIntPrev_; // Value of interrupt pin
  uint64_t timer_ = 0;
  uint64_t timer_advance = 200;
  uint64_t timer_rand_intr = 500;
  uint64_t aplic_driver_base = 0x9000000;
  uint64_t disable_mask = 0;
  uint64_t disable_mask_neg = 0;

  std::atomic<bool> terminate_ = false;
  std::mutex mutex_;

  std::thread timerThread_;
  pcg_extras::seed_seq_from<std::random_device> seed_source;
  pcg32 rng;
};

