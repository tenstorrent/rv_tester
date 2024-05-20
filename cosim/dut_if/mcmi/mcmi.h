#pragma once

#include <fstream>
#include <fmt/format.h>
#include <unordered_map>

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "rv_tester_transactions.hpp"

#include "svdpi.h"
#include "bridge_if.h"
#include "bridge.h"
#include "cosim/utils/eot/eot.h"

#include "rv_tester/rv_tester_structs.h"

class mcmi {

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

    mcmi(cvm::topology::loc_t loc, unsigned id);

    ~mcmi();

  private:

    void init();
    void set_scope(svScope s) { scope_ = s; }
    void process(const rv_tester_transactions::cosim::m_mcmi_read<>& m_mcmi_read);
    void process(const rv_tester_transactions::cosim::m_mcmi_insert<>& m_mcmi_insert);
    void process(const rv_tester_transactions::cosim::m_mcmi_bypass<>& m_mcmi_bypass);
    void process(const rv_tester_transactions::cosim::m_mcmi_write<>& m_mcmi_write);
    void process(const rv_tester_transactions::cosim::m_mcmi_ifetch_req<>& m_mcmi_ifetch_req);
    void process(const rv_tester_transactions::cosim::m_mcmi_ifetch_resp<>& m_mcmi_ifetch_resp);
    void process(const rv_tester_transactions::cosim::m_mcmi_ievict<>& m_mcmi_ievict);
    void process(const rv_tester_transactions::cosim::m_ncio_axi_wr_req<>& m_ncio_axi_wr_req);
    void process(const rv_tester::terminate_called&);
    void check();

  private:

    cvm::topology::loc_t loc_;
    unsigned id_;

    std::unique_ptr<bridge> bridge_;

    void process_amo(mem_t& read);
    bool sc_failed(mem_t& write);
    void amo_modify_write_data(amo_op op, uint64_t& read_data, uint64_t& write_data, uint8_t size);
    template <typename T>
    void amo_arithmetic(amo_op op, uint64_t& read_data, uint64_t& write_data, uint8_t size);

    std::unordered_map<uint64_t, mem_t> ifetch_reqs_;
    std::unordered_map<uint64_t, mem_t> amo_writes_;
    std::unordered_map<uint64_t, mem_t> sc_result_;
    std::unordered_map<uint64_t, mem_t> sc_bypass_;

    std::unordered_map<uint64_t, mem_t> mcmi_bypass_ncio_map_;

    svScope scope_;

    bool terminated_ = false;

};
