#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "uc_helper.h"


DECLARE_string(load);


uc_helper::uc_helper(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc)
  : subdevice(tag, addr, 0x4000, loc)
{
  rng.seed(FLAGS_seed);
  uc_helper_base = addr;

  reset();
  checkUsage();
  if (FLAGS_load != "") {
    std::cout << "loading " << FLAGS_load << "\n";
    init_elf(FLAGS_load);
  }
}




cvm::messenger::task<void>
uc_helper::read(uint64_t addr, size_t, data_t&)
{
  co_return;
}

// void uc_helper::read(const transactor::read_t& r, data_t& data) {
//   auto& addr = r.addr;
//   auto& length = r.length;

//   m_.read(addr, length, data.data());
//   return;
// }




uc_helper::~uc_helper()
{
  terminate_ = true;
 
}

void
uc_helper::checkUsage()
{
  
}



bool uc_helper::init_elf(const std::string& path) {
  std::cout<<"[IO_DEV]: Device init elf\n";
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}


void
uc_helper::write(uint64_t addr, size_t, const data_t& data,
		 const strb_t&)
{
  //std::cout<<"uc_helper write: 0x"<<std::hex<<addr;
  //auto& addr = w.addr;
  //auto& length = w.length;
  //auto& data = w.data;
  //auto& strb = w.strb;

  //for (size_t i = 0; i < length; i++) {
  //  if (strb[i]) {
  //    m_.write(addr + i, 1, &data[i]);
  //  }
  //}
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  if(addr==uc_helper_base)
  {
    

    }
    else if((addr > uc_helper_base)&& (addr < (uc_helper_base + 0x1000)))
    {
    
     //std::cout<<"\nuc_helper DELAYED write: 0x"<<std::hex<<addr<<" intr_loc: "<<intr_loc<<" time: "<<timer_<<" eventDelay: "<<eventDelay<<" timercompare :"<<timeCompare_.at(intr_loc)<<" hart "<<hart<<" flag: "<<eventFlag<<"\n";
    }
    else if(addr==(uc_helper_base + 0x4000))
    {
     //TODO If needed enable/disable random interrupts from asm
     //unsigned hart = t_data & 0xfff;
     //int eventFlag = (t_data >> 12) & 0x1;
     //TODO
    }

}
