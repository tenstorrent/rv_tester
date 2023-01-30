#include <stdexcept>
#include <cassert>
#include <iostream>
#include "Interruptor.h"
#include "svdpi.h"
extern "C" void SetHartITP(int hart, int interruptId, int value);
extern "C" void SetHartDelayedITP(int hart, int interruptId, int flag,int value);
extern "C" void SetHartRandomITP(int hart,int eventFlag);
Interruptor::Interruptor(unsigned n, uint64_t addr,uint64_t size)
: device("interruptor", addr,  size )
{
  if (n >= 4095)
    throw std::range_error("Interruptor::Interruptor: hart count (" + std::to_string(n) + ") out of bounds");
  hartCount_ = n;
  addr_ = addr;
  std::cout<<"CONST:Constructing  Interruptor with addr_: "<<std::hex<<addr_<<" Size :"<<size<<"\n";
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
  int sysmod_mode =1;
  if(sysmod_mode){
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
  }}else{
    return 0;
  }
}



bool
Interruptor::handleWriteHelper(uint64_t addr, uint64_t value)
{

    std::cout <<"Interrupt HandleWrite: "<<std::hex<< addr<<" data "<<value;
    std::cout <<"\nTrickBox Addr: "<<std::hex<< addr_<<" \n";
    //std::cout <<"Interrupt HandleWrite: "<<std::hex<< addr<<" data "<<value;
    //std::cout <<"\nTrickBox Addr: "<<std::hex<< addr_<<" \n";
    svScope scope = svGetScopeFromName("top.dut_harness");
    svSetScope(scope); 
    std::cout << "Scope is " << svGetNameFromScope(svGetScope()) << std::endl; 
    std::cout <<"Trigger Interrupt: "<<std::hex<< addr<<" data "<<value;
    if(addr==0x9000000){
    unsigned hart = value & 0xfff;
    int event = (value >> 12) & 0xff;
    int eventValue = (value >> 20);
    SetHartITP(hart,event,eventValue);
    }else if((addr > 0x9000000)&& (addr < 0x9001000)){
   
     int itp_loc  = addr & 0xfc;  
     unsigned hart = value & 0xfff; 
     int eventFlag = (value >> 12) & 0x1;
     int eventDelay = (value >> 13); 
     SetHartDelayedITP(hart,itp_loc,eventFlag,eventDelay);
    }else if(addr==0x9004000){
     unsigned hart = value & 0xfff; 
     int eventFlag = (value >> 12) & 0x1; 
     SetHartRandomITP(hart,eventFlag);
    }
  
  return true;
}

void
Interruptor::read(uint64_t addr, size_t length, data_t& data, cbs_t& cbs)
{
}



void
Interruptor::write(uint64_t addr, size_t length, const data_t& data,
	    const strb_t& strb, cbs_t& cbs)
{

}

#if 0

#endif

