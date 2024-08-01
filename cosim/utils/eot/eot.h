#pragma once
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
    eot(cvm::topology::loc_t , unsigned id) {
      // Read tohost symbol address from elf
      id_ = id;
      get_tohost_addr();
      for (uint32_t i = 0; i < num_harts_; i++) {
        instr_count_.push_back(0);
        connect<
          rv_tester_transactions::cosim::m_rvfi<>,
          rv_tester_transactions::cosim::m_steps<>,
          rv_tester_transactions::cosim::m_mcmi_insert<>,
          rv_tester_transactions::cosim::m_mcmi_bypass<>
        >(cvm::topology::get_from_type("COSIM", i));
      }
    }
    eot(cvm::topology::loc_t loc): eot(loc, 1){}
    ~eot();

  private:

    void get_tohost_addr();
    void process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi);
    void process(const rv_tester_transactions::cosim::m_steps<>& m_steps);
    void process(const rv_tester_transactions::cosim::m_mcmi_insert<>& m_mcmi_insert);
    void process(const rv_tester_transactions::cosim::m_mcmi_bypass<>& m_mcmi_bypass);
    void process_tohost(uint64_t hartid, uint64_t cycle, uint64_t address, uint64_t data);

  private:

    unsigned id_;
    uint32_t previous_cycle_ = 0;
    uint32_t num_harts_ = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
    std::vector<uint32_t> instr_count_;
    std::vector<uint64_t> terminated_harts_;
    std::uint64_t tohost_addr_ = -1;
    const std::uint8_t tohost_status_ = 1;
    const std::uint8_t tohost_device_syscall_ = 0;
    bool ended_ = false;
    std::chrono::time_point<std::chrono::system_clock> start, end;
};

