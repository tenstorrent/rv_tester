#pragma once

#include <string>
#include <map>

#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "cvm/registry.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/topology.hpp"
#include "rv_tester_transactions.hpp"

#include "util.h"

class eot {

  template<typename T, typename... Args> void connect(cvm::topology::loc_t loc) {
    cvm::registry::messenger.connect<T>(
      loc,
      [this] (const T& v) {
        return this->process(v);
      }
    );
    if constexpr (sizeof...(Args)) {
      connect<Args...>(loc);
    }
  }

  public:

    // End-of-test (eot) options:
    // eot=tohost -- Look for mem store to 'tohost' address = success/fail

    eot(cvm::topology::loc_t loc) {
      // Read tohost symbol address from elf
      get_tohost_addr();

      connect<
        rv_tester_transactions::cosim::m_rvfi,
        rv_tester_transactions::cosim::m_mcmi_insert
      >(loc);
    }

  private:

    void get_tohost_addr();
    void process(const rv_tester_transactions::cosim::m_rvfi& m_rvfi);
    void process(const rv_tester_transactions::cosim::m_mcmi_insert& m_mcmi_insert);

  private:

    uint32_t instr_count_ = 0;
    std::uint64_t tohost_addr_ = -1;
    const std::uint8_t tohost_status_ = 1;
    const std::uint8_t tohost_device_syscall_ = 0;
};
