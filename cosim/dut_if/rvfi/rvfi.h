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
    void process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi);
    void process(const rv_tester_transactions::cosim::m_trap<>& m_trap);
    void process(const rv_tester_transactions::cosim::m_core_intr<>& m_core_intr);
    void process(const rv_tester_transactions::cosim::m_imsic_msi<>& m_imsic_msi);
    void process(const rv_tester_transactions::cosim::m_debug<>& m_debug);
    void process(const rv_tester_transactions::cosim::m_csri<>& m_csri);
    void process(const rv_tester::terminate_called&);

    std::tuple<uint64_t, uint64_t, uint8_t> get_mem_attributes(uint64_t addr, uint8_t mask, uint64_t data);

    void make_instr(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi, rv_instr_t& instr);
    void print_csr(csr_t& csr);
    void append_uop_changes_to_instr(rv_instr_t& instr);
    void print_instr(const rv_instr_t& instr);
    void print_instr_resource(const rv_instr_t& instr, std::string resource_str);
    void send_instr(rv_instr_t& instr);
    void send_csr(csr_t& csr);
    void send_instr_group(hart_id_t hart, rv_instr_group_t& group);
    void enter_debug_mode(rv_instr_t& instr);
    void exit_debug_mode(rv_instr_t& instr);
    std::string mem_attr_to_string(uint32_t mem_attr); 

  private:

    cvm::file_logger log;
    cvm::topology::loc_t loc_;
    unsigned id_;

    std::unique_ptr<bridge> bridge_;
    std::unique_ptr<eot> eot_;

    rv_instr_t prev_instr_;

    uint64_t count_ = 1;

    bool ucode_ = false;
    bool intr_ = false;
    bool excp_ = false;
    uint64_t icause_ = 0;
    uint64_t ecause_ = 0;
    uint8_t priv_ = 3;
    bool ucode_priv_change_ = false;

    std::vector<rv_instr_t> instrs_;
    std::vector<vr_t> cracked_vrs_;
    std::vector<csr_t> hw_csrs_, ucode_csrs_;
    gpr_s cracked_gpr_;

    svScope scope_;

    bool terminated_ = false;
};
