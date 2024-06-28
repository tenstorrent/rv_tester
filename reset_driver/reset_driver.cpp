#include <cassert>
#include <cstring>
#include <map>
#include <memory>
#include <vector>
#include <bitset>
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/bitmanip.hpp"
#include "reset_driver.hpp"

DEFINE_bool(reset_driver_en, true, "Enable reset driver");
DEFINE_bool(rst_sram_hold, false, "Enable reset driver sram hold");
DEFINE_bool(rst_debug_hold, true, "Enable reset driver debug hold");
DEFINE_bool(rst_critical_hold, false, "Enable reset driver critical hold");
DEFINE_bool(mid_sim_reset_en, false, "Enable mid sim reset driving");
DEFINE_bool(mid_sim_warm_reset_en, true, "Enable mid sim warm reset driving");
// TODO: control which are dumped? might not be useful
DEFINE_uint32(reset_pulse_period, 16, "Hold Reset pin value for N cycles");
DEFINE_uint32(num_resets, 1, "toggle resets N times");
DEFINE_uint32(num_warm_resets, 1, "toggle warm resets N times");
DEFINE_uint32(hold_pulse_period, 16, "Hold HOLD pin value for N cycles");
DEFINE_uint64(mid_sim_reset_period, 1500, "Drive midsim reset every N cycles");
DEFINE_uint64(mid_sim_warm_reset_period, 1000, "Drive midsim reset every N cycles");
DEFINE_uint64(reset_chk_threshold_period, 2, "Check for reset conditions before n cycles of reset period");
DEFINE_uint64(reset_chk_period, 300, "Time to execute reset sequences  after reset");
REGISTRY_register(reset_driver, TOP.PLATFORM.RESET_DRIVER, 0);

reset_driver::reset_driver(cvm::topology::loc_t loc, unsigned id) 
: scope_(nullptr), loc_(loc), id_(id) ,phase1_cycles(3),phase2_cycles(3),driveResetValid(3),driveResetInProgress(3),ResetRelVal(3)
{
  cvm::registry::messenger.connect<svScope>(
      loc_,
      [this](svScope s) { return this->set_scope(s); });
  cvm::registry::messenger.connect<rv_tester_transactions::reset_driver::tick<>>(
      loc_,
      [this](const rv_tester_transactions::reset_driver::tick<>& t) { return this->tick(t.num_clocks); });
                                                                    
  auto reset_driver_loc = cvm::topology::get_from_type("RESET_DRIVER", 0); 
  cvm::registry::messenger.connect<smc_xtor::smc_reset_driver_data_t>(
            reset_driver_loc,
            [&](smc_xtor::smc_reset_driver_data_t i) { return this->update_smc_status(i); });                                                                           
 
  reset();
}


void reset_driver::checkUsage()
{

}

void reset_driver::update_smc_status(smc_xtor::smc_reset_driver_data_t i)
{
    cvm::log(cvm::FULL, "[Reset Driver] GOT data from smc ::update_smc_status  {} \n",i.data);
    if (cold_boot_ack_from_smc_xtor == false)
        cold_boot_ack_from_smc_xtor = true; 
    else 
        reset_ack_from_smc_xtor = true;
}

void reset_driver::init_pins()
{
    cvm::log(cvm::NONE, "[Reset Driver] Initializing reset pins\n");
    reset_data_t rst_data = {0,4,0,0,0};
    hold_data_t hold_data = {1,0};
    forceclk_data_t forceclk_data = {1,1};
    driveResetPulse(rst_data);
    driveHoldPulse(hold_data);
    driveforceclkPulse(forceclk_data);
}

void reset_driver::perform_cold_reset(){
    cvm::log(cvm::NONE, "[Reset Driver] Performing Cold Reset Sequence\n");
    reset_data_t rst_data = {1,0,0,FLAGS_reset_pulse_period,1};
    driveResetPulse(rst_data);
}

void reset_driver::wait_for_reset_completion_ack(){
//poll for pin add pckt gen
}

void reset_driver::assert_force_clock(){
    cvm::log(cvm::NONE, "[Reset Driver] Assertting force_ss_to_ref_clk pin\n");
    forceclk_data_t forceclk_data;
    forceclk_data = {FLAGS_hold_pulse_period,1};
    driveforceclkPulse(forceclk_data);
}

void reset_driver::deassert_force_clock(){
    cvm::log(cvm::NONE, "[Reset Driver] De-assertting force_ss_to_ref_clk pin\n");
    forceclk_data_t forceclk_data;
    forceclk_data = {FLAGS_hold_pulse_period,1};
    driveforceclkPulse(forceclk_data);
}

void reset_driver::assert_warm_reset_holds(){
    unsigned hold_value = 0;
    hold_data_t hold_data;
    cvm::log(cvm::NONE, "[Reset Driver] Assertting Warm Reset Holds\n");
    if(num_warm_resets >= FLAGS_num_warm_resets)
       return;
    
    if(FLAGS_rst_sram_hold)
        hold_value = hold_value | 1<<0;
    
    if(FLAGS_rst_critical_hold)
        hold_value = hold_value | 1<<1;
    
    if(FLAGS_rst_debug_hold)
        hold_value = hold_value | 1<<2;
    
    hold_data = {FLAGS_hold_pulse_period,hold_value};
    driveHoldPulse(hold_data);
}

void reset_driver::perform_warm_reset(){
    cvm::log(cvm::NONE, "[Reset Driver] Performing Warm Reset Sequence\n");
    reset_data_t rst_data = {1,0,2,FLAGS_reset_pulse_period,1};
    driveResetPulse(rst_data);
}

void reset_driver:: deassert_warm_reset_holds(){
    unsigned hold_value = 0;
    hold_data_t hold_data;
    cvm::log(cvm::NONE, "[Reset Driver] De-Assertting Warm Reset Holds\n");
    if(num_warm_resets > FLAGS_num_warm_resets)
       return;
    // if(FLAGS_rst_sram_hold)
    //     hold_value = hold_value & ~(0<<0);
    
    // if(FLAGS_rst_critical_hold)
    //     hold_value = hold_value & ~(1<<1);
    
    // if(FLAGS_rst_debug_hold)
    //     hold_value = hold_value & ~(1<<2);
    
    hold_data = {FLAGS_hold_pulse_period,hold_value};
    driveHoldPulse(hold_data);
}
void reset_driver::reset()
{
  cvm::log(cvm::HIGH,"[RESET DRIVER] Reset \n");
}

extern "C" {

  void reset_driver_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();
     cvm::registry::messenger.signal<svScope>(
         loc,
         scope);
  }
}
