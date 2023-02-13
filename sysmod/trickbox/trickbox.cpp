#include "cvm/plusargs.hpp"
#include "trickbox.h"

DEFINE_bool(random_intr, false, "Drive random interrups");
DEFINE_int32(intr_delay_min, 3, "Minimum Delay between 2 consecutive interrupts");
DEFINE_int32(intr_delay_max, 5, "Maximum Delay between 2 consecutive interrupts");
DEFINE_int32(seed, 1, "Simulation seed passed down for randomization");
trickbox::trickbox(const std::string& tag, uint64_t addr, unsigned hartCount)
  : device(tag, addr, 0xc0000 /* size */), hartCount_(hartCount), soft_(hartCount),
    timeCompare_(6),IntrHart_(6),delayedRandomIntValid_(6),IntrValue_(6), timerIntPrev_(hartCount), timer_(0)
{
  rng.seed(FLAGS_seed);
  interrupter_base = addr;
  interrupter_size = 0x5000;
  
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
  if(addr==interrupter_base)
  {
    unsigned hart = t_data & 0xfff;
    unsigned event = (t_data >> 12) & 0xff;
    unsigned eventValue = (t_data >> 20);
    trickboxInterrupt(hart,event,eventValue,cbs); 
    }
    else if((addr > interrupter_base)&& (addr < (interrupter_base + 0x1000)))
    {
     //std::cout<<"\nTrickbox DELAYED write: 0x"<<std::hex<<addr<<" data: "<<std::hex<<t_data<<"\n";
     int itp_loc  = addr & 0xf8;  
     itp_loc = (itp_loc >>3);
     unsigned hart = t_data & 0xfff; 
     int eventFlag = (t_data >> 12) & 0x1;
     int eventDelay = (t_data >> 13); 
     timeCompare_.at(itp_loc) = timer_ + (eventDelay * timer_advance);
     IntrHart_.at(itp_loc) = hart;  // Hart to be interrupted.
     delayedRandomIntValid_.at(itp_loc) = 1; // Valid 
     IntrValue_.at(itp_loc) = eventFlag;
     //std::cout<<"\nTrickbox DELAYED write: 0x"<<std::hex<<addr<<" itp_loc: "<<itp_loc<<" time: "<<timer_<<" eventDelay: "<<eventDelay<<" timercompare :"<<timeCompare_.at(itp_loc)<<" hart "<<hart<<" flag: "<<eventFlag<<"\n";
    }
    else if(addr==(interrupter_base + 0x4000))
    {
     unsigned hart = t_data & 0xfff; 
     int eventFlag = (t_data >> 12) & 0x1; 
     //TODO 
    }
    
}
