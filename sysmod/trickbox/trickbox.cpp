#include "trickbox.h"

trickbox::trickbox(const std::string& tag, uint64_t addr, unsigned hartCount)
  : device(tag, addr, 0xc0000 /* size */), hartCount_(hartCount), soft_(hartCount),
    timeCompare_(hartCount), timerIntPrev_(hartCount), timer_(0)
{
}


trickbox::~trickbox()
{
  terminate_ = true;
  if (timerThread_.joinable())
    timerThread_.join();
}


void
trickbox::selfTick(useconds_t delta)
{
  auto func = [this, delta]() {
    while (true)
      {
	usleep(delta);
	if (terminate_)
	  return;
	else
	  {
	    std::lock_guard<std::mutex> lock(mutex_);
	   //  tick();
	  }
      }
  };

  timerThread_ = std::thread(func);
}


void
trickbox::read(uint64_t addr, size_t length, data_t& data, cbs_t& cbs)
{
  if (not has_addr(addr))
    return;

 // uint64_t offset = addr - device::addr();
 // if (offset < 0x4000)
 //   {
 //     // Sofware interrupt: 1 word per hart.
 //     if ((offset % 4) != 0)
 //       return;  // Address must be a multiple of 4.
 //     unsigned hartIx = offset / 4;
 //     uint32_t word = (hartIx < hartCount_) ? soft_.at(hartIx) : 0;
 //     serializeInt(word, length, data);
 //     return;
 //   }

 // if (offset == 0xbff8)
 //   {
 //     serializeInt(timer_, length, data);
 //     return;
 //   }

 // // Time compare. 1 double word per hart.
 // if ((offset % 8) != 0)
 //   return;
 // offset -= 0x4000;
 // unsigned hartIx = offset / 8;
 // uint64_t dword = hartIx < hartCount_ ? timeCompare_.at(hartIx) : 0;
 // serializeInt(dword, length, data);
}


void
trickbox::write(uint64_t addr, size_t length, const data_t& data,
		 const strb_t& strb, cbs_t& cbs)
{
  std::cout<<"Trickbox write: 0x"<<std::hex<<addr;
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  if(addr==0x9000000){
    
    unsigned hart = t_data & 0xfff;
    unsigned event = (t_data >> 12) & 0xff;
    unsigned eventValue = (t_data >> 20);
    trickboxInterrupt(hart,event,eventValue,cbs); 
    // SetHartITP(hart,event,eventValue);
    }else if((addr > 0x9000000)&& (addr < 0x9001000)){
   
     int itp_loc  = addr & 0xfc;  
     unsigned hart = t_data & 0xfff; 
     int eventFlag = (t_data >> 12) & 0x1;
     int eventDelay = (t_data >> 13); 
     //SetHartDelayedITP(hart,itp_loc,eventFlag,eventDelay);
    }else if(addr==0x9004000){
     unsigned hart = t_data & 0xfff; 
     int eventFlag = (t_data >> 12) & 0x1; 
     //SetHartRandomITP(hart,eventFlag);
    }

  // uint64_t offset = addr - device::addr();
  // if (offset < 0x4000)
  //   {
  //     // Sofware interrupt: 1 word per hart.
  //     unsigned hartIx = offset / 4;
  //     if ((offset % 4) != 0 or length < 4 or hartIx >= hartCount_)
	// return;
  //     unsigned word = 0;
  //     deserializeInt(data, word);
  //     soft_.at(hartIx) = word & 1;
  //     softwareInterrupt(hartIx, word & 1, cbs);
  //   }

  // if (length == 8)
  //   {
  //     std::lock_guard<std::mutex> lock(mutex_);

  //     offset -= 0x4000;

  //     if (offset == 0x7ff8)
	// deserializeInt(data, timer_);
  //     else
	// {
	//   unsigned hartIx = offset / 8;
	//   if (hartIx >= hartCount_)
	//     return;

	//   // Time compare. 1 double word per hart.
	//   uint64_t dword = 0;
	//   deserializeInt(data, dword);
	//   timeCompare_.at(hartIx) = dword;
	// }

  //     processTimerInterrupts(cbs);
  //   }
}
