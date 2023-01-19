#include "trickbox_helper.h"

DECLARE_string(load);

void trickbox_helper::get_trickbox_helper_addr() {

  memmap::load(memmap_);
  
  //std::cout<< "#### GETTING TRICKBOX ADDR ####\n"; 
  trickbox_helper_addr_ = memmap_.at("trickbox").base;
  trickbox_helper_size = memmap_.at("trickbox").size;
  trickbox_helper_addr_begin = memmap_.at("trickbox").base;
  trickbox_helper_addr_end = memmap_.at("trickbox").end;

}

void trickbox_helper::process(const transactions::m_mcmi_store& m_mcmi_store) {

  if (m_mcmi_store.addr >= trickbox_helper_addr_begin && 
       m_mcmi_store.addr < trickbox_helper_addr_end){
   
    uint64_t cycle = m_mcmi_store.cycle;
    cvm::log(cvm::NONE, "<{}> ------------------trickbox_helper ADDR --------------\n", cycle);
    std::cout << "trickbox_helper Addr : " <<std::hex<< m_mcmi_store.addr << " \n";
    std::cout << "trickbox_helper Addr ###########################\n";
    tbox.write_helper(m_mcmi_store.addr,m_mcmi_store.data);
  }else{
   
    std::cout << "Non trickbox_helper Addr : " <<std::hex<< m_mcmi_store.addr << " \n";

  }

}
