#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "msi_driver.h"

 DEFINE_bool(random_msi, false, "Drive random interrups");
 DEFINE_int32(msi_delay_min, 3, "Minimum Delay between 2 consecutive interrupts");
 DEFINE_int32(msi_delay_max, 5, "Maximum Delay between 2 consecutive interrupts");
 DEFINE_int32(max_simul_msi, 1, "Maximum simultanious interrupts driven in single example");
 DEFINE_int32(msi_start_delay, 0, "delay after which random interrupts should start");
 DEFINE_string(msi_disable_mask,"0x00","Set bit in hex string to disable random generation of interrupt i.e. +msi_disable_mask=0x01 will disable interrupt corresponding to bit 0 ");

msi_driver::msi_driver(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc,cvm::topology::loc_t axi_mst_loc)
  : subdevice(tag, addr, 0x4000 /* size */, loc), axi_mst_loc_l(axi_mst_loc),
    timeCompare_(6),IntrHart_(6),delayedRandomIntValid_(6),IntrValue_(6), timerIntPrev_(hartCount), timer_(0) 
{
  rng.seed(FLAGS_seed);
  msi_driver_base = addr;

  reset();
  //populate disable mask as per plusargs
  disable_mask = (FLAGS_disable_meip <<5)|(FLAGS_disable_seip <<4)|(FLAGS_disable_mtip <<3)|(FLAGS_disable_stip <<2)| (FLAGS_disable_msip << 1) |FLAGS_disable_ssip;
  disable_mask_neg = (~disable_mask) & 0xff;
  cvm::log(cvm::LOW, "[Trickbox] Random Interrupt disable_mask :  {} disable_mask_neg {} \n",disable_mask,disable_mask_neg);
  checkUsage();
  cvm::log (cvm::HIGH,"axi_mst_loc_l for msi_driver :{}",axi_mst_loc_l);
}


msi_driver::~msi_driver()
{
  terminate_ = true;
  if (timerThread_.joinable())
    timerThread_.join();
}

void
msi_driver::checkUsage()
{
  unsigned active_interrupts = numInterrupts_ - (FLAGS_disable_ssip + FLAGS_disable_msip + FLAGS_disable_stip + FLAGS_disable_mtip + FLAGS_disable_seip + FLAGS_disable_meip);
  if(FLAGS_random_msi){
    if(disable_mask == ((1<<numInterrupts_)-1)){
    //Error: asked to generate random interrupts when all interrupts are disabled
    cvm::log(cvm::ERROR, "[Trickbox] Can not drive random interrupts when all interrupts are disabled\n");
    }
    //max simul msi can't be more than enabled interrupts
    if((unsigned)FLAGS_max_simul_msi > active_interrupts){
    //Cant drive more interrupts than active
    cvm::log(cvm::ERROR, "[Trickbox] Can not drive {} interrupts when {} interrupts are enabled\n",FLAGS_max_simul_msi,active_interrupts);
    }
  }
}

void
msi_driver::selfTick(useconds_t delta)
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
msi_driver::read(uint64_t addr, size_t, data_t&)
{
  co_return;
}


void
msi_driver::write(uint64_t addr, size_t, const data_t& data,
		 const strb_t&)
{
  //std::cout<<"msi_driver write: 0x"<<std::hex<<addr;
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  if(addr==msi_driver_base)
  {
    unsigned hart = t_data & 0xfff;
    unsigned event = (t_data >> 12) & 0xfb;//ignore supervisor timer interrupt
    unsigned eventValue = (t_data >> 20) & 0xfb; //ignore supervisor timer interrupt data

    driveInterrupt(hart,event,eventValue);

    }
    else if((addr > msi_driver_base)&& (addr < (msi_driver_base + 0x1000)))
    {
     std::cout<<"\nmsi_driver DELAYED write: 0x"<<std::hex<<addr<<" data: "<<std::hex<<t_data<<"\n";
    // int msi_loc  = addr & 0xf8;
   //  msi_loc = (msi_loc >>3);
     //if(msi_loc!=2){ //ignore supervisor timer interrupt
     //unsigned hart = t_data & 0xfff;
     //int eventFlag = (t_data >> 12) & 0x1;
     //int eventDelay = (t_data >> 13);
     //timeCompare_.at(msi_loc) = timer_ + (eventDelay * timer_advance);
    // msiHart_.at(msi_loc) = hart;  // Hart to be interrupted.
   //  delayedRandomIntValid_.at(msi_loc) = 1; // Valid
    // msiValue_.at(msi_loc) = eventFlag;
     //}
     //std::cout<<"\nmsi_driver DELAYED write: 0x"<<std::hex<<addr<<" intr_loc: "<<intr_loc<<" time: "<<timer_<<" eventDelay: "<<eventDelay<<" timercompare :"<<timeCompare_.at(intr_loc)<<" hart "<<hart<<" flag: "<<eventFlag<<"\n";
    }
    else if(addr==(msi_driver_base + 0x4000))
    {
     //TODO If needed enable/disable random interrupts from asm
     //unsigned hart = t_data & 0xfff;
     //int eventFlag = (t_data >> 12) & 0x1;
     //TODO
    }

}
