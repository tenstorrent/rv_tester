// -*- c++ -*-

#pragma once

#include <unistd.h>
#include "subdevice.h"
#include <iostream>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <cmath>
#include <vector>
#include <bitset>
#include "pcg_random.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "aplic_driver.h"
#include "sysmod/trickbox/interrupter.h"

DECLARE_bool(random_aplic_intr);//, false, "Drive random interrups");
DECLARE_bool(debug_aplic_driver);
DECLARE_int32(num_interrupts);
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
    num_ticks++;
    timer_ += advance;
    timer_advance = advance;
    cvm::log(cvm::FULL, "[APLIC_DRIVER] Timer tick {} advance interval {} \n", timer_, timer_advance);
    InterruptLogicStatus();
    processDirectedInterrupts();
    processDelayedRandomInterrupts();
  }

  void reset() override {
    if(FLAGS_random_aplic_intr){
      cvm::log(cvm::MEDIUM, "[APLIC_DRIVER] Enable random interrupts. Mask: {:#x}\n", FLAGS_random_aplic_intr);
      uint32_t rand_num =  (rng() %  2)+1;  //default delay
      if(FLAGS_intr_delay_min){
         rand_num = (rng() % ( FLAGS_intr_delay_max - FLAGS_intr_delay_min + 1)) + FLAGS_intr_delay_min;
      }
      timer_ = 0;

      timer_rand_intr = timer_ + FLAGS_tbox_start_delay +(rand_num*timer_advance);

    }
    //reset all aplic vars to zero
    toggle_cycles = 0;
    num_toggles = 0;
    memset(enables,0,16);
    memset(toggle0,0,16);
    memset(toggle1,0,16);

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
  struct aplic_driver_write_t {
        //std::vector<uint64_t> aplic_pin_values_vec;
        uint64_t aplic_pin_values_vec[16];
  };
protected:

  void processDirectedInterrupts()
  {
    for(size_t i = toggle_in_progress._Find_first(); i < toggle_in_progress.size(); i = toggle_in_progress._Find_next(i) ) {
        
        cycle_count[i]--;
        
        if(cycle_count[i] == 0){
          cycle_count[i] = toggle_cycles;
          toggle_count[i]--;
          //toggle pin
          toggle_aplic_pin(i);
          if(toggle_count[i] == 0){
            toggle_in_progress[i] = 0;
            //based on toggle 0 or toggle 1 set last value
            if(toggle_type[i])
              set_aplic_pin(i);
            else
              clr_aplic_pin(i);

          }
        }

       aplic_driver_write_t aplic_driver_info;
       
       for(int i=0;i<16;i++){
        aplic_driver_info.aplic_pin_values_vec[i] = aplic_pin_values_vec[i];
       }
       cvm::registry::messenger.signal(loc(), aplic_driver_info);
     }
  }

  void InterruptLogicStatus()
  {
    if(FLAGS_debug_aplic_driver){
    cvm::log(cvm::DEBUG, " all set bits in toggle_enable\n");
    for (int i = toggle_enable._Find_first();
         i < int(toggle_enable.size());
         i = toggle_enable._Find_next(i))
        cvm::log(cvm::DEBUG,"{} ",i);
    cvm::log(cvm::DEBUG, "\n");

   cvm::log(cvm::DEBUG, " all set bits in toggle_in_progress\n");
   for (int i = toggle_in_progress._Find_first();
         i < int(toggle_in_progress.size());
         i = toggle_in_progress._Find_next(i))
        cvm::log(cvm::DEBUG,"{} ",i);
   cvm::log(cvm::DEBUG, "\n");
   
   cvm::log(cvm::DEBUG, " all set bits in toggle_type\n");
   for (int i = toggle_type._Find_first();
         i < int(toggle_type.size());
         i = toggle_type._Find_next(i))
        cvm::log(cvm::DEBUG,"{} ",i);
   cvm::log(cvm::DEBUG, "\n");
   
   cvm::log(cvm::DEBUG, " all set bits in aplic_interrupt_bitset\n");
   for (int i = aplic_interrupt_bitset._Find_first();
         i < int(aplic_interrupt_bitset.size());
         i = aplic_interrupt_bitset._Find_next(i))
        cvm::log(cvm::DEBUG,"{} ",i);
   cvm::log(cvm::DEBUG, "\n");
   
   cvm::log(cvm::DEBUG, " all set bits in toggle_in_progress cycle count\n");
   for (int i = toggle_in_progress._Find_first();
         i < int(toggle_in_progress.size());
         i = toggle_in_progress._Find_next(i)){
        cvm::log(cvm::DEBUG, " index: {} toggle_count: {} cycle_count: {} \n",i, toggle_count[i],cycle_count[i]);
         }    
    cvm::log(cvm::DEBUG, "\n");
    }
  }
  /// Assert/deassert the timer interrupt for each hart where the
  /// time-compare value is greater-than-or-equal/less-than the timer
  /// value.
  void processDelayedRandomInterrupts()
  {
    //RANDOM INTR
    if(FLAGS_random_aplic_intr){
      if(timer_ >= timer_rand_intr){
         //aplic_pin_values_vec = {};
         memset(aplic_pin_values_vec, 0, 16);
         //unsigned rand_intr = 0;//1 << rng(5); //select random pin between 0 to 5
         unsigned iter = 1;
         uint64_t values[FLAGS_max_simul_intr];
         memset(values, 0, FLAGS_max_simul_intr);
         if( (FLAGS_max_simul_intr >1 ) && (FLAGS_max_simul_intr < (static_cast<int>(FLAGS_num_interrupts +1)))){
           iter = (rng() % (FLAGS_max_simul_intr )) + 1 ; //gen iter between 1 to max simul instr
         }

	       cvm::log(cvm::HIGH, "[APLIC_DRIVER] Driving  {} interrupts in a cycle \n", iter);
         for (unsigned i = 0; i < iter; ++i) {
           do{
             values[i] = rng() % (FLAGS_num_interrupts) ;
	           cvm::log(cvm::HIGH, "[APLIC_DRIVER] attempting to generate legal interrupts,gen_result  {} \n", values[i]);
	          }while(disable_mask & (1<<values[i]));

           for (unsigned j = 0; j < i; ++j) {
               if (values[i] == values[j]) {
                i--;
                 break;
                }
            }

	         cvm::log(cvm::HIGH, "[APLIC_DRIVER] Driving interrupt  {}  \n", values[i]);
           update_aplic_pins(values[i]);
         }


         uint32_t rand_num =  (rng() % ( FLAGS_intr_delay_max - FLAGS_intr_delay_min + 1)) + FLAGS_intr_delay_min;
         timer_rand_intr = timer_ +(rand_num*timer_advance);
	       cvm::log(cvm::HIGH, "[APLIC_DRIVER] Next random interrupt will be sent at  {}  \n", timer_rand_intr);
         aplic_driver_write_t aplic_driver_info;
         for(int i=0;i<16;i++){
          aplic_driver_info.aplic_pin_values_vec[i] = aplic_pin_values_vec[i];
         }
         cvm::registry::messenger.signal(loc(), aplic_driver_info);
      }
    }

  }
  // Used to assert/deassert a aplic_driver interrupt (PIPI) for given hart.
  virtual void driveInterrupt(unsigned , unsigned , unsigned )
  {
  }

  virtual void update_aplic_pins(uint64_t interrupt_num){
    uint64_t index   =  interrupt_num / 64;
    uint64_t bit_pos =  interrupt_num % 64;
    aplic_pin_values_vec[index] = aplic_pin_values_vec[index] | (1<<bit_pos);
  }
 
  virtual void toggle_aplic_pin(uint64_t interrupt_num){
    uint64_t index   =  interrupt_num / 64;
    uint64_t bit_pos =  interrupt_num % 64;
    aplic_pin_values_vec[index] = aplic_pin_values_vec[index] ^ (1<<bit_pos);
  } 

  virtual void set_aplic_pin(uint64_t interrupt_num){
    uint64_t index   =  interrupt_num / 64;
    uint64_t bit_pos =  interrupt_num % 64;
    aplic_pin_values_vec[index] = aplic_pin_values_vec[index] | (1<<bit_pos);
  } 

  virtual void clr_aplic_pin(uint64_t interrupt_num){
    uint64_t index   =  interrupt_num / 64;
    uint64_t bit_pos =  interrupt_num % 64;
    aplic_pin_values_vec[index] = aplic_pin_values_vec[index] & ~(1<<bit_pos);
  } 

  bool NthBitValue(uint64_t input_data, uint64_t n)
  {
    if (input_data & (1ULL << n))
       return true ;
    else
        return false;
  }
  //Check plusarg usage
  void checkUsage();

private:
  
  
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
  
  //APLIC MODEL
  uint32_t toggle_cycles = 0;
  uint32_t num_toggles = 0;
  uint64_t num_ticks = 0;
  uint64_t enables[16] = {0};
  uint64_t toggle0[16] = {0};
  uint64_t toggle1[16] = {0};
  uint64_t aplic_pin_values_vec[16] = {0};
  std::bitset<1024> toggle_in_progress;
  std::bitset<1024> toggle_enable;
  std::bitset<1024> toggle_type;
  std::bitset<1024> aplic_interrupt_bitset;
  uint32_t toggle_count[1024];
  uint32_t cycle_count[1024];

  pcg_extras::seed_seq_from<std::random_device> seed_source;
  pcg32 rng;
};

