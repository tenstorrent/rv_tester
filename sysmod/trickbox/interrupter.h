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
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "interrupter.h"

DECLARE_int32(intr_delay_min);//, 4, "Minimum Delay between 2 consecutive interrupts");
DECLARE_int32(intr_delay_max);//, 7, "Maximum Delay between 2 consecutive interrupts");
DECLARE_bool(random_intr);//, false, "Drive random interrups");
DECLARE_int32(max_simul_intr ); 
DECLARE_int32(tbox_start_delay); 
// Define a core local interruptor (interrupter) at the given address
// and for the given hart count. The size will be 48k bytes.
class interrupter : public device
{
public:

  /// Define a interrupter device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  interrupter(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc);

  // Destructor.
  virtual ~interrupter();

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
  /// No-op if address is outside the range of this interrupter or if
  /// address is not properly aligned.
  virtual void read(uint64_t addr, size_t length, data_t& data) override;

  // Write to this interrupter. Call softwareInterrupt with flag set to 0/1
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
    //std::cout<<"[interrupter]: tick\n";
    std::lock_guard<std::mutex> lock(mutex_);
    timer_ += advance;
    timer_advance = advance;
    processDelayedRandomInterrupts();
  }

  void reset(){
      std::cout<<"[TRICKBOX]: Reset Interrupter\n";
    if(FLAGS_random_intr){
      std::cout<<"[TRICKBOX]: Enable random interrupts "<<FLAGS_random_intr<<"\n";
      uint32_t rand_num =  (rng() %  2)+1;  //default delay 
      if(FLAGS_intr_delay_min){
         rand_num = (rng() % ( FLAGS_intr_delay_max - FLAGS_intr_delay_min + 1)) + FLAGS_intr_delay_min;
      }
      timer_ = 0;
      
      timer_rand_intr = timer_ + FLAGS_tbox_start_delay +(rand_num*timer_advance);

    }

  }

  struct interrupt_t {
    unsigned hart;
    unsigned intr_select;
    unsigned intr_value;
  };

protected:

  /// Assert/deassert the timer interrupt for each hart where the
  /// time-compare value is greater-than-or-equal/less-than the timer
  /// value.
  void processDelayedRandomInterrupts()
  {
    for (unsigned i = 0; i < numInterrupts_; ++i)
      {
        bool flag = timer_ >= timeCompare_.at(i);
        if ((delayedRandomIntValid_.at(i) == 1)&&(flag)){
          unsigned intr_select, intr_value;
          intr_select = 1<<i;
          intr_value = IntrValue_.at(i)<<i;
          cvm::registry::messenger.signal(loc(), interrupt_t{IntrHart_.at(i), intr_select, intr_value});
          delayedRandomIntValid_.at(i) = 0;
          timeCompare_.at(i) = 0xffffffffffffffff;
        }
      }
    //RANDOM INTR
    if(FLAGS_random_intr){
      if(timer_ >= timer_rand_intr){
         unsigned rand_intr = 0;//1 << rng(5); //select random pin between 0 to 5
         unsigned iter = 1;
         unsigned values[FLAGS_max_simul_intr] = {};
         if( (FLAGS_max_simul_intr >1 ) && (FLAGS_max_simul_intr < (numInterrupts_ +1))){
           iter = (rng() % (FLAGS_max_simul_intr )) + 1 ; //gen iter between 1 to max simul instr
         } 
         
         //std::cout<<"[TRICKBOX]: iteration interrupts "<<iter<<"\n";
         for (int i = 0; i < iter; ++i) {
           values[i] = rng() % (numInterrupts_) ;
           for (int j = 0; j < i; ++j) {
               if (values[i] == values[j]) {
                i--;
                 break;
                }
            }
          
          rand_intr =  rand_intr |(1<<values[i]);

         //std::cout<<"[TRICKBOX]: UNIQRAND random interrupts "<< values[i]<<" rand intr :"<<std::hex<<rand_intr<<"\n";
         }


         
         //std::cout<<"[TRICKBOX]: Drive random interrupts "<<rand_intr<<"\n";
         cvm::registry::messenger.signal(loc(), interrupt_t{0, rand_intr, rand_intr});
         uint32_t rand_num =  (rng() % ( FLAGS_intr_delay_max - FLAGS_intr_delay_min + 1)) + FLAGS_intr_delay_min;
         timer_rand_intr = timer_ +(rand_num*timer_advance);
      }
    }

  }
  // Used to assert/deassert a interrupter interrupt (PIPI) for given hart.
  virtual void driveInterrupt(unsigned hart, unsigned intr_select, unsigned intr_value)
  {
    cvm::registry::messenger.signal(loc(), interrupt_t{hart, intr_select, intr_value});
  }

  // Start a thread to increment timer after n microseconds.
  void selfTick(useconds_t n);

private:

  unsigned hartCount_ = 1;
  unsigned numInterrupts_ = 6;

  std::vector<uint64_t> timeCompare_;  // One per interrupt type.
  std::vector<uint32_t> IntrHart_;  // Hart to be interrupted.
  std::vector<bool> delayedRandomIntValid_; // Valid bit for interrupt
  std::vector<bool> IntrValue_; // Value of interrupt pin
  std::vector<bool> timerIntPrev_; // Value of interrupt pin
  uint64_t timer_ = 0;
  uint64_t timer_advance = 200;
  uint64_t timer_rand_intr = 500;
  uint64_t interrupter_base = 0x9000000;
  uint64_t interrupter_size = 0x4000;
  
  std::atomic<bool> terminate_ = false;
  std::mutex mutex_;

  std::thread timerThread_;
  pcg_extras::seed_seq_from<std::random_device> seed_source;
  pcg32 rng;
};

