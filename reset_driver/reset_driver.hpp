#ifndef _Reset_driver_MODULE_H
#define _Reset_driver_MODULE_H
#include <iostream>
#include <memory>
#include <stdint.h>
#include <set>
#include <vector>
#include <cassert>
#include <unordered_set>

#include "cvm/logger.hpp"
#include "cvm/topology.hpp"
#include "rv_tester_transactions.hpp"
//#include "smc_xtor.h"
#include "sysmod/smc_xtor/smc_xtor.h"
#include "Aplic.hpp"


#define max_hartid 1 // Define the maximum number of harts in the system
#define halt_on_reset false

typedef uint64_t reg_t;


extern "C" {
  void reset_driver_drive_resets(unsigned reset_pins) ;
  void reset_driver_drive_holds(unsigned hold_pins) ;
}

class reset_driver
{
public:
  reset_driver(cvm::topology::loc_t, unsigned);
  //virtual ~reset_driver() = default;

  struct reset_data_t {
    unsigned txn_type; //0:init 1:regular
    unsigned init_value;
    unsigned reset_num;
    unsigned pulse_width;
    bool release_val;
  };
  struct hold_data_t {
    unsigned txn_type; //0:init 1:regular
    unsigned init_value;
    unsigned reset_num;
    unsigned pulse_width;
    bool release_val;
  };
  struct hold_sigs_t{
    unsigned hold_sigs;
  };
  struct reset_sigs_t{
    unsigned reset_sigs;
  };
  // Called for every cycle the JTAG TAP spends in Run-Test/Idle.
  // void run_test_idle();

  void tick(uint64_t advance){
   std::cout<<"[tick] RESET DRIVER STANDALONE adv by "<<advance<<" ticks "<<ticks<<"\n";
   timer_ += advance;
    timer_advance = advance;
    ticks++;
    cvm::log(cvm::FULL, "[Reset Driver] Timer tick {} advance interval {} \n", timer_, timer_advance);
    processResets();
    if(ticks==10){
      perform_warm_reset();
    }
    if(ticks ==20){
      smc_xtor::smc_ip_data_t smc_data;
      smc_data.data = 1;
    }
    if(send_update_to_smc & (ticks >10)){
      std::cout<<"\nreset driver sending update to smc xtor\n";
      auto smc_xtor_loc = cvm::topology::get_from_type("SMC_XTOR", 0);
      cvm::registry::messenger.signal(smc_xtor_loc, smc_xtor::smc_ip_data_t{123});
    }

  }
   
  void processResets()
  {
   if(ticks==2){
    init_pins();
   }else{
       for (size_t i = 0; i < driveResetValid.size(); ++i) {
        if (driveResetValid[i]) {
            //std::cout << i << " ";
            if(ticks == phase1_cycles[i]){
              driveResetPin(i,ResetRelVal[i]);
              phase1_cycles[i] = 0;
            }
             if(ticks == phase2_cycles[i]){
               phase2_cycles[i] = 0;
               driveResetValid[i] = 0;
            }
        }
    }
   }
   
  }
  // Called when one of the attached harts was reset.
  //void proc_reset(unsigned id);

  void set_scope(svScope s) {
    std::cout<<"\n RESET driver setsvscope\n";
     scope_ = s; 
     }
  void process(const rv_tester_transactions::reset_driver::tick<>& tick);

  
  
  // Used to assert/deassert a reset_driver pin for given hart.
   void driveResetPulse(reset_data_t t)
   {
    cvm::log(cvm::FULL, "[Reset Driver] driveResetPulse t.resetnum {} txntype {} iniyt {}\n",t.reset_num,t.txn_type,t.init_value);
    if(t.txn_type == 1){// pulse transaction
       driveResetValid.at(t.reset_num) = true;
       driveResetInProgress.at(t.reset_num) = true;
       phase1_cycles.at(t.reset_num) = ticks+t.pulse_width;
       phase2_cycles.at(t.reset_num) = ticks+(2*t.pulse_width);
       ResetRelVal.at(t.reset_num) = t.release_val;
       driveResetPin(t.reset_num,!t.release_val);
    }
    if(t.txn_type == 0){
      for(unsigned i=0;i<ResetRelVal.size();i++){
       driveResetPin(i,((t.init_value>>i)&0x1));
      }
    }
     //cvm::registry::messenger.signal(loc(), t);
   }
    void driveHoldSigs(){
    //cvm msg to hold sigs

   }
   void driveResetPin(unsigned pin, unsigned value){
    //SOME CVM msg
    std::cout<<"\n Driving PIN: "<<pin<<" with value :"<<value;
    reset_sigs_t i;
    i.reset_sigs =  (value << pin);
      cvm::registry::callbacks.push(
       scope(),
       [i]() {
          reset_driver_drive_resets(i.reset_sigs);
        });
 }
    //cvm::registry::messenger.signal(loc(), i);

  //Check plusarg usage
void checkUsage();
void init_pins();
void perform_cold_reset();
void deassert_warm_reset_holds();
void perform_warm_reset();
void assert_warm_reset_holds();
void update_smc_status(smc_xtor::smc_reset_driver_data_t i);
void wait_for_reset_completion_ack();
private:
  // cvm::file_logger log;
     svScope scope() { return scope_; }
    unsigned id() { return id_; }

    svScope scope_;
    cvm::topology::loc_t loc_;
    unsigned id_;
  void reset();

   
  // std::vector<uint64_t> timeCompare_;  // One per interrupt type.
   std::vector<uint32_t> phase1_cycles;  // Number of cycles counted.
   std::vector<uint32_t> phase2_cycles;  // Number of cycles counted.
   std::vector<bool> driveResetValid; // Valid bit for interrupt
   std::vector<bool> driveResetInProgress; // Valid bit for interrupt
   std::vector<bool> ResetRelVal; // Valid bit for interrupt
  // std::vector<bool> IntrValue_; // Value of interrupt pin
  // std::vector<bool> timerIntPrev_; // Value of interrupt pin
  uint64_t timer_ = 0;
  uint64_t ticks = 0;
  uint64_t timer_advance = 200;
  bool run_cold_reset_seq = false;
  bool run_warm_reset_seq = false;
  bool send_update_to_smc = true;
  // uint64_t timer_rand_intr = 500;
  uint64_t reset_driver_base = 0x9000000;
  
};

#endif
