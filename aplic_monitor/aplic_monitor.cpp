#include <cassert>
#include <cstring>
#include <map>
#include <memory>
#include <vector>
#include <bitset>
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/bitmanip.hpp"
#include "aplic_monitor.hpp"
REGISTRY_register(aplic_monitor, TOP.PLATFORM.APLIC_MONITOR, 0);
//REGISTRY_register(aplic_monitor, APLIC_MONITOR, cvm::registry::all);

aplic_monitor::aplic_monitor(cvm::topology::loc_t loc, unsigned) 
{

  //std::cout<<"APLIC_MONITOR CPP location "<<loc<<"\n";
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
  bool isMachine = true;
  root = aplic.createDomain(nullptr, addr, isMachine);

  reset();
}

void aplic_monitor::process(const rv_tester_transactions::aplic_monitor::msi_req<> &msi_req)
{
  //APLIC MSI request
  //on observing MSI , we can query the aplic model for MSI or can check in a queue if current MSI should be observed or not 
   cvm::log(cvm::HIGH,"\n[APLIC MONITOR] Process0: location {} \n ",msi_req.location);
}

void aplic_monitor::process(const rv_tester_transactions::aplic_monitor::aplic_intr_req<> &aplic_intr_req)
{
  //std::cout<<"\nProcess1: location "<<aplic_intr_req.location<<"\n";
  cvm::log(cvm::HIGH,"\n[APLIC MONITOR] Process1: location {} \n ",aplic_intr_req.location);
  std::vector<int> setIndices;
  for (int i = 0; i < 1024; ++i) {
        if (aplic_intr_req.pin_value[i]) {
            setIndices.push_back(i);
        }
    }
   
  cvm::log(cvm::HIGH,"\n[APLIC MONITOR] Following interrupts will be driven \n ");
   for (int index : setIndices) {
        cvm::log(cvm::HIGH," {}\n", index );
    }
    std::cout << std::endl;
}


void aplic_monitor::process(const rv_tester_transactions::aplic_monitor::aplic_mmr_load_cmd<> &aplic_mmr_load_cmd)
{

 // std::cout<<"\nProcess2: location"<<aplic_mmr_load_cmd.location;
  cvm::log(cvm::HIGH,"\n[APLIC MONITOR] Process2: location {} \n ",aplic_mmr_load_cmd.location);
}

void aplic_monitor::process(const rv_tester_transactions::aplic_monitor::aplic_mmr_load_data<> &aplic_mmr_load_data)
{
  
  //std::cout<<"\nProcess3: location"<<aplic_mmr_load_data.location;
  cvm::log(cvm::HIGH,"\n[APLIC MONITOR] Process2: location {} \n ",aplic_mmr_load_data.location);

}

void aplic_monitor::process(const rv_tester_transactions::aplic_monitor::aplic_mmr_store<> &aplic_mmr_store)
{
  //APLIC configuration will happen here
  // APlic MMR stores are visible here and we can call aplic model api to program the model in this process function
  cvm::log(cvm::HIGH,"\nProcess4: Aplic mmr store location {} ",aplic_mmr_store.location);
  cvm::log(cvm::HIGH,"\nProcess4: Aplic mmr store Data {} ",aplic_mmr_store.data);
  cvm::log(cvm::HIGH,"\nProcess4: Aplic mmr store addr {} ",aplic_mmr_store.addr);
  
}

void aplic_monitor::reset()
{
  
  cvm::log(cvm::HIGH,"\n[APLIC MONITOR] Rese \n ");
  
}

