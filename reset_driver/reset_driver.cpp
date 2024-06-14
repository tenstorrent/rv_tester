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
DEFINE_bool(mid_sim_reset_en, true, "Enable mid sim reset driving");
DEFINE_bool(mid_sim_warm_reset_en, false, "Enable mid sim warm reset driving");
// TODO: control which are dumped? might not be useful
DEFINE_uint64(mid_sim_reset_period, 7000, "Drive midsim reset every N cycles");
DEFINE_uint64(mid_sim_warm_reset_period, 7000, "Drive midsim reset every N cycles");
REGISTRY_register(reset_driver, TOP.PLATFORM.RESET_DRIVER, 0);

reset_driver::reset_driver(cvm::topology::loc_t loc, unsigned id) 
: scope_(nullptr), loc_(loc), id_(id) ,phase1_cycles(3),phase2_cycles(3),driveResetValid(3),driveResetInProgress(3),ResetRelVal(3), timer_(0)
{
std::cout<<"\n constructing reset driver loc "<<std::dec<<loc <<" id "<<id<<"\n";
  cvm::registry::messenger.connect<svScope>(
      loc_,
      [this](svScope s) { return this->set_scope(s); });
  cvm::registry::messenger.connect<rv_tester_transactions::reset_driver::tick<>>(
      loc_,
      [this](const rv_tester_transactions::reset_driver::tick<>& t) { return this->tick(t.advance); });
                                                                    
                                                                              
 

  reset();
}


// void reset_driver::process(const rv_tester_transactions::reset_driver::msi_req<> &msi_req)
// {
//   //APLIC MSI request
//   //on observing MSI , we can query the aplic model for MSI or can check in a queue if current MSI should be observed or not 
//    cvm::log(cvm::DEBUG,"[APLIC MONITOR] Process0: location {} \n",msi_req.location);
// }

void
reset_driver::checkUsage()
{

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
    driveResetPulse(rst_data);
}
void reset_driver::perform_cold_reset(){
    //   misc_vif.cluster_cold_reset_n = 0;
    // repeat(16) @(posedge misc_vif.sc_clk);
    // misc_vif.cluster_cold_reset_n = 1;
    // repeat(16) @(posedge misc_vif.sc_clk);
    reset_data_t rst_data = {1,0,0,16,1};
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
    // if($test$plusargs("CRITICAL_HOLD")) begin
    //   `uvm_info(get_name(),$sformatf("[RST_CTRL] Asserted Critical Signal Hold...."),UVM_LOW)
    //   misc_vif.cluster_critical_signal_hold = 1;
    // end
    // if($test$plusargs("DEBUG_HOLD")) begin
    //   `uvm_info(get_name(),$sformatf("[RST_CTRL] Asserted Debug Hold...."),UVM_LOW)
    //   misc_vif.cluster_debug_hold = 1;
    // end
    // repeat(16) @(posedge misc_vif.sc_clk);
}

void reset_driver::perform_warm_reset(){
    // `uvm_info(get_name(),$sformatf("[RST_CTRL] Asserting Warm Reset"),UVM_LOW)
    // misc_vif.cluster_warm_reset_n = 0;
    // repeat(16) @(posedge misc_vif.sc_clk);
    // misc_vif.cluster_warm_reset_n = 1;
    // repeat(16) @(posedge misc_vif.sc_clk);
    // `uvm_info(get_name(),$sformatf("[RST_CTRL] De-asserted warm reset"),UVM_LOW)

    reset_data_t rst_data = {1,0,2,16,1};
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
