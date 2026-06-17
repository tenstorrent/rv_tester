// Licensed under the Apache License, Version 2.0, see LICENSE.TT for details

#pragma once

#include <string>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <vector>
#include <map>
#include <regex>

#include "cvm/topology.hpp"
#include "bridge_base.h"
#include "common/memmap.h"
#include "cvm/logger.hpp"
#include "src/cac_core.h"
#include "src/cac_lib.h"
#include "src/common/parser.hpp"

#include "whisper_client.h"
#include "rv_tester_structs.h"
#include "cvm/registry.hpp"
#include <fmt/format.h>
#include "csr_param.hpp"
using namespace CSR;

class bridge : public bridge_base {

private:
  using src_t = cac::src_t;
  using resource_t = cac::resource_t;
  using resource_id_t = cac::resource_id_t;
  using CacCore = cac::CacCore;


public:
  struct error_loc {};

  bridge(int num_harts, int xlen, int vlen, cvm::topology::loc_t loc, unsigned id);
  ~bridge();

  void configure();
  // DUT Interface API
  // Process instruction called on retire
  //   - Metadata
  //   - PC
  //   - Register Read/Write
  //   - CSRs
  //   - Memory Access
  //   - AMO
  //   - Table Walks
  //   - Exceptions/interrupt
  virtual void process_dut_excp(hart_id_t hart, uint64_t cause, uint64_t order, uint64_t vec_cmode_first_tag);
  virtual void process_dut_instr_retire(hart_id_t hart, rv_instr_t& d) override;
  virtual void process_steps(hart_id_t hart, uint32_t n_retire, uint64_t cycle, uint64_t steps, uint64_t skips, uint64_t final_steps) override;
  virtual void process_dut_instr_group_retire(hart_id_t hart, rv_instr_group_t& d) override;
  virtual void process_dut_csr_hw_update(hart_id_t hart, csr_t& c) override;
  virtual void process_counter_overflow(csr_t& c) override;
  virtual void process_compare_gp_regs(hart_id_t hart, uint64_t cycle, const std::array<std::uint64_t, 32>& array);
  virtual void process_compare_fp_regs(hart_id_t hart, uint64_t cycle, const std::array<std::uint64_t, 32>& array);
  virtual void process_compare_vc_regs(hart_id_t hart, uint64_t cycle, const std::array<std::bitset<256>, 32>& array);
  virtual void process_compare_vc_regs(hart_id_t hart, uint64_t cycle, const std::array<std::uint64_t, 32>& array);

  // Process memory access
  //   - Read (Ld completion)
  //   - Insert (St merge buffer insertion)
  //   - Write (St cache write)
  virtual void process_dut_mcm_read(hart_id_t hart, mem_t& m, bool cache) override;
  virtual void process_dut_mcm_insert(hart_id_t hart, mem_t& m) override;
  virtual void process_dut_mcm_bypass(hart_id_t hart, mem_t& m, bool cache) override;
  virtual void process_dut_mcm_write(hart_id_t hart, mem_cl_t& m) override;
  virtual void process_dut_mcm_ifetch(hart_id_t hart, mem_t& m) override;
  virtual void process_dut_mcm_ievict(hart_id_t hart, mem_t& m) override;

  // Interrupts
  virtual void process_dut_nmi(hart_id_t hart, rv_nmi_t& n) override;
  virtual void process_dut_interrupt(hart_id_t hart, rv_intr_t& i) override;
  virtual void process_dut_timer(hart_id_t hart, rv_intr_t& i) override;
  virtual void process_dut_mtip(hart_id_t hart, uint64_t cycle, bool mtip, bool trap_intr) override;
  virtual void process_dut_imsic_msi(hart_id_t hart, mem_t& m) override;

  // Debug mode
  virtual void enter_debug_mode(rv_debug_t& d) override;
  virtual void exit_debug_mode(rv_debug_t& d) override;
  virtual void process_debug_haltreq(bool haltreq) override;

  void reset(uint64_t restart_pc=0);
  void csr_init();

  void final_phase();
  void report_metrics();
  void process(const rv_tester::terminate_called &);
  void process(const rv_tester::terminate_called_mem_checks &);
  void set_patch_mode(int patch) { patch_mode_ = static_cast<patch_mode> (patch); }

private:

  typedef enum {
    read,
    write,
    fetch
  } memclass_t;


  // Overload for vector<string> that allows regex matching.
  // The provided pattern is compiled to a std::regex and used to match each element.
  inline bool find(const std::vector<std::string>& container, const std::string &pattern) {
      if (container.empty()) return false;
      std::regex re(pattern);
      for (const auto &str : container) {
          if (std::regex_match(str, re)) {
              return true;
          }
      }
      return false;
  }

  template <typename... Args>
      void print(cvm::verbosity_level v, Args&&... args) {
          cvm::log(v, std::forward<Args>(args)...);
          if (v <= cvm::verbosity_level::ERROR) {
              cvm::registry::messenger.signal<error_loc>(loc_, {});
          }
      }
  template <typename... Args>
      void error(std::string_view format, Args&&... args) {
          std::string prefix = "Error: ";
          if (patch_mode_) { prefix += "PATCH ";}
          std::string out ="\n" + prefix + fmt::format(fmt::runtime(format), std::forward<Args>(args)...) + "\n"; // for those who forget newline
          print(cvm::ERROR, out);
      }
  bool flags_bridge_log_;
  template <typename... Args>
      void bridge_log(Args&&... args) {
        if (flags_bridge_log_) bridge_log_(std::forward<Args>(args)...);
      }

private:

  void update_dut_state(hart_id_t hart, rv_instr_t& d);
  void arch_state(whisper_state_t& w);
  void update_whisper_state(hart_id_t hart, whisper_state_t& w, bool dut_is_compressed=false, bool page4kX=false, bool dut_opcode_rewritten=false);
  void step(hart_id_t hart, whisper_state_t& w);
  void compare_dut_whisper_state(hart_id_t hart, const whisper_state_t& w, rv_instr_t& d);
  void print_instr(hart_id_t hart, const whisper_state_t& w);
  void print_instr_stdout(hart_id_t hart, const rv_instr_t& d);
  void print_instr_stdout(hart_id_t hart, const whisper_state_t& w);
  void print_resource(hart_id_t hart, const whisper_state_t& w);
  void update_pc(hart_id_t hart, src_t src, uint64_t data);
  void update_priv(hart_id_t hart, src_t src, uint32_t data);
  void update_insn(hart_id_t hart, src_t src, uint32_t data);
  void update_flags(hart_id_t hart, src_t src, uint32_t data);
  void update_regs(hart_id_t hart, const rv_instr_t& d);
  void update_regs(hart_id_t hart, const whisper_state_t& w, uint32_t vec_slice_index = 0);
  void update_regs(hart_id_t hart, src_t src, resource_t resource, uint64_t addr, const std::vector<uint64_t>&& dword_vec);
  void update_mem_attr(hart_id_t hart, src_t src, uint32_t data, uint32_t offset = 0);
  void update_csr(hart_id_t hart, src_t src, uint64_t addr, uint64_t data, cac::optional_const_ref<uint64_t> mask_ref = std::nullopt, bool shadow_csr = false, bool check_en = true);
  uint64_t modify_csr_data(hart_id_t hart, uint64_t addr, uint64_t data, uint8_t priv);
  uint64_t modify_csr_mask(hart_id_t hart, uint64_t addr, uint64_t data, uint64_t mask);
  uint64_t get_csr(hart_id_t hart, src_t src, uint64_t addr);
  uint64_t get_csr_mask(hart_id_t hart, uint64_t addr);
  uint64_t get_csr_poke_mask(hart_id_t hart, uint64_t addr);
  std::string get_csr_name(const std::string& addr);
  bool is_custom_csr(uint64_t addr);
  bool is_csr_allowlist(uint64_t addr);
  bool is_csr_allowlist(const std::string& csr_name);
  bool is_mtimecmp_mmr(uint64_t addr);
  bool is_mtime_mmr(uint64_t addr);
  void peek_resource(hart_id_t hart, char resource, uint64_t addr, uint64_t& data);
  void poke_resource(hart_id_t hart, uint64_t cycle, char resource, uint64_t addr, uint64_t data);
  void poke_mem(hart_id_t hart, uint64_t cycle, uint64_t addr, unsigned size, uint64_t data, bool cache, bool skipmem);

  void translation_check(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w);
  uint64_t translate(hart_id_t hart, uint64_t va, uint8_t priv, memclass_t memclass);

  // Process pre/post-step
  void pre_step_exception_poke(hart_id_t hart, const rv_instr_t& d);
  void pre_step_lrsc_poke(       hart_id_t hart, const rv_instr_t& d);
  void pre_step_debug_poke(      hart_id_t hart, const rv_instr_t& d);
  void pre_step_debug_entry(     hart_id_t hart, const rv_instr_t& d);
  void check_debug_mode_entry_via_ebreak(const rv_instr_t& d);
  void post_step_debug_poke(      hart_id_t hart, const rv_instr_t& d);
  void pre_step_debug_exit(      hart_id_t hart, const rv_instr_t& d);
  void pre_step_nmi_check(  hart_id_t hart, const rv_instr_t& d,       whisper_state_t& w);
  void pre_step_interrupt_process(  hart_id_t hart, const rv_instr_t& d);
  void post_step_nmi_check( hart_id_t hart, const rv_instr_t& d,       whisper_state_t& w);
  void post_step_interrupt_check( hart_id_t hart, const rv_instr_t& d, const whisper_state_t& w);
  void post_step_exception_check( hart_id_t hart, const rv_instr_t& d,       whisper_state_t& w);
  void post_step_satp_write_poke(hart_id_t hart, const rv_instr_t& d, const whisper_state_t& w);

  std::string to_string(rv_intr_t& i);
  void process_imsic_msi(hart_id_t hart, const mem_t& m);
  void poke_non_standard_interrupt(hart_id_t hart, uint64_t cycle, std::bitset<64> non_std_mip_bits, bool trap_intr);
  bool check_and_defer_interrupt(hart_id_t hart, uint64_t time, std::bitset<64> mip, bool trap_intr = false);
  void check_interrupt(hart_id_t hart, uint64_t cycle, bool& taken, uint64_t& cause, bool& virt_mode);
  void defer_interrupt(hart_id_t hart, uint64_t time, uint64_t mip);
  void defer_nmi(hart_id_t hart, uint64_t time, uint64_t nmi);
  void peek_deferred_interrupts(hart_id_t hart, uint64_t& DeferredInterrupts);
  void poke_nmi(hart_id_t hart, uint64_t time, uint64_t cause);
  void clear_nmi(hart_id_t hart, uint64_t time);
  void clear_nmi(hart_id_t hart, uint64_t time, uint64_t cause);
  void poke_mip(hart_id_t hart, uint64_t time, std::bitset<64> mip);
  void peek_mip(hart_id_t hart, uint64_t time, std::bitset<64>& mip);
  void peek_seip(hart_id_t hart, uint64_t time, bool& seip);
  void get_gp_reg(uint32_t reg, uint64_t& data);
  void get_fp_reg(uint32_t reg, uint64_t& data);
  void get_vec_reg(uint32_t reg, std::array<std::uint8_t, 32>& data);
  void store_cbo_inv_addr(const uint64_t& payload);

  bool is_custom_excp(uint64_t cause);
  bool is_vector(const std::string& instr);
  bool is_indirect_reg(const std::string& instr);
  bool disable_pa_check_vec(hart_id_t hart);
  bool is_compressed(const std::string& instr);
  bool is_ucode(const std::string& instr);
  bool is_cracked_csr(const std::string& instr);
  bool found_in_list(const std::string& num, const std::string& list);
  bool resynch_needed(const hart_id_t& hart, const rv_instr_t& d, const std::string& instr, const whisper_state_t& w, std::string& resource, std::string& dut, std::string& iss);

  bool resynch_on_pa(const uint64_t& pa, const uint64_t& cycle=0);
  bool resynch_on_instr(const hart_id_t& hart, const std::string& instr, const uint64_t& cycle, std::string& resource, std::string& dut, std::string& iss, const rv_instr_t& d, const whisper_state_t& w);
  void resynch_whisper_on_patch(hart_id_t hart, rv_instr_t& d, const std::string& instr, const whisper_state_t& w);
  bool clint_read(const uint64_t& pa);
  bool tbox_read(const uint64_t& pa);
  bool boot_read(const uint64_t& pa);
  bool debug_mem_access(const uint64_t& pa);
  bool cbo_inv_access(const uint64_t& pa);
  bool uart_access(const uint64_t& pa);
  bool sc_slice_status(const uint64_t& pa);
  bool htif_read(const uint64_t& pa);
  bool unsupported_mmr_access(const uint64_t& pa);
  bool unsupported_csr_access(const std::string& instr);
  bool hpm_counter_read(const std::string& instr);
  bool intr_csrs_mismatch(const hart_id_t& hart, const std::string& instr, std::string& resource, std::string& dut, std::string& iss, const uint64_t cycle, const rv_instr_t& d, const whisper_state_t& w);
  void topei_resynch(hart_id_t hart, const rv_instr_t& d, const csr_t& csr);
  void resynch(hart_id_t hart, const rv_instr_group_t& d);
  void resynch(hart_id_t hart, const rv_instr_t& d);
  std::string get_nth_word(const std::string& s, int n);
  bool hyp_enabled() { return  (get_csr(id_, src_t::dut, misa.address) & 0x80) == 0x80; }
  bool may_peek_csr(uint64_t& csr_data, uint64_t csr_addr);
  void check_mip_change(std::bitset<64>& mip_prev, std::bitset<64> mip_new, bool seip_prev=false, bool seip_new=false, bool consider_seip=false);

private:

  const uint64_t sc_slice_base_;

  // CSRs where some bits are masked by misa.H
  std::map<uint64_t, std::string> hypervisor_masked_csr_map_ = {
    {0x300, "mstatus"},
    {0x302, "medeleg"},
    {0x303, "mideleg"},
    {0x344, "mip"},
    {0x309, "mvip"},
    {0x304, "mie"},
    {0x244, "sip"},
    // {0x60A, "henvcfg"},  // henvcfg will be disabled when misa.H is zero
    // {0x244, "vsip"},     // vsip will be disabled when misa.H is zero
    {0x30C, "mstateen0"},
    // {0x60C, "hstateen0"}, // RVDE-24897 - whisper retains the value of hstateen even when misa.H is zero
    {0x10C, "sstateen0"}
  };

    // Bit masks for fields that are masked by misa.H in each CSR
  std::map<uint64_t, uint64_t> hypervisor_mask_map_ = {
    {0x300, 0x0000000300000000}, // mstatus: MPV(39), GVA(38)
    {0x302, 0x00000000000F1000}, // medeleg: medeleg_3(23:20), medeleg_masked_0(10)
    {0x303, 0x0000000000001444}, // mideleg: SGEIP(12), VSEIP(10), VSTIP(6), VSSIP(2)
    {0x344, 0x0000000000001444}, // mip: SGEIP(12), VSEIP(10), VSTIP(6), VSSIP(2)
    {0x304, 0x0000000000001444}, // mie: SGEIE(12), VSEIE(10), VSTIE(6), VSSIE(2)
    {0x244, 0x0000000000001444}, // sip: same as mip (alias)
    {0x30C, 0x0000000000000000}, // mstateen0: no H-masked fields
    {0x10C, 0x0000000000000000}  // sstateen0: no H-masked fields
  };

  std::map<uint64_t, std::string> hypervisor_csr_map_ = {
        {0x600, "hstatus"},      // Hypervisor status register -
        {0x602, "hedeleg"},      // Hypervisor exception delegation register -
        {0x603, "hideleg"},      // Hypervisor interrupt delegation register -
        {0x604, "hie"},          // Hypervisor interrupt-enable register -
        {0x605, "htimedelta"},   // Hypervisor time delta register -
        {0x606, "hcounteren"},   // Hypervisor counter-enable register -
        {0x607, "hgeie"},        // Hypervisor guest external interrupt-enable register -
        //{0x608, "hvien"}, -> hvip is defined by H extension whereas mvien and hvien are defined by Smaia/SSaia
        {0x609, "hvictl"},
        {0x60A, "henvcfg"},      // Hypervisor Environment Configuration regsiter -
        {0x643, "htval"},        // Hypervisor Trap Value register -
        {0x644, "hip"}, // -
        {0x645, "hvip"},         // Hypervisor virtual interrupt pending -
        {0x646, "hviprio1"},         //
        {0x647, "hviprio2"},         //
        {0x680, "hgatp"},        // Hypervisor trap value register -
        {0x64A, "htinst"},       // Hypervisor trap instruction register -
        {0xE12, "hgeip"},     // Hypervisor Guest Interrupt Pending -
        {0x34B, "mtval2"},       // Machine Trap Value register -
        {0x34A, "mtinst"},        // Machine Trap Instruction register -
        {0x200, "vsstatus"}, // -
        {0x204, "vsie"}, // -
        {0x205, "vstvec"}, // -
        {0x240, "vsscratch"}, // -
        {0x241, "vsepc"}, // -
        {0x242, "vscause"}, // -
        {0x243, "vstval"}, // -
        {0x24D, "vstimecmp"},
        {0x244, "vsip"}, // -
        {0x280, "vsatp"}, // -
        {0x25C, "vstopei"},         // Virtual Supervisor Top External Interrupt
        {0xEB0, "vstopi"},          // Virtual Supervisor Top Interrupt
    };

  // MCM order map needed for periodic cosim
  std::unordered_map<uint64_t , int> mcm_orders_;

  std::map<uint64_t, std::string> MayPeekCSR_map_ = {
    {0x25C, "vstopei"}        // Virtual Supervisor Top External Interrupt
  };

  std::unordered_set<csr_base*> interrupt_csrs_to_resynch_ = {&mip, &sip, &hip, &vsip, &hgeip, &mtopi, &vstopi, &stopi};
  // TODO: Add interrupt CSRs for check
  // std::unordered_set<csr_base*> interrupt_csrs_for_check_ = {&mvip, &sip, &hip, &vsip, &mie, &sie, &vsie, &hie, &mstatus, &sstatus, &hstatus, &vsstatus, &mnstatus, &mideleg, &mvien, &hideleg, &hvien};



  cvm::file_logger bridge_log_;
  cvm::topology::loc_t loc_;
  unsigned id_;

  int num_harts_ = 0;
  int xlen_ = 0;
  int vlen_ = 0;
  CacCore cac_;
  CacCore csr_cac_;

  uint64_t order_ = 0;
  uint64_t prev_dut_trap_cause_ = 0;
  uint64_t prev_dut_trap_order_ = 0;
  uint64_t prev_vec_cmode_first_tag_ = 0;

  // Previous instruction's whisper state
  whisper_state_t pw_{};
  whisper_state_t ppw_{};

  // Create a copy of whisper instr in similar format as dut
  rv_instr_t w_;
  rv_instr_t pd_;

  uint32_t step_ = 1;
  uint32_t cycle_ = 1;
  uint64_t whisper_time_=0;
  uint64_t rvfi_calls_=0;
  bool psc_stepping_ = false;

  // State variables
  bool ecall_ = false;
  bool is_priv_debug_mode_ = false;
  bool debug_mode_ = false;
  bool dtvec_ebreak_ = false;
  bool debug_haltreq_asserted = false;
  bool excp_in_debug_mode = false;
  bool lrsc_fail_ = false;
  bool twoStage_ = false;
  bool zicbom_ = false;

  uint64_t satp_ = 0;
  uint64_t new_satp_ = 0;
  uint64_t curr_cbo_inv_addr_=0;

  uint16_t mprv_ = 0;
  uint16_t mpp_ = 0;
  uint16_t mpv_ = 0;

  uint64_t dummy_data_ = 0;
  hart_id_t dummy_hart_ = 0;

  bool resynch_intr_cause_mismatch_ = false;
  bool resynch_csr_ = false;

  bool deferred_intr_ = false;
  bool vstimecmppoked_ = false;
  bool stimecmppoked_ = false;
  uint64_t intrtopriv_ = 3;
  std::vector<mem_t> msi_{};
  rv_nmi_t nmi_ {};
  rv_nmi_t prev_nmi_ {};
  std::unordered_map<uint64_t, uint64_t> nmi_age_{};
  bool nmi_poke_pending_ = false;
  bool nmi_poke_in_debug_mode_ = false;
  uint64_t mvip_;
  std::bitset<64> mip_ = 0;
  std::bitset<64> last_step_mip_ = 0;
  std::bitset<64> hw_mip_ = 0;
  std::bitset<64> e_mip_ = 0;
  std::bitset<64> prev_hw_mip_ = 0;
  std::bitset<64> prev_e_mip_ = 0;
  std::bitset<64> nmip_ = 0;

  uint64_t timing_case2 = 0;
  uint64_t hw_mip_age_ = 0;
  uint64_t e_mip_age_ = 0;
  std::unordered_map<uint32_t, uint32_t> deferred_intr_age_;

  std::unordered_map<uint32_t, uint32_t> whisper_mip_age_, whisper_mip_clr_age_, dut_mip_age_, dut_mip_clr_age_;
  std::bitset<64> tmp_mip_prev_, tmp_mip_latest_;

  bool prev_resync_excp_defer_intr_ = 0;
  uint64_t pre_csr_defermip_ = 0;
  uint64_t resynch_icause_ = 0;
  std::array<uint32_t, max_intr> intr_age_{};
  uint32_t max_pend_intr_age_ = 0;
  uint32_t nmi_taken_count_ = 0;
  std::unordered_map<uint64_t, uint64_t> sw_intr_clear_cycle_;
  std::chrono::high_resolution_clock::time_point end_time_;
  std::chrono::high_resolution_clock::time_point start_of_test_;
  bool first_call_ = true;
  bool debug_on_ = false;
  bool cvm_debug_ = false;
  uint64_t previous_cycle_;

  uint64_t sep_base_=0, sep_end_=0;
  uint64_t maplic_base_=0, maplic_end_=0;
  uint64_t saplic_base_=0, saplic_end_=0;


  std::unordered_map<intr, int> num_taken_interrupts_{};
  std::unordered_map<excp, int> num_exceptions_{};
  int num_exceptions_insn_err_access_fault_ = 0;
  int num_exceptions_ld_err_access_fault_ = 0;
  int num_exceptions_late_st_err_access_fault_ = 0;
  int num_exceptions_insn_hwerr_fault_ = 0;
  int num_exceptions_ld_hwerr_fault_ = 0;
  int num_exceptions_late_st_hwerr_fault_ = 0;
  int num_trig_breakpoint_ = 0;
  int num_sp_accesses_ = 0;

  uint64_t dword_vec_array [vlen/64] = {0};
  int unmask_bits_instr, unmask_bits_uop = 0;
  std::vector<std::string> cosim_resynch_csr_defaults;


  bool terminated_=false, end_mcm_=false, metrics_reported_=false;
  bool check_nmi_at_patch_exit_ = false;
  uint64_t check_nmi_at_patch_cause_ = 0;
  bool check_intr_at_patch_exit_ = false;
  uint64_t check_intr_at_patch_cause_ = 0;
  bool check_debug_entry_at_patch_exit_ = false;
  bool skip_de_until_debug_vector_ = false;
  rv_debug_t deferred_debug_entry_{};
  enum patch_mode patch_mode_ = NO_PATCH;

  // Containers for storing result of parsing plusargs
  parser::pair_map<uint64_t, uint64_t> cosim_resynch_excp_addr_{};
  parser::vector<uint64_t> cosim_resynch_excp_{};
  parser::vector<uint64_t> cosim_error_excp_{};
  parser::vector<std::string> cosim_error_instr_{};
  parser::map<uint32_t, uint32_t> cosim_remap_opcode_{};
  bool cosim_remap_opcode_enabled_{false};

  std::map<uint64_t, uint64_t> hypervisor_masked_csrs_;
  bool misa_h_ = true;
  std::pair<uint64_t /*pa*/, uint64_t/*age*/> latest_imsic_{0, 0};

  std::string mismatch_res_ = "", mismatch_dut_, mismatch_iss_;
  bool custom_vlzero_excp_ = false;

  std::bitset<64> intr_during_trap_ = 0;
  std::bitset<64> intr_cleared_during_trap_ = 0;
  bool intr_partially_deferred_ = false;
  bool intr_undeferred_due_to_xret_intr_csr_ = false;
  bool nmi_undeferred_due_to_xret_intr_csr_ = false;
};
