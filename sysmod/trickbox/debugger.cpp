#include "cvm/plusargs.hpp"
#include "debugger.h"


debugger::debugger(const std::string& tag, uint64_t addr, unsigned hartCount)
  : device(tag, addr, 0x4000 /* size */), hartCount_(hartCount), soft_(hartCount),
    timeCompare_(6),IntrHart_(6),delayedRandomIntValid_(6),IntrValue_(6), timerIntPrev_(hartCount), timer_(0)
{
  debugger_base = addr;
  
}


debugger::~debugger()
{
  terminate_ = true;
  
}



void
debugger::read(uint64_t addr, size_t length, data_t& data, cbs_t& cbs)
{
  if (not has_addr(addr))
    return;

}


void
debugger::write(uint64_t addr, size_t length, const data_t& data,
		 const strb_t& strb, cbs_t& cbs)
{
  std::cout<<"debugger write: 0x"<<std::hex<<addr;
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  if(addr==debugger_base)
  {
    unsigned upper_dmi_data = 0;
    unsigned lower_dmi_data = 0;
    unsigned hart = 0;
    upper_dmi_data = t_data >>32;
    lower_dmi_data = t_data & 0xffffffff;
    hart = 0; //hart bits position TBD, till TBD it is always zero
    //trickboxDmiWrite(hart,upper_dmi_data,lower_dmi_data,cbs); Commented until DMI PORT is not in master
 
  }
    
}
