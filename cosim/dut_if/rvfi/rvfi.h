#pragma once

#include <fstream>

#include "cvm/messenger.hpp"
#include "cvm/logger.hpp"
#include "cosim/transactions/transactions.hpp"

#include "bridge_if.h"
#include "bridge.h"
#include "eot.h"
#include "clint_helper.h"
#include "trickbox_helper.h"

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
    
    std::tuple<uint64_t, uint64_t, uint8_t> get_mem_attributes(uint64_t addr, uint8_t mask, uint64_t data);

    void make_instr(const transactions::m_rvfi& m_rvfi, rv_instr_t& instr);
    void print_instr(rv_instr_t& instr);
    void send_instr(rv_instr_t& instr);

  private:

    cvm::file_logger log;

    std::unique_ptr<bridge> bridge_;
    std::unique_ptr<eot> eot_;
    std::unique_ptr<trickbox_helper> trickbox_helper_;
    std::unique_ptr<clint_helper> clint_helper_;

    uint64_t count_ = 0;

    bool intr_ = false;
    bool excp_ = false;
    uint64_t icause_ = 0;
    uint64_t ecause_ = 0;
};
