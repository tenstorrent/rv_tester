#include "rvfi.h"
#include "util.h"
#include "whisper_decoder.h"
#include "cvm/plusargs.hpp"
#include "cvm/bitmanip.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/registry.hpp"

#include <iostream>
#include <chrono>
#include <cmath>

DEFINE_bool(rvfi, true, "Enable rvfi");
// TODO(mboisvert): See if we can combine the rvfi flags. The reason why the
// rvfi_log flag was created is that +norvfi causes the max # of cycles to be
// exceeded.
DEFINE_bool(rvfi_log, true, "Enable rvfi logging");
DEFINE_bool(rvfi_log_36b_uop, true, "rvfi log - print 36b uop instead of default 32b riscv opcode");
DEFINE_bool(mcm, false, "Enable mcm");
DEFINE_bool(cosim, true, "Enable cosim checking");
DEFINE_bool(emulate_amo_arithmetic, true, "Emulate amo arithmetic if dut harness does not provide amo outputs");
DECLARE_string(load);

DEFINE_uint64(debug_entry_pc, 0xa110800, "Debug Mode entry PC");
DEFINE_uint64(debug_exit_pc, 0xa110860, "Debug Mode exit PC");

REGISTRY_register(rvfi, COSIM, cvm::registry::all);

rvfi::rvfi(cvm::topology::loc_t loc, unsigned id)
  : log("h" + std::to_string(id) + "_dut_rvfi.log"), loc_(loc), id_(id) {
  init();

  whisper::initialize();

  cvm::registry::messenger.connect<svScope>(
    loc_,
    [&](svScope s) { return this->set_scope(s); });

  connect<
    rv_tester_transactions::cosim::m_rvfi<>,
    rv_tester_transactions::cosim::m_csri<>,
    rv_tester_transactions::cosim::m_trap<>,
    rv_tester_transactions::cosim::m_core_intr<>,
    rv_tester_transactions::cosim::m_imsic_msi<>,
    rv_tester_transactions::cosim::m_mcmi_read<>,
    rv_tester_transactions::cosim::m_mcmi_insert<>,
    rv_tester_transactions::cosim::m_mcmi_write<>,
    rv_tester_transactions::cosim::m_mcmi_bypass<>,
    rv_tester_transactions::cosim::m_mcmi_ifetch_req<>,
    rv_tester_transactions::cosim::m_mcmi_ifetch_resp<>,
    rv_tester_transactions::cosim::m_mcmi_ievict<>,
    rv_tester_transactions::cosim::m_debug<>
  >(loc);

  connect<
    rv_tester::terminate_called
  >(cvm::topology::get_from_type("PLATFORM", 0));
}

rvfi::~rvfi() {
}

void rvfi::init() {

  if (FLAGS_cosim) {
    cvm::log(cvm::MEDIUM, "[RVFI loc {} id{}] Constructing bridge...\n", loc_, id_);
    auto platform_loc = cvm::topology::get_from_type("PLATFORM", 0);
    bridge_ = std::make_unique<bridge>(cvm::topology::attr(platform_loc, "NHARTS").second, xlen, vlen, loc_, id_);
    bridge_->reset();
    count_ = 1;
  } else {
    cvm::log(cvm::MEDIUM, "Running with cosim is disabled\n");
  }
}

void rvfi::process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi) {
  if (terminated_)
    return;

  if (loc_ != m_rvfi.location)
    return;

  // Construct rv_instr_t and send to bridge
  rv_instr_t instr;
  make_instr(m_rvfi, instr);
  print_instr(instr);

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

  // Clear state
  intr_ = false;
  excp_ = false;
}

void rvfi::process(const rv_tester_transactions::cosim::m_trap<>& m_trap) {
  if (terminated_)
    return;

  if (loc_ != m_trap.location)
    return;

  if ((m_trap.cause >> 63) & 0x1) {
    intr_ = true;
    icause_ = (m_trap.cause & 0x3f);
  } else {
    excp_ = true;
    ecause_ = (m_trap.cause & 0xff);
  }
}

void rvfi::process(const rv_tester_transactions::cosim::m_core_intr<>& m_core_intr) {
  if (terminated_)
    return;

  if (loc_ != m_core_intr.location)
    return;

  if (!FLAGS_cosim)
    return;

  rv_intr_t intr;
  intr.cycle = m_core_intr.cycle;
  intr.mip = m_core_intr.mip;
  intr.mip_mask = m_core_intr.mip_mask;
  intr.mip_assert = m_core_intr.mip_assert;

  bridge_->process_dut_interrupt(id_, intr);
  if (FLAGS_rvfi_log) {
    log(cvm::NONE, "#{} {} 0 (mip:{:#x} mask:{:#x} assert:{:#x})\n", count_, intr.cycle, intr.mip, intr.mip_mask, intr.mip_assert);
  }
}

void rvfi::process(const rv_tester_transactions::cosim::m_imsic_msi<>& m_imsic_msi) {
  if (terminated_)
    return;

  if (loc_ != m_imsic_msi.location)
    return;

  if (!FLAGS_cosim)
    return;

  mem_t mem;
  mem.cycle = m_imsic_msi.cycle;
  mem.pa = m_imsic_msi.addr;
  mem.data = m_imsic_msi.data.to_ullong();

  bridge_->process_dut_imsic_msi(id_, mem);
  if (FLAGS_rvfi_log) {
    log(cvm::NONE, "#{} {} {} (imsic: [addr={:#x} data={:#x}])\n", count_, mem.cycle, id_, mem.pa, mem.data);
  }
}

void rvfi::process(const rv_tester_transactions::cosim::m_debug<>&) {

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
  instr.tag = m_rvfi.order;
  instr.opcode = m_rvfi.insn;
  instr.disasm = whisper::disassemble(m_rvfi.insn);
  instr.uop = m_rvfi.uop;
  instr.vec_cracked = m_rvfi.vec_cracked;
  instr.trap = m_rvfi.trap || intr_ || excp_;
  instr.intr = intr_;
  instr.excp = excp_;
  instr.icause = icause_;
  instr.ecause = ecause_;

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
    count_++;
  }

  // Priv mode
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
    priv_ = m_rvfi.mode;
    if (!priv_to_string.count(static_cast<priv>(instr.priv)))
      cvm::log(cvm::ERROR, "Error: Invalid rvfi privilege mode: {:#x}\n", instr.priv);
  }
  if ((instr.priv & 0x3) == 0x3) { // Ignore V bit if M mode
    instr.priv = 0x3;
  }

  // PC
  instr.pc.valid = true;
  instr.pc.pc_rdata = m_rvfi.pc_rdata;

  // GPR
  instr.gpr.valid = (m_rvfi.rd_addr != 0);
  instr.gpr.rd_addr = m_rvfi.rd_addr;
  instr.gpr.rd_wdata = m_rvfi.rd_wdata;
  // Collect vec cracked uop gpr write
  if (instr.gpr.valid && instr.vec_cracked){
    cracked_gpr_.valid = (m_rvfi.rd_addr != 0);
    cracked_gpr_.rd_addr = m_rvfi.rd_addr;
    cracked_gpr_.rd_wdata = m_rvfi.rd_wdata;
  }

  // FPR
  instr.fpr.valid = m_rvfi.frd_valid;
  instr.fpr.frd_addr = m_rvfi.frd_addr;
  instr.fpr.frd_wdata = m_rvfi.frd_wdata;

  // VR
  if (m_rvfi.vrd_valid) {
    vr_t vr {true, m_rvfi.vrd_addr, m_rvfi.vrd_wdata};
    instr.vr.push_back(vr);
    // Accumulate cracked vr writes
    if (m_rvfi.vrd_addr < 32) {
      cracked_vrs_.push_back(vr);
    }
  }

  // CSR
  if (m_rvfi.csr_valid) {
    csr_t c {true, m_rvfi.hart, m_rvfi.cycle, m_rvfi.csr_addr, m_rvfi.csr_wmask, m_rvfi.csr_wdata};
    instr.csr.push_back(c);
    // Accumulate ucode csr writes
    if (!m_rvfi.last_uop) {
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
  if(mem_read_data_.count(instr.tag)>0)
  {
    instr.mem_read.data = mem_read_data_.at(instr.tag); 
    // removing the key value pair from the map
    mem_read_data_.erase(instr.tag);
  }
  else
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
    instr.gpr.valid = cracked_gpr_.valid;
    instr.gpr.rd_addr = cracked_gpr_.rd_addr;
    instr.gpr.rd_wdata = cracked_gpr_.rd_wdata;
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

  // CSR
  if (!ucode_csrs_.empty()) {
    for (auto& c : ucode_csrs_) {
      instr.csr.push_back(c);
    }
    ucode_csrs_.clear();
  }
}

std::tuple<uint64_t, uint64_t, uint8_t> rvfi::get_mem_attributes(uint64_t addr, uint8_t mask, uint64_t data) {
  uint64_t aligned_addr = 0;
  uint64_t aligned_data = 0;
  uint8_t size = 0;

  uint8_t offset = 0;
  if (mask != 0)
    while ((mask & 1) == 0) {
      offset++;
      mask >>= 1;
    }

  aligned_data = data >> (offset * 8);
  aligned_addr = addr | offset;

  if (mask != 0)
    while (mask != 0) {
      size++;
      mask >>= 1;
    }

  aligned_data &= cvm::bitmanip::mask<decltype(aligned_data)>(8*size);

  return std::make_tuple(aligned_addr, aligned_data, size);
}

void rvfi::print_csr(csr_t& csr) {
  log(cvm::NONE, "#NA {} {} {} {:016x} {:09x} c {:016x} {:016x} {:016x} (hw update)\n", csr.cycle, csr.hart, priv_to_string.at(static_cast<priv>(priv_)), 0, 0, csr.csr_addr, csr.csr_wdata, csr.csr_wmask);
}

void rvfi::print_instr(const rv_instr_t& instr) {
  if (!FLAGS_rvfi_log) {
    return;
  }

  int resource_count = instr.gpr.valid + instr.fpr.valid + instr.vr.size() + instr.csr.size() + instr.mem_write.valid;

  // Print r0 = 0 if nothing modified
  if (!resource_count) {
    print_instr_resource(instr, fmt::format(" r {:016x} {:016x}", 0, 0));
    return;
  }

  // Print modified resources in this order - r, f, v, m, c
  if (instr.gpr.valid)
    print_instr_resource(instr, fmt::format(" r {:016x} {:016x}", instr.gpr.rd_addr, instr.gpr.rd_wdata));

  if (instr.fpr.valid)
    print_instr_resource(instr, fmt::format(" f {:016x} {:016x}", instr.fpr.frd_addr, instr.fpr.frd_wdata));

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
  log(cvm::NONE, "#{} {} {} {} {:016x}", FLAGS_mcm ? instr.tag : instr.id, instr.cycle, instr.hart, priv_to_string.at(static_cast<priv>(instr.priv)),
     instr.pc.pc_rdata);

  if (FLAGS_rvfi_log_36b_uop)
    log(cvm::NONE, " {:09x}", instr.uop);
  else
    log(cvm::NONE, " {:08x}", instr.opcode);

  log(cvm::NONE, resource_str);

  if (!instr.ucode)
    log(cvm::NONE, " {}", whisper::disassemble(instr.opcode));
  else
    log(cvm::NONE, " {} (microcode)", cosim_util::get_nth_word(instr.disasm, 1));

  if (instr.mem_write.valid)
    log(cvm::NONE, " [{:#x}:{:#x}:{}]", instr.mem_write.va, instr.mem_write.pa, mem_attr_to_string(instr.mem_write.attr));

  if (instr.mem_read.valid)
    log(cvm::NONE, " [{:#x}:{:#x}:{}]", instr.mem_read.va, instr.mem_read.pa, mem_attr_to_string(instr.mem_read.attr));

  if (instr.intr)
    log(cvm::NONE, " (interrupt:{})", instr.icause);

  if (instr.excp)
    log(cvm::NONE, " (exception:{})", instr.ecause);

  if (instr.comp)
    log(cvm::NONE, " (compressed)");

  log(cvm::NONE, "\n");
}

void rvfi::send_instr(rv_instr_t& instr) {
  if (!FLAGS_cosim)
    return;

  if (terminated_)
    return;

  bridge_->process_dut_instr_retire(instr.hart, instr);
}

void rvfi::send_instr_group(hart_id_t hart, rv_instr_group_t& group) {
  if (!FLAGS_cosim)
    return;

  if (terminated_)
    return;

  bridge_->process_dut_instr_group_retire(hart, group);
}

void rvfi::send_csr(csr_t& csr) {
  if (!FLAGS_cosim)
    return;

  if (terminated_)
    return;

  bridge_->process_dut_csr_hw_update(csr.hart, csr);
}


void rvfi::enter_debug_mode(rv_instr_t& instr) {
  if (!FLAGS_cosim)
    return;

  if (terminated_)
    return;

  if ((uint64_t)instr.pc.pc_rdata == FLAGS_debug_entry_pc) {

    rv_debug_t debug;

    debug.cycle = instr.cycle;
    debug.enter = true;
    debug.exit  = false;
    debug.hart  = instr.hart;

    if (FLAGS_rvfi_log) {
      log(cvm::NONE, "#{} {} 0 (enter debug mode)\n", count_, debug.cycle);
    }

    bridge_->enter_debug_mode(debug);
  }
}

void rvfi::exit_debug_mode(rv_instr_t& instr) {
  if (!FLAGS_cosim)
    return;

  if (terminated_)
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
  }
}

void rvfi::process(const rv_tester_transactions::cosim::m_csri<>& m_csri) {
  if (terminated_)
    return;

  csr_t c {true, m_csri.hart, m_csri.cycle, m_csri.addr, m_csri.mask, m_csri.data};

  // check fscr CSR in CAC in the next step
  if (m_csri.addr == 0x003) temp_fcsr = c;
  else {
    if (temp_fcsr.valid && (c.cycle > temp_fcsr.cycle)) {
      hw_csrs_.push_back(temp_fcsr);
      print_csr(temp_fcsr);
      send_csr(temp_fcsr);
      temp_fcsr.valid = false;
    }
    hw_csrs_.push_back(c);
    print_csr(c);
    send_csr(c);
  }
}

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_read<>& m_mcmi_read) {

  mem_t m;
  m.valid  = true;
  m.hart   = m_mcmi_read.hart;
  m.cycle  = m_mcmi_read.cycle;
  m.tag    = m_mcmi_read.order;
  m.pa     = m_mcmi_read.addr;
  m.size   = std::popcount(m_mcmi_read.mask);
  m.data   = m_mcmi_read.data;
  m.amo    = m_mcmi_read.amo;
  m.amo_op = m_mcmi_read.amo_op;

  mem_read_data_.emplace(m.tag,m.data);  

  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
    return;

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

  bridge_->process_dut_mcm_read(m_mcmi_read.hart, m);

  if (m.amo && m.amo_op != LR && FLAGS_emulate_amo_arithmetic) {
    process_amo(m);
  }
}

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_insert<>& m_mcmi_insert) {
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
    return;

  mem_t m;
  m.valid = true;
  m.cycle = m_mcmi_insert.cycle;
  m.tag   = m_mcmi_insert.order;
  m.pa    = m_mcmi_insert.addr;
  m.size  = std::popcount(m_mcmi_insert.mask);
  m.data  = m_mcmi_insert.data;

  bridge_->process_dut_mcm_insert(m_mcmi_insert.hart, m);
}

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_bypass<>& m_mcmi_bypass) {
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
    return;

  mem_t m;
  m.valid  = true;
  m.hart   = m_mcmi_bypass.hart;
  m.cycle  = m_mcmi_bypass.cycle;
  m.tag    = m_mcmi_bypass.order;
  m.pa     = m_mcmi_bypass.addr;
  m.size   = std::popcount(m_mcmi_bypass.mask);
  m.data   = m_mcmi_bypass.data;
  m.amo    = m_mcmi_bypass.amo;
  m.amo_op = m_mcmi_bypass.amo_op;

  if (m.amo && m.amo_op != SC && FLAGS_emulate_amo_arithmetic) {
    amo_writes_.emplace(m.tag, m);
    return;
  }

  if (m.amo && m.amo_op == SC && sc_failed(m)) {
    sc_bypass_.emplace(m.tag, m);
    return;
  }

  bridge_->process_dut_mcm_bypass(m_mcmi_bypass.hart, m);
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
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
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
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
    return;

  mem_t m;
  m.valid = true;
  m.tag = m_mcmi_ifetch_req.order;
  m.pa = m_mcmi_ifetch_req.addr;

  ifetch_reqs_.emplace(m.tag, m);
}

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_ifetch_resp<>& m_mcmi_ifetch_resp) {
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
    return;

  if (ifetch_reqs_.find(m_mcmi_ifetch_resp.order) == ifetch_reqs_.end()) {
    cvm::log(cvm::ERROR, "Error: Ifetch resp with no matching req - [id={}]", m_mcmi_ifetch_resp.order);
  }

  mem_t m;
  m = ifetch_reqs_.at(m_mcmi_ifetch_resp.order);
  m.cycle = m_mcmi_ifetch_resp.cycle;

  bridge_->process_dut_mcm_ifetch(m_mcmi_ifetch_resp.hart, m);

  ifetch_reqs_.erase(m_mcmi_ifetch_resp.order);
}

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_ievict<>& m_mcmi_ievict) {
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  if (!FLAGS_cosim)
    return;
    
  mem_t m;
  m.valid = true;
  m.cycle = m_mcmi_ievict.cycle;
  m.pa = m_mcmi_ievict.addr;

  bridge_->process_dut_mcm_ievict(m_mcmi_ievict.hart, m);
}

void rvfi::process(const rv_tester::terminate_called&) {
  cvm::log(cvm::HIGH, "[RVFI] termination signaled, stopping further rvfi processing\n");
  terminated_ = true;
}


std::string rvfi::mem_attr_to_string(uint32_t mem_attr) {
    std::string result;
    result += (mem_attr & 0x800) ? "io," : "mem,";
    result += (mem_attr & 0x1000) ? "c"   : "nc";

    return result;
};

extern "C" {
  void cosim_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();
    cvm::registry::messenger.signal<svScope>(loc, scope);
  }
}
