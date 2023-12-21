#include <cassert>
#include <cstring>
#include <map>
#include <memory>
#include <vector>

#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/bitmanip.hpp"
#include "aplic_monitor.hpp"
REGISTRY_register(aplic_monitor_t, TOP.PLATFORM.APLIC_MON, 0);

aplic_monitor_t::aplic_monitor_t(cvm::topology::loc_t loc, unsigned) :
{
  cvm::registry::messenger.connect<rv_tester_transactions::aplic_monitor::msi_req<>>(loc, [this](const auto &v)
                                                                              { return this->process(v); });
  cvm::registry::messenger.connect<rv_tester_transactions::aplic_monitor::aplic_mmr_load_cmd<>>(loc, [this](const auto &v)
                                                                                  { return this->process(v); });
  cvm::registry::messenger.connect<rv_tester_transactions::aplic_monitor::aplic_mmr_load_data<>>(loc, [this](const auto &v)
                                                                                   { return this->process(v); });
  cvm::registry::messenger.connect<rv_tester_transactions::aplic_monitor::aplic_mmr_store<>>(loc, [this](const auto &v)
                                                                               { return this->process(v); });

 

  reset();
}

void aplic_monitor_t::process(const rv_tester_transactions::aplic_monitor::msi_req<> &msi_req)
{
  std::cout<<"\nProcess1: location"<<msi_req.data.location;
}



void aplic_monitor_t::process(const rv_tester_transactions::aplic_monitor::aplic_mmr_load_cmd<> &aplic_mmr_load_cmd)
{

  std::cout<<"\nProcess2: location"<<aplic_mmr_load_cmd.data.location;
}

void aplic_monitor_t::process(const rv_tester_transactions::aplic_monitor::aplic_mmr_load_data<> &aplic_mmr_load_data)
{
  
  std::cout<<"\nProcess3: location"<<aplic_mmr_load_data.data.location;

}

void aplic_monitor_t::process(const rv_tester_transactions::aplic_monitor::aplic_mmr_store<> &aplic_mmr_store)
{
 
  std::cout<<"\nProcess4: location"<<aplic_mmr_store.data.location;
}

void aplic_monitor_t::reset()
{
  
std:;cout<<"Reset APlic Mon\n";
  
}

