#pragma once

#include <string>
#include <map>

#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "cvm/messenger.hpp"
#include "cosim/transactions/transactions.hpp"
#include "memmap.h"
#include "vpi_user.h"
#include "util.h"
#include "trickbox.h"
class trickbox_helper {

  template<typename T, typename... Args> void connect() {
    cvm::messenger<T>::connect(
      [this] (const T& v) {
        return this->process(v);
      }
    );
    if constexpr (sizeof...(Args)) {
      connect<Args...>();
    }
  }
  
  public:

    // End-of-test (trickbox_helper) options:
    // trickbox_helper=trickbox_helper -- Look for mem store to 'trickbox_helper' address = success/fail
    
    trickbox tbox{"trickbox_h",0x9000000,0xc000};

    trickbox_helper() {
      // Read trickbox_helper symbol address from elf
      //trickbox tbox1{"trickbox_h",trickbox_helper_addr_,trickbox_helper_size};
      //tbox = &tbox1;
      std::cout<< "CONST:constructing Trickbox Helper\n";
      get_trickbox_helper_addr();
      
      connect<
        transactions::m_mcmi_store
      >();
    }

  private:

    void get_trickbox_helper_addr();
    void process(const transactions::m_mcmi_store& m_mcmi_store);

  private:

    std::uint64_t trickbox_helper_addr_ = -1;
    std::uint64_t trickbox_helper_addr_begin = -1;
    std::uint64_t trickbox_helper_addr_end = -1;
    std::uint64_t trickbox_helper_size = -1;
    const std::uint8_t trickbox_helper_status_ = 1;
	  const std::uint8_t trickbox_helper_device_syscall_ = 0; 
    
    //trickbox* tbox;//{"trickbox_h",0x9000000,0x3000};
     // Memmap
    memmap::memmap_list_t memmap_;
};
