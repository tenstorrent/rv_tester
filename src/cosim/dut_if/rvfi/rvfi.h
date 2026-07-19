#pragma once

#include <fstream>
#include <fmt/format.h>
#include <unordered_map>

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "rv_tester_transactions.hpp"

#include "svdpi.h"
#include "src/cosim/bridge/bridge_if.h"
#include "src/cosim/bridge/bridge.h"
#include "src/cosim/utils/eot/eot.h"
#include "src/cosim/dut_if/mcmi/mcmi.h"
#include "rvfi_plusargs.h"

#include "rv_tester_structs.h"
#include "eot_plusargs.h"

class rvfi {

  template <typename T, typename... Args>
  void connect(cvm::topology::loc_t loc) {
    cvm::registry::messenger.connect<T>(
        loc,
        [this](const T& v) {
          return this->process(v);
        });
    if constexpr (sizeof...(Args)) {
      connect<Args...>(loc);
    }
  }

public:
  rvfi(cvm::topology::loc_t loc, unsigned id);
  ~rvfi();
  void configure();
  void check();

private:
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
  void process(const rv_tester_transactions::cosim::m_mtip<>& m_mtip);
  void process(const rv_tester_transactions::cosim::m_mtime<>& m_mtime);
  void process(const rv_tester_transactions::cosim::m_imsic_msi<>& m_imsic_msi);
  void process(const rv_tester_transactions::cosim::m_debug<>& m_debug);
  void process(const rv_tester_transactions::cosim::m_csri<>& m_csri);
  void process(const rv_tester_transactions::cosim::m_mhpm_counter_ovf<>& m_mhpm_counter_ovf);

  void process(const rv_tester::terminate_called&);
  void process(const rv_tester::terminate_called_mem_checks&);
  void process(const bridge::error_loc&);

  void process_ncio_fetches(const rv_instr_t& instr);
  bool sc_failed(mem_t& write);

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
  bool patch_access(uint64_t addr);
  bool is_ncio(uint32_t mem_attr);
  bool check_axi_error(uint64_t addr);

private:
  std::string mem_attr_to_string(uint32_t mem_attr);
  std::bitset<256> stringToBitset(const std::string& hexString);
  std::bitset<256> extract_bits_as_bitset(const std::bitset<256>& bitset, size_t msb, size_t lsb);
  cvm::file_logger log;
  cvm::topology::loc_t loc_;
  unsigned id_;

  std::shared_ptr<bridge> bridge_;
  std::unique_ptr<mcmi> mcmi_;
  std::unique_ptr<eot> eot_;

  rv_instr_t prev_instr_;

  bool in_reset_ = true;
  uint64_t count_ = 1;

  uint64_t prev_instr_tag_ = 0;
  uint64_t prev_uop_tag_ = 0;
  uint64_t prev_branch_tag_ = 0;

  bool vec_cmode_ = false;
  uint64_t vec_cmode_first_tag_ = 0;
  std::unordered_map<uint64_t, uint64_t> vec_cmode_tags_;
  uint64_t vec_cmode_pc_addr_ = 0;

  // RVDE-24355: Track memory errors during vector conservative mode
  std::unordered_map<uint64_t, bool> vec_cmode_mem_errors_;

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
  bool opcode_modified_ = false;
  bool nmi_ = false;
  bool intr_ = false;
  bool intr_virt_mode_ = false;
  bool excp_ = false;
  bool pc_error_ = false;
  bool mem_error_ = false;
  uint64_t ncause_ = 0;
  uint64_t icause_ = 0;
  uint64_t ecause_ = 0;
  uint8_t priv_ = 3;
  bool ucode_priv_change_ = false;
  uint32_t trap_insn_ = 0;
  uint64_t trap_addr_ = 0;

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

  bool terminated_ = false;
  bool in_debug_mode_ = false;
  bool whisper_reloaded = false;
};
