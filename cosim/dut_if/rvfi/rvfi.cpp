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
DEFINE_bool(mcm, false, "Enable mcm");
DEFINE_bool(cosim, true, "Enable cosim checking");
DECLARE_string(load);
DECLARE_bool(csr_check);

DEFINE_uint64(debug_entry_pc, 0x800, "Debug Mode entry PC");
DEFINE_uint64(debug_exit_pc, 0x860, "Debug Mode exit PC");

REGISTRY_register(rvfi, COSIM, cvm::registry::all);

extern "C" {
  void cosim_terminate();
}

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
    rv_tester_transactions::cosim::m_intr<>,
    rv_tester_transactions::cosim::m_mcmi_read<>,
    rv_tester_transactions::cosim::m_mcmi_insert<>,
    rv_tester_transactions::cosim::m_mcmi_write<>,
    rv_tester_transactions::cosim::m_debug<>
  >(loc);

  connect<
    htif::terminate_t
  >(cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0));
}

rvfi::~rvfi() {
}

void rvfi::init() {
  eot_ = std::make_unique<eot>(loc_);;

  if (FLAGS_cosim) {
    cvm::log(cvm::MEDIUM, "[RVFI loc {} id{}] Constructing bridge...\n", loc_, id_);
    auto platform_loc = cvm::topology::get_from_type("PLATFORM", 0);
    bridge_ = std::make_unique<bridge>(cvm::topology::attr(platform_loc, "NHARTS").second, xlen, vlen, loc_, id_);
    bridge_->reset();
    count_ = 1;
    prev_instr_.clear();
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
  prev_instr_ = instr;

  if (!m_rvfi.last_uop)
    return;

  enter_debug_mode(instr);
  send_instr(instr);
  exit_debug_mode(instr);
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

void rvfi::process(const rv_tester_transactions::cosim::m_intr<>& m_intr) {
  if (terminated_)
    return;

  if (loc_ != m_intr.location)
    return;

  rv_intr_t intr;
  intr.cycle = m_intr.cycle;
  intr.mip_posedge = m_intr.mip_posedge;
  intr.mip = m_intr.mip;
  intr.seip_posedge = m_intr.seip_posedge;
  intr.seip_negedge = m_intr.seip_negedge;
  intr.seip = m_intr.seip;
  intr.stip_negedge = m_intr.stip_negedge;

  bridge_->process_dut_interrupt(id_, intr);
  if (FLAGS_rvfi_log) {
    log(cvm::NONE, "#{} {} 0 (mip:{:#x} seip:{})\n", count_, intr.cycle, intr.mip, intr.seip);
  }
}

void rvfi::process(const rv_tester_transactions::cosim::m_debug<>&) {

}

void rvfi::make_instr(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi, rv_instr_t& instr) {

  static bool started = true;
  if (started) {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << "start time: " << std::ctime(&now) << std::endl;
    started = false;
  }


  // Metadata
  instr.valid = true;
  instr.hart = m_rvfi.hart;
  instr.cycle = m_rvfi.cycle;
  instr.id = count_;
  instr.last_uop = m_rvfi.last_uop;
  instr.comp = m_rvfi.comp;
  instr.tag = m_rvfi.order;
  instr.opcode = m_rvfi.insn;
  instr.uop = m_rvfi.uop;
  instr.priv = m_rvfi.mode;
  instr.trap = m_rvfi.trap || intr_ || excp_;
  instr.intr = intr_;
  instr.excp = excp_;
  instr.icause = icause_;
  instr.ecause = ecause_;
  instr.ucode = ucode_;
  if (!m_rvfi.last_uop) {
    ucode_ = true;
  } else {
    ucode_ = false;
    count_++;
  }

  // PC
  instr.pc.valid = true;
  instr.pc.pc_rdata = m_rvfi.pc_rdata;

  // GPR
  instr.gpr.valid = (m_rvfi.rd_addr != 0);
  instr.gpr.rd_addr = m_rvfi.rd_addr;
  instr.gpr.rd_wdata = m_rvfi.rd_wdata;

  // FPR
  instr.fpr.valid = m_rvfi.frd_valid;
  instr.fpr.frd_addr = m_rvfi.frd_addr;
  instr.fpr.frd_wdata = m_rvfi.frd_wdata;

  // VR
  instr.vr.valid = m_rvfi.vrd_valid;
  instr.vr.vrd_addr = m_rvfi.vrd_addr;
  instr.vr.vrd_wdata = m_rvfi.vrd_wdata;

  // CSR
  for (auto & csr : csrs_) {
    instr.csr.push_back(csr);
  }
  csrs_.clear();

  // tlb
  instr.mem_va = m_rvfi.mem_addr;
  instr.mem_pa = m_rvfi.mem_paddr;

  // Mem reads
  instr.mem_read.valid = (m_rvfi.mem_rmask != 0);
  instr.mem_read.va = m_rvfi.mem_addr;
  instr.mem_read.pa = m_rvfi.mem_paddr;
  instr.mem_read.data = m_rvfi.mem_rdata;
  instr.mem_read.size = log2(m_rvfi.mem_rmask + 1);

  // Mem writes
  instr.mem_write.valid = (m_rvfi.mem_wmask != 0);
  instr.mem_write.va = m_rvfi.mem_addr;
  instr.mem_write.pa = m_rvfi.mem_paddr;
  instr.mem_write.data = m_rvfi.mem_wdata;
  instr.mem_write.size = log2(m_rvfi.mem_wmask + 1);
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

void rvfi::print_instr(rv_instr_t& instr) {
  if (!FLAGS_rvfi_log) {
    return;
  }

  int resource_count = instr.gpr.valid + instr.fpr.valid + instr.vr.valid + instr.mem_write.valid + instr.csr.size();

  // Print r0 = 0 if nothing modified
  if (!resource_count || !instr.last_uop)
    print_instr_resource(instr, fmt::format(" r {:016x} {:016x}", 0, 0));
  else {
    // Print modified resources in this order - r, f, m, c
    if (instr.gpr.valid)
      print_instr_resource(instr, fmt::format(" r {:016x} {:016x}", instr.gpr.rd_addr, instr.gpr.rd_wdata));

    if (instr.fpr.valid)
      print_instr_resource(instr, fmt::format(" f {:016x} {:016x}", instr.fpr.frd_addr, instr.fpr.frd_wdata));

    if (instr.vr.valid){
      uint64_t chunks[4] = {0};
      for (int i = 0; i < 4; ++i) {
          for (int j = 0; j < 64; ++j) {
              chunks[i] |= static_cast<uint64_t>(instr.vr.vrd_wdata[i * 64 + j]) << j;
          }
      }
      print_instr_resource(instr, fmt::format(" v {:002x} {:016x}{:016x}{:016x}{:016x}", instr.vr.vrd_addr, chunks[3], chunks[2], chunks[1], chunks[0]));
    }

    if (instr.mem_write.valid)
      print_instr_resource(instr, fmt::format(" m {:016x} {:016x}", instr.mem_write.va, instr.mem_write.data));

    for (auto& c : instr.csr)
      print_instr_resource(instr, fmt::format(" c {:016x} {:016x}", c.csr_addr, c.csr_wdata));
  }
}

void rvfi::print_instr_resource(rv_instr_t& instr, std::string resource_str) {
  log(cvm::NONE, "#{} {} {} {} {:016x} {:09x}", instr.id, instr.cycle, instr.hart, instr.priv,
     instr.pc.pc_rdata, instr.uop);

  log(cvm::NONE, resource_str);

  if (instr.last_uop && prev_instr_.last_uop)
    log(cvm::NONE, " {}", whisper::disassemble(instr.opcode));
  else
    log(cvm::NONE, " {} (microcode)", cosim_util::get_nth_word(whisper::disassemble(instr.opcode), 1));

  if (instr.mem_write.valid)
    log(cvm::NONE, " [{:#x}:{:#x}]", instr.mem_write.va, instr.mem_write.pa);

  if (instr.mem_read.valid)
    log(cvm::NONE, " [{:#x}:{:#x}]", instr.mem_read.va, instr.mem_read.pa);

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

  bridge_->process_dut_instr_retire(instr.hart, instr);
}

void rvfi::enter_debug_mode(rv_instr_t& instr) {
  if (!FLAGS_cosim)
    return;

  if ((uint64_t)instr.pc.pc_rdata == FLAGS_debug_entry_pc) {

    rv_debug_t debug;
    std::cout <<" Enter Debug Mode debug_entry_pc :"<<std::hex<<FLAGS_debug_entry_pc<<"\n";

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

  if ((uint64_t)instr.pc.pc_rdata == FLAGS_debug_exit_pc) {

    rv_debug_t debug;
    std::cout <<" Exit Debug Mode debug_exit_pc :"<<std::hex<<FLAGS_debug_exit_pc<<"\n";

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
  if (!FLAGS_csr_check)
    return;

  csr_t c {true, m_csri.cycle, m_csri.addr, m_csri.data, m_csri.mask};
  csrs_.push_back(c);
}

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_read<>& m_mcmi_read) {
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  mem_t m;
  m.valid = true;
  m.cycle = m_mcmi_read.cycle;
  m.tag = m_mcmi_read.order;
  m.pa = m_mcmi_read.addr;
  m.size = std::popcount(m_mcmi_read.mask);
  m.data = m_mcmi_read.data;

  bridge_->process_dut_mcm_read(0, m);
}

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_insert<>& m_mcmi_insert) {
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  mem_t m;
  m.valid = true;
  m.cycle = m_mcmi_insert.cycle;
  m.tag = m_mcmi_insert.order;
  m.pa = m_mcmi_insert.addr;
  m.size = std::popcount(m_mcmi_insert.mask);
  m.data = m_mcmi_insert.data;

  bridge_->process_dut_mcm_insert(0, m);
}

void rvfi::process(const rv_tester_transactions::cosim::m_mcmi_write<>& m_mcmi_write) {
  if (!FLAGS_mcm)
    return;

  if (terminated_)
    return;

  mem_cl_t m;
  m.valid = true;
  m.cycle = m_mcmi_write.cycle;
  m.pa = m_mcmi_write.addr;
  m.mask = m_mcmi_write.mask;
  m.data = m_mcmi_write.data;

  bridge_->process_dut_mcm_write(0, m);
}

void rvfi::process(const htif::terminate_t&) {
  cvm::log(cvm::MEDIUM, "[RVFI] termination signaled, stopping further rvfi processing\n");
  terminated_ = true;
}

extern "C" {
    void cosim_set_scope(cvm::topology::loc_t loc) {
      svScope scope = svGetScope();
      cvm::registry::messenger.signal<svScope>(loc, scope);
    }
}
