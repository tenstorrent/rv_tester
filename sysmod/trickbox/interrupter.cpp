#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "interrupter.h"

 DEFINE_bool(random_intr, false, "Drive random interrups");
 DEFINE_int32(intr_delay_min, 3, "Minimum Delay between 2 consecutive interrupts");
 DEFINE_int32(intr_delay_max, 5, "Maximum Delay between 2 consecutive interrupts");
 DEFINE_int32(seed, 1, "Simulation seed passed down for randomization");
 DEFINE_int32(max_simul_intr, 1, "Maximum simultanious interrupts driven in single example");
 DEFINE_int32(tbox_start_delay, 0, "delay after which random interrupts should start");
 DEFINE_string(intr_disable_mask,"0x00","Set bit in hex string to disable random generation of interrupt i.e. +intr_disable_mask=0x01 will disable interrupt corresponding to bit 0 ");
 DEFINE_bool(disable_ssip,false,"Disable Random SSW interrupt generation ");
 DEFINE_bool(disable_msip,false,"Disable Random MSW interrupt generation ");
 DEFINE_bool(disable_stip,true,"Disable Random ST interrupt generation ");
 DEFINE_bool(disable_mtip,false,"Disable Random MT interrupt generation ");
 DEFINE_bool(disable_seip,false,"Disable Random S EXT interrupt generation ");
 DEFINE_bool(disable_meip,false,"Disable Random M EXT interrupt generation ");
interrupter::interrupter(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc)
  : device(tag, addr, 0x4000 /* size */, loc),
    timeCompare_(6),IntrHart_(6),delayedRandomIntValid_(6),IntrValue_(6), timerIntPrev_(hartCount), timer_(0)
{
  rng.seed(FLAGS_seed);
  interrupter_base = addr;

  reset();
  //populate disable mask as per plusargs
  disable_mask = (FLAGS_disable_meip <<5)|(FLAGS_disable_seip <<4)|(FLAGS_disable_mtip <<3)|(FLAGS_disable_stip <<2)| (FLAGS_disable_msip << 1) |FLAGS_disable_ssip;
  disable_mask_neg = (~disable_mask) & 0xff; 
  cvm::log(cvm::LOW, "[Trickbox] Random Interrupt disable_mask :  {} disable_mask_neg {} \n",disable_mask,disable_mask_neg);
  
}


interrupter::~interrupter()
{
  terminate_ = true;
  if (timerThread_.joinable())
    timerThread_.join();
}


void
interrupter::selfTick(useconds_t delta)
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


cvm::messenger::task<void>
interrupter::read(uint64_t addr, size_t, data_t&)
{
  co_return;
}


void
interrupter::write(uint64_t addr, size_t, const data_t& data,
		 const strb_t&)
{
  //std::cout<<"interrupter write: 0x"<<std::hex<<addr;
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  if(addr==interrupter_base)
  {
    unsigned hart = t_data & 0xfff;
    unsigned event = (t_data >> 12) & 0xfb;//ignore supervisor timer interrupt
    unsigned eventValue = (t_data >> 20) & 0xfb; //ignore supervisor timer interrupt data

    driveInterrupt(hart,event,eventValue);

    }
    else if((addr > interrupter_base)&& (addr < (interrupter_base + 0x1000)))
    {
     //std::cout<<"\ninterrupter DELAYED write: 0x"<<std::hex<<addr<<" data: "<<std::hex<<t_data<<"\n";
     int intr_loc  = addr & 0xf8;
     intr_loc = (intr_loc >>3);
     if(intr_loc!=2){ //ignore supervisor timer interrupt
     unsigned hart = t_data & 0xfff;
     int eventFlag = (t_data >> 12) & 0x1;
     int eventDelay = (t_data >> 13);
     timeCompare_.at(intr_loc) = timer_ + (eventDelay * timer_advance);
     IntrHart_.at(intr_loc) = hart;  // Hart to be interrupted.
     delayedRandomIntValid_.at(intr_loc) = 1; // Valid
     IntrValue_.at(intr_loc) = eventFlag;
     }
     //std::cout<<"\ninterrupter DELAYED write: 0x"<<std::hex<<addr<<" intr_loc: "<<intr_loc<<" time: "<<timer_<<" eventDelay: "<<eventDelay<<" timercompare :"<<timeCompare_.at(intr_loc)<<" hart "<<hart<<" flag: "<<eventFlag<<"\n";
    }
    else if(addr==(interrupter_base + 0x4000))
    {
     //TODO If needed enable/disable random interrupts from asm
     //unsigned hart = t_data & 0xfff;
     //int eventFlag = (t_data >> 12) & 0x1;
     //TODO
    }

}
