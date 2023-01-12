#include "trickbox.h"

DECLARE_string(load);

void trickbox::get_trickbox_addr() {

  memmap::load(memmap_);
  
  
  trickbox_addr_ = memmap_.at("trickbox").base;
  trickbox_addr_begin = memmap_.at("trickbox").base;
  trickbox_addr_end = memmap_.at("trickbox").end;

}

void trickbox::process(const transactions::m_mcmi_store& m_mcmi_store) {

  if (m_mcmi_store.addr >= trickbox_addr_begin && 
       m_mcmi_store.addr < trickbox_addr_end){
   
    uint64_t cycle = m_mcmi_store.cycle;
    cvm::log(cvm::NONE, "<{}> ------------------TRICKBOX ADDR --------------\n", cycle);
    std::cout << "TrickBox Addr : " << m_mcmi_store.addr << " \n";
    std::cout << "TrickBox Addr ###########################\n";
  }else{
   
    std::cout << "TrickBox Addr : " << m_mcmi_store.addr << " \n";

  }

}
