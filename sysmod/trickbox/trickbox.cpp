#include <iostream>
#include <cassert>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include "trickbox.h"
#include "Interruptor.h"

trickbox::trickbox(const std::string& tag, uint64_t addr,uint64_t size)
  : device(tag, addr, size /* size */)
{
  std::cout<<"CONST: Constructing Trickbox  with addr: "<<std::hex<<addr<<" Size :"<<size<<"\n";
  Interruptor ic1(100,addr,size);
  ic = &ic1;
   //Interrupt ic1(100,addr);
}


trickbox::~trickbox()
{
}


void
trickbox::read(uint64_t addr, size_t length, data_t& data, cbs_t& cbs)
{
  if (not has_addr(addr) or length != 8 or (addr % 8) != 0)
    return;

  uint64_t offset = addr - this->addr();
  uint64_t di = offset / 8;  // Double word index
  uint64_t dword = di == 0? to_ : from_;
  serializeInt(dword, length, data);
  return;
}



void
trickbox::write(uint64_t addr, size_t length, const data_t& data,
	    const strb_t& strb, cbs_t& cbs)
{
  if (not has_addr(addr) or length != 1 ){
     std::cerr << "TrickBox: Unexpected addr or length\n";
    return;
  }

  uint64_t dword = 0;
  deserializeInt(data, dword);

  uint64_t offset = addr - this->addr();
  uint64_t di = offset / 8;  // Double word index

  if (di == 1)
    {
      from_ = dword;
      return;
    }
  std::cout<<"PRT TRICKBOX Addr: "<<std::hex<< addr <<" data :" <<dword<<" \n";
  ic->handleWrite(addr,dword,cbs);
  // Writing to-host.
  //uint64_t payload = (dword << 16) >> 16;
  //unsigned cmd = (dword >> 48) & 0xff;
  //unsigned dev = (dword >> 56) & 0xff;
  //itp.handle_itp(offset,dword,cbs);
  //if(offset == 0){
  // //immidiate interrupt
  //unsigned hart = dword & 0xfff;
  //unsigned  event = (dword >> 12) & 0xff;
  //unsigned eventValue = (dword >> 20);
  //bool flag = eventValue != 0;
  //trickbox::trickboxInterrupt(hart, flag, event, cbs);
  //}else if(offset < 1000){
  ////delayed interrupt
  ////unsigned hart = dword & 0xfff;
  ////unsigned  event = (dword >> 12) & 0xff;
  ////unsigned eventValue = (dword >> 20);
  ////unsigned flag = eventValue != 0;
  ////delayed interrupts
  //  unsigned event = (addr>> 7 )& 0xf; //[10:7];//(value >> 12) & 0xff;
  //  unsigned eventValue_delay = (dword>>12) & 0x7ffff;//[30:0] ;
  //  unsigned flag_m = dword & 0x80000000;
  //  bool flag = flag_m !=0;
  //  unsigned hart = dword & 0xfff; //
  //trickbox::trickboxDelayedInterrupt(hart, flag, event,eventValue_delay, cbs);
  //}

}
