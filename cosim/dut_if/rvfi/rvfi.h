#pragma once

#include "cvm/messenger.hpp"
#include "cosim/transactions/transactions.hpp"

#include "bridge_if.h"
#include "bridge.h"


class rvfi {

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

    rvfi();
    void reset();

  private:

    void init();
    void process(const transactions::m_rvfi& m_rvfi);
    void process(const transactions::m_trap& m_trap);
    void process(const transactions::m_intr& m_intr);
    
    void make_instr(const transactions::m_rvfi& m_rvfi, rv_instr_t& instr);
    void print_instr(rv_instr_t& instr);
    void send_instr(rv_instr_t& instr);

  private:

    std::unique_ptr<bridge> bridge_;

    bool intr = false;
    bool excp = false;
    uint64_t icause = 0;
    uint64_t ecause = 0;
};
