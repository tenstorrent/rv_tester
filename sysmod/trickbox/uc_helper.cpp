#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "uc_helper.h"


DECLARE_string(load);


uc_helper::uc_helper(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc, mem_manager &m_)
  : subdevice(tag, addr, 0x4000, loc),m_(m_)
{
  rng.seed(FLAGS_seed);
  uc_helper_base = addr;
  reset();
  checkUsage();

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




void
uc_helper::write(uint64_t addr, size_t, const data_t& data,
		 const strb_t&)
{
  std::cout<<"UC HELPER write addr: "<<std::hex<<addr<<"\n";
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  std::cout<<"UC HELPER write data: "<<std::hex<<t_data<<"\n";
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
     std::cout<<"UC HELPER tx addr: "<<std::hex<<t_data<<"\n";
     //std::cout<<"\nuc_helper DELAYED write: 0x"<<std::hex<<addr<<" intr_loc: "<<intr_loc<<" time: "<<timer_<<" eventDelay: "<<eventDelay<<" timercompare :"<<timeCompare_.at(intr_loc)<<" hart "<<hart<<" flag: "<<eventFlag<<"\n";
    }
    else if(addr ==(uc_helper_base + 0x200))
    {
     tx_size = t_data;
     std::cout<<"UC HELPER tx size: "<<std::hex<<t_data<<"\n";
    }
    else if(addr ==(uc_helper_base + 0x300))
    {
     std::cout<<"UC HELPER tx trigger: "<<std::hex<<t_data<<"\n";
     tx_trigger = 0;
      for (size_t i = 0; i < tx_size; i++) {
        uint32_t pcg_op = rng();
        pcg_op = pcg_op & 0xff;
        uint8_t m_data = (uint8_t)pcg_op;
        std::cout<<"\nwriting random data to uc area addr: "<<std::hex<< tx_addr+i<<" Data: "<<std::hex<<pcg_op<<"\n";
        mem::datum_t m_data_p[1] = {(mem::datum_t)m_data};
        //*m_data_p = (mem::datum_t)m_data;
        
         m_.write(tx_addr + i, 1, const_cast<mem::datum_t*>(m_data_p));
      }
      tx_trigger = 1;
      mem::datum_t m_data_p1[1] = {1};
      m_.write(uc_helper_base,1,const_cast<mem::datum_t*>(m_data_p1));
    }

}
