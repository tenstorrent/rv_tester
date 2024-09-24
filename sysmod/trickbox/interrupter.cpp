#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "interrupter.h"
#include "sysmod/sysmod_plusargs.h"

 DEFINE_bool(random_intr, false, "Drive random interrups");
 DEFINE_bool(injectintr, false, "Drive interrups at uarch events based on harness code");
 DEFINE_int32(intr_delay_min, 3, "Minimum Delay between 2 consecutive interrupts");
 DEFINE_int32(max_intr_count, 0, "Maximum interrupts that can be driven per test");
 DEFINE_int32(intr_delay_max, 5, "Maximum Delay between 2 consecutive interrupts");
 DEFINE_int32(max_simul_intr, 1, "Maximum simultanious interrupts driven in single example");
 DEFINE_int32(tbox_start_delay, 25, "delay after which random interrupts should start");
 DEFINE_string(intr_disable_mask,"0x00","Set bit in hex string to disable random generation of interrupt i.e. +intr_disable_mask=0x01 will disable interrupt corresponding to bit 0 ");
 DEFINE_bool(disable_ssip,true,"Disable Random SSW interrupt generation ");
 DEFINE_bool(disable_msip,true,"Disable Random MSW interrupt generation ");
 DEFINE_bool(disable_stip,true,"Disable Random ST interrupt generation ");
 DEFINE_bool(disable_mtip,false,"Disable Random MT interrupt generation ");
 DEFINE_bool(disable_seip,true,"Disable Random S EXT interrupt generation ");
 DEFINE_bool(disable_meip,true,"Disable Random M EXT interrupt generation ");
interrupter::interrupter(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc)
  : subdevice(tag, addr, 0x4000 /* size */, loc),
    timeCompare_(6),IntrHart_(6),delayedRandomIntValid_(6),IntrValue_(6), timerIntPrev_(hartCount), timer_(0)
{
  rng.seed(FLAGS_seed);
  interrupter_base = addr;

  reset();

  //populate disable mask as per plusargs
  disable_mask = (FLAGS_disable_meip <<5)|(FLAGS_disable_seip <<4)|(FLAGS_disable_mtip <<3)|(FLAGS_disable_stip <<2)| (FLAGS_disable_msip << 1) |FLAGS_disable_ssip;
  disable_mask_neg = (~disable_mask) & 0xff;

  if (FLAGS_random_intr)
    cvm::log(cvm::LOW, "[Trickbox] Random Interrupts enabled - [mei:{}, sei:{}, mti:{}, sti:{}, msi:{}, ssi:{}]\n",
      FLAGS_disable_meip, FLAGS_disable_seip, FLAGS_disable_mtip, FLAGS_disable_stip, FLAGS_disable_msip, FLAGS_disable_ssip);

 cvm::log(cvm::LOW, "[Trickbox][Random Interrupts] Limit for random interrupts {} \n",FLAGS_max_intr_count);
  if(FLAGS_max_intr_count>0)
    limit_interrupts = 1;

  checkUsage();
}


interrupter::~interrupter()
{
}

void
interrupter::checkUsage()
{
  unsigned active_interrupts = numInterrupts_ - (FLAGS_disable_ssip + FLAGS_disable_msip + FLAGS_disable_stip + FLAGS_disable_mtip + FLAGS_disable_seip + FLAGS_disable_meip);
  if(FLAGS_random_intr){
    if(disable_mask == ((1<<numInterrupts_)-1)){
    //Error: asked to generate random interrupts when all interrupts are disabled
    cvm::log(cvm::ERROR, "Error: Trickbox cannot drive random interrupts when all interrupts are disabled\n");
    }
    //max simul intr can't be more than enabled interrupts
    if((unsigned)FLAGS_max_simul_intr > active_interrupts){
    //Cant drive more interrupts than active
    cvm::log(cvm::ERROR, "Error: Trickbox cannot drive max_simul_intr={} interrupts when {} interrupts are enabled\n",FLAGS_max_simul_intr,active_interrupts);
    }
  }
}

cvm::messenger::task<void>
interrupter::read(uint64_t addr, size_t, data_t&)
{
  co_return;
}
uint64_t mnscratch_array[2];
void interrupter::read_dev(uint64_t addr, size_t ,  data_t& data){
  if(addr==(interrupter_base + 0x4000)){
    data[0] = mnscratch_array[0];
  }
  else if(addr==(interrupter_base + 0x4008)){
    data[0] = mnscratch_array[1];
  }
  return;
}
void
interrupter::write(uint64_t addr, size_t, const data_t& data,
		 const strb_t&)
{
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
    cvm::log(cvm::LOW, "[Trickbox] Driving event {:#x} eventVal {:#x}\n", event, eventValue);


    if ( eventValue==0 && event!=0 ) 
    disable_dontpick = event;
    cvm::log(cvm::LOW, "[Trickbox] Disabledontpick {:#x} \n", disable_dontpick);

    }
    else if((addr > interrupter_base)&& (addr < (interrupter_base + 0x1000)))
    {
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
    }
    else if(addr==(interrupter_base + 0x4000)){
      mnscratch_array[0] = data[0];
    }
    else if(addr==(interrupter_base + 0x4008)){
      mnscratch_array[1] = data[0];
    }
    else if(addr==(interrupter_base + 0x4016))
    {
     //TODO If needed enable/disable random interrupts from asm
     //unsigned hart = t_data & 0xfff;
     //int eventFlag = (t_data >> 12) & 0x1;
     //TODO
    }

}
