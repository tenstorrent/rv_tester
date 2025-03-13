#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "interrupter.h"
#include "sysmod/sysmod_plusargs.h"

 DEFINE_bool(dbg_rnmi_ebreak_trigger, false, "To generate trigger for RNMI when ebreak is executed");
 DEFINE_bool(dbg_rnmi_priority_trigger, false, "To generate trigger for RNMI when debug haltreq is asserted");
 DEFINE_bool(dbg_rnmi_priv_de_trigger, false, "To generate trigger for RNMI when privmode is DE");
 DEFINE_bool(dbg_rnmi_priv_dp_trigger, false, "To generate trigger for RNMI when privmode is DP");
 DEFINE_bool(injectintr, false, "Drive interrups at uarch events based on harness code");
 DEFINE_bool(random_imsic_intr, false, "Drive random interrups");
 DEFINE_bool(disable_m_imsic_intr, false, "Drive random imsic  interrups to M file");
 DEFINE_bool(disable_s_imsic_intr, false, "Drive random imsic  interrups to S file");
 DEFINE_bool(disable_vs_imsic_intr, true, "Drive random imsic  interrups to VS file");
 DEFINE_bool(disable_random_hart_imsic_intr, false, "Drive random imsic  interrups to random harts");
 DEFINE_int32(imsic_intr_delay_min, 3, "Minimum Delay between 2 consecutive interrupts");
 DEFINE_int32(imsic_intr_delay_max, 5, "Maximum Delay between 2 consecutive interrupts");
 DEFINE_int32(imsic_intr_threshold, 256, "imsic_intr interrupts threshold value");
 DEFINE_int32(imsic_vs_intr_threshold, 63, "imsic_vs_intr interrupts threshold value");
 DEFINE_int32(imsic_vs_id_threshold, 5, "imsic guest file id threshold value");
 DEFINE_int32(imsic_hart_threshold, 1, "harts threshold value");
 DEFINE_int32(imsic_intr_start_delay, 5000, "delay after which random interrupts should start");
 DEFINE_string(imsic_intr_disable_mask,"0x00","Set bit in hex string to disable random generation of interrupt i.e. +imsic_intr_disable_mask=0x01 will disable interrupt corresponding to bit 0 ");
 DEFINE_int32(max_intr_count, 0, "Maximum interrupts that can be driven per test");

interrupter::interrupter(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc,cvm::topology::loc_t axi_mst_loc)
  : subdevice(tag, addr, 0x50000 /* size */, loc), axi_mst_loc_l(axi_mst_loc),
    timeCompare_(6),IntrHart_(6),delayedRandomIntValid_(6),IntrValue_(6), timerIntPrev_(hartCount), timer_(0) 
{
  rng.seed(FLAGS_seed);
  interrupter_base = addr;
  reset();
  checkUsage();
   if(FLAGS_max_intr_count>0){
     limit_interrupts = 1;
   }
  intr_loc_ = cvm::topology::get_from_type("INTERRUPTS", 0);
}


interrupter::~interrupter()
{
}

uint64_t mnscratch_array[2];
void interrupter::read_dev(uint64_t addr, size_t length,  data_t& data){
  if(addr==(interrupter_base + 0x2000)){
    serializeInt(mnscratch_array[0], length, data);
  }
  else if(addr==(interrupter_base + 0x2008)){
    serializeInt(mnscratch_array[1], length, data);
  }
  return;
}


void
interrupter::checkUsage()
{
  
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
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  if (addr == interrupter_base) {
    //63:0 -> supervisor/hypervisor id hart[], mode_h_s_m[3-> 1:0 ],interrupt_num[1024->9:0] 
    //mask:    0xfff                   0xfff        0xf                  0xfff
    cvm::log(cvm::HIGH, "[Trickbox] IMSIC write - addr={:#x} data={:#x}\n", addr, t_data);
    driveMSIInterrupt(t_data);
  }
  else if ((addr > interrupter_base) && (addr < (interrupter_base + 0x1000))) {
     cvm::log(cvm::HIGH, "[Trickbox] IMSIC write delayed - addr={:#x} data={:#x}\n", addr, t_data);
  }
  else if(addr==(interrupter_base + 0x2000)){
    mnscratch_array[0] = t_data;
  }
  else if(addr==(interrupter_base + 0x2008)){
    mnscratch_array[1] = t_data;
  }
  else if (addr == (interrupter_base + 0x4000)) { // FIXME missing functionality
     cvm::log(cvm::HIGH, "[Trickbox] IMSIC write - no match - addr={:#x} data={:#x}\n", addr, t_data);
  }
  else if (addr == (interrupter_base + 0x5000)) {
     cvm::registry::messenger.signal<uint8_t>(intr_loc_, 0);
     cvm::log(cvm::HIGH, "[Trickbox] NMI deassert\n");
  }

}
