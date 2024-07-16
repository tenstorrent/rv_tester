#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "imsic_driver.h"
#include "sysmod/sysmod_plusargs.h"

 DEFINE_bool(random_imsic_intr, false, "Drive random interrups");
 DEFINE_bool(disable_m_imsic_intr, false, "Drive random imsic  interrups to M file");
 DEFINE_bool(disable_s_imsic_intr, true, "Drive random imsic  interrups to S file");
 DEFINE_bool(disable_vs_imsic_intr, true, "Drive random imsic  interrups to VS file");
 DEFINE_bool(disable_random_hart_imsic_intr, false, "Drive random imsic  interrups to random harts");
 DEFINE_int32(imsic_intr_delay_min, 3, "Minimum Delay between 2 consecutive interrupts");
 DEFINE_int32(imsic_intr_delay_max, 5, "Maximum Delay between 2 consecutive interrupts");
 DEFINE_int32(imsic_intr_threshold, 255, "imsic_intr interrupts threshold value");
 DEFINE_int32(imsic_vs_intr_threshold, 63, "imsic_vs_intr interrupts threshold value");
 DEFINE_int32(imsic_hart_threshold, 1, "harts threshold value");
 DEFINE_int32(imsic_intr_start_delay, 5000, "delay after which random interrupts should start");
 DEFINE_string(imsic_intr_disable_mask,"0x00","Set bit in hex string to disable random generation of interrupt i.e. +imsic_intr_disable_mask=0x01 will disable interrupt corresponding to bit 0 ");

imsic_driver::imsic_driver(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc,cvm::topology::loc_t axi_mst_loc)
  : subdevice(tag, addr, 0x4000 /* size */, loc), axi_mst_loc_l(axi_mst_loc),
    timeCompare_(6),IntrHart_(6),delayedRandomIntValid_(6),IntrValue_(6), timerIntPrev_(hartCount), timer_(0) 
{
  rng.seed(FLAGS_seed);
  imsic_driver_base = addr;
  reset();
  checkUsage();
   if(FLAGS_max_intr_count>0){
     limit_interrupts = 1;
   }
}


imsic_driver::~imsic_driver()
{
}

void
imsic_driver::read_dev(uint64_t addr, size_t , data_t& )
{
  cvm::log(cvm::HIGH, "[Trickbox] IMSIC read addr={:#x} \n", addr);
  return;
}


void
imsic_driver::checkUsage()
{
  
}


cvm::messenger::task<void>
imsic_driver::read(uint64_t addr, size_t, data_t&)
{
  co_return;
}


void
imsic_driver::write(uint64_t addr, size_t, const data_t& data,
		 const strb_t&)
{
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  if (addr == imsic_driver_base) {
    //63:0 -> supervisor/hypervisor id hart[], mode_h_s_m[3-> 1:0 ],interrupt_num[1024->9:0] 
    //mask:    0xfff                   0xfff        0xf                  0xfff
    cvm::log(cvm::HIGH, "[Trickbox] IMSIC write - addr={:#x} data={:#x}\n", addr, t_data);
    driveMSIInterrupt(t_data);
  }
  else if ((addr > imsic_driver_base) && (addr < (imsic_driver_base + 0x1000))) {
     cvm::log(cvm::HIGH, "[Trickbox] IMSIC write delayed - addr={:#x} data={:#x}\n", addr, t_data);
  }
  else if (addr == (imsic_driver_base + 0x4000)) { // FIXME missing functionality
     cvm::log(cvm::HIGH, "[Trickbox] IMSIC write - no match - addr={:#x} data={:#x}\n", addr, t_data);
    }

}
