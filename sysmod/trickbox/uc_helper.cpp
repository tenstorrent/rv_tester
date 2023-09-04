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
  
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  if(addr==uc_helper_base)
  {
    if(t_data>0){
      cvm::log(cvm::ERROR, "[UC_Helper] Illegal to set status bit manually \n");
    }
    tx_status = t_data & 0x1;

    }
    else if(addr == (uc_helper_base + 0x100))
    {
     tx_addr = t_data;
     //std::cout<<"\nuc_helper DELAYED write: 0x"<<std::hex<<addr<<" intr_loc: "<<intr_loc<<" time: "<<timer_<<" eventDelay: "<<eventDelay<<" timercompare :"<<timeCompare_.at(intr_loc)<<" hart "<<hart<<" flag: "<<eventFlag<<"\n";
    }
    else if(addr ==(uc_helper_base + 0x200))
    {
     tx_size = t_data;
    }
    else if(addr ==(uc_helper_base + 0x300))
    {
     tx_trigger = 0;
      for (size_t i = 0; i < tx_size; i++) {
        uint8_t m_data = rng();
        mem::datum_t *m_data_p = NULL;
        *m_data_p = (mem::datum_t)m_data;
         m_.write(tx_addr + i, 1, m_data_p);
      }
      tx_trigger = 1;
    }

}
