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
void
uc_helper::read_dev(uint64_t addr, size_t length, data_t& data)
{
  mem::datum_t m_data;
  m_.read(addr, 1, &m_data);
  
  uint32_t word = (uint32_t)m_data;
  std::cout<<"UC HELPER READ addr: "<<std::hex<<addr<<" data: "<<word<<"\n";
  serializeInt(word, length, data);
  return;
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
uc_helper::write(uint64_t addr, size_t , const data_t& data,
		 const strb_t&)
{
  std::cout<<"UC HELPER write addr: "<<std::hex<<addr<<"\n";

  if (not has_addr(addr))
    return;

  uint64_t t_data=0;
  deserializeInt(data, t_data);
  std::cout<<"UC HELPER write data: "<<std::hex<<t_data<<"\n";
  datum_t ip_data;
  std::vector<uint8_t> data_l={1,2,3,4};
  std::vector<bool> strb_l={1,1,1,1,1};
  //cvm::registry::messenger.signal(loc(), uc_helper_write_t{addr, length, data_l, strb_l});
  if(addr==uc_helper_base)
  {
    ip_data = (datum_t)t_data;
    m_.write(addr, 1, &ip_data);
    if(t_data>0){
      cvm::log(cvm::ERROR, "[UC_Helper] Illegal to set status bit manually \n");
    }
    tx_status = t_data & 0x1;

    }
    else if(addr == (uc_helper_base + 0x100))
    {
     tx_addr = t_data;
     ip_data = (datum_t)t_data;
     m_.write(addr, 1, &ip_data);
     std::cout<<"UC HELPER tx addr: "<<std::hex<<t_data<<"\n";
     //std::cout<<"\nuc_helper DELAYED write: 0x"<<std::hex<<addr<<" intr_loc: "<<intr_loc<<" time: "<<timer_<<" eventDelay: "<<eventDelay<<" timercompare :"<<timeCompare_.at(intr_loc)<<" hart "<<hart<<" flag: "<<eventFlag<<"\n";
    }
    else if(addr ==(uc_helper_base + 0x200))
    {
     tx_size = t_data;
     ip_data = (datum_t)t_data;
     m_.write(addr, 1, &ip_data);
     std::cout<<"UC HELPER tx size: "<<std::hex<<t_data<<"\n";
    }
    else if(addr ==(uc_helper_base + 0x300))
    {
     std::cout<<"UC HELPER tx trigger: "<<std::hex<<t_data<<"\n";
    ip_data = (datum_t)t_data;
    m_.write(addr, 1, &ip_data);
     tx_trigger = 0;
      
      //unsigned byte_c = 0;
      
      for (size_t i = 0; i < tx_size; i++) {
        uint32_t pcg_op = rng();
        pcg_op = pcg_op & 0xff;
        uint8_t m_data = (uint8_t)pcg_op;
        std::cout<<"\nwriting random data to uc area addr: "<<std::hex<< tx_addr+i<<" Data: "<<std::hex<<pcg_op<<"\n";
        mem::datum_t m_data_p = (mem::datum_t)m_data;
        m_.write(tx_addr + i, 1, &m_data_p);
        mem::datum_t m_data_p1;
        m_.read(tx_addr + i, 1, &m_data_p1);
        std::cout<<"\nreading data from: "<<std::hex<<tx_addr<<" Data: "<< (uint32_t)m_data_p1<<"\n";
        uint64_t push_addr = tx_addr + i;
        std::vector<uint8_t> data_vec={};
        std::vector<bool> strb_vec={};
        data_vec.push_back(m_data_p);
        strb_vec.push_back(1);
        std::cout<<"\n###################\n";
        
        for (auto i: data_vec){
         std::cout << (unsigned )i << ' ';
        }
        
        std::cout<<"\n###################\n";
        cvm::registry::messenger.signal(loc(), uc_helper_write_t{push_addr, 1, data_vec, strb_vec});

      }
      tx_trigger = 1;
      mem::datum_t m_data_p1 = 0xff;
      //m_.write(uc_helper_base,3,const_cast<mem::datum_t*>(m_data_p1));
      m_.write(uc_helper_base,1,&m_data_p1);

      std::cout << "TRICKBOX UCH READ::::: ADDR: "<<std::hex<<uc_helper_base<<"\n";
      //data_t rd_data;
//      m_.read(uc_helper_base, 1, &rd);
      //std::cout << "TRICKBOX UCH READ::::: DATA: "<<std::hex<<rd_data<<"\n";
      return;
    }

}
