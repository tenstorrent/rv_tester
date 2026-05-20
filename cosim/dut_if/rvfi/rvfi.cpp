#include "rvfi.h"
#include "util.h"
#include "whisper_decoder.h"
#include "rv_tester_plusargs.h"
#include "cvm/plusargs.hpp"
#include "cvm/bitmanip.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/registry.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "cosim/bridge/bridge_plusargs.h"
#include "rv_tester_plusargs.h"
#include "transactors/axi_sw/axi.h"
#include "common/device_address_map/device_address_map.h"

#include <iostream>
#include <chrono>
#include <cmath>
#include <regex>

DEFINE_bool(rvfi, true, "Enable rvfi");
DEFINE_bool(rvfi_log,  true, "Enable rvfi logging");
DEFINE_bool(rvfi_log_36b_uop, true, "rvfi log - print 36b uop instead of default 32b riscv opcode");
DEFINE_bool(cosim, true, "Enable cosim checking");
DEFINE_bool(cache_model_en, false, "Enable MCM Cache Model");
DEFINE_bool(emulate_amo_arithmetic, true, "Emulate amo arithmetic if dut harness does not provide amo outputs");
DEFINE_uint64(debug_entry_pc_offset, 0x800, "Debug Mode entry PC");
DEFINE_uint64(debug_exit_pc_offset, 0x8cc, "Debug Mode exit PC");
DEFINE_uint64(debug_mem_base_offset, 0x0, "Debug Memory Base Address");
DEFINE_uint64(debug_mem_size, 0x1000, "Debug Memory Size");
DEFINE_bool(use_sw_priv, false, "Enable use of SW generation of priv/patch_mode values instead of hw");
DEFINE_bool(patch_mode_tag_override, true, "In Patch mode, override subsequent rvfi/mcmi tag with original instruction tag");
DEFINE_bool(vec_cmode_tag_override, true, "If vector instruction enters conservative mode, override subsequent rvfi/mcmi tags with original instruction tag");

bool get_csr_name_instr(const std::string& input, std::string& modified_string);

REGISTRY_register(rvfi, COSIM, cvm::registry::all);

rvfi::rvfi(cvm::topology::loc_t loc, unsigned id)
  : log("h" + std::to_string(id) + "_dut_rvfi.log"), loc_(loc), id_(id) {
  whisper::initialize();

  cvm::registry::messenger.connect<svScope>(
    loc_,
    [&](svScope s) { return this->set_scope(s); });

  connect<
    rv_tester_transactions::cosim::m_reset<>,
    rv_tester_transactions::cosim::m_disable_checks<>,
    rv_tester_transactions::cosim::m_rvfi<>,
    rv_tester_transactions::cosim::m_steps<>,
    rv_tester_transactions::cosim::m_gp_regs<>,
    rv_tester_transactions::cosim::m_fp_regs<>,
    rv_tester_transactions::cosim::m_vc_regs<>,
    rv_tester_transactions::cosim::m_csri<>,
    rv_tester_transactions::cosim::m_mhpm_counter_ovf<>,
    rv_tester_transactions::cosim::m_trap<>,
    rv_tester_transactions::cosim::m_core_nmi<>,
    rv_tester_transactions::cosim::m_interrupt_pend<>,
    rv_tester_transactions::cosim::m_mtip<>,
    rv_tester_transactions::cosim::m_mtime<>,
    rv_tester_transactions::cosim::m_imsic_msi<>,
    rv_tester_transactions::cosim::m_debug<>,
    bridge::error_loc
  >(loc);

  connect<
    rv_tester::terminate_called,
    rv_tester::terminate_called_mem_checks
  >(cvm::topology::get_from_type("PLATFORM", 0));

  // Reset/init configuration
  init();
}

rvfi::~rvfi() {
  uint32_t ncores = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  if (FLAGS_rvfi && ncores == 1 && (count_ == 1) && (FLAGS_offline_dpi == false))
    cvm::log(cvm::ERROR, "Error: rvfi termination without processing any instructions\n");
}

void rvfi::check() {
  // bridge_->report_metrics();
}

void rvfi::init() {

  if (FLAGS_cosim) {
    cvm::log(cvm::MEDIUM, "[RVFI loc {} id{}] Constructing bridge...\n", loc_, id_);
    auto platform_loc = cvm::topology::get_from_type("PLATFORM", 0);
    bridge_ = std::make_shared<bridge>(cvm::topology::attr(platform_loc, "NHARTS").second, xlen, vlen, loc_, id_);
    count_ = 1;

    // Flags configuration
    uint32_t ncores = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
    if (ncores > 1) {
      FLAGS_mcm = true;
      cvm::log(cvm::NONE, "[plusargs] +mcm\n");
    }

    // Share bridge with mcmi
    if (FLAGS_mcm) mcmi_ = std::make_unique<mcmi>(loc_, id_, bridge_);
  } else {
    cvm::log(cvm::MEDIUM, "Running with cosim is disabled\n");
  }
}

bool rvfi::patch_access (uint64_t addr) {
  if (!patch_mode_)
      return false;
  
  uint32_t ncores = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  uint32_t cluster_id = 0;
  uint64_t patch_lo = generate_cpl_sram_device_addr(cluster_id) + device_address_map_patch_ram_start_offset();
  uint64_t patch_hi = patch_lo + device_address_map_patch_ram_size() - 1;
  if (addr >= patch_lo && addr < patch_hi)
      return true;

  for (uint32_t i = 0; i < ncores; i++) {
    if (addr == generate_cr_device_addr(cluster_id, i) + 0x5040)
      return true;
  }
  return false;
}

void rvfi::process(const rv_tester_transactions::cosim::m_reset<>& m_reset) {

  if (terminated_)
    return;

  if (loc_ != m_reset.location)
    return;

  in_reset_ = false;
  cvm::log(cvm::MEDIUM, "[rvfi] reset\n");

  if (FLAGS_cosim){
    bridge_->reset();
    // mcmi_->reset();
  }
  else
    FLAGS_whisper_client_check = false;
}

void rvfi::process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi) {

  if (terminated_ || in_reset_)
    return;

  if (loc_ != m_rvfi.location)
    return;

  if (patch_mode_) {
    if (!patch_mode_first_tag_) {
      patch_mode_first_tag_ = m_rvfi.order;
    }
    if (patch_mode_tags_.find(m_rvfi.order) == patch_mode_tags_.end())
      patch_mode_tags_.emplace(m_rvfi.order, patch_mode_first_tag_);
  }

  // Construct rv_instr_t and send to bridge
  rv_instr_t instr;
  make_instr(m_rvfi, instr);
  print_instr(instr);

  pc_error_ = pc_error_ || m_rvfi.pc_error;
  mem_error_ = mem_error_ || m_rvfi.mem_error;
  if (m_rvfi.trap) {
    trap_insn_ = m_rvfi.insn;
    trap_addr_ = (m_rvfi.insn == 0) ? m_rvfi.pc_rdata : ((m_rvfi.mem_rmask != 0) || (m_rvfi.mem_wmask != 0)) ? m_rvfi.mem_addr : 0x0;
    return;
  }

  prev_uop_tag_ = m_rvfi.order;

  if (!m_rvfi.last_uop)
    return;

  // Append accumulated uop changes for ucode instructions
  append_uop_changes_to_instr(instr);
  enter_debug_mode(instr);
  send_instr(instr);
  exit_debug_mode(instr);

  // Send instruction group with information that cannot be
  // correlated with precise instruction boundaries (like hw non-zicsr csr updates)
  instrs_.push_back(instr);
  if (m_rvfi.last_insn) {
    rv_instr_group_t group;
    group.cycle  = instr.cycle;
    group.instrs = instrs_;
    group.csrs   = hw_csrs_;

    send_instr_group(instr.hart, group);

    instrs_.clear();
    hw_csrs_.clear();
  }

  // Save state
  prev_instr_tag_ = m_rvfi.order;
  prev_branch_tag_ = instr.branch_tag;

  // Clear state
  intr_ = false;
  excp_ = false;
  nmi_ = false;
  vec_cmode_ = false;
  vec_cmode_pc_addr_ = 0;
  trap_insn_ = 0;
  trap_addr_ = 0;
  pc_error_ = false;
  mem_error_ = false;

  // RVDE-24355: Clean up conservative mode memory errors when vec_cmode_ is cleared
  if (!vec_cmode_ && !vec_cmode_mem_errors_.empty()) {
    cvm::log(cvm::HIGH, "[RVDE-24355] Clearing vector conservative mode memory errors\n");
    vec_cmode_mem_errors_.clear();
  }
}

void rvfi::process(const rv_tester_transactions::cosim::m_trap<>& m_trap) {

  if (terminated_ || in_reset_)
    return;

  if (loc_ != m_trap.location)
    return;

  if (m_trap.id == NMI) {
    nmi_ = true;
    intr_ = false;
    excp_ = false;
    ncause_ = m_trap.cause & 0x3;

  } else if (m_trap.id == INTR) {
    nmi_ = false;
    intr_ = true;
    excp_ = false;
    intr_virt_mode_ = m_trap.virt_mode;
    icause_ = m_trap.cause & 0x3f;

  } else if (m_trap.id == EXCP) {

    nmi_ = false;
    intr_ = false;
    excp_ = true;
    ecause_ = m_trap.cause & 0xff;
    if (FLAGS_cosim && ecause_ == 60) {
      cvm::log(cvm::HIGH, "Enter patch via exception\n");
      if (FLAGS_cosim) bridge_->set_patch_mode(ENTER_PATCH);
      patch_mode_ = true;
    } else if (FLAGS_vec_cmode_tag_override && (ecause_ == CUSTOM_VEC_CMODE)) {
      if (!(vec_cmode_ && (m_trap.pc_addr == vec_cmode_pc_addr_))) {
        vec_cmode_ = true;                      // RVTOOLS-3265, RVTOOLS-3479: Adjust tag for conservative mode vector instructions
        vec_cmode_first_tag_ = m_trap.order;    // Capture the tag and use it for all activity related to the vector instruction
        vec_cmode_pc_addr_ = m_trap.pc_addr;
      }
      // RVDE-24355: Store memory error for conservative mode vector instruction
      if (mem_error_) {
        vec_cmode_mem_errors_[vec_cmode_first_tag_] = true;
      }
    }
    if (FLAGS_cosim) bridge_->process_dut_excp(id_, m_trap.cause, m_trap.order, vec_cmode_first_tag_);
  }
}

void rvfi::process(const rv_tester_transactions::cosim::m_interrupt_pend<>& m_interrupt_pend) {
  if (terminated_ || in_reset_)
    return;

  if (loc_ != m_interrupt_pend.location)
    return;

  rv_intr_t intr;
  intr.cycle = m_interrupt_pend.cycle;
  intr.hw = m_interrupt_pend.hw;
  intr.mip = std::bitset<64>(m_interrupt_pend.mip);
  intr.mip_set = std::bitset<64>(m_interrupt_pend.mip_set);
  intr.mip_clr = std::bitset<64>(m_interrupt_pend.mip_clr);
  intr.seip = m_interrupt_pend.seip;
  intr.seip_set = m_interrupt_pend.seip_set;
  intr.seip_clr = m_interrupt_pend.seip_clr;
  intr.buserr_bit = m_interrupt_pend.buserr_bit;
  intr.trap_intr = m_interrupt_pend.trap_intr;

  std::string dut_log;
  dut_log += fmt::format("#NA {} {} {} ({} : mip={:#x} : ", intr.cycle, m_interrupt_pend.core_cycle, id_, intr.hw ? "hw" : "sw", intr.mip.to_ullong());
  for (const auto& [k,v] : intr_to_string) {
    if (k == DEBUG)
      continue;
    if (intr.mip_set[k])
      dut_log += fmt::format("{}+,", v);
    else if (intr.mip_clr[k])
      dut_log += fmt::format("{}-,", v);
    else if (intr.mip[k])
      dut_log += fmt::format("{},", v);
  }
  dut_log += fmt::format(" : seip={}{}", intr.seip ? 1 : 0, intr.seip_set ? " : SEIpin+" : intr.seip_clr ? " : SEIpin-" : "");
  dut_log += fmt::format(")\n");

  if (FLAGS_rvfi_log)
    log(cvm::NONE, fmt::to_string(dut_log));

  if (!FLAGS_cosim)
    return;

  bridge_->process_dut_interrupt(id_, intr);
}

void rvfi::process(const rv_tester_transactions::cosim::m_mtime<>& m_mtime) {
  if (terminated_ || in_reset_)
    return;

  rv_intr_t intr;
  intr.cycle = m_mtime.cycle;
  intr.mip   = std::bitset<64>(m_mtime.mip);
  intr.mtime = m_mtime.mtime;
  intr.timeCsr = m_mtime.timeCsr;
  intr.trap_intr = m_mtime.trap_intr;
  intr.size  = m_mtime.size;

  if (FLAGS_rvfi_log)
    log(cvm::NONE, "#NA {} {} (time={:#x}, mtime={:#x}, size={}, cause={:#x})\n", intr.cycle, id_, intr.timeCsr, intr.mtime, intr.size, m_mtime.cause);

  if (!FLAGS_cosim)
    return;

  bridge_->process_dut_timer(id_, intr);
}

void rvfi::process(const rv_tester_transactions::cosim::m_mtip<>& m_mtip) {
  if (terminated_ || in_reset_)
    return;

  if (FLAGS_rvfi_log)
    log(cvm::NONE, "#NA {} MTIP[{}] = {}\n", m_mtip.cycle, id_, m_mtip.mtip);
  
  if (!FLAGS_cosim)
    return;
  bridge_->process_dut_mtip(id_, m_mtip.cycle, m_mtip.mtip, m_mtip.trap_intr);
}

void rvfi::process(const rv_tester_transactions::cosim::m_core_nmi<>& m_core_nmi) {
  if (terminated_ || in_reset_)
    return;

  if (loc_ != m_core_nmi.location)
    return;

  rv_nmi_t nmi;
  nmi.cycle = m_core_nmi.cycle;
  nmi.valid = m_core_nmi.nmi_assert;
  nmi.cause = m_core_nmi.nmi_cause;

  if (FLAGS_rvfi_log)
    log(cvm::NONE, "#{} {} {} (nmi:{} cause:{})\n", count_, nmi.cycle, id_, nmi.valid, nmi.cause);

  if (!FLAGS_cosim)
    return;

  bridge_->process_dut_nmi(id_, nmi);
}

void rvfi::process(const rv_tester_transactions::cosim::m_imsic_msi<>& m_imsic_msi) {
  if (terminated_ || in_reset_)
    return;

  if (loc_ != m_imsic_msi.location)
    return;

  mem_t mem;
  mem.valid = true;
  mem.cycle = m_imsic_msi.cycle;
  mem.pa = m_imsic_msi.addr;
  mem.data = m_imsic_msi.data;
  mem.trap_intr = m_imsic_msi.trap_intr;
  mem.size = 4;

  if (FLAGS_rvfi_log && (mem.data != 0))
    log(cvm::NONE, "#{} {} {} (imsic: [addr={:#x} data={:#x}])\n", count_, mem.cycle, id_, mem.pa, mem.data);

  if (!FLAGS_cosim)
    return;

  bridge_->process_dut_imsic_msi(id_, mem);
}

void rvfi::process(const rv_tester_transactions::cosim::m_debug<>& m_debug) {
  if (terminated_ || in_reset_)
    return;

  if (!FLAGS_cosim)
    return;

  bridge_->process_debug_haltreq(m_debug.haltreq);
}

void rvfi::make_instr(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi, rv_instr_t& instr) {

  static bool started = true;
  if (started) {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    cvm::log(cvm::HIGH, "start time: {}\n", std::ctime(&now));
    started = false;
  }

  // Metadata
  instr.valid = true;
  instr.hart = m_rvfi.hart;
  instr.cycle = m_rvfi.cycle;
  instr.core_cycle = m_rvfi.core_cycle;
  instr.id = count_;
  instr.comp = m_rvfi.comp;
  instr.tag = patch_mode_ && FLAGS_patch_mode_tag_override ? patch_mode_first_tag_ : vec_cmode_ && vec_cmode_pc_addr_ == m_rvfi.pc_rdata ? vec_cmode_first_tag_ : m_rvfi.order;
  instr.branch_tag = m_rvfi.branch_tag;
  instr.opcode = m_rvfi.insn;
  instr.disasm = whisper::disassemble(m_rvfi.insn);
  instr.uop = m_rvfi.uop;
  instr.cracked = !m_rvfi.last_uop;
  instr.trap = intr_ || excp_;
  instr.trap_valid = m_rvfi.trap;
  instr.trap_opcode = trap_insn_;
  instr.trap_addr = trap_addr_;
  instr.nmi = nmi_;
  instr.ncause = ncause_;
  instr.intr = intr_;
  instr.icause = icause_;
  instr.excp = excp_;
  instr.ecause = ecause_;
  instr.virt_mode = intr_virt_mode_;

  cvm::log(cvm::HIGH, "CLOCK={}: HW: ucode={} first_uop={} last_uop={} rvfi.mode={} instr.priv={} priv_change={} set_pmode={} clr_pmode={} patch_={} disasm={}\n", m_rvfi.cycle,
                            m_rvfi.ucode, m_rvfi.first_uop, m_rvfi.last_uop, m_rvfi.mode, m_rvfi.priv, m_rvfi.priv_change, m_rvfi.set_pmode, m_rvfi.clr_pmode, static_cast<int>(patch_mode_),instr.disasm);

  // RVDE-17736: Manage fetch/evict signaling for ncio region
  // Using branch tag as a marker for when the previous fetch stops supplying instruction bytes
  // and we need a new fetch performed non-speculatively
  if (((ncio_fetches_.size() != 0) || ncio_mem_transition_) &&  m_rvfi.last_uop) {
    if (m_rvfi.branch_tag != prev_branch_tag_) {
      process_ncio_fetches(instr);
    }
    active_ncio_fetches_.clear();
    ncio_mem_transition_ = false;
  }

  if (FLAGS_use_sw_priv == false) {
  // First/last uops for ucode sequences

  instr.first_uop = m_rvfi.first_uop;
  instr.last_uop  = m_rvfi.last_uop;
  instr.ucode  = m_rvfi.ucode;
  instr.priv  = m_rvfi.priv;
  ucode_priv_change_ = m_rvfi.priv_change;

  if (m_rvfi.last_uop)
    priv_ = m_rvfi.mode;

  if (!priv_to_string.count(static_cast<priv>(instr.priv))) {
    cvm::log(cvm::ERROR, "Error: Invalid rvfi privilege mode: {:#x}\n", instr.priv);
    return;
  }

  if (m_rvfi.set_pmode) { // when we enter patch mode via ucode
    cvm::log(cvm::HIGH, "CLOCK={}: Patch mode turned ON\n",m_rvfi.cycle);
    if (FLAGS_cosim) bridge_->set_patch_mode(ENTER_PATCH);
    patch_mode_ = true;

    if (FLAGS_patch_mode_tag_override) {
      patch_mode_first_tag_ = m_rvfi.order;
      instr.tag = patch_mode_first_tag_;
    }
  }
  if (m_rvfi.clr_pmode) {
    cvm::log(cvm::HIGH, "CLOCK={}: Patch mode turned OFF\n",m_rvfi.cycle);
    if (FLAGS_cosim) bridge_->set_patch_mode(EXIT_PATCH);
    patch_mode_ = false;
    patch_mode_first_tag_ = 0;
  }

  if ((instr.priv & 0x7) == 0x3)
     instr.priv = 0x3;

  }
  else {

// First/last uops for ucode sequences
  instr.first_uop = false;
  instr.last_uop = m_rvfi.last_uop;
  instr.ucode = ucode_ || !m_rvfi.last_uop;
  if (!m_rvfi.last_uop) {
    if (!ucode_)
      instr.first_uop = true;
    ucode_ = true;
  } else {
    ucode_ = false;
  }

  // Priv mode
  if (FLAGS_cosim && priv_ == 0x4 && !patch_mode_) { // when we enter patch mode via ucode
    cvm::log(cvm::HIGH, "Patch mode: turned ON with Ucode instruction={} time={}\n", m_rvfi.insn, m_rvfi.cycle);
    if (FLAGS_cosim) bridge_->set_patch_mode(ENTER_PATCH);
    patch_mode_ = true;
  }
  instr.priv = m_rvfi.mode;
  if (instr.ucode && (m_rvfi.mode != priv_)) {
    if (instr.first_uop) {
      priv_ = m_rvfi.mode;
    } else {
      instr.priv = priv_;
      ucode_priv_change_ = true;
    }
  }
  if (m_rvfi.last_uop) {
    if (ucode_priv_change_) {
      instr.priv = priv_;
      ucode_priv_change_ = false;
    }
    if (m_rvfi.mode == 0x4 && patch_mode_) { // dret changes mode from D to M/S/U (exit from patch mode)
      cvm::log(cvm::HIGH, "Patch mode: turned OFF with Ucode instruction={} time={}\n",m_rvfi.insn,m_rvfi.cycle);
      if (FLAGS_cosim) bridge_->set_patch_mode(EXIT_PATCH);
      patch_mode_ = false;
      patch_mode_first_tag_ = 0;
    }
    priv_ = m_rvfi.mode;
    if (!priv_to_string.count(static_cast<priv>(instr.priv))) {
      cvm::log(cvm::ERROR, "Error: Invalid rvfi privilege mode: {:#x}\n", instr.priv);
      return;
    }
  }
  cvm::log(cvm::HIGH, "CLOCK={}: SW: ucode={} first_uop={} last_uop={} rvfi.mode={} instr.priv={} priv_change={} set_pmode={} clr_pmode={} patch_={} disasm={}\n", m_rvfi.cycle,
                                      static_cast<int>(ucode_), static_cast<int>(instr.first_uop), static_cast<int>(instr.last_uop), m_rvfi.mode, instr.priv, static_cast<int>(ucode_priv_change_), m_rvfi.set_pmode,m_rvfi.clr_pmode, static_cast<int>(patch_mode_), instr.disasm);
  }

  if (m_rvfi.last_uop && !patch_mode_) {
    count_++;
  }

  // if (instr.priv == 0x7) { // Make the DP mode as well same as DE mode for Cosim Checks
  //   instr.priv = 0x6;
  // }

  // PC
  instr.pc.valid = true;
  instr.pc.pc_rdata = m_rvfi.pc_rdata;
  instr.pc.error = m_rvfi.pc_error || pc_error_;

  // GPR
  if ((m_rvfi.rd_addr > 0) && (m_rvfi.rd_addr <= 31)) {
    if (instr.last_uop) {
      instr.gpr.emplace_back(true, m_rvfi.rd_addr, m_rvfi.rd_wdata);
    } else {
      if (!m_rvfi.trap) {
        // Collect gpr write from a cracked uop
        cracked_gpr_.valid = true;
        cracked_gpr_.rd_addr = m_rvfi.rd_addr;
        cracked_gpr_.rd_wdata = m_rvfi.rd_wdata;
      }
      // This is for print in the rvfi log
      instr.gpr.emplace_back(false, m_rvfi.rd_addr, m_rvfi.rd_wdata);
    }
  }

  // FPR
  if (m_rvfi.frd_valid) {
    instr.fpr.emplace_back(true, m_rvfi.frd_addr, m_rvfi.frd_wdata);
  }

  // VR
  if (m_rvfi.vrd_valid) {
    vr_t vr {true, m_rvfi.vrd_addr, m_rvfi.vrd_wdata};
    instr.vr.push_back(vr);
    // Accumulate vr writes across cracked uops
    if ((m_rvfi.vrd_addr < 32) && !m_rvfi.trap) {
      cracked_vrs_.push_back(vr);
    }
  }

  // Flags
  instr.flags = 0;
  if (m_rvfi.flags_valid && !m_rvfi.trap) {
    instr.flags = m_rvfi.flags;
    // Accumulate flags writes across cracked uops
    cracked_flags_ |= m_rvfi.flags;
  }
  if (!m_rvfi.trap) {
    // Accumulate vec_cracked bool across cracked uops
    cracked_ |= instr.cracked;
  }

  // CSR
  if (m_rvfi.csr_valid) {
    csr_t c {true, m_rvfi.hart, m_rvfi.cycle, m_rvfi.csr_addr, m_rvfi.csr_wmask, m_rvfi.csr_wdata};
    instr.csr.push_back(c);
    // Accumulate ucode csr writes
    if (!m_rvfi.last_uop && !m_rvfi.trap) {
      ucode_csrs_.push_back(c);
    }
  }

  // tlb
  instr.mem_va = m_rvfi.mem_addr;
  instr.mem_pa = m_rvfi.mem_paddr;

  // Mem reads
  instr.mem_read.valid = (m_rvfi.mem_rmask != 0);
  instr.mem_read.va = m_rvfi.mem_addr;
  instr.mem_read.pa = m_rvfi.mem_paddr;
  instr.mem_read.data = m_rvfi.mem_rdata;
  instr.mem_read.size = log2(m_rvfi.mem_rmask + 1);
  // RVDE-24355: Check if this instruction tag has a stored conservative mode memory error
  bool vec_cmode_mem_error = false;
  if (vec_cmode_mem_errors_.find(instr.tag) != vec_cmode_mem_errors_.end()) {
    vec_cmode_mem_error = vec_cmode_mem_errors_[instr.tag];
  }
  instr.mem_read.error = m_rvfi.mem_error || mem_error_ || vec_cmode_mem_error;
  instr.mem_read.attr = m_rvfi.mem_attr;
  instr.mem_read.page4kX = m_rvfi.mem_page4kX;
  instr.mem_read.page4kX_attr = m_rvfi.mem_page4kX_attr;

  // RVDE-24355: Track memory errors during conservative mode (for any UOP, not just vector-looking ones)
  if ((m_rvfi.mem_error || mem_error_) && vec_cmode_) {
    // Store memory error for the conservative mode tag (this will be applied to the final UOP sent to whisper)
    vec_cmode_mem_errors_[vec_cmode_first_tag_] = true;
  }

  // Mem writes
  instr.mem_write.valid = (m_rvfi.mem_wmask != 0);
  instr.mem_write.va = m_rvfi.mem_addr;
  instr.mem_write.pa = m_rvfi.mem_paddr;
  instr.mem_write.data = m_rvfi.mem_wdata;
  instr.mem_write.size = log2(m_rvfi.mem_wmask + 1);
  instr.mem_write.attr = m_rvfi.mem_attr;
  instr.mem_write.page4kX = m_rvfi.mem_page4kX;
  instr.mem_write.page4kX_attr = m_rvfi.mem_page4kX_attr;
  instr.mem_write.error = m_rvfi.mem_error || mem_error_ || vec_cmode_mem_error;
}

void rvfi::append_uop_changes_to_instr(rv_instr_t& instr) {
  // GPR
  if (cracked_gpr_.valid) {
    instr.gpr.emplace_back(true, cracked_gpr_.rd_addr, cracked_gpr_.rd_wdata);
    cracked_gpr_.valid = false;
  }

  // VR
  if (!cracked_vrs_.empty()) {
    instr.vr.clear();
    for (auto& c : cracked_vrs_) {
      instr.vr.push_back(c);
    }
    cracked_vrs_.clear();
  }

  // Flags
  if (cracked_) instr.flags |= cracked_flags_;
  cracked_ = 0;
  cracked_flags_ = 0;

  // CSR
  if (!ucode_csrs_.empty()) {
    for (auto& c : ucode_csrs_) {
      instr.csr.push_back(c);
    }
    ucode_csrs_.clear();
  }
}

void rvfi::print_csr(csr_t& csr) {
  if (FLAGS_rvfi_log)
    log(cvm::NONE, "#NA {} {} {} {:016x} {:09x} c {:016x} {:016x} {:016x} (hw update)\n",
      csr.cycle, csr.hart, priv_to_string.at(static_cast<priv>(priv_)), 0, 0, csr.csr_addr, csr.csr_wdata, csr.csr_wmask);
}

void rvfi::print_instr(const rv_instr_t& instr) {
  if (!FLAGS_rvfi_log) {
    return;
  }

  int resource_count = instr.gpr.size() + instr.fpr.size() + instr.vr.size() + instr.csr.size() + instr.mem_write.valid;

  // Print r0 = 0 if nothing modified
  if (!resource_count) {
    print_instr_resource(instr, fmt::format(" r {:016x} {:016x}", 0, 0));
    return;
  }

  // Print modified resources in this order - r, f, v, m, c
  for (const auto& gpr : instr.gpr)
    print_instr_resource(instr, fmt::format(" r {:016x} {:016x}", gpr.rd_addr, gpr.rd_wdata));

  for (const auto& fpr : instr.fpr)
    print_instr_resource(instr, fmt::format(" f {:016x} {:016x}", fpr.frd_addr, fpr.frd_wdata));

  for (auto& vr : instr.vr){
    if (vr.valid) {
    uint64_t chunks[4] = {0};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 64; ++j) {
            chunks[i] |= static_cast<uint64_t>(vr.vrd_wdata[i * 64 + j]) << j;
        }
    }
    print_instr_resource(instr, fmt::format(" v {:002x} {:016x}{:016x}{:016x}{:016x}", vr.vrd_addr, chunks[3], chunks[2], chunks[1], chunks[0]));
    }
  }

  if (instr.mem_write.valid)
    print_instr_resource(instr, fmt::format(" m {:016x} {:016x}", instr.mem_write.va, instr.mem_write.data));

  if (!instr.ucode || !instr.last_uop)
    for (auto& c : instr.csr)
      print_instr_resource(instr, fmt::format(" c {:016x} {:016x} {:016x}", c.csr_addr, c.csr_wdata, c.csr_wmask));
}

void rvfi::print_instr_resource(const rv_instr_t& instr, std::string resource_str) {
  std::string dut_log;

  dut_log += fmt::format("#{} {} {} {} {} {:016x}", FLAGS_mcm ? instr.tag : instr.id, instr.cycle, instr.core_cycle, instr.hart, priv_to_string.at(static_cast<priv>(instr.priv)),
     instr.pc.pc_rdata);

  if (FLAGS_rvfi_log_36b_uop)
    dut_log += fmt::format(" {:09x}", instr.uop);
  else
    dut_log += fmt::format(" {:08x}", instr.opcode);

  dut_log += fmt::format("{}", resource_str);

  if (!instr.ucode || cracked_gpr_.valid) {
    std::string instr_dis = whisper::disassemble(instr.opcode);
    std::string csr_replaced_instr = instr_dis;
    uint32_t csr_opcode = instr.opcode & 0x7F;
    uint32_t csr_funct = (instr.opcode >> 12) & 0x7;
    // Check if the instruction is a CSR instruction and try to replace it
    if ((csr_opcode == 0x73) && (csr_funct != 0 && csr_funct != 4) && get_csr_name_instr(instr_dis, csr_replaced_instr))
      dut_log += fmt::format(" {}", csr_replaced_instr);
    else
      dut_log += fmt::format(" {}", instr_dis);
  }
  else
    dut_log += fmt::format(" {} (microcode)", cosim_util::get_nth_word(instr.disasm, 1));

  if (instr.flags)
    dut_log += fmt::format(" (flags:{:#x})", instr.flags);

  if (instr.mem_write.valid)
    dut_log += fmt::format(" [{:#x}:{:#x}:{}{}]", instr.mem_write.va, instr.mem_write.pa, mem_attr_to_string(instr.mem_write.attr), instr.mem_write.error ? ":ras/bus_error" : "");

  if (instr.mem_read.valid)
    dut_log += fmt::format(" [{:#x}:{:#x}:{}{}]", instr.mem_read.va, instr.mem_read.pa, mem_attr_to_string(instr.mem_read.attr), instr.mem_read.error ? ":ras/bus_error" : "");

  if (instr.trap_valid)
    dut_log += fmt::format(" {}(trap)", instr.pc.error ? "[ras/bus_error] " : "");

  if (instr.nmi)
    dut_log += fmt::format(" (nmi: {})", nmi_to_string.count(static_cast<nmi>(instr.ncause)) ? nmi_to_string.at(static_cast<nmi>(instr.ncause)) : std::to_string(instr.ncause));

  if (instr.intr)
    dut_log += fmt::format(" (interrupt: {})", (intr_to_string.count(static_cast<intr>(instr.icause)) && (instr.icause != 0 || !intr_virt_mode_)) ? intr_to_string.at(static_cast<intr>(instr.icause)) : std::to_string(instr.icause));

  if (instr.excp)
    dut_log += fmt::format(" (exception: {})", excp_to_string.count(static_cast<excp>(instr.ecause)) ? excp_to_string.at(static_cast<excp>(instr.ecause)) : std::to_string(instr.ecause));

  if (instr.comp)
    dut_log += fmt::format(" (compressed)");

  if (patch_mode_)
    dut_log += fmt::format(" (patch)");

  dut_log += fmt::format("\n");

  if (FLAGS_rvfi_log)
    log(cvm::NONE, fmt::to_string(dut_log));
}
void rvfi::process(const rv_tester_transactions::cosim::m_steps<>& m_steps) {
  if (terminated_ || in_reset_)
    return;
  if (!FLAGS_cosim)
    return;
  bridge_->process_steps(m_steps.hart, m_steps.n_retire, m_steps.cycle, m_steps.steps, m_steps.skips, m_steps.final_steps);
}

void rvfi::process(const rv_tester_transactions::cosim::m_gp_regs<>& m_gp_regs) {
  if (terminated_ || in_reset_)
    return;
  if (!FLAGS_cosim)
    return;
  if (loc_ != m_gp_regs.location)
    return;
  bridge_->process_compare_gp_regs(m_gp_regs.hart,m_gp_regs.cycle,m_gp_regs.value);
}

void rvfi::process(const rv_tester_transactions::cosim::m_fp_regs<>& m_fp_regs) {
  if (terminated_ || in_reset_)
    return;
  if (!FLAGS_cosim)
    return;
  if (loc_ != m_fp_regs.location)
    return;
  bridge_->process_compare_fp_regs(m_fp_regs.hart ,m_fp_regs.cycle, m_fp_regs.value);
}

void rvfi::process(const rv_tester_transactions::cosim::m_vc_regs<>& m_vc_regs) {
  if (terminated_ || in_reset_)
    return;
  if (!FLAGS_cosim)
    return;
  if (loc_ != m_vc_regs.location)
    return;
  bridge_->process_compare_vc_regs(m_vc_regs.hart ,m_vc_regs.cycle, m_vc_regs.value);
}

void rvfi::send_instr(rv_instr_t& instr) {
  if (!FLAGS_cosim)
    return;

  if (terminated_ || in_reset_)
    return;

  if (!instr.trap_valid)
    bridge_->process_dut_instr_retire(instr.hart, instr);
}

void rvfi::send_instr_group(hart_id_t hart, rv_instr_group_t& group) {
  if (!FLAGS_cosim)
    return;

  if (terminated_ || in_reset_)
    return;

  bridge_->process_dut_instr_group_retire(hart, group);
}

void rvfi::send_csr(csr_t& csr) {
  if (!FLAGS_cosim)
    return;

  if (terminated_ || in_reset_)
    return;

  bridge_->process_dut_csr_hw_update(csr.hart, csr);
}


void rvfi::enter_debug_mode(rv_instr_t& instr) {
  if (!FLAGS_cosim)
    return;

  if (terminated_ || in_reset_)
    return;

  if ((instr.intr && !instr.icause && !intr_virt_mode_) ||
      (instr.excp && (instr.ecause == CUSTOM_SINGLE_STEP))
     ) {
    rv_debug_t debug;

    debug.cycle = instr.cycle;
    debug.enter = false;
    debug.exit  = false;
    debug.hart  = instr.hart;

    if (FLAGS_rvfi_log) {
      log(cvm::NONE, "#{} {} 0 (enter debug mode)\n", count_, debug.cycle);
    }
    if (instr.intr && (instr.icause == 0)){
      debug.enter = true;
    }
    bridge_->enter_debug_mode(debug);
    in_debug_mode_ = true;
  }
}

void rvfi::exit_debug_mode(rv_instr_t& instr) {
  if (!FLAGS_cosim)
    return;

  if (terminated_ || in_reset_)
    return;

  uint32_t cluster_id = 0;
  if (!FLAGS_debugrom && (uint64_t)instr.pc.pc_rdata == generate_dm_device_addr(cluster_id) + FLAGS_debug_exit_pc_offset) {

    rv_debug_t debug;

    debug.cycle = instr.cycle;
    debug.enter = false;
    debug.exit  = true;
    debug.hart  = instr.hart;

    if (FLAGS_rvfi_log) {
      log(cvm::NONE, "#{} {} 0 (exit debug mode)\n", count_, debug.cycle);
    }

    bridge_->exit_debug_mode(debug);
    in_debug_mode_ = false;
  }
}

void rvfi::process(const rv_tester_transactions::cosim::m_csri<>& m_csri) {
  if (terminated_ || in_reset_)
    return;

  csr_t c {true, m_csri.hart, m_csri.cycle, m_csri.addr, m_csri.mask, m_csri.data};

  hw_csrs_.push_back(c);
  print_csr(c);
  send_csr(c);
}

void rvfi::process(const rv_tester_transactions::cosim::m_mhpm_counter_ovf<>& m_mhpm_counter_ovf) {
  if (terminated_ || in_reset_)
    return;

  csr_t c {true, m_mhpm_counter_ovf.hart, m_mhpm_counter_ovf.cycle, m_mhpm_counter_ovf.addr, 0, 0};
  bridge_->process_counter_overflow(c);
}

bool rvfi::sc_failed(mem_t& write) {

  if (sc_result_.find(write.tag) == sc_result_.end()) {
    return true;
  }

  mem_t m = sc_result_.at(write.tag);

  if (m.data == 0x0) {
    return false;
  }

  return true;
}





void rvfi::process_ncio_fetches(const rv_instr_t& instr) {
  if (!FLAGS_cosim || !FLAGS_mcm)
    return;

  if (terminated_ || in_reset_)
    return;

  ncio_fetches_.erase(
    std::remove_if(ncio_fetches_.begin(), ncio_fetches_.end(), [&](const mem_t& fetch) {
      bool evict = std::find(active_ncio_fetches_.begin(), active_ncio_fetches_.end(), fetch) == active_ncio_fetches_.end();
      if (evict) {
        // Send m_mcmi_ievict message to mcmi via registry messenger
        // mcmi is registered at the same location and handles m_mcmi_ievict
        cvm::registry::messenger.signal<rv_tester_transactions::cosim::m_mcmi_ievict<>>(
          loc_, rv_tester_transactions::cosim::m_mcmi_ievict<>(loc_, instr.cycle, instr.hart, fetch.pa));
      }
      return evict;
    }),
    ncio_fetches_.end());
}

bool rvfi::is_ncio(uint32_t mem_attr) {
  return ((mem_attr & 0x800) != 0) || ((mem_attr & 0x1000) == 0);
}

std::string rvfi::mem_attr_to_string(uint32_t mem_attr) {
    std::string result;
    result += (mem_attr & 0x800) ? "io," : "mem,";
    result += (mem_attr & 0x1000) ? "c"   : "nc";

    return result;
};

std::bitset<256> rvfi::extract_bits_as_bitset(const std::bitset<256>& bitset, size_t msb, size_t lsb) {
    std::bitset<256> result;

    size_t width = msb - lsb;
    for (size_t i = 0; i < width; ++i) {
        result[i] = bitset[lsb + i];
    }

    return result;
}

bool rvfi::check_axi_error(uint64_t addr) {
    // Check if the address is expected to have error response by calling AXI instances
    // Similar logic to enable_axi_error in bus_error_agent.cpp

    // Determine AXI type based on VIP flags (same logic as bus_error_agent)
    auto type = (FLAGS_vip && !FLAGS_vip_axi_dpi) ? "VIP_AXI" : "AXI";

    // Check all AXI instances
    for (const auto& loc : cvm::topology::get_from_type(type)) {
        if (loc != cvm::topology::null) {
            size_t count = 0;
            bool has_error = cvm::registry::messenger.call<axi::check_error_rpc>(loc, addr, count);
            if (has_error) {
                cvm::log(cvm::HIGH, "[rvfi] check_axi_error: addr={:#x} has error response configured, count={}\n", addr, count);
                return true;
            }
        }
    }

    // Also check NCIO_AXI instances
    for (const auto& loc : cvm::topology::get_from_type("NCIO_AXI")) {
        if (loc != cvm::topology::null) {
            size_t count = 0;
            bool has_error = cvm::registry::messenger.call<axi::check_error_rpc>(loc, addr, count);
            if (has_error) {
                cvm::log(cvm::HIGH, "[rvfi] check_axi_error: addr={:#x} has error response configured (NCIO_AXI), count={}\n", addr, count);
                return true;
            }
        }
    }

    return false;
}

void rvfi::process(const rv_tester::terminate_called_mem_checks&) {
  cvm::log(cvm::HIGH, "[RVFI] termination signaled by EOT memory checks, stopping further rvfi processing\n");
  terminated_ = true;
}

void rvfi::process(const rv_tester::terminate_called&) {
  cvm::log(cvm::HIGH, "[RVFI] termination signaled, stopping further rvfi processing\n");
  terminated_ = true;
}


void rvfi::process(const bridge::error_loc&) {
  cvm::log(cvm::HIGH, "[RVFI] cosim error, stopping further rvfi processing\n");
  terminated_ = true;
}

void rvfi::process(const rv_tester_transactions::cosim::m_disable_checks<>&) {
  cvm::log(cvm::HIGH, "[RVFI] disable_checks indication, stopping further rvfi processing\n");
  terminated_ = true;
}

extern "C" {
  void cosim_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();
    cvm::registry::messenger.signal<svScope>(loc, scope);
  }
}

extern "C" long long get_max_cycle() {
    return FLAGS_max_cycle;
}

extern "C" long long get_max_stall_cycle() {
  return FLAGS_max_stall_cycle;
}

bool get_csr_name_instr(const std::string& input, std::string& modified_string) {
    // Define the regex pattern to find 'c' followed by digits
    std::regex pattern(R"(c(\d+))");
    std::smatch match;

    // Search for the first match in the input string
    if (std::regex_search(input, match, pattern)) {
      try {
        // Extract the numeric part and convert to uint64_t
        uint64_t search_addr = std::stoull(match[1].str());
        // Find the entry with the matching address
        if (auto* csr = find_csr_by_address(search_addr); csr != nullptr) {
            modified_string = std::regex_replace(input, pattern, csr->name);
            return true;
        }
      } catch (...) {
        return false;  // Handle any exception and return false
      }
    }
    return false;  // No valid match found or entry not found
}
