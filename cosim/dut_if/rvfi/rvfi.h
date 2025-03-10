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
#include "cosim/utils/eot/eot_plusargs.h"


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
    void check();

  private:

    void init();
    void set_scope(svScope s) { scope_ = s; }
    void process(const rv_tester_transactions::cosim::m_reset<>& m_reset);
    void process(const rv_tester_transactions::cosim::m_disable_checks<>& m_disable_checks);
    void process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi);
    void process(const rv_tester_transactions::cosim::m_steps<>& m_steps);
    void process(const rv_tester_transactions::cosim::m_trap<>& m_trap);
    void process(const rv_tester_transactions::cosim::m_gp_regs<>& m_gp_regs);
    void process(const rv_tester_transactions::cosim::m_fp_regs<>& m_fp_regs);
    void process(const rv_tester_transactions::cosim::m_vc_regs<>& m_vc_regs);
    void process(const rv_tester_transactions::cosim::m_core_nmi<>& m_core_nmi);
    void process(const rv_tester_transactions::cosim::m_interrupt_pend<>& m_interrupt_pend);
    void process(const rv_tester_transactions::cosim::m_mtime<>& m_mtime);
    void process(const rv_tester_transactions::cosim::m_imsic_msi<>& m_imsic_msi);
    void process(const rv_tester_transactions::cosim::m_debug<>& m_debug);
    void process(const rv_tester_transactions::cosim::m_csri<>& m_csri);

    // FIXME Move out to a different file?
    void process(const rv_tester_transactions::cosim::m_mcmi_read<>& m_mcmi_read);
    void process(const rv_tester_transactions::cosim::m_mcmi_insert<>& m_mcmi_insert);
    void process(const rv_tester_transactions::cosim::m_mcmi_bypass<>& m_mcmi_bypass);
    void process(const rv_tester_transactions::cosim::m_mcmi_write<>& m_mcmi_write);
    void process(const rv_tester_transactions::cosim::m_mcmi_ifetch_req<>& m_mcmi_ifetch_req);
    void process(const rv_tester_transactions::cosim::m_mcmi_ifetch_resp<>& m_mcmi_ifetch_resp);
    void process(const rv_tester_transactions::cosim::m_mcmi_ievict<>& m_mcmi_ievict);

    void process(const rv_tester::terminate_called&);
    void process(const bridge::error_loc &);

    void process_ncio_fetches(const rv_instr_t& instr);
    void process_amo(mem_t& read);
    bool sc_failed(mem_t& write);
    void amo_modify_write_data(amo_op op, uint64_t& read_data, uint64_t& write_data, uint8_t size);

    template <typename T>
    void amo_arithmetic(amo_op op, uint64_t& read_data, uint64_t& write_data, uint8_t size);

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
    bool patch_access (uint64_t addr);
    bool is_ncio(uint32_t mem_attr);

  private:

    std::string mem_attr_to_string(uint32_t mem_attr);
    std::bitset<256> stringToBitset(const std::string& hexString);
    std::bitset<256> extract_bits_as_bitset(const std::bitset<256>& bitset, size_t msb, size_t lsb);
    cvm::file_logger log;
    cvm::topology::loc_t loc_;
    unsigned id_;

    std::unique_ptr<bridge> bridge_;
    std::unique_ptr<eot> eot_;

    rv_instr_t prev_instr_;

    bool in_reset_ = true;
    uint64_t count_ = 1;

    uint64_t prev_instr_tag_ = 0;
    uint64_t prev_uop_tag_ = 0;
    uint64_t prev_branch_tag_ = 0;

    bool     vec_cmode_ = false;
    uint64_t vec_cmode_first_tag_ = 0;
    std::unordered_map<uint64_t, uint64_t> vec_cmode_tags_;

    bool patch_mode_ = false;
    uint64_t patch_mode_first_tag_ = 0;
    std::unordered_map<uint64_t, uint64_t> patch_mode_tags_;

    bool ncio_mem_transition_ = false;
    std::vector<mem_t> ncio_fetches_;
    std::vector<mem_t> active_ncio_fetches_;

    //---------------------------------------------------------------------------------------------------------
    // USE_OLD_CODE selects C code for priv_, first_uop,ucode_ generation instead of SV code (for debug ONLY)
    //   - eventually we will remove this
    //---------------------------------------------------------------------------------------------------------
    //#define USE_OLD_CODE 1
    bool ucode_ = false;
    bool nmi_ = false;
    bool intr_ = false;
    bool excp_ = false;
    uint64_t ncause_ = 0;
    uint64_t icause_ = 0;
    uint64_t ecause_ = 0;
    uint8_t priv_ = 3;
    bool ucode_priv_change_ = false;
    uint32_t trap_insn_ = 0;

    std::vector<rv_instr_t> instrs_;
    std::vector<vr_t> cracked_vrs_;
    std::vector<csr_t> hw_csrs_, ucode_csrs_;
    uint32_t cracked_flags_ = 0;
    bool cracked_ = false;
    gpr_s cracked_gpr_;

    std::unordered_map<uint64_t, mem_t> ifetch_reqs_;
    std::unordered_map<uint64_t, mem_t> amo_writes_;
    std::unordered_map<uint64_t, mem_t> sc_result_;
    std::unordered_map<uint64_t, mem_t> sc_bypass_;

    svScope scope_;

    bool terminated_ = false;
    bool in_debug_mode_ = false;
};
