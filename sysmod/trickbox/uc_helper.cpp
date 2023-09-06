#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "uc_helper.h"


DECLARE_string(load);
DEFINE_bool(debug_uc_helper, false, "Enable internal uc helper debug logging");

uc_helper::uc_helper(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc, mem_manager &m_)
  : subdevice(tag, addr, 0x1000, loc),m_(m_)
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
  cvm::log(cvm::HIGH, "[UC_HELPER] read_dev read addr {:#x} data {:#x} \n",addr,word);
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
 //For Future FLAG usage  
}


void
 uc_helper::write(uint64_t addr, size_t , const data_t& data,
 		 const strb_t&)
 {
   uint64_t t_data=0;
   //datum_t  ip_data;

   cvm::log(cvm::HIGH, "[UC_HELPER] write addr {:#x}  \n",addr);

   if (not has_addr(addr))
     return;
  
   deserializeInt(data, t_data);
   cvm::log(cvm::HIGH, "[UC_HELPER] write data {:#x} \n",t_data);

   //std::vector<uint8_t> data_l={1,2,3,4};
   //std::vector<bool> strb_l={1,1,1,1,1};

   if(addr==uc_helper_base)
   {
     // ip_data = (datum_t)t_data;
     //m_.write(addr, 1, &ip_data);

     if(t_data>0){
       cvm::log(cvm::ERROR, "[UC_Helper] Only Clearing of UC_helper Status allowed, Illegal to set status bit manually \n");
     }
     tx_status = t_data & 0x1;
     }
     else if(addr == (uc_helper_base + 0x100))
     {
      tx_addr = t_data;
      //ip_data = (datum_t)t_data;
      //m_.write(addr, 1, &ip_data);
      cvm::log(cvm::HIGH, "[UC_HELPER] Transfer Start Addr {:#x} \n",t_data);
      //std::cout<<"\nuc_helper DELAYED write: 0x"<<std::hex<<addr<<" intr_loc: "<<intr_loc<<" time: "<<timer_<<" eventDelay: "<<eventDelay<<" timercompare :"<<timeCompare_.at(intr_loc)<<" hart "<<hart<<" flag: "<<eventFlag<<"\n";
     }
     else if(addr ==(uc_helper_base + 0x200))
     {
      tx_size = t_data;
      //ip_data = (datum_t)t_data;
      //m_.write(addr, 1, &ip_data);
      cvm::log(cvm::HIGH, "[UC_HELPER] Transfer Size {:#x} bytes \n",t_data);
      //std::cout<<"UC HELPER tx size: "<<std::hex<<t_data<<"\n";
     }
     else if(addr ==(uc_helper_base + 0x300))
     {
      //std::cout<<"UC HELPER tx trigger: "<<std::hex<<t_data<<"\n";
      int hart = 0;
      cvm::log(cvm::HIGH, "[UC_HELPER] Transfer triggered {:#x}  \n",t_data);
      //ip_data = (datum_t)t_data;
      //m_.write(addr, 1, &ip_data);
      tx_trigger = 0;
    
       //unsigned byte_c = 0;
    
       for (size_t i = 0; i < tx_size; i++) {
         uint32_t pcg_op = rng();
         bool valid;
         pcg_op = pcg_op & 0xff;
         uint8_t m_data = (uint8_t)pcg_op;
         uint64_t poke_data = m_data;
         //std::cout<<"\nwriting random data to uc area addr: "<<std::hex<< tx_addr+i<<" Data: "<<std::hex<<pcg_op<<"\n";
         cvm::log(cvm::HIGH, "[UC_HELPER] writing random data to uc area : addr {:#x} data {:#x} \n",tx_addr+i,pcg_op);
      
         mem::datum_t m_data_p = (mem::datum_t)m_data;
      
         m_.write(tx_addr + i, 1, &m_data_p);
         //Poke same data to whisper memory        
         if (!client_->whisperPoke(hart, 0, 'm', tx_addr + i, poke_data, valid)) {
          cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
          return;
         }
      
         //send signal to sysmod memory
         uint64_t push_addr = tx_addr + i;
         std::vector<uint8_t> data_vec={};
         std::vector<bool> strb_vec={};
         data_vec.push_back(m_data_p);
         strb_vec.push_back(1);
         cvm::registry::messenger.signal(loc(), uc_helper_write_t{push_addr, 1, data_vec, strb_vec});
      
         if(FLAGS_debug_uc_helper){
           mem::datum_t m_data_p1;
           m_.read(tx_addr + i, 1, &m_data_p1);
           std::cout<<"\n[UC_HELPER] reading data from mem_manager : "<<std::hex<<tx_addr<<" Data: "<< (uint32_t)m_data_p1<<"\n";
           std::cout<<"\n[UC_HELPER] Printing data sent to sysmod sysmem address :"<<std::hex<<push_addr<<" Data : ";
           for (auto i: data_vec){
             std::cout << (unsigned )i << ' ';
           }
           std::cout<<"\n";
         }
       }
       //Transfer(DMA) writes completed, Indicate completion by setting status bit to non zero
       tx_trigger = 1;
       mem::datum_t m_data_p1 = 0xff;
       m_.write(uc_helper_base,1,&m_data_p1);
   
       //std::cout << "TRICKBOX UCH READ::::: ADDR: "<<std::hex<<uc_helper_base<<"\n";
       cvm::log(cvm::HIGH, "[UC_HELPER] Init of Address Range Completed  \n");
   
       return;
     }

 }
