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

DEFINE_bool(reset_driver, true, "Enable reset driver");
DEFINE_bool(rst_sram_hold, true, "Enable reset driver sram hold");
DEFINE_bool(rst_debug_hold, false, "Enable reset driver debug hold");
DEFINE_bool(rst_critical_hold, true, "Enable reset driver critical hold");
DEFINE_bool(mid_sim_reset_en, true, "Enable mid sim reset driving");
DEFINE_bool(mid_sim_warm_reset_en, true, "Enable mid sim warm reset driving");
// TODO: control which are dumped? might not be useful
DEFINE_uint32(reset_pulse_period, 16, "Hold Reset pin value for N cycles");
DEFINE_uint32(hold_pulse_period, 16, "Hold HOLD pin value for N cycles");
DEFINE_uint64(mid_sim_reset_period, 7000, "Drive midsim reset every N cycles");
DEFINE_uint64(mid_sim_warm_reset_period, 2000, "Drive midsim reset every N cycles");
DEFINE_uint64(reset_chk_threshold_period, 2, "Check for reset conditions before n cycles of reset period");
DEFINE_uint64(reset_chk_period, 300, "Time to execute reset sequences  after reset");
REGISTRY_register(reset_driver, TOP.PLATFORM.RESET_DRIVER, 0);

reset_driver::reset_driver(cvm::topology::loc_t loc, unsigned id) 
: scope_(nullptr), loc_(loc), id_(id) ,phase1_cycles(3),phase2_cycles(3),driveResetValid(3),driveResetInProgress(3),ResetRelVal(3)
{
std::cout<<"\n constructing reset driver loc "<<std::dec<<loc <<" id "<<id<<"\n";
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


void
reset_driver::checkUsage()
{

}
void
reset_driver::update_smc_status(smc_xtor::smc_reset_driver_data_t i)
{

    cvm::log(cvm::FULL, "[Reset Driver] GOT data from smc ::update_smc_status  {} \n",i.data);
}
void
reset_driver::init_pins()
{
    // misc_vif.cluster_warm_reset_n = 1;
    // misc_vif.cluster_sram_hold = 0;
    // misc_vif.cluster_critical_signal_hold = 0;
    // misc_vif.cluster_debug_hold = 0;
    cvm::log(cvm::FULL, "[Reset Driver] Init pins \n");
    reset_data_t rst_data = {0,4,0,0,0};
    hold_data_t hold_data = {1,5};
    driveResetPulse(rst_data);
    driveHoldPulse(hold_data);
}
void reset_driver::perform_cold_reset(){
    //   misc_vif.cluster_cold_reset_n = 0;
    // repeat(16) @(posedge misc_vif.sc_clk);
    // misc_vif.cluster_cold_reset_n = 1;
    // repeat(16) @(posedge misc_vif.sc_clk);
    reset_data_t rst_data = {1,0,0,FLAGS_reset_pulse_period,1};
    driveResetPulse(rst_data);
}

void reset_driver::wait_for_reset_completion_ack(){
//poll for pin add pckt gen
}

void reset_driver::assert_warm_reset_holds(){
    // if($test$plusargs("SRAM_HOLD")) begin
    //   `uvm_info(get_name(),$sformatf("[RST_CTRL] Asserted SRAM Hold...."),UVM_LOW)
    //   misc_vif.cluster_sram_hold = 1;
    // end
    unsigned hold_value = 0;
    hold_data_t hold_data;
    if(FLAGS_rst_sram_hold)
        hold_value = hold_value | 1<<0;
    // if($test$plusargs("CRITICAL_HOLD")) begin
    //   `uvm_info(get_name(),$sformatf("[RST_CTRL] Asserted Critical Signal Hold...."),UVM_LOW)
    //   misc_vif.cluster_critical_signal_hold = 1;
    // end
    if(FLAGS_rst_debug_hold)
        hold_value = hold_value | 1<<1;
    // if($test$plusargs("DEBUG_HOLD")) begin
    //   `uvm_info(get_name(),$sformatf("[RST_CTRL] Asserted Debug Hold...."),UVM_LOW)
    //   misc_vif.cluster_debug_hold = 1;
    // end
    // repeat(16) @(posedge misc_vif.sc_clk);
    if(FLAGS_rst_critical_hold)
        hold_value = hold_value | 1<<2;
    
    hold_data = {FLAGS_hold_pulse_period,hold_value};
    driveHoldPulse(hold_data);
}

void reset_driver::perform_warm_reset(){
    // `uvm_info(get_name(),$sformatf("[RST_CTRL] Asserting Warm Reset"),UVM_LOW)
    // misc_vif.cluster_warm_reset_n = 0;
    // repeat(16) @(posedge misc_vif.sc_clk);
    // misc_vif.cluster_warm_reset_n = 1;
    // repeat(16) @(posedge misc_vif.sc_clk);
    // `uvm_info(get_name(),$sformatf("[RST_CTRL] De-asserted warm reset"),UVM_LOW)

    reset_data_t rst_data = {1,0,2,FLAGS_reset_pulse_period,1};
    driveResetPulse(rst_data);
}
void reset_driver:: deassert_warm_reset_holds(){
    // if($test$plusargs("SRAM_HOLD")) begin
    //   `uvm_info(get_name(),$sformatf("[RST_CTRL] Asserted SRAM Hold...."),UVM_LOW)
    //   misc_vif.cluster_sram_hold = 0;
    // end
    // if($test$plusargs("CRITICAL_HOLD")) begin
    //   `uvm_info(get_name(),$sformatf("[RST_CTRL] Asserted Critical Signal Hold...."),UVM_LOW)
    //   misc_vif.cluster_critical_signal_hold = 0;
    // end
    // if($test$plusargs("DEBUG_HOLD")) begin
    //   `uvm_info(get_name(),$sformatf("[RST_CTRL] Asserted Debug Hold...."),UVM_LOW)
    //   misc_vif.cluster_debug_hold = 0;
    // end
    // repeat(16) @(posedge misc_vif.sc_clk); 
    unsigned hold_value = 0;
    hold_data_t hold_data;
    if(FLAGS_rst_sram_hold)
        hold_value = hold_value & ~(0<<0);
    // if($test$plusargs("CRITICAL_HOLD")) begin
    //   `uvm_info(get_name(),$sformatf("[RST_CTRL] Asserted Critical Signal Hold...."),UVM_LOW)
    //   misc_vif.cluster_critical_signal_hold = 1;
    // end
    if(FLAGS_rst_debug_hold)
        hold_value = hold_value & ~(1<<1);
    // if($test$plusargs("DEBUG_HOLD")) begin
    //   `uvm_info(get_name(),$sformatf("[RST_CTRL] Asserted Debug Hold...."),UVM_LOW)
    //   misc_vif.cluster_debug_hold = 1;
    // end
    // repeat(16) @(posedge misc_vif.sc_clk);
    if(FLAGS_rst_critical_hold)
        hold_value = hold_value & ~(1<<2);
    
    hold_data = {FLAGS_hold_pulse_period,hold_value};
    driveHoldPulse(hold_data);
}
void reset_driver::reset()
{
  cvm::log(cvm::HIGH,"[RESET DRIVER] Reset \n");
}

extern "C" {

  void reset_driver_set_scope(cvm::topology::loc_t loc) {
    std::cout<<"reset drv setting scope from sv to c loc:"<<loc<<"\n";
    svScope scope = svGetScope();
    //set_scope(scope);
     cvm::registry::messenger.signal<svScope>(
         loc,
         scope);
  }
}
