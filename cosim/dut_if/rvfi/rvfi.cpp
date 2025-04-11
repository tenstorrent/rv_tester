#include "rvfi.h"
#include "util.h"
#include "whisper_decoder.h"
#include "cvm/plusargs.hpp"
#include "cvm/bitmanip.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/registry.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "cosim/bridge/bridge_plusargs.h"

#include <iostream>
#include <chrono>
#include <cmath>
#include <regex>

DEFINE_bool(rvfi, true, "Enable rvfi");
DEFINE_bool(rvfi_log,  true, "Enable rvfi logging");
DEFINE_bool(rvfi_log_36b_uop, true, "rvfi log - print 36b uop instead of default 32b riscv opcode");
DEFINE_bool(mcm, true, "Enable mcm");
DEFINE_bool(cosim, true, "Enable cosim checking");
DEFINE_bool(emulate_amo_arithmetic, true, "Emulate amo arithmetic if dut harness does not provide amo outputs");
DEFINE_bool(vec_cmode_tag_override, true, "If vector instruction enters conservative mode, override subsequent rvfi/mcmi tags with original instruction tag");
DEFINE_bool(patch_mode_tag_override, true, "In Patch mode, override subsequent rvfi/mcmi tag with original instruction tag");
DEFINE_bool(offline_cosim, false , "Enables Offline DPI capture of COSIM");

DEFINE_uint64(debug_entry_pc, 0x42190800, "Debug Mode entry PC");
DEFINE_uint64(debug_exit_pc, 0x421908cc, "Debug Mode exit PC");
DEFINE_uint64(debug_mem_base, 0x42190000, "Debug Memory Base Address");
DEFINE_uint64(debug_mem_size, 0x1000, "Debug Memory Size");
DEFINE_bool(use_sw_priv, false, "Enable use of SW generation of priv/patch_mode values instead of hw");

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
    rv_tester_transactions::cosim::m_trap<>,
    rv_tester_transactions::cosim::m_core_nmi<>,
    rv_tester_transactions::cosim::m_interrupt_pend<>,
    rv_tester_transactions::cosim::m_imsic_msi<>,
    rv_tester_transactions::cosim::m_mcmi_read<>,
    rv_tester_transactions::cosim::m_mcmi_insert<>,
    rv_tester_transactions::cosim::m_mcmi_write<>,
    rv_tester_transactions::cosim::m_mcmi_bypass<>,
    rv_tester_transactions::cosim::m_mcmi_ifetch_req<>,
    rv_tester_transactions::cosim::m_mcmi_ifetch_resp<>,
    rv_tester_transactions::cosim::m_mcmi_ievict<>,
    rv_tester_transactions::cosim::m_debug<>,
    bridge::error_loc
  >(loc);

  // Special case: Subscribe to mtime packets from all cores
  for (const auto& cosim_loc : cvm::topology::get_from_type("COSIM")) {
    connect< rv_tester_transactions::cosim::m_mtime<> >(cosim_loc);
  }

  connect<
    rv_tester::terminate_called
  >(cvm::topology::get_from_type("PLATFORM", 0));

  // Flags configuration
  uint32_t ncores = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  if (ncores > 1) {
    FLAGS_mcm = true;
    cvm::log(cvm::NONE, "[plusargs] +mcm\n");
  }

  // Reset/init configuration
  init();
}

rvfi::~rvfi() {
  uint32_t ncores = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  if (FLAGS_rvfi && ncores == 1 && (count_ == 1) && (FLAGS_offline_cosim == false))
    cvm::log(cvm::ERROR, "Error: rvfi termination without processing any instructions\n");
}

void rvfi::check() {
  // bridge_->report_metrics();
}

void rvfi::init() {

  if (FLAGS_cosim) {
    cvm::log(cvm::MEDIUM, "[RVFI loc {} id{}] Constructing bridge...\n", loc_, id_);
    auto platform_loc = cvm::topology::get_from_type("PLATFORM", 0);
    bridge_ = std::make_unique<bridge>(cvm::topology::attr(platform_loc, "NHARTS").second, xlen, vlen, loc_, id_);
    count_ = 1;

  } else {
    cvm::log(cvm::MEDIUM, "Running with cosim is disabled\n");
  }
}

bool rvfi::patch_access (uint64_t addr) {
  if (!patch_mode_)
      return false;

  if (addr >= patch_ram_lo && addr < patch_ram_hi)
      return true;

  uint64_t pcontrol0 = 0x42005040; //areddy
  for (int i=0; i<8; i++) // do this for all cores0-8
    if (addr == (pcontrol0 + (i*0x10000)))
      return true;
  return false;
}

void rvfi::process(const rv_tester_transactions::cosim::m_reset<>& m_reset) {

  if (terminated_)
    return;

  if (loc_ != m_reset.location)
    return;

  in_reset_ = false;
  cvm::log(cvm::MEDIUM, "[rvfi] reset\n");

  if (FLAGS_cosim)
    bridge_->reset();
  else
    FLAGS_whisper_client_check = false;
}

void rvfi::process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi) {

  if (terminated_ || in_reset_)
    return;

  if (loc_ != m_rvfi.location)
    return;


  // Construct rv_instr_t and send to bridge
  rv_instr_t instr;
  make_instr(m_rvfi, instr);
  print_instr(instr);

  if (m_rvfi.trap) {
    trap_insn_ = m_rvfi.insn;
    trap_addr_ = (m_rvfi.insn == 0) ? m_rvfi.pc_rdata : ((m_rvfi.mem_rmask != 0) || (m_rvfi.mem_wmask != 0)) ? m_rvfi.mem_addr : 0x0;
    return;
  }

  prev_uop_tag_ = m_rvfi.order;

  if (vec_cmode_)
    if (vec_cmode_tags_.find(m_rvfi.order) == vec_cmode_tags_.end())
      vec_cmode_tags_.emplace(m_rvfi.order, vec_cmode_first_tag_);

  if (patch_mode_ && FLAGS_patch_mode_tag_override)
    if (patch_mode_tags_.find(m_rvfi.order) == patch_mode_tags_.end())
      patch_mode_tags_.emplace(m_rvfi.order, patch_mode_first_tag_);


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
    icause_ = m_trap.cause & 0x3f;

  } else if (m_trap.id == EXCP) {
    
    if (FLAGS_cosim) {
      if (m_trap.cause == 60) { // Patch special case
        cvm::log(cvm::HIGH, "Enter patch via exception\n");
        bridge_->set_patch_mode(ENTER_PATCH);
        patch_mode_ = true;
        if (FLAGS_patch_mode_tag_override)
          patch_mode_first_tag_ = m_trap.order;
      }
    }
    // Set exception state
    nmi_ = false;
    intr_ = false;
    excp_ = true;
    ecause_ = m_trap.cause & 0xff;
    // RVTOOLS-3265, RVTOOLS-3479: Adjust tag for conservative mode vector instructions
    // Capture the tag and use it for all activity related to
    // the vector instruction
    if (FLAGS_vec_cmode_tag_override && (ecause_ == CUSTOM_VEC_CMODE)) {
      vec_cmode_ = true;
      vec_cmode_first_tag_ = m_trap.order;
    } else {
      // Capture the tag of any exceptions that happen in the shadow of conservative mode
      if (vec_cmode_)
        if (vec_cmode_tags_.find(m_trap.order) == vec_cmode_tags_.end())
          vec_cmode_tags_.emplace(m_trap.order, vec_cmode_first_tag_);
    }
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

  std::string dut_log;
  dut_log += fmt::format("#NA {} {} ({} : mip={:#x} : ", intr.cycle, id_, intr.hw ? "hw" : "sw", intr.mip.to_ullong());
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
  intr.mip = std::bitset<64>(m_mtime.mip);
  intr.mtime = m_mtime.mtime;

  if (FLAGS_rvfi_log)
    log(cvm::NONE, "#NA {} {} (mtime={:#x})\n", intr.cycle, id_, intr.mtime);

  if (!FLAGS_cosim)
    return;

  bridge_->process_dut_timer(id_, intr);
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
    log(cvm::NONE, "#{} {} 0 (nmi:{} cause:{})\n", count_, nmi.cycle, nmi.valid, nmi.cause);

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
  instr.id = count_;
  instr.comp = m_rvfi.comp;
  instr.tag = vec_cmode_ ? vec_cmode_first_tag_ : patch_mode_ && FLAGS_patch_mode_tag_override? patch_mode_first_tag_ : m_rvfi.order;
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

  // Renamed csr sequence
  uint64_t src = (m_rvfi.uop >> 16) & 0x3f;
  bool src_renamed = (src >= 35) && (src <= 37);
  bool dest_renamed = (m_rvfi.rd_addr >= 35) && (m_rvfi.rd_addr <= 37);
  instr.csr_renamed = src_renamed || dest_renamed;
  instr.csr_renamed_name = "";
  if (instr.csr_renamed) {
    auto renamed_addr = src_renamed  ? renamed_csr.at(src) :
                                       renamed_csr.at(m_rvfi.rd_addr);
    if (auto it = csrs.find(renamed_addr); it != csrs.end()) {
        instr.csr_renamed_name = it->second.name;
    }
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
    bridge_->set_patch_mode(ENTER_PATCH);
    patch_mode_ = true;
    if (FLAGS_patch_mode_tag_override) {
      patch_mode_first_tag_ = m_rvfi.order;
      instr.tag = patch_mode_first_tag_;
    }
  }
  if (m_rvfi.clr_pmode) {
    cvm::log(cvm::HIGH, "CLOCK={}: Patch mode turned OFF\n",m_rvfi.cycle);
    bridge_->set_patch_mode(EXIT_PATCH);
    patch_mode_ = false;
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
    bridge_->set_patch_mode(ENTER_PATCH);
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
      bridge_->set_patch_mode(EXIT_PATCH);
      patch_mode_ = false;
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

  // CSR renaming
  if (renamed_csr.count(m_rvfi.rd_addr)) {
    const auto& new_name = renamed_csr.at(m_rvfi.rd_addr);
    uint32_t addr = 0;
    if (auto it = csrs.find(new_name); it != csrs.end()) {
        addr = it->second.addr;
    }
    csr_t c {true, m_rvfi.hart, m_rvfi.cycle, addr, std::numeric_limits<uint64_t>::max(), m_rvfi.rd_wdata};
    instr.csr.push_back(c);
      // This is for print in the rvfi log
    instr.gpr.emplace_back(false, m_rvfi.rd_addr, m_rvfi.rd_wdata);
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
  instr.mem_read.attr = m_rvfi.mem_attr;

  // Mem writes
  instr.mem_write.valid = (m_rvfi.mem_wmask != 0);
  instr.mem_write.va = m_rvfi.mem_addr;
  instr.mem_write.pa = m_rvfi.mem_paddr;
  instr.mem_write.data = m_rvfi.mem_wdata;
  instr.mem_write.size = log2(m_rvfi.mem_wmask + 1);
  instr.mem_write.attr = m_rvfi.mem_attr;
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
  if (!FLAGS_rvfi_log)
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

  if ((!instr.ucode || !instr.last_uop) && !instr.csr_renamed)
    for (auto& c : instr.csr)
      print_instr_resource(instr, fmt::format(" c {:016x} {:016x} {:016x}", c.csr_addr, c.csr_wdata, c.csr_wmask));
}

void rvfi::print_instr_resource(const rv_instr_t& instr, std::string resource_str) {
  std::string dut_log;

  dut_log += fmt::format("#{} {} {} {} {:016x}", FLAGS_mcm ? instr.tag : instr.id, instr.cycle, instr.hart, priv_to_string.at(static_cast<priv>(instr.priv)),
     instr.pc.pc_rdata);

  if (FLAGS_rvfi_log_36b_uop)
    dut_log += fmt::format(" {:09x}", instr.uop);
  else
    dut_log += fmt::format(" {:08x}", instr.opcode);

  dut_log += fmt::format("{}", resource_str);

  if (!instr.ucode || instr.csr_renamed || cracked_gpr_.valid) {
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

  if (instr.csr_renamed)
    dut_log += fmt::format(" (csr_rename:{})", instr.csr_renamed_name);

  if (instr.flags)
    dut_log += fmt::format(" (flags:{:#x})", instr.flags);

  if (instr.mem_write.valid)
    dut_log += fmt::format(" [{:#x}:{:#x}:{}]", instr.mem_write.va, instr.mem_write.pa, mem_attr_to_string(instr.mem_write.attr));

  if (instr.mem_read.valid)
    dut_log += fmt::format(" [{:#x}:{:#x}:{}]", instr.mem_read.va, instr.mem_read.pa, mem_attr_to_string(instr.mem_read.attr));

  if (instr.trap_valid)
    dut_log += fmt::format(" (trap)");

  if (instr.nmi)
    dut_log += fmt::format(" (nmi: {})", nmi_to_string.count(static_cast<nmi>(instr.ncause)) ? nmi_to_string.at(static_cast<nmi>(instr.ncause)) : std::to_string(instr.ncause));

  if (instr.intr)
    dut_log += fmt::format(" (interrupt: {})", intr_to_string.count(static_cast<intr>(instr.icause)) ? intr_to_string.at(static_cast<intr>(instr.icause)) : std::to_string(instr.icause));

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

  if ((instr.intr && (instr.icause == 0)) || (instr.excp && (instr.ecause == 31))) {
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

  if ((uint64_t)instr.pc.pc_rdata == FLAGS_debug_exit_pc) {

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

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_read<>& m_mcmi_read) {
  if (!FLAGS_cosim || !FLAGS_mcm)
    return;

  if (terminated_ || in_reset_)
    return;

  if (patch_access(m_mcmi_read.addr))
    return;

  mem_t m;
  m.valid  = true;
  m.hart   = m_mcmi_read.hart;
  m.cycle  = m_mcmi_read.cycle;
  m.opcode  = m_mcmi_read.opcode;
  // Handle tags
  if (vec_cmode_tags_.contains(m_mcmi_read.order))
      m.tag = vec_cmode_tags_[m_mcmi_read.order];
  else if (patch_mode_tags_.contains(m_mcmi_read.order))
      m.tag = patch_mode_tags_[m_mcmi_read.order];
  else if (patch_mode_) {
      patch_mode_tags_.emplace(m_mcmi_read.order, patch_mode_first_tag_);
      m.tag = patch_mode_first_tag_;
  } else
      m.tag = m_mcmi_read.order;
  m.pa     = m_mcmi_read.addr;
  m.size   = std::popcount(m_mcmi_read.mask);
  m.data   = m_mcmi_read.data;
  m.data_vec   = m_mcmi_read.data_vec;
  m.amo    = m_mcmi_read.amo;
  m.amo_op = m_mcmi_read.amo_op;
  m.v_ext  = m_mcmi_read.v_ext;
  m.field = m_mcmi_read.field;
  m.elem_idx = m_mcmi_read.elem_idx;
  m.splat = m_mcmi_read.splat;
  uint8_t elemsize = m_mcmi_read.elem_size;

  // Handle SC
  // If read before bypass, store pass/fail result
  // If bypass before read, check pass/fail result and send/don't send bypass
  if (m.amo && m.amo_op == SC) {
    if (sc_bypass_.find(m.tag) == sc_bypass_.end()) {
      sc_result_.emplace(m.tag, m);
    } else {
      if (!sc_failed(sc_bypass_.at(m.tag))) {
        bridge_->process_dut_mcm_bypass(m.hart, sc_bypass_.at(m.tag));
        sc_bypass_.erase(m.tag);
      }
    }
    return;
  }

  uint64_t mask = m_mcmi_read.mask;
  uint64_t numones = std::popcount(mask);
  std::bitset<256> data_vec = m.data_vec;

  // Find the number of consecutive ones starting from the first set bit
  uint64_t leadingZeros = std::countr_zero(mask);  // Find the number of trailing zeros
  mask >>= leadingZeros;
  uint64_t consecutiveOnes = std::countr_zero(~mask);  // Count ones until the first zero
  if (numones == consecutiveOnes) {
      if (m_mcmi_read.v_ext & m_mcmi_read.splat){
        uint16_t total_elements;
        if (numones / elemsize) {
          total_elements = numones / elemsize;
          m.size = elemsize;
        } else {
          total_elements = 1;
          m.size = numones;
        }
        for (int i=0; i<total_elements; i++){
          uint64_t value = 0;
          // Extract the bits for the current element
          for (size_t j = 0; j < elemsize*8; ++j) {
              size_t bit_index = i * elemsize*8 + j;
              if (bit_index >= data_vec.size()) break; // Avoid overflow
              if (data_vec[bit_index]) {
                  value |= (1ULL << j);
              }
          }
          m.data_vec = value;
          m.elem_idx = m_mcmi_read.elem_idx + i;
          bridge_->process_dut_mcm_read(m_mcmi_read.hart, m);
        }
      }
      else {
        m.data_vec = extract_bits_as_bitset(m.data_vec, m.size*8, 0);
        bridge_->process_dut_mcm_read(m_mcmi_read.hart, m);
      }
  } else {
      std::bitset<32> mask = m_mcmi_read.mask;
      std::vector<uint64_t> addresses;
      std::vector<uint8_t> datas;
      for (int i = 0; i < 32; i++) {
          if (mask[i]) {
              addresses.push_back(m_mcmi_read.addr + i);
              uint8_t byte = 0;
              for (int bit = i*8; bit < 8*(i+1); ++bit) {
                  if (m_mcmi_read.data_vec[bit]) {
                      byte |= (1 << (bit - (i*8)));  // Set the corresponding bit in first_byte
                  }
              }
              datas.push_back(byte);
          }
      }

      uint64_t start_addr = addresses[0];
      size_t size = 1;
      std::string dataAccumulated = fmt::format("{:02x}", datas[0]);  

      for (size_t i = 1; i < addresses.size(); ++i) {
          if (addresses[i] == addresses[i - 1] + 1) {
              ++size;
              dataAccumulated = fmt::format("{:02x}", datas[i]) + dataAccumulated;
          } else {
              mem_t m;
              m.valid = true;
              m.cycle = m_mcmi_read.cycle;
              m.tag = vec_cmode_tags_.contains(m_mcmi_read.order) ? vec_cmode_tags_[m_mcmi_read.order] :
                        patch_mode_tags_.contains(m_mcmi_read.order)? patch_mode_tags_[m_mcmi_read.order] : m_mcmi_read.order;
              m.v_ext = m_mcmi_read.v_ext;
              m.field = m_mcmi_read.field;
              if (m_mcmi_read.v_ext & m_mcmi_read.splat){
                uint16_t total_elements;
                if (size / elemsize) {
                  total_elements = size / elemsize;
                  m.size = elemsize;
                } else {
                  total_elements = 1;
                  m.size = size;
                }
                m.pa = m_mcmi_read.addr;
                for (int i=0; i<total_elements; i++){
                  size_t start, end;
                  if (size / elemsize) {
                    start = dataAccumulated.size() - (i + 1) * 2 * elemsize;
                    end = dataAccumulated.size() - i * 2 * elemsize;
                  } else {
                    start = 0;
                    end = dataAccumulated.size();
                  }
                  std::bitset<256> value = stringToBitset(dataAccumulated.substr(start, end - start));
                  m.data_vec = value;
                  m.elem_idx = ((start_addr - m_mcmi_read.addr) / elemsize) + m_mcmi_read.elem_idx + i;
                  bridge_->process_dut_mcm_read(m_mcmi_read.hart, m);
                }
              } else{
                m.pa = start_addr;
                m.size = size;
                std::bitset<256> value = stringToBitset(dataAccumulated);  // Use a helper to convert the accumulated string
                m.data_vec = value;
                m.elem_idx = ((start_addr - m_mcmi_read.addr) / elemsize) + m_mcmi_read.elem_idx;
                bridge_->process_dut_mcm_read(m_mcmi_read.hart, m);
              }
              start_addr = addresses[i];
              size = 1;
              dataAccumulated = fmt::format("{:02x}", datas[i]);
          }
      }
      mem_t m;
      m.valid = true;
      m.cycle = m_mcmi_read.cycle;
      m.tag = vec_cmode_tags_.contains(m_mcmi_read.order) ? vec_cmode_tags_[m_mcmi_read.order] :
                patch_mode_tags_.contains(m_mcmi_read.order)? patch_mode_tags_[m_mcmi_read.order] : m_mcmi_read.order;
      m.v_ext = m_mcmi_read.v_ext;
      m.size   = std::popcount(m_mcmi_read.mask);
      m.field = m_mcmi_read.field;
      if (m_mcmi_read.v_ext & m_mcmi_read.splat){
        uint16_t total_elements;
        if (size / elemsize) {
          total_elements = size / elemsize;
          m.size = elemsize;
        } else {
          total_elements = 1;
          m.size = size;
        }
        m.pa = m_mcmi_read.addr;
        for (int i=0; i<total_elements; i++){
          size_t start, end;
          if (size / elemsize) {
            start = dataAccumulated.size() - (i + 1) * 2 * elemsize;
            end = dataAccumulated.size() - i * 2 * elemsize;
          } else {
            start = 0;
            end = dataAccumulated.size();
          }
          std::bitset<256> value = stringToBitset(dataAccumulated.substr(start, end - start));          m.data_vec = value;
          m.elem_idx = ((start_addr - m_mcmi_read.addr) / elemsize) + m_mcmi_read.elem_idx + i;
          bridge_->process_dut_mcm_read(m_mcmi_read.hart, m);
        }
      } else{
        m.pa = start_addr;
        m.size = size;
        std::bitset<256> value = stringToBitset(dataAccumulated);  // Use a helper to convert the accumulated string
        m.data_vec = value;
        m.elem_idx = ((start_addr - m_mcmi_read.addr) / elemsize) + m_mcmi_read.elem_idx;
        bridge_->process_dut_mcm_read(m_mcmi_read.hart, m);
      }
  }
  if (m.amo && m.amo_op != LR && FLAGS_emulate_amo_arithmetic) {
    process_amo(m);
  }
}

// Helper function to convert a hex string into a bitset
std::bitset<256> rvfi::stringToBitset(const std::string& hexString) {
  std::bitset<256> bits;
  size_t len = hexString.length();
  for (size_t i = 0; i < len; ++i) {
      int hexDigit = (hexString[len - 1 - i] >= '0' && hexString[len - 1 - i] <= '9') 
                     ? hexString[len - 1 - i] - '0' 
                     : hexString[len - 1 - i] - 'a' + 10;
      for (int j = 3; j >= 0; --j) {
          bits[(i * 4) + j] = (hexDigit >> j) & 1;
      }
  }
  return bits;
}

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_insert<>& m_mcmi_insert) {
  if (!FLAGS_cosim || !FLAGS_mcm)
    return;

  if (terminated_ || in_reset_)
    return;

  if (patch_access(m_mcmi_insert.addr))
    return;

  uint64_t mask = m_mcmi_insert.mask;
  uint64_t numones = std::popcount(mask);

  // Find the number of consecutive ones starting from the first set bit
  uint64_t leadingZeros = std::countr_zero(mask);  // Find the number of trailing zeros
  mask >>= leadingZeros;
  uint64_t consecutiveOnes = std::countr_zero(~mask);  // Count ones until the first zero

  if (numones == consecutiveOnes) {
      mem_t m;
      m.valid = true;
      m.cycle = m_mcmi_insert.cycle;
      m.tag = vec_cmode_tags_.contains(m_mcmi_insert.order) ? vec_cmode_tags_[m_mcmi_insert.order] :  
                patch_mode_tags_.contains(m_mcmi_insert.order)? patch_mode_tags_[m_mcmi_insert.order] : m_mcmi_insert.order;
      m.pa = m_mcmi_insert.addr;
      m.size = numones;
      m.data = m_mcmi_insert.data;
      m.data_vec = m_mcmi_insert.data_vec;
      m.v_ext = m_mcmi_insert.v_ext;
      m.elem_idx = m_mcmi_insert.elem_idx;
      bridge_->process_dut_mcm_insert(m_mcmi_insert.hart, m);
  } else {
      std::bitset<32> mask = m_mcmi_insert.mask;
      std::vector<uint64_t> addresses;
      std::vector<uint8_t> datas;

      for (int i = 0; i < 32; i++) {
          if (mask[i]) {
              addresses.push_back(m_mcmi_insert.addr + i);
              uint8_t byte = 0;
              for (int bit = i*8; bit < 8*(i+1); ++bit) {
                  if (m_mcmi_insert.data_vec[bit]) {
                      byte |= (1 << (bit - (i*8)));  // Set the corresponding bit in first_byte
                  }
              }
              datas.push_back(byte);
          }
      }

      uint64_t start_addr = addresses[0];
      size_t size = 1;
      std::string dataAccumulated = fmt::format("{:02x}", datas[0]);  

      for (size_t i = 1; i < addresses.size(); ++i) {
          if (addresses[i] == addresses[i - 1] + 1) {
              ++size;
              dataAccumulated = fmt::format("{:02x}", datas[i]) + dataAccumulated;
          } else {
              mem_t m;
              m.valid = true;
              m.cycle = m_mcmi_insert.cycle;
              m.tag = vec_cmode_tags_.contains(m_mcmi_insert.order) ? vec_cmode_tags_[m_mcmi_insert.order] :
                        patch_mode_tags_.contains(m_mcmi_insert.order)? patch_mode_tags_[m_mcmi_insert.order] : m_mcmi_insert.order;
              m.pa = start_addr;
              m.size = size;
              std::bitset<256> value = stringToBitset(dataAccumulated);  // Use a helper to convert the accumulated string
              m.data_vec = value;
              m.v_ext = m_mcmi_insert.v_ext;
              m.elem_idx = m_mcmi_insert.elem_idx;
              bridge_->process_dut_mcm_insert(m_mcmi_insert.hart, m);
              start_addr = addresses[i];
              size = 1;
              dataAccumulated = fmt::format("{:02x}", datas[i]);
          }
      }
      mem_t m;
      m.valid = true;
      m.cycle = m_mcmi_insert.cycle;
      m.tag = vec_cmode_tags_.contains(m_mcmi_insert.order) ? vec_cmode_tags_[m_mcmi_insert.order] :
                patch_mode_tags_.contains(m_mcmi_insert.order)? patch_mode_tags_[m_mcmi_insert.order] : m_mcmi_insert.order;
      m.pa = start_addr;
      m.size = size;
      m.data_vec = stringToBitset(dataAccumulated);  // Final range processing
      m.v_ext = m_mcmi_insert.v_ext;
      m.elem_idx = m_mcmi_insert.elem_idx;
      bridge_->process_dut_mcm_insert(m_mcmi_insert.hart, m);
  }
}

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_bypass<>& m_mcmi_bypass) {
  if (!FLAGS_cosim || !FLAGS_mcm)
    return;

  if (terminated_ || in_reset_)
    return;

  if (patch_access(m_mcmi_bypass.addr))
    return;

  uint64_t mask = m_mcmi_bypass.mask;
  uint64_t numones = std::popcount(mask);

  // Find the number of consecutive ones starting from the first set bit
  uint64_t leadingZeros = std::countr_zero(mask);  // Find the number of trailing zeros
  mask >>= leadingZeros;
  uint64_t consecutiveOnes = std::countr_zero(~mask);  // Count ones until the first zero
  
  if (numones == consecutiveOnes) {
      mem_t m;
      m.valid  = true;
      m.hart   = m_mcmi_bypass.hart;
      m.cycle  = m_mcmi_bypass.cycle;
      m.tag    = vec_cmode_tags_.contains(m_mcmi_bypass.order) ? vec_cmode_tags_[m_mcmi_bypass.order] :
                    patch_mode_tags_.contains(m_mcmi_bypass.order)? patch_mode_tags_[m_mcmi_bypass.order] : m_mcmi_bypass.order;
      m.pa     = m_mcmi_bypass.addr;
      m.size   = std::popcount(m_mcmi_bypass.mask);
      m.data   = m_mcmi_bypass.data;
      m.data_vec = extract_bits_as_bitset(m_mcmi_bypass.data_vec, m.size*8, 0);
      m.v_ext  = m_mcmi_bypass.v_ext;
      m.amo    = m_mcmi_bypass.amo;
      m.amo_op = m_mcmi_bypass.amo_op;
      m.elem_idx = m_mcmi_bypass.elem_idx;

      if (m.amo && m.amo_op != SC && FLAGS_emulate_amo_arithmetic) {
        amo_writes_.emplace(m.tag, m);
        return;
      }

      if (m.amo && m.amo_op == SC && sc_failed(m)) {
        sc_bypass_.emplace(m.tag, m);
        return;
      }

      bridge_->process_dut_mcm_bypass(m_mcmi_bypass.hart, m);
  } else {
      std::bitset<32> mask = m_mcmi_bypass.mask;
      std::vector<uint64_t> addresses;
      std::vector<uint8_t> datas;

      for (int i = 0; i < 32; i++) {
          if (mask[i]) {
              addresses.push_back(m_mcmi_bypass.addr + i);
              uint8_t byte = 0;
              for (int bit = i*8; bit < 8*(i+1); ++bit) {
                  if (m_mcmi_bypass.data_vec[bit]) {
                      byte |= (1 << (bit - (i*8)));  // Set the corresponding bit in first_byte
                  }
              }
              datas.push_back(byte);
          }
      }

      uint64_t start_addr = addresses[0];
      size_t size = 1;
      std::string dataAccumulated = fmt::format("{:02x}", datas[0]);  

      for (size_t i = 1; i < addresses.size(); ++i) {
          if (addresses[i] == addresses[i - 1] + 1) {
              ++size;
              dataAccumulated = fmt::format("{:02x}", datas[i]) + dataAccumulated;
          } else {
              mem_t m;
              m.valid = true;
              m.cycle = m_mcmi_bypass.cycle;
              m.tag = vec_cmode_tags_.contains(m_mcmi_bypass.order) ? vec_cmode_tags_[m_mcmi_bypass.order] :
                        patch_mode_tags_.contains(m_mcmi_bypass.order)? patch_mode_tags_[m_mcmi_bypass.order] : m_mcmi_bypass.order;
              m.pa = start_addr;
              m.size = size;
              std::bitset<256> value = stringToBitset(dataAccumulated);  // Use a helper to convert the accumulated string
              m.data_vec = value;
              m.v_ext = m_mcmi_bypass.v_ext;
              m.elem_idx = m_mcmi_bypass.elem_idx;
              bridge_->process_dut_mcm_bypass(m_mcmi_bypass.hart, m);
              start_addr = addresses[i];
              size = 1;
              dataAccumulated = fmt::format("{:02x}", datas[i]);
          }
      }
      mem_t m;
      m.valid = true;
      m.cycle = m_mcmi_bypass.cycle;
      m.tag = vec_cmode_tags_.contains(m_mcmi_bypass.order) ? vec_cmode_tags_[m_mcmi_bypass.order] :
                patch_mode_tags_.contains(m_mcmi_bypass.order)? patch_mode_tags_[m_mcmi_bypass.order] : m_mcmi_bypass.order;
      m.pa = start_addr;
      m.size = size;
      m.data_vec = stringToBitset(dataAccumulated);  // Final range processing
      m.v_ext = m_mcmi_bypass.v_ext;
      m.elem_idx = m_mcmi_bypass.elem_idx;
      bridge_->process_dut_mcm_bypass(m_mcmi_bypass.hart, m);
  }
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

void rvfi::process_amo(mem_t& read) {

  if (amo_writes_.find(read.tag) == amo_writes_.end()) {
    cvm::log(cvm::ERROR, "Error: Amo read with no matching bypass write - inst tag={}\n", read.tag);
    return;
  }

  mem_t m = amo_writes_.at(read.tag);
  m.cycle = read.cycle;
  amo_modify_write_data(static_cast<amo_op>(m.amo_op), read.data, m.data, m.size);

  bridge_->process_dut_mcm_bypass(m.hart, m);

  amo_writes_.erase(read.tag);
}

void rvfi::amo_modify_write_data(amo_op op, uint64_t& read_data, uint64_t& write_data, uint8_t size) {

  bool sign = (op == AMOADD) || (op == AMOSWAP) || (op == AMOMAX) || (op == AMOMIN);
  switch (size) {
    case 1:
      if (sign) amo_arithmetic<int8_t>(op, read_data, write_data, size);
      else      amo_arithmetic<uint8_t>(op, read_data, write_data, size);
      break;
    case 2:
      if (sign) amo_arithmetic<int16_t>(op, read_data, write_data, size);
      else      amo_arithmetic<uint16_t>(op, read_data, write_data, size);
      break;
    case 4:
      if (sign) amo_arithmetic<int32_t>(op, read_data, write_data, size);
      else      amo_arithmetic<uint32_t>(op, read_data, write_data, size);
      break;
    case 8:
      if (sign) amo_arithmetic<int64_t>(op, read_data, write_data, size);
      else      amo_arithmetic<uint64_t>(op, read_data, write_data, size);
      break;
    default:
      cvm::log(cvm::ERROR, "Error: Invalid amo op size - [op={}, size={}]\n", amo_op_to_string.at(op), size);
      break;
  }
}

template <typename T>
void rvfi::amo_arithmetic(amo_op op, uint64_t& read_data, uint64_t& write_data, uint8_t size) {
  T read = 0, write = 0;

  read = T(read_data) & cvm::bitmanip::mask<T>(size*8);
  write = T(write_data) & cvm::bitmanip::mask<T>(size*8);

  T result = 0;

  switch (op) {
    case AMOADD:
      result = read + write;
      break;
    case AMOAND:
      result = read & write;
      break;
    case AMOOR:
      result = read | write;
      break;
    case AMOXOR:
      result = read ^ write;
      break;
    case AMOSWAP:
      result = write;
      break;
    case AMOMIN:
    case AMOMINU:
      result = std::min(read, write);
      break;
    case AMOMAX:
    case AMOMAXU:
      result = std::max(read, write);
      break;
    default:
      assert(false && "Error: Unknown amo operation");
      break;
  }

  write_data = uint64_t(uint64_t(result) & cvm::bitmanip::mask<uint64_t>(size*8));
}

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_write<>& m_mcmi_write) {
  if (!FLAGS_cosim || !FLAGS_mcm)
    return;

  if (terminated_ || in_reset_)
    return;

  if (patch_access(m_mcmi_write.addr))
    return;

  mem_cl_t m;
  m.valid = true;
  m.cycle = m_mcmi_write.cycle;
  m.pa = m_mcmi_write.addr;
  m.mask = m_mcmi_write.mask;
  m.data = m_mcmi_write.data;

  bridge_->process_dut_mcm_write(m_mcmi_write.hart, m);
}

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_ifetch_req<>& m_mcmi_ifetch_req) {
  if (!FLAGS_cosim || !FLAGS_mcm)
    return;

  if (terminated_ || in_reset_)
    return;

  mem_t m;
  m.valid = true;
  m.tag = m_mcmi_ifetch_req.order;
  m.pa = m_mcmi_ifetch_req.addr;
  m.attr = m_mcmi_ifetch_req.attr;

  ifetch_reqs_.emplace(m.tag, m);
}

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_ifetch_resp<>& m_mcmi_ifetch_resp) {
  if (!FLAGS_cosim || !FLAGS_mcm)
    return;

  if (terminated_ || in_reset_)
    return;

  if (ifetch_reqs_.find(m_mcmi_ifetch_resp.order) == ifetch_reqs_.end()) {
    cvm::log(cvm::ERROR, "Error: Ifetch resp with no matching req - [id={}]\n", m_mcmi_ifetch_resp.order);
  }

  mem_t m;
  m = ifetch_reqs_.at(m_mcmi_ifetch_resp.order);
  m.cycle = m_mcmi_ifetch_resp.cycle;

  // RVDE-17736: Manage fetch/evict signaling for ncio region
  if (is_ncio(m.attr)) {
    auto it = std::find_if(ncio_fetches_.begin(), ncio_fetches_.end(), [&](const mem_t& fetch) { return fetch.pa == m.pa; });
    if (it == ncio_fetches_.end()) {
      bridge_->process_dut_mcm_ifetch(m_mcmi_ifetch_resp.hart, m);
      ncio_fetches_.emplace_back(m);
    }
    active_ncio_fetches_.emplace_back(m);
  } else {
    bridge_->process_dut_mcm_ifetch(m_mcmi_ifetch_resp.hart, m);
    if (!ncio_fetches_.empty()) {
      ncio_mem_transition_ = true;
    }
  }

  ifetch_reqs_.erase(m_mcmi_ifetch_resp.order);

}

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_ievict<>& m_mcmi_ievict) {
  if (!FLAGS_cosim || !FLAGS_mcm)
    return;

  if (terminated_ || in_reset_)
    return;

  if (patch_access(m_mcmi_ievict.addr))
    return;

  mem_t m;
  m.valid = true;
  m.cycle = m_mcmi_ievict.cycle;
  m.pa = m_mcmi_ievict.addr;

  bridge_->process_dut_mcm_ievict(m_mcmi_ievict.hart, m);
}

void rvfi::process_ncio_fetches(const rv_instr_t& instr) {
  if (!FLAGS_cosim || !FLAGS_mcm)
    return;

  if (terminated_ || in_reset_)
    return;

  ncio_fetches_.erase(
    std::remove_if(ncio_fetches_.begin(), ncio_fetches_.end(), [&](const mem_t& fetch) {
      bool evict = std::find(active_ncio_fetches_.begin(), active_ncio_fetches_.end(), fetch) == active_ncio_fetches_.end();
      if (evict)
        process(rv_tester_transactions::cosim::m_mcmi_ievict<>(loc_, instr.cycle, instr.hart, fetch.pa));
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
        if (auto it = csrs.find(search_addr); it != csrs.end()) {
            modified_string = std::regex_replace(input, pattern, it->second.name);
            return true;
        }
      } catch (...) {
        return false;  // Handle any exception and return false
      }
    }
    return false;  // No valid match found or entry not found
}
