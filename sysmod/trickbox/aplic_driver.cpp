#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "aplic_driver.h"

 DEFINE_bool(random_aplic_intr, false, "Drive random aplic interrups");
 DEFINE_int32(num_interrupts, 10, "Maximum interrupt index driven in single example");
 DEFINE_int32(toggle_prob, 1, "Maximum interrupts toggle probability ");
 DEFINE_bool(debug_aplic_driver, false, "enables internal state dump of aplic driver ");

aplic_driver::aplic_driver(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc)
  : subdevice(tag, addr, 0x4000 /* size */, loc),
    timeCompare_(6),IntrHart_(6),delayedRandomIntValid_(6),IntrValue_(6), timerIntPrev_(hartCount), timer_(0)
{
  rng.seed(FLAGS_seed);
  aplic_driver_base = addr;
  reset();
  checkUsage();
}


aplic_driver::~aplic_driver()
{
}

void
aplic_driver::checkUsage()
{
  
}


cvm::messenger::task<void>
aplic_driver::read(uint64_t addr, size_t, data_t&)
{
  co_return;
}

void aplic_driver::read_dev(uint64_t , size_t ,  data_t& ){
  return;
}
void
aplic_driver::write(uint64_t addr, size_t, const data_t& data,
		 const strb_t&)
{
  cvm::log(cvm::DEBUG,"\n[APLIC DRIVER] Inside APLIC write Addr {:#x} \n ",addr);
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  cvm::log(cvm::DEBUG,"\n[APLIC DRIVER] Inside APLIC write Data {:#x} \n ",t_data);

  if(addr==aplic_driver_base)
  {

     toggle_cycles  = (t_data & 0xfff0)>>4; 
     num_toggles    = t_data & 0xf;
     cvm::log(cvm::HIGH,"\n[APLIC DRIVER] Set toggle cycles to {} and number of toggles to {} \n ",toggle_cycles,num_toggles);

    }
    else if((addr >= aplic_driver_base+0x100)&& (addr < (aplic_driver_base + 0x200)))
    {
      uint64_t enables_offset = addr - aplic_driver_base ;
      enables_offset = enables_offset - 0x100 ;
      uint64_t enables_value  =   t_data;
      cvm::log(cvm::HIGH,"\n[APLIC DRIVER] Setting enables with offset {:#x} and values {:#x} \n ",enables_offset,enables_value);
      for(int i=0;i<64;i++){
        toggle_enable[enables_offset + i] = NthBitValue(t_data,i);
      }

    }
    else if((addr >= aplic_driver_base+0x200)&& (addr < (aplic_driver_base + 0x300)))
    {
      uint64_t toggle0_offset = addr - (aplic_driver_base+0x200);
      uint64_t toggle0_value  =   t_data;
      cvm::log(cvm::HIGH,"\n[APLIC DRIVER] Setting toggle0 with offset {:#x} and values {:#x} \n ",toggle0_offset,toggle0_value);
      for(int i=0;i<64;i++){
        if(NthBitValue(t_data,i)){
          toggle_type[toggle0_offset + i] = 0;
          toggle_in_progress[toggle0_offset + i] = 1;
          toggle_count[toggle0_offset + i] = num_toggles;
          cycle_count[toggle0_offset + i] = toggle_cycles;
        }
      }
      
    }
    else if((addr >= aplic_driver_base+0x300)&& (addr < (aplic_driver_base + 0x400)))
    {
      uint64_t toggle1_offset = addr - (aplic_driver_base+0x300);
      uint64_t toggle1_value  =   t_data;
      cvm::log(cvm::HIGH,"\n[APLIC DRIVER] Setting toggle1 with offset {:#x} and values {:#x} \n ",toggle1_offset,toggle1_value);
      for(int i=0;i<64;i++){
        if(NthBitValue(t_data,i)){
          toggle_type[toggle1_offset + i] = 1;
          toggle_in_progress[toggle1_offset + i] = 1;
          toggle_count[toggle1_offset + i] = num_toggles;
          cycle_count[toggle1_offset + i] = toggle_cycles;
        }
      }
    }
    else if(addr==(aplic_driver_base + 0x500))
    {

    }
   InterruptLogicStatus();
}
