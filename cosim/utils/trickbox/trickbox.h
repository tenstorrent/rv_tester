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

class trickbox {

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

    // End-of-test (trickbox) options:
    // trickbox=trickbox -- Look for mem store to 'trickbox' address = success/fail
    
    trickbox() {
      // Read trickbox symbol address from elf
      get_trickbox_addr();
      
      connect<
        transactions::m_mcmi_store
      >();
    }

  private:

    void get_trickbox_addr();
    void process(const transactions::m_mcmi_store& m_mcmi_store);

  private:

    std::uint64_t trickbox_addr_ = -1;
    std::uint64_t trickbox_addr_begin = -1;
    std::uint64_t trickbox_addr_end = -1;
    const std::uint8_t trickbox_status_ = 1;
	  const std::uint8_t trickbox_device_syscall_ = 0; 
     // Memmap
    memmap::memmap_list_t memmap_;
};
