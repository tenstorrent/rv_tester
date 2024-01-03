#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "aplic_driver.h"

 DEFINE_bool(random_aplic_intr, false, "Drive random aplic interrups");
 DEFINE_int32(num_interrupts, 10, "Maximum interrupt index driven in single example");
 DEFINE_int32(toggle_prob, 1, "Maximum interrupts toggle probability ");

aplic_driver::aplic_driver(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc)
  : subdevice(tag, addr, 0x4000 /* size */, loc),
    timeCompare_(6),IntrHart_(6),delayedRandomIntValid_(6),IntrValue_(6), timerIntPrev_(hartCount), timer_(0)
{
  rng.seed(FLAGS_seed);
  aplic_driver_base = addr;
  reset();

  //  aplic_pin_values[0] = 1;
  //  aplic_pin_values[1] = 2;
  //  aplic_pin_values[2] = 7;
  //   //aplic_pin_values_vec;
  // cvm::registry::messenger.signal(loc, aplic_driver_write_t{aplic_pin_values_vec});
  checkUsage();
}


aplic_driver::~aplic_driver()
{
  terminate_ = true;
  if (timerThread_.joinable())
    timerThread_.join();
}

void
aplic_driver::checkUsage()
{
  
}

void
aplic_driver::selfTick(useconds_t delta)
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
  //std::cout<<"\n*****INSIDE APLIC WRITE***\n";
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);

  //std::cout<<"\n*****INSIDE APLIC WRITE data:"<<std::hex<<t_data<<"\n";
  if(addr==aplic_driver_base)
  {

     aplic_tx_type_e tx_type = APLIC_CFG;
     unsigned toggle_cycles  = (t_data & 0xfff0)>>4; 
     unsigned num_toggles    = t_data & 0xf;
     cvm::registry::messenger.signal(loc(), aplic_data_t{tx_type, toggle_cycles, num_toggles, 0,0,0,0,0,0});

    }
    else if((addr >= aplic_driver_base+100)&& (addr < (aplic_driver_base + 0x200)))
    {
      aplic_tx_type_e tx_type = APLIC_EN;
      uint64_t enables_offset = addr - (aplic_driver_base+100);
      uint64_t enables_value  =   t_data;
      cvm::registry::messenger.signal(loc(), aplic_data_t{tx_type, 0, 0, enables_offset,enables_value,0,0,0,0});

    }
    else if((addr >= aplic_driver_base+200)&& (addr < (aplic_driver_base + 0x300)))
    {
      aplic_tx_type_e tx_type = APLIC_T0;
      uint64_t toggle0_offset = addr - (aplic_driver_base+200);
      uint64_t toggle0_value  =   t_data;
      cvm::registry::messenger.signal(loc(), aplic_data_t{tx_type, 0,0, 0,0,toggle0_offset,toggle0_value,0,0});

    }
    else if((addr >= aplic_driver_base+300)&& (addr < (aplic_driver_base + 0x400)))
    {
      aplic_tx_type_e tx_type = APLIC_T1;
      uint64_t toggle1_offset = addr - (aplic_driver_base+300);
      uint64_t toggle1_value  =   t_data;
      cvm::registry::messenger.signal(loc(), aplic_data_t{tx_type, 0,0 , 0,0,0,0,toggle1_offset,toggle1_value});

    }
    else if(addr==(aplic_driver_base + 0x500))
    {

    }

}
