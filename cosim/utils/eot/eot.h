#pragma once

#include <string>
#include <map>

#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "cvm/messenger.hpp"
#include "cosim/transactions/transactions.hpp"

#include "vpi_user.h"
#include "util.h"

class eot {

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

    // End-of-test (eot) options:
    // eot=tohost -- Look for mem store to 'tohost' address = success/fail
    
    eot() {
      // Read tohost symbol address from elf
      get_tohost_addr();
      
      connect<
        transactions::m_mcmi_store
      >();
    }

  private:

    void get_tohost_addr();
    void process(const transactions::m_mcmi_store& m_mcmi_store);

  private:

    std::uint64_t tohost_addr_ = -1;
    const std::uint8_t tohost_status_ = 1;
	  const std::uint8_t tohost_device_syscall_ = 0; 
};
