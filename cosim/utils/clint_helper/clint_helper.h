#pragma once

#include <string>
#include <map>

#include <mutex>
#include <atomic>
#include <thread>
#include <unistd.h>
#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "cvm/messenger.hpp"
#include "cosim/transactions/transactions.hpp"
#include <iostream>
#include "vpi_user.h"
#include "util.h"
#include "svdpi.h"
extern "C" {
  // used by CLINT to assert/deassert timer interrupt
  void sysmod_timer_interrupt(unsigned hartid, unsigned val);

  // used by CLINT to assert/deassert sw interrupt
  void sysmod_sw_interrupt(unsigned hartid, unsigned val);
  
}
class clint_helper {

  template<typename T, typename... Args> void connect() {
    cvm::messenger<T>::connect(
      [this] (const T& v) {
        return this->process(v);
      }
    );
    if constexpr (sizeof...(Args)) {
      connect<Args...>();
    }
  }
  
  public:

    // End-of-test (clint_helper) options:
    // clint_helper=tohost -- Look for mem store to 'tohost' address = success/fail
    
    clint_helper() {
      // Read tohost symbol address from elf
      soft_.push_back(0);
      timeCompare_.push_back(0);
      timerIntPrev_.push_back(0); 
      selfTick(100);
      connect<
        transactions::m_mcmi_store
      >();
    }

  private:

    void process(const transactions::m_mcmi_store& m_mcmi_store);
    bool has_addr(uint64_t val)   const { return val >= 0x2000000 && (val < 0x20c0000); }
    void tick(uint64_t advance) 
  {
   // std::cout<<"\nCLINT HELPER: tick() timer "<<timer_<<" timercompare "<<timeCompare_.at(0)<<"\n";
    //std::lock_guard<std::mutex> lock(mutex_);
    if(run_timer_){
    timer_ += advance;
     //std::cout<<"\nCLINT HELPER: tick() timer "<<timer_<<" timercompare "<<timeCompare_.at(0)<<"\n";
    processTimerInterrupts();
    }
  }


  /// Assert/deassert the timer interrupt for each hart where the
  /// time-compare value is greater-than-or-equal/less-than the timer
  /// value.
  void processTimerInterrupts()
  {
    //svScope scope = svGetScopeFromName("top.tester.sysmod");
    //svSetScope(scope); 
    //std::cout<<"\nCLINT HELPER processTimerInterrupts\n";
    for (unsigned i = 0; i < hartCount_; ++i)
      {
        if(timeCompare_.at(i) >0){
        bool flag = timer_ >= timeCompare_.at(i);
         //std::cout<<"\nCLINT HELPER :processTimerInterrupts  iter: "<<i<<" timerIntPrev_.at(i) "<< timerIntPrev_.at(i) <<" timer "<<timer_<<" timercompare "<<timeCompare_.at(i)<<" flag "<<flag<<"\n";
        //if (timerIntPrev_.at(i) != flag){
        if (flag){
          //timerInterrupt(i, flag);
	 svScope scope = svGetScopeFromName("top.tester.sysmod");
         svSetScope(scope); 
    //cbs.push_back(cb_t{Callback::TIMER_INT, hart, flag});
        //std::cout<<"\nCLINT HELPER : CALL TMR ITP DPI WITH FLAG "<<flag<<"\n";
        sysmod_timer_interrupt(i, flag);
        timerIntPrev_.at(i) = 0;
        timeCompare_.at(i) = 0;
        timer_ = 0;
        flag = 0;
        }
        timerIntPrev_.at(i) = flag;
      }
      }
  }

  // Used to assert/deassert a software interrupt (PIPI) for given hart.
  virtual void softwareInterrupt(unsigned hart, bool flag)
  {
    //svScope scope = svGetScopeFromName("top.tester.sysmod");
    //svSetScope(scope); 
    //cbs.push_back(cb_t{Callback::SW_INT, hart, flag});
    std::cout<<"\nCLINT HELPER : CALL SW ITP DPI WITH FLAG "<<flag<<"\n";
    sysmod_sw_interrupt(hart, flag);
  }

  // Used to assert/deassert a timer interrupt for given hart.
  virtual void timerInterrupt(unsigned hart, bool flag)
  {
    //svScope scope = svGetScopeFromName("top.tester.sysmod");
    //svSetScope(scope); 
    //cbs.push_back(cb_t{Callback::TIMER_INT, hart, flag});
    std::cout<<"\nCLINT HELPER : CALL TMR ITP DPI WITH FLAG "<<flag<<"\n";
    sysmod_timer_interrupt(hart, flag);
  }

  // Start a thread to increment timer after n microseconds.
  void selfTick(useconds_t n);


  private:

    unsigned hartCount_ = 1;
    //From CLINT.h
    std::vector<uint32_t> soft_;  // Software interrupt: one per hart.
    std::vector<uint64_t> timeCompare_;  // One per hart.
    std::vector<bool> timerIntPrev_; // Previous value of timer interrupt
    uint64_t timer_ = 0;
    uint64_t run_timer_ = 0;

    std::atomic<bool> terminate_ = false;
    std::mutex mutex_;

    std::thread timerThread_; 
};
