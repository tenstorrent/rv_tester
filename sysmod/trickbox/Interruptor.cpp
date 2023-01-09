#include <stdexcept>
#include <cassert>
#include <iostream>
#include "Interruptor.h"


Interruptor::Interruptor(unsigned n, uint64_t addr)
: device("interruptor", addr, 16 /* size */)
{
  if (n >= 4095)
    throw std::range_error("Interruptor::Interruptor: hart count (" + std::to_string(n) + ") out of bounds");
  hartCount_ = n;
  addr_ = addr;
}


bool
Interruptor::configure(unsigned n, uint64_t addr)
{
  if (n >= 4095)
    return false;
  hartCount_ = n;
  addr_ = addr;
  return true;
}


Interruptor::~Interruptor()
{
  hartCount_ = 0;
}
void Interruptor::triggerInterrupt(int hart, int eventId, int flag)
  { 
    std::cout <<"call SetHart MIP HandleWrite: \n";
    //SetHartMip(hart, eventId, flag); 
   // tbox.read();
    }


void Interruptor::triggerDelayedInterrupt(int hart, int eventId, int flag,int delay)
  { 
    std::cout <<"call SetHart MIP HandleWrite: \n";
   // SetHartDelayedMip(hart, eventId, flag, delay); 
    }
bool
Interruptor::handleWrite(uint64_t addr, uint64_t value,cbs_t& cbs)
{
  std::cout <<"HandleWrite: "<<std::hex<< addr<<" data "<<value;
  //if (addr != addr_){
  if ((addr >= addr_)&& (addr <= (addr_ + 0x1000))){
   //std::cout<<"Addr not matching "<<std::hex<<addr <<" with addr_ "<<std::hex<<addr_<<std::endl;
    //return false;
  }

  // Value consists of 3 fields.
  // Least sig 12 bits: hart id
  // Bits 12 to 19: interrupt/event id
  // Bits 20 to 31: event value
  if(addr == addr_){
  unsigned hart = value & 0xfff;
  if (hart >= hartCount_){
    std::cout<<"HArt number not matching  hart "<<hart <<" hartcount "<<hartCount_<<std::endl;
    return false;
     
  }

  int event = (value >> 12) & 0xff;
  int eventValue = (value >> 20);
  int flag = eventValue != 0;
   
  std::cout <<"Trigger Interrupt: "<<std::hex<< addr<<" data "<<value;
  //triggerInterrupt(hart, event, flag);
  cbstriggerInterrupt(hart, flag,event,cbs);
  return true;
  }else if ((addr > addr_)&& (addr < (addr_ + 0x1000))){
    //delayed interrupts
    int event = (addr>> 7 )& 0xf; //[10:7];//(value >> 12) & 0xff;
    int eventValue_delay = (value>>12) & 0x7ffff;//[30:0] ;
    int flag_m = value & 0x80000000;
    int flag = flag_m !=0;
    unsigned hart = value & 0xfff; //
    //triggerDelayedInterrupt(hart, event, flag, eventValue_delay);
    cbstriggerDelayedInterrupt(hart, event, flag, eventValue_delay,cbs);
     std::cout << "PRT Delay Interruptor Addr :"<<std::hex<<addr<<"  data : "<<value<<std::endl;;
  return true;

  }else if(addr == (addr_ + 0x1000) ){
    //enable disable random interruptor
     std::cout << "PRT Full Random Interruptor Addr :"<<std::hex<<addr<<"  data : "<<value<<std::endl;;

  return true;

  }else{
     std::cout << "PRT DBG  Interruptor Addr :"<<std::hex<<addr<<"  data : "<<value<<std::endl;;
  return false;
  }
}

#if 0

#endif

