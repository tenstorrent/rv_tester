#include <cassert>
#include <cstring>
#include <map>
#include <memory>
#include <vector>

#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/bitmanip.hpp"
#include "aplic_monitor.hpp"
//REGISTRY_register(aplic_monitor_t, TOP.PLATFORM.APLIC_MONITOR, 0);
REGISTRY_register(aplic_monitor, APLIC_MONITOR, cvm::registry::all);

aplic_monitor::aplic_monitor(cvm::topology::loc_t loc, unsigned) 
{

  std::cout<<"APLIC_MONITOR CPP location"<<loc<<"\n";
  cvm::registry::messenger.connect<rv_tester_transactions::aplic_monitor::msi_req<>>(loc, [this](const auto &v)
                                                                              { return this->process(v); });
  cvm::registry::messenger.connect<rv_tester_transactions::aplic_monitor::aplic_mmr_load_cmd<>>(loc, [this](const auto &v)
                                                                                  { return this->process(v); });
  cvm::registry::messenger.connect<rv_tester_transactions::aplic_monitor::aplic_mmr_load_data<>>(loc, [this](const auto &v)
                                                                                   { return this->process(v); });
  cvm::registry::messenger.connect<rv_tester_transactions::aplic_monitor::aplic_intr_req<>>(loc, [this](const auto &v)
                                                                                   { return this->process(v); });
                                                                                   
  cvm::registry::messenger.connect<rv_tester_transactions::aplic_monitor::aplic_mmr_store<>>(loc, [this](const auto &v)
                                                                               { return this->process(v); });

 
  TT_APLIC::Aplic aplic(addr, stride, hartCount, domainCount, interruptCount);
  reset();
}

void aplic_monitor::process(const rv_tester_transactions::aplic_monitor::msi_req<> &msi_req)
{
  std::cout<<"\nProcess1: location"<<msi_req.location;
}

void aplic_monitor::process(const rv_tester_transactions::aplic_monitor::aplic_intr_req<> &aplic_intr_req)
{
  std::cout<<"\nProcess1: location"<<aplic_intr_req.location;
}


void aplic_monitor::process(const rv_tester_transactions::aplic_monitor::aplic_mmr_load_cmd<> &aplic_mmr_load_cmd)
{

  std::cout<<"\nProcess2: location"<<aplic_mmr_load_cmd.location;
}

void aplic_monitor::process(const rv_tester_transactions::aplic_monitor::aplic_mmr_load_data<> &aplic_mmr_load_data)
{
  
  std::cout<<"\nProcess3: location"<<aplic_mmr_load_data.location;

}

void aplic_monitor::process(const rv_tester_transactions::aplic_monitor::aplic_mmr_store<> &aplic_mmr_store)
{
 
  std::cout<<"\nProcess4: Aplic mmr store location  "<<aplic_mmr_store.location;
  std::cout<<"\nProcess4: Aplic mmr store Data  "<<aplic_mmr_store.data;
  //std::cout<<"\nProcess4: Aplic mmr store Id  "<<aplic_mmr_store.id;
}

void aplic_monitor::reset()
{
  
std::cout<<"Reset APlic Mon\n";
  
}

