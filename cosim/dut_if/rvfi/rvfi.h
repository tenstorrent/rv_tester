#pragma once

#include <fstream>
#include <fmt/format.h>

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "rv_tester_transactions.hpp"

#include "svdpi.h"
#include "bridge_if.h"
#include "bridge.h"
#include "cosim/utils/eot/eot.h"
#include "pmcounters/pmcounters.hpp"

class rvfi {

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

    rvfi(cvm::topology::loc_t loc, unsigned id);

    ~rvfi();

  private:

    void init();
    void set_scope(svScope s) { scope_ = s; }
    void process(const rv_tester_transactions::cosim::m_rvfi& m_rvfi);
    void process(const rv_tester_transactions::cosim::m_trap& m_trap);
    void process(const rv_tester_transactions::cosim::m_intr& m_intr);
    void process(const rv_tester_transactions::cosim::m_debug& m_debug);

    std::tuple<uint64_t, uint64_t, uint8_t> get_mem_attributes(uint64_t addr, uint8_t mask, uint64_t data);

    void make_instr(const rv_tester_transactions::cosim::m_rvfi& m_rvfi, rv_instr_t& instr);
    void print_instr(rv_instr_t& instr);
    void print_instr_resource(rv_instr_t& instr, std::string resource_str);
    void send_instr(rv_instr_t& instr);
    void enter_debug_mode(rv_instr_t& instr);
    void exit_debug_mode(rv_instr_t& instr);

    void initialize_perf();
    void collect_perf(const rv_tester_transactions::cosim::m_rvfi& m_rvfi);
    void report_perf();

  private:

    cvm::file_logger log;

    std::unique_ptr<bridge> bridge_;
    std::unique_ptr<eot> eot_;

    rv_instr_t prev_instr_;

    uint64_t count_ = 1;

    bool ucode_ = false;
    bool intr_ = false;
    bool excp_ = false;
    uint64_t icause_ = 0;
    uint64_t ecause_ = 0;

    // perf
    bool perf_ok_ = false;
    uint64_t perf_start_pc_;
    uint64_t perf_start_cycle_ = 0;
    uint64_t perf_end_pc_;
    uint64_t perf_end_cycle_ = 0;
    uint64_t perf_instrs_ = 0;
    pmcounters pmcs;

    cvm::topology::loc_t loc_;
    svScope scope_;
};
