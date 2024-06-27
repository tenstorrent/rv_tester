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
DECLARE_uint32(num_resets);
DECLARE_uint32(num_warm_resets);
DECLARE_uint32(reset_pulse_period);
DECLARE_uint32(hold_pulse_period);
DECLARE_uint64(mid_sim_reset_period);
DECLARE_uint64(mid_sim_warm_reset_period);
typedef uint64_t reg_t;


extern "C" {
  void reset_driver_drive_resets(unsigned reset_pins) ;
  void reset_driver_drive_holds(unsigned hold_pins) ;
  void reset_driver_drive_force_clk(unsigned force_clk_pin) ;
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
  struct forceclk_data_t {
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
    processforceclk();

    if(cold_boot_reset_done && (ticks>500) && FLAGS_mid_sim_reset_en){ //perform mid sim cold reset
      if(num_resets >= FLAGS_num_resets)
        return;
      else {
        cvm::log(cvm::FULL, "[Reset Driver] Ticks={}, Inside the mid-sim cold reset function\n",ticks); 
        cvm::log(cvm::FULL, "[Reset Driver] Progress flags are, force_clk_assert_in_progress={}, reset_sequence_in_progress={}\n",force_clk_assert_in_progress,reset_sequence_in_progress);
        cvm::log(cvm::FULL, "[Reset Driver] Ticks are, force_clk_assert_ticks={}, reset_sequence_ticks={}\n",force_clk_assert_ticks,reset_sequence_ticks);

        if ((force_clk_assert_in_progress == false) && (reset_sequence_in_progress == false)) {
          cvm::log(cvm::NONE, "[Reset Driver] Mid-sim cold reset - Force clocks asserted\n");
          force_clk_assert_in_progress = true;
          force_clk_assert_ticks = ticks;

          assert_force_clock();
        }

        if (ticks == (force_clk_assert_ticks + FLAGS_hold_pulse_period) && (force_clk_assert_in_progress == true)){
          cvm::log(cvm::NONE, "[Reset Driver] Mid-sim cold reset - Reset Sequece Started\n");
          force_clk_assert_in_progress = false;
          reset_sequence_in_progress = true;
          reset_sequence_ticks = ticks;

          perform_cold_reset();
        }
        
        if ((cold_boot_reset_done == true) && (reset_sequence_in_progress == true)) {
          reset_sequence_in_progress = false;
          num_resets++;
          cvm::log(cvm::NONE, "[Reset Driver] Cold Boot Reset done\n");

          auto smc_xtor_loc = cvm::topology::get_from_type("SMC_XTOR", 0);
          cvm::registry::messenger.signal(smc_xtor_loc, smc_xtor::smc_ip_data_t{1}); 

          deassert_force_clock();
        }
      }
    }

    if(cold_boot_reset_done && (ticks>500) && FLAGS_mid_sim_warm_reset_en){ //perform mid sim warm reset
      if(num_warm_resets >= FLAGS_num_warm_resets)
        return;
      else {
        cvm::log(cvm::FULL, "[Reset Driver] Ticks={}, Inside the mid-sim warm reset function\n",ticks); 
        cvm::log(cvm::FULL, "[Reset Driver] Progress flags are, holds_assert_in_progress={}, reset_sequence_in_progress={}, holds_deassert_in_progress={}\n",holds_assert_in_progress,reset_sequence_in_progress,holds_deassert_in_progress);
        cvm::log(cvm::FULL, "[Reset Driver] Ticks are, holds_assert_ticks={}, reset_sequence_ticks={}, holds_deassert_ticks={}\n",holds_assert_ticks,reset_sequence_ticks,holds_deassert_ticks);
        
        if ((ticks > (holds_deassert_ticks + FLAGS_hold_pulse_period)) && (force_clk_assert_in_progress == false) && (holds_assert_in_progress == false) && (reset_sequence_in_progress == false) && (holds_deassert_in_progress == false)) {
          cvm::log(cvm::NONE, "[Reset Driver] Mid-sim warm reset - Force clocks asserted\n");
          force_clk_assert_in_progress = true;
          force_clk_assert_ticks = ticks;

          assert_force_clock();
        }

        if (ticks == (force_clk_assert_ticks + FLAGS_hold_pulse_period) && (force_clk_assert_in_progress == true)){
          cvm::log(cvm::NONE, "[Reset Driver] Mid-sim warm reset - Holds asserted\n");
          force_clk_assert_in_progress = false;
          holds_assert_in_progress = true;
          holds_assert_ticks = ticks;

          assert_warm_reset_holds();
        }
        
        if (ticks == (holds_assert_ticks + FLAGS_hold_pulse_period) && (holds_assert_in_progress == true)) {
          cvm::log(cvm::NONE, "[Reset Driver] Mid-sim warm reset - Reset Sequece Started\n");
          holds_assert_in_progress = false;
          reset_sequence_in_progress = true;
          reset_sequence_ticks = ticks;

          perform_warm_reset();
        }
        
        if (ticks == (reset_sequence_ticks + 2*FLAGS_reset_pulse_period) && (reset_sequence_in_progress == true)) {
          cvm::log(cvm::NONE, "[Reset Driver] Mid-sim warm reset - Holds deasserted\n");
          reset_sequence_in_progress = false;
          holds_deassert_in_progress = true;
          holds_deassert_ticks = ticks;

          auto smc_xtor_loc = cvm::topology::get_from_type("SMC_XTOR", 0);
          cvm::registry::messenger.signal(smc_xtor_loc, smc_xtor::smc_ip_data_t{2});
          deassert_warm_reset_holds();
        }

        if (ticks == (holds_deassert_ticks + FLAGS_hold_pulse_period) && (holds_deassert_in_progress == true)) {
          cvm::log(cvm::NONE, "[Reset Driver] Mid-sim warm reset done\n");
          holds_deassert_in_progress = false;
          num_warm_resets++;

          deassert_force_clock();
        }
      }
    }

    if((ticks>=5) && !cold_boot_reset_done) { //perform cold boot bringup
      if ((force_clk_assert_in_progress == false) && (reset_sequence_in_progress == false)) {
        cvm::log(cvm::NONE, "[Reset Driver] Cold Boot Reset - Force clocks asserted\n");
        force_clk_assert_in_progress = true;
        force_clk_assert_ticks = ticks;

        assert_force_clock();
      }

      if (ticks == (force_clk_assert_ticks + FLAGS_hold_pulse_period) && (force_clk_assert_in_progress == true)){
        cvm::log(cvm::NONE, "[Reset Driver] Cold Boot Reset - Reset Sequece Started\n");
        force_clk_assert_in_progress = false;
        reset_sequence_in_progress = true;
        reset_sequence_ticks = ticks;

        perform_cold_reset();
      }
      
      if ((cold_boot_ack_from_smc_xtor == true) && (reset_sequence_in_progress == true)) {
        reset_sequence_in_progress = false;
        cold_boot_reset_done = true;
        cvm::log(cvm::NONE, "[Reset Driver] Cold Boot Reset done\n");

        deassert_force_clock();
      }
    }
}
   
void processResets()
{
  // cvm::log(cvm::NONE, "[Reset Driver] Timer tick: {}\n", ticks);
   if(ticks==3){
    cvm::log(cvm::NONE, "[Reset Driver] Ticks==2 starting init of pins\n");
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

void processforceclk()
{
  if (driveforceclkValid) {
      if(ticks == force_clk_cycles){
        driveforceclkValid = false;
        force_clk_cycles = 0;
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

void driveforceclkPulse(forceclk_data_t t)
{
    if(!driveforceclkValid){  
      driveforceclkValid = true;
      force_clk_cycles = ticks + t.pulse_width;
      driveforceclkPin(t.pinval);
    }
}

void driveforceclkPin(unsigned pinvalue){
    //SOME CVM msg
      cvm::registry::callbacks.push(
       scope(),
       [pinvalue]() {
          reset_driver_drive_force_clk(pinvalue);
        });
 }

void driveResetPin(unsigned pin, unsigned value){
    reset_sigs_t i;

    if (value) {
        // Set the bit at position 'pin' to 1
        glob_reset_state |= (1 << pin);
    } else {
        // Set the bit at position 'pin' to 0
        glob_reset_state &= ~(1 << pin);
    }

    i.reset_sigs =  glob_reset_state;

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
void assert_force_clock();
void deassert_force_clock();
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
   unsigned force_clk_cycles; 
   bool driveforceclkValid = false;
   bool driveHoldValid = false; // Valid bit for interrupt
   bool driveHoldInProgress = false; // Valid bit for interrupt
   std::vector<bool> HoldVal; // Valid bit for interrupt
   bool drive_holds = true;


   unsigned num_resets = 0;
   unsigned num_warm_resets = 0;

   uint64_t ticks = 0;
   uint64_t force_clk_assert_ticks = 0;
   uint64_t holds_assert_ticks = 0;
   uint64_t holds_deassert_ticks = 0;
   uint64_t reset_sequence_ticks = 0;
   uint8_t glob_reset_state = 0;
   bool force_clk_assert_in_progress = false;
   bool holds_assert_in_progress = false;
   bool reset_sequence_in_progress = false;
   bool holds_deassert_in_progress = false;

   bool cold_boot_ack_from_smc_xtor = false;
   bool cold_boot_reset_done = false;

   bool run_cold_reset_seq = false;
   bool run_warm_reset_seq = false;
   bool send_update_to_smc = true;
   // uint64_t timer_rand_intr = 500;
   uint64_t reset_driver_base = 0x9000000;
  
};

#endif
