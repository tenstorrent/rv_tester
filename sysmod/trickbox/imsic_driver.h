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
#include "imsic_driver.h"

DECLARE_int32(imsic_intr_delay_min);//, 4, "Minimum Delay between 2 consecutive interrupts");
DECLARE_int32(imsic_intr_delay_max);//, 7, "Maximum Delay between 2 consecutive interrupts");
DECLARE_bool(random_imsic_intr);//, false, "Drive random interrups");
DECLARE_bool(disable_m_imsic_intr);
DECLARE_bool(disable_s_imsic_intr);
DECLARE_bool(disable_vs_imsic_intr);
DECLARE_bool(disable_random_hart_imsic_intr);
DECLARE_int32(imsic_intr_threshold);
DECLARE_int32(imsic_intr_start_delay);
DECLARE_int32(seed);
// Define a core local interruptor (imsic_driver) at the given address
// and for the given hart count. The size will be 48k bytes.
class imsic_driver : public subdevice
{
public:

  /// Define a imsic_driver device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  imsic_driver(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);

  // Destructor.
  virtual ~imsic_driver();

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
  /// No-op if address is outside the range of this imsic_driver or if
  /// address is not properly aligned.
  cvm::messenger::task<void> read(uint64_t addr, size_t length, data_t& data);

   void read_dev(uint64_t addr, size_t length,  data_t& data) override;
  // Write to this imsic_driver. Call softwareInterrupt with flag set to 0/1
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
    cvm::log(cvm::FULL, "[imsic_intr Driver] Timer tick :  {} advance interval {} \n",timer_,timer_advance);
    processDelayedRandomInterrupts();

  }
  // Used to assert/deassert a interrupter interrupt (PIPI) for given hart.
  virtual void driveMSIInterrupt(unsigned t_data)
  {
    uint8_t interrupt_num = t_data & 0xfff;
    unsigned interrupt_file = (t_data>>12) & 0xf;
    unsigned interrupt_hart = (t_data>>16) & 0xfff;
    unsigned vs_id = (t_data>>28) & 0xfff;

    std::cout<<"Requested imsic_intr interrupt num "<<(uint32_t)interrupt_num <<" interrupt file: "<<interrupt_file <<" Interrupt hart:"<< interrupt_hart <<" hypervisor/supervisor id : "<<vs_id<<"\n";
    
    uint32_t addr1 = 0x900;
    if(interrupt_file == 0x0){
       addr1 = msi_m_file_addr;
    }else if(interrupt_file == 0x01){
       addr1 = msi_v_file_addr;
    }else if(interrupt_file == 0x02){
       addr1 = msi_vs_file_addr;
    }else{
       cvm::log(cvm::ERROR, "[imsic_intr driver] Wrong interrupt file specified\n");
    }
    uint32_t length1 = 1;
    std::vector<uint8_t> data1 = {interrupt_num};
    //std::vector<uint8_t> data1 = {0xba,0xad,0xf0,0x12};
    std::vector<bool> strb1 = {1,1,1,1,1,1,1};

    cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr1, length1, data1, strb1});
 
  }

  void reset() override {
      std::cout<<"[TRICKBOX]: Reset imsic_driver\n";
    if(FLAGS_random_imsic_intr){
      std::cout<<"[TRICKBOX]: Enable random IMSIC interrupts "<<FLAGS_random_imsic_intr<<"\n";
      uint32_t rand_num =  (rng() %  2)+1;  //default delay
      if(FLAGS_imsic_intr_delay_min){
         rand_num = (rng() % ( FLAGS_imsic_intr_delay_max - FLAGS_imsic_intr_delay_min + 1)) + FLAGS_imsic_intr_delay_min;
      }
      timer_ = 0;

      timer_rand_intr = timer_ + FLAGS_imsic_intr_start_delay +(rand_num*timer_advance);

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
    
    //RANDOM imsic_intr
    if(FLAGS_random_imsic_intr){
      if(timer_ >= timer_rand_intr){
        unsigned intr_num = 1;
        unsigned intr_file = 0;
        unsigned intr_hart = 0;
        unsigned intr_vs_id = 0;
	unsigned disable_flags = FLAGS_disable_m_imsic_intr |( FLAGS_disable_s_imsic_intr <<1) |( FLAGS_disable_vs_imsic_intr <<2);
        if(disable_flags == 0x7)
	      cvm::log(cvm::ERROR, "[Trickbox] Cant generate IMSIC interrupts when all interrupts are disabled \n");

        unsigned values[FLAGS_imsic_intr_threshold];
        memset(values, 0, FLAGS_imsic_intr_threshold);
        intr_num = (rng() % (FLAGS_imsic_intr_threshold )) ; //gen iter between 1 to max simul instr
	do{
        intr_file = (rng() % (3 )) ; //gen iter between 1 to max simul instr
	}while(((1<< intr_file)& disable_flags) != 0);
	


	if(!FLAGS_disable_random_hart_imsic_intr)
          intr_hart = (rng() % (FLAGS_imsic_intr_threshold )) ; //gen iter between 1 to max simul instr
	if(!FLAGS_disable_vs_imsic_intr)
          intr_vs_id = (rng() % (FLAGS_imsic_intr_threshold )) ; //gen iter between 1 to max simul instr
       
        intr_num = intr_num |(intr_file<<12)|(intr_hart<<16)|(intr_vs_id<<28);
        cvm::log(cvm::HIGH, "[Trickbox] Driving imsic_intr {} interrupts in a cycle \n", intr_num);
        driveMSIInterrupt(intr_num); 
        uint32_t rand_num =  (rng() % ( FLAGS_imsic_intr_delay_max - FLAGS_imsic_intr_delay_min + 1)) + FLAGS_imsic_intr_delay_min;
        timer_rand_intr = timer_ +(rand_num*timer_advance);
	      cvm::log(cvm::HIGH, "[Trickbox] Next random imsic_intr will be sent at  {}  \n", timer_rand_intr);
      }
    }

  }
 

  // Start a thread to increment timer after n microseconds.
  void selfTick(useconds_t n);
  //Check plusarg usage
  void checkUsage();

private:
 //unsigned numMSIs_ = 6;
  cvm::topology::loc_t axi_mst_loc_l;
  std::vector<uint64_t> timeCompare_;  // One per interrupt type.
  std::vector<uint32_t> IntrHart_;  // Hart to be interrupted.
  std::vector<bool> delayedRandomIntValid_; // Valid bit for interrupt
  std::vector<bool> IntrValue_; // Value of interrupt pin
  std::vector<bool> timerIntPrev_; // Value of interrupt pin
  uint64_t timer_           = 0;
  uint64_t timer_advance    = 200;
  uint64_t timer_rand_intr  = 500;
  uint64_t imsic_driver_base  = 0x9070000;
  uint32_t msi_m_file_addr  = 0x1000000;
  uint32_t msi_v_file_addr  = 0x1800000;
  uint32_t msi_vs_file_addr = 0x1800000;
  //IMSIC_ADDR_TARGET_M   = 32'h0100_0000,//32'h0800_0000;
  // IMSIC_ADDR_TARGET_S   = 32'h0180_0000,//32'h0A00_0000;

  std::atomic<bool> terminate_ = false;
  std::mutex mutex_;

  std::thread timerThread_;
  pcg_extras::seed_seq_from<std::random_device> seed_source;
  pcg32 rng;
};

