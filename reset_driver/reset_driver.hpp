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
#include "sysmod/smc_xtor/smc_xtor.h"


#define max_hartid 1 // Define the maximum number of harts in the system
#define halt_on_reset false

DECLARE_bool(reset_driver_en);
DECLARE_bool(rst_sram_hold);
DECLARE_bool(rst_debug_hold);
DECLARE_bool(rst_critical_hold);
DECLARE_bool(mid_sim_reset_en);
DECLARE_bool(mid_sim_warm_reset_en);
DECLARE_uint32(reset_pulse_period);
DECLARE_uint32(hold_pulse_period);
DECLARE_uint64(mid_sim_reset_period);
DECLARE_uint64(mid_sim_warm_reset_period);
typedef uint64_t reg_t;


extern "C" {
  void reset_driver_drive_resets(unsigned reset_pins) ;
  void reset_driver_drive_holds(unsigned hold_pins) ;
}

class reset_driver
{
public:
  reset_driver(cvm::topology::loc_t, unsigned);

  struct reset_data_t {
    unsigned txn_type; //0:init 1:regular
    unsigned init_value;
    unsigned reset_num;
    unsigned pulse_width;
    bool release_val;
  };
  struct hold_data_t {
    unsigned pulse_width;
    unsigned pinval;
  };
  struct hold_sigs_t{
    unsigned hold_sigs;
  };
  struct reset_sigs_t{
    unsigned reset_sigs;
  };

  void tick(uint64_t num_clocks){
    if(!FLAGS_reset_driver_en)
      return;

    ticks = num_clocks;
    cvm::log(cvm::FULL, "[Reset Driver] Timer tick {} num_clocks {} \n", num_clocks, ticks);
    processResets();
    processHolds();

    if((ticks >500)&& FLAGS_mid_sim_reset_en && (ticks % FLAGS_mid_sim_reset_period == 0)){
      //perform mid sim cold reset
    }
    if((ticks>500)&& FLAGS_mid_sim_warm_reset_en && (ticks % FLAGS_mid_sim_warm_reset_period == 0)){
      //perform mid sim cold reset
      perform_warm_reset();
      assert_warm_reset_holds();
    }
    if((ticks>500)&& FLAGS_mid_sim_warm_reset_en && (ticks % FLAGS_mid_sim_warm_reset_period == 20)){
      //perform mid sim cold reset
      //perform_warm_reset();
      deassert_warm_reset_holds();
    }
    if(ticks==10){
      perform_warm_reset();
    }
    if(ticks==11){
    assert_warm_reset_holds();
    }
    if(ticks==20){
    deassert_warm_reset_holds();
    }
    if(ticks==28){
    deassert_warm_reset_holds();
    }
    if(ticks ==20){
      smc_xtor::smc_ip_data_t smc_data;
      smc_data.data = 1;
    }
    if(send_update_to_smc & (ticks >10)){
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
            if(ticks == phase1_cycles[i]){
              driveResetPin(i,ResetRelVal[i]);
              phase1_cycles[i] = 0;
            }
             if(ticks == phase2_cycles[i]){
               phase2_cycles[i] = 0;
               driveResetValid[i] = 0;
               driveResetInProgress[i] = false;
            }
        }
    }
   }
   
}

void processHolds()
{
        if (driveHoldValid) {
            if(ticks == hold_cycles){
              driveHoldValid = false;
              hold_cycles = 0;
            }
        }
   
   
}

void set_scope(svScope s) {
     scope_ = s; 
}

void process(const rv_tester_transactions::reset_driver::tick<>& tick);

// Used to assert/deassert a reset_driver pin for given hart.
void driveHoldPulse(hold_data_t t)
{
    if(!driveHoldValid){  
      driveHoldValid = true;
      hold_cycles = ticks + t.pulse_width;
      driveHoldPin(t.pinval);
    }
}
  
// Used to assert/deassert a reset_driver pin for given hart.
void driveResetPulse(reset_data_t t)
   {
    cvm::log(cvm::FULL, "[Reset Driver] driveResetPulse t.resetnum {} txntype {} init {}\n",t.reset_num,t.txn_type,t.init_value);
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
}

void driveHoldSigs(){
    //cvm msg to hold sigs

}

void driveResetPin(unsigned pin, unsigned value){
    reset_sigs_t i;
    i.reset_sigs =  (value << pin);
      cvm::registry::callbacks.push(
       scope(),
       [i]() {
          reset_driver_drive_resets(i.reset_sigs);
        });
 }

void driveHoldPin(unsigned pinvalue){
    //SOME CVM msg
      cvm::registry::callbacks.push(
       scope(),
       [pinvalue]() {
          reset_driver_drive_holds(pinvalue);
        });
 }

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
    svScope scope() { return scope_; }
    unsigned id() { return id_; }

    svScope scope_;
    cvm::topology::loc_t loc_;
    unsigned id_;
    void reset();

   

   std::vector<uint32_t> phase1_cycles;  // Number of cycles counted.
   std::vector<uint32_t> phase2_cycles;  // Number of cycles counted.
   
   std::vector<bool> driveResetValid; // Valid bit for interrupt
   std::vector<bool> driveResetInProgress; // Valid bit for interrupt
   std::vector<bool> ResetRelVal; // Valid bit for interrupt

   unsigned hold_cycles; 
   bool driveHoldValid = false; // Valid bit for interrupt
   bool driveHoldInProgress = false; // Valid bit for interrupt
   std::vector<bool> HoldVal; // Valid bit for interrupt
   bool drive_holds = true;

   bool holds_asseted = false;

   uint64_t ticks = 0;
   bool run_cold_reset_seq = false;
   bool run_warm_reset_seq = false;
   bool send_update_to_smc = true;
   // uint64_t timer_rand_intr = 500;
   uint64_t reset_driver_base = 0x9000000;
  
};

#endif
