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
DECLARE_int32(ITP_DELAY_MIN);//, 4, "Minimum Delay between 2 consecutive interrupts");
DECLARE_int32(ITP_DELAY_MAX);//, 7, "Maximum Delay between 2 consecutive interrupts");
DECLARE_bool(RANDOM_ITP);//, false, "Drive random interrups");
// Define a core local interruptor (trickbox) at the given address
// and for the given hart count. The size will be 48k bytes.
class trickbox : public device
{
public:

  /// Define a trickbox device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  trickbox(const std::string& tag, uint64_t addr, unsigned hartCount);

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

  // Write to this trickbox. Call softwareInterrupt with flag set to 0/1
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
    timer_advance = advance;
    //std::cout<<"Timer is :"<<timer_<<"\n";
    processDelayedRandomInterrupts(cbs);
  }

protected:

  /// Assert/deassert the timer interrupt for each hart where the
  /// time-compare value is greater-than-or-equal/less-than the timer
  /// value.
  void processDelayedRandomInterrupts(cbs_t& cbs)
  {
    //std::cout<<"\nPROCESS DELAYED ITP\n";
    for (unsigned i = 0; i < numInterrupts_; ++i)
      {
        bool flag = timer_ >= timeCompare_.at(i);
        //std::cout<<"PROCESS DELAYED ITP: "<<i<<" timer_ "<<timer_<<" timercompare_ "<<timeCompare_.at(i)<<" valid: "<<delayedRandomIntValid_.at(i)<<"\n";
        if ((delayedRandomIntValid_.at(i) == 1)&&(flag)){
          unsigned int_vec, int_vec_val;
          int_vec = 1<<i;
          int_vec_val = IntValue_.at(i)<<i;
          //cbs.push_back(cb_t{Callback::TRICKBOX_INT,IntHart_.at(i) , i, IntValue_.at(i)});
          cbs.push_back(cb_t{Callback::TRICKBOX_INT,IntHart_.at(i) , int_vec, int_vec_val});
          //std::cout<<"PROCESS DELAYED ITP DRIVE: "<<i<<" HART_ "<< IntHart_.at(i) <<" Value "<< IntValue_.at(i)<<"\n";
          delayedRandomIntValid_.at(i) = 0;
          timeCompare_.at(i) = 0xffffffffffffffff;
        }
      }
      //RANDOM ITP
      
    //std::cout<<"\nPROCESS RANDOM ITP timer "<<timer_<<" timer_delay "<<timer_rand_itp<<"\n";
    if(FLAGS_RANDOM_ITP){
    if(timer_ >= timer_rand_itp){
       unsigned rand_itp = 1 << rng(5);
       //std::cout<<"\nRandom ITP to Drive: "<<rand_itp<<"\n";
       cbs.push_back(cb_t{Callback::TRICKBOX_INT,0 , rand_itp, rand_itp});
       uint32_t rand_num =  (rng() % ( FLAGS_ITP_DELAY_MAX - FLAGS_ITP_DELAY_MIN + 1)) + FLAGS_ITP_DELAY_MIN;
       //std::cout<<"RAND NUM "<<rand_num<<"\n";
       timer_rand_itp = timer_ +(rand_num*timer_advance);
       //std::cout<<"RAND NUM "<<rand_num<<" timer "<<timer_<<" timer_rand "<<timer_rand_itp<<"\n";
    }
    }

  }

  // Used to assert/deassert a software interrupt (PIPI) for given hart.
  virtual void softwareInterrupt(unsigned hart, bool flag, cbs_t& cbs)
  {
    cbs.push_back(cb_t{Callback::SW_INT, hart, flag});
  }
  
  // Used to assert/deassert a trickbox interrupt (PIPI) for given hart.
  virtual void trickboxInterrupt(unsigned hart, unsigned itp, unsigned itp_val, cbs_t& cbs)
  {
    cbs.push_back(cb_t{Callback::TRICKBOX_INT, hart, itp, itp_val});
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
  unsigned numInterrupts_ = 6;

  std::vector<uint32_t> soft_;  // Software interrupt: one per hart.
  std::vector<uint64_t> timeCompare_;  // One per interrupt type.
  std::vector<uint32_t> IntHart_;  // Hart to be interrupted.
  std::vector<bool> delayedRandomIntValid_; // Valid bit for interrupt
  std::vector<bool> IntValue_; // Value of interrupt pin
  std::vector<bool> timerIntPrev_; // Value of interrupt pin
  uint64_t timer_ = 0;
  uint64_t timer_advance = 200;
  uint64_t timer_rand_itp = 500;
  
  std::atomic<bool> terminate_ = false;
  std::mutex mutex_;

  std::thread timerThread_;
  pcg_extras::seed_seq_from<std::random_device> seed_source;
  pcg32 rng;
};

