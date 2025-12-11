#pragma once

#include <fstream>
#include <fmt/format.h>
#include <unordered_map>
#include <memory>

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "rv_tester_transactions.hpp"

#include "cosim/bridge/bridge.h"
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

    mcmi(cvm::topology::loc_t loc, unsigned id, std::shared_ptr<bridge> bridge);
    ~mcmi();
    void check();

  private:

    void init();
    void process(const rv_tester_transactions::cosim::m_reset<>& m_reset);
    void process(const rv_tester_transactions::cosim::m_trap<>& m_trap);
    void process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi);
    void process(const rv_tester_transactions::cosim::m_mcmi_read<>& m_mcmi_read);
    void process(const rv_tester_transactions::cosim::m_mcmi_insert<>& m_mcmi_insert);
    void process(const rv_tester_transactions::cosim::m_mcmi_bypass<>& m_mcmi_bypass);
    void process(const rv_tester_transactions::cosim::m_mcmi_write<>& m_mcmi_write);
    void process(const rv_tester_transactions::cosim::m_mcmi_ifetch_req<>& m_mcmi_ifetch_req);
    void process(const rv_tester_transactions::cosim::m_mcmi_ifetch_resp<>& m_mcmi_ifetch_resp);
    void process(const rv_tester_transactions::cosim::m_mcmi_ievict<>& m_mcmi_ievict);
    void process(const rv_tester_transactions::cosim::m_mcmi_devict<>& m_mcmi_devict);
    void process(const rv_tester_transactions::cosim::m_mcmi_flush<>& m_mcmi_flush);
    void process(const rv_tester_transactions::cosim::m_mcmi_writeback<>& m_mcmi_writeback);
    void process(const rv_tester_transactions::cosim::m_mcmi_dfetch<>& m_mcmi_dfetch);

    void process(const rv_tester::terminate_called&);
    void process(const rv_tester::terminate_called_mem_checks&);
    void process(const bridge::error_loc &);

    bool patch_access (uint64_t addr);
    bool is_ncio(uint32_t mem_attr);
    bool check_axi_error(uint64_t addr);
    std::bitset<256> stringToBitset(const std::string& hexString);
    std::bitset<256> extract_bits_as_bitset(const std::bitset<256>& bitset, size_t msb, size_t lsb);
    bool sc_failed(mem_t& write);
    void process_amo(mem_t& read);
    void amo_modify_write_data(amo_op op, uint64_t& read_data, uint64_t& write_data, uint8_t size);
    void process_ncio_fetches(const rv_instr_t& instr);

    template <typename T>
    void amo_arithmetic(amo_op op, uint64_t& read_data, uint64_t& write_data, uint8_t size);

  private:

    cvm::file_logger log;
    [[maybe_unused]] cvm::topology::loc_t loc_;
    [[maybe_unused]] unsigned id_;

    std::shared_ptr<bridge> bridge_;

    bool in_reset_ = true;
    bool terminated_ = false;

    // Shared state that may need to be synchronized with rvfi
    bool ncio_mem_transition_ = false;
    std::vector<mem_t> ncio_fetches_;
    std::vector<mem_t> active_ncio_fetches_;

    std::unordered_map<uint64_t, mem_t> ifetch_reqs_;
    std::unordered_map<uint64_t, mem_t> amo_writes_;
    std::unordered_map<uint64_t, mem_t> sc_result_;
    std::unordered_map<uint64_t, mem_t> sc_bypass_;

    bool mem_error_ = false;
    uint64_t ecause_ = 0;
    std::unordered_map<uint64_t, bool> vec_cmode_mem_errors_;
    bool patch_mode_ = false;
    bool vec_cmode_ = false;
    std::unordered_map<uint64_t, uint64_t> vec_cmode_tags_;
    uint64_t vec_cmode_first_tag_ = 0;
    uint64_t vec_cmode_pc_addr_ = 0;

    uint64_t patch_mode_first_tag_ = 0;
    std::unordered_map<uint64_t, uint64_t> patch_mode_tags_;

};
