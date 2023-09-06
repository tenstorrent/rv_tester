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
  checkUsage();
  cvm::log (cvm::HIGH,"axi_mst_loc_l for msi_driver :{}\n",axi_mst_loc_l);
  //uint32_t addr1 = 0x900;
  //  uint32_t length1 = 4;
  //  std::vector<uint8_t> data1 = {0xba,0xad,0xf0,0x12};
  //  std::vector<bool> strb1 = {1,1,1,1,1,1,1,1,1};

  //cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr1, length1, data1, strb1});
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
  
    cvm::log(cvm::HIGH, "[MSI Driver] check usage\n");
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

    driveInterrupt(hart,event,eventValue); //temp placeholder for msi call

    }
    else if((addr > msi_driver_base)&& (addr < (msi_driver_base + 0x1000)))
    {
     std::cout<<"\nmsi_driver DELAYED write: 0x"<<std::hex<<addr<<" data: "<<std::hex<<t_data<<"\n";
    }
    else if(addr==(msi_driver_base + 0x4000))
    {
     //TODO If needed enable/disable random interrupts from asm
     //unsigned hart = t_data & 0xfff;
     //int eventFlag = (t_data >> 12) & 0x1;
     //TODO
    }

}
