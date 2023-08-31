#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "imsic_driver.h"

 DEFINE_bool(random_imsic_intr, false, "Drive random interrups");
 DEFINE_int32(imsic_intr_delay_min, 3, "Minimum Delay between 2 consecutive interrupts");
 DEFINE_int32(imsic_intr_delay_max, 5, "Maximum Delay between 2 consecutive interrupts");
 DEFINE_int32(imsic_intr_threshold, 63, "imsic_intr interrupts threshold value");
 DEFINE_int32(imsic_intr_start_delay, 0, "delay after which random interrupts should start");
 DEFINE_string(imsic_intr_disable_mask,"0x00","Set bit in hex string to disable random generation of interrupt i.e. +imsic_intr_disable_mask=0x01 will disable interrupt corresponding to bit 0 ");

imsic_driver::imsic_driver(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc,cvm::topology::loc_t axi_mst_loc)
  : subdevice(tag, addr, 0x4000 /* size */, loc), axi_mst_loc_l(axi_mst_loc),
    timeCompare_(6),IntrHart_(6),delayedRandomIntValid_(6),IntrValue_(6), timerIntPrev_(hartCount), timer_(0) 
{
  rng.seed(FLAGS_seed);
  imsic_driver_base = addr;

  reset();
  //populate disable mask as per plusargs
  checkUsage();
  cvm::log (cvm::HIGH,"axi_mst_loc_l for imsic_driver :{}",axi_mst_loc_l);
  //uint32_t addr1 = 0x900;
  //  uint32_t length1 = 4;
  //  std::vector<uint8_t> data1 = {0xba,0xad,0xf0,0x12};
  //  std::vector<bool> strb1 = {1,1,1,1,1,1,1,1,1};

  //cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr1, length1, data1, strb1});
}


imsic_driver::~imsic_driver()
{
  terminate_ = true;
  if (timerThread_.joinable())
    timerThread_.join();
}

void
imsic_driver::checkUsage()
{
  
    cvm::log(cvm::HIGH, "[imsic_intr Driver] check usage\n");
}

void
imsic_driver::selfTick(useconds_t delta)
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
imsic_driver::read(uint64_t addr, size_t, data_t&)
{
  co_return;
}


void
imsic_driver::write(uint64_t addr, size_t, const data_t& data,
		 const strb_t&)
{
  std::cout<<"imsic_driver write: 0x"<<std::hex<<addr<<" has_addr : "<<has_addr(addr)<<"\n";
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  if(addr==imsic_driver_base)
  {
    //63:0 -> supervisor/hypervisor id hart[], mode_h_s_m[3-> 1:0 ],interrupt_num[1024->9:0] 
    //mask:    0xfff                   0xfff        0xf                  0xfff
    //uint8_t interrupt_num = t_data & 0xff;
    //unsigned interrupt_file = (t_data>>12) & 0xf;
    //unsigned interrupt_hart = (t_data>>16) & 0xfff;
    //unsigned vs_id = (t_data>>28) & 0xfff;

    //std::cout<<"Requested MSI interrupt num "<<interrupt_num <<" interrupt file: "<<interrupt_file <<" Interrupt hart:"<< interrupt_hart <<" hypervisor/supervisor id : "<<vs_id<<"\n";
    
    //uint32_t addr1 = 0x900;
    //if(interrupt_file == 0x0){
       //addr1 = msi_m_file_addr;
    //}else if(interrupt_file == 0x01){
       //addr1 = msi_v_file_addr;
    //}else if(interrupt_file == 0x02){
       //addr1 = msi_vs_file_addr;
    //}else{
       //cvm::log(cvm::ERROR, "[MSI driver] Wrong interrupt file specified\n");
    //}
    //uint32_t length1 = 1;
    //std::vector<uint8_t> data1 = {interrupt_num};
    ////std::vector<uint8_t> data1 = {0xba,0xad,0xf0,0x12};
    //std::vector<bool> strb1 = {1,1,1,1,1,1,1};

    //cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr1, length1, data1, strb1});
    std::cout<<"imsic_driver sending data to MSI parsing 0x"<<std::hex<< t_data <<"\n";
    driveMSIInterrupt(t_data);
    }
    else if((addr > imsic_driver_base)&& (addr < (imsic_driver_base + 0x1000)))
    {
     std::cout<<"\nimsic_driver DELAYED write: 0x"<<std::hex<<addr<<" data: "<<std::hex<<t_data<<"\n";
    }
    else if(addr==(imsic_driver_base + 0x4000))
    {
     //TODO If needed enable/disable random interrupts from asm
     //unsigned hart = t_data & 0xfff;
     //int eventFlag = (t_data >> 12) & 0x1;
     //TODO
     std::cout<<"imsic_intr DRIVER no condition met \n";
    }

}
