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
#include "msi_driver.h"

DECLARE_int32(msi_delay_min);//, 4, "Minimum Delay between 2 consecutive interrupts");
DECLARE_int32(msi_delay_max);//, 7, "Maximum Delay between 2 consecutive interrupts");
DECLARE_bool(random_msi);//, false, "Drive random interrups");
DECLARE_int32(max_simul_msi );
DECLARE_int32(msi_start_delay);
DECLARE_int32(seed);
DECLARE_bool(disable_ssip);
DECLARE_bool(disable_msip);
DECLARE_bool(disable_stip);
DECLARE_bool(disable_mtip);
DECLARE_bool(disable_seip);
DECLARE_bool(disable_meip);
// Define a core local interruptor (msi_driver) at the given address
// and for the given hart count. The size will be 48k bytes.
class msi_driver : public subdevice
{
public:

  /// Define a msi_driver device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  msi_driver(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);

  // Destructor.
  virtual ~msi_driver();

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
  /// No-op if address is outside the range of this msi_driver or if
  /// address is not properly aligned.
  cvm::messenger::task<void> read(uint64_t addr, size_t length, data_t& data);

  // Write to this msi_driver. Call softwareInterrupt with flag set to 0/1
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
    cvm::log(cvm::FULL, "[Trickbox] Timer tick :  {} advance interval {} \n",timer_,timer_advance);
    processDelayedRandomInterrupts();
  }

  void reset() override {
      std::cout<<"[TRICKBOX]: Reset msi_driver\n";
    if(FLAGS_random_msi){
      std::cout<<"[TRICKBOX]: Enable random interrupts "<<FLAGS_random_msi<<"\n";
      uint32_t rand_num =  (rng() %  2)+1;  //default delay
      if(FLAGS_msi_delay_min){
         rand_num = (rng() % ( FLAGS_msi_delay_max - FLAGS_msi_delay_min + 1)) + FLAGS_msi_delay_min;
      }
      timer_ = 0;

      timer_rand_intr = timer_ + FLAGS_msi_start_delay +(rand_num*timer_advance);

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
    //RANDOM msi
    if(FLAGS_random_msi){
      if(timer_ >= timer_rand_intr){
         unsigned rand_intr = 0;//1 << rng(5); //select random pin between 0 to 5
         unsigned iter = 1;
         unsigned values[FLAGS_max_simul_msi];
         memset(values, 0, FLAGS_max_simul_msi);
         if( (FLAGS_max_simul_msi >1 ) && (FLAGS_max_simul_msi < (static_cast<int>(numInterrupts_ +1)))){
           iter = (rng() % (FLAGS_max_simul_msi )) + 1 ; //gen iter between 1 to max simul instr
         }

	 cvm::log(cvm::HIGH, "[Trickbox] Driving  {} interrupts in a cycle \n", iter);
         for (unsigned i = 0; i < iter; ++i) {
           do{
             values[i] = rng() % (numInterrupts_) ;
	     cvm::log(cvm::HIGH, "[Trickbox] attempting to genertae legal interrupts,gen_result  {} \n", values[i]);
	   }while(disable_mask & (1<<values[i]));

           for (unsigned j = 0; j < i; ++j) {
               if (values[i] == values[j]) {
                i--;
                 break;
                }
            }

	  cvm::log(cvm::HIGH, "[Trickbox] Driving interrupt  {}  \n", values[i]);
          rand_intr =  rand_intr |(1<<values[i]);

          rand_intr = rand_intr & disable_mask_neg;
	  cvm::log(cvm::HIGH, "[Trickbox] Send  interrupt vec to sysmod  {:#x}  \n", rand_intr);

         }


	 cvm::log(cvm::HIGH, "[Trickbox] Send  sig to  sysmod  {:#x}  \n", rand_intr);
         cvm::registry::messenger.signal(loc(), interrupt_t{0, rand_intr, rand_intr});
         uint32_t rand_num =  (rng() % ( FLAGS_msi_delay_max - FLAGS_msi_delay_min + 1)) + FLAGS_msi_delay_min;
         timer_rand_intr = timer_ +(rand_num*timer_advance);
	 cvm::log(cvm::HIGH, "[Trickbox] Next random interrupt will be sent at  {}  \n", timer_rand_intr);
      }
    }

  }
  // Used to assert/deassert a msi_driver interrupt (PIPI) for given hart.
  virtual void driveInterrupt(unsigned hart, unsigned intr_select, unsigned intr_value)
  {
    cvm::registry::messenger.signal(loc(), interrupt_t{hart, intr_select, intr_value});
  }

  // Start a thread to increment timer after n microseconds.
  void selfTick(useconds_t n);
  //Check plusarg usage
  void checkUsage();

private:
  unsigned numInterrupts_ = 6;
  cvm::topology::loc_t axi_mst_loc_l;
  std::vector<uint64_t> timeCompare_;  // One per interrupt type.
  std::vector<uint32_t> IntrHart_;  // Hart to be interrupted.
  std::vector<bool> delayedRandomIntValid_; // Valid bit for interrupt
  std::vector<bool> IntrValue_; // Value of interrupt pin
  std::vector<bool> timerIntPrev_; // Value of interrupt pin
  uint64_t timer_ = 0;
  uint64_t timer_advance = 200;
  uint64_t timer_rand_intr = 500;
  uint64_t msi_driver_base = 0x9000000;
  uint64_t disable_mask = 0;
  uint64_t disable_mask_neg = 0;

  std::atomic<bool> terminate_ = false;
  std::mutex mutex_;

  std::thread timerThread_;
  pcg_extras::seed_seq_from<std::random_device> seed_source;
  pcg32 rng;
};

