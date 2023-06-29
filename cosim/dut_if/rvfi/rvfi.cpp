#include "rvfi.h"
#include "cvm/plusargs.hpp"
#include "cvm/bitmanip.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/registry.hpp"
#include "sysmod/htif/htif.h"

#include <iostream>
#include <chrono>

DEFINE_bool(rvfi, true, "Enable rvfi");
// TODO(mboisvert): See if we can combine the rvfi flags. The reason why the
// rvfi_log flag was created is that +norvfi causes the max # of cycles to be
// exceeded.
DEFINE_bool(rvfi_log, true, "Enable rvfi logging");
DEFINE_bool(cosim, true, "Enable cosim checking");
DEFINE_bool(perf, false, "Enable core performance metrics");
DECLARE_string(load);

DEFINE_uint64(debug_entry_pc, 0x800, "Debug Mode entry PC");
DEFINE_uint64(debug_exit_pc, 0x860, "Debug Mode exit PC");

REGISTRY_register(rvfi, TOP.PLATFORM.COSIM, 0);

extern "C" {
  void cosim_terminate();
}

rvfi::rvfi(cvm::topology::loc_t loc, unsigned)
  : log("dut_rvfi.log"), loc_(loc) {
  init();

  cvm::registry::messenger.connect<svScope>(
    loc_,
    [&](svScope s) { return this->set_scope(s); });

  connect<
    rv_tester_transactions::cosim::m_rvfi,
    rv_tester_transactions::cosim::m_trap,
    rv_tester_transactions::cosim::m_intr,
    rv_tester_transactions::cosim::m_debug
  >(loc);
}

rvfi::~rvfi() {
  report_perf();
}

void rvfi::init() {
  eot_ = std::make_unique<eot>(loc_);;

  if (FLAGS_cosim) {
    cvm::log(cvm::MEDIUM, "[RVFI] Constructing bridge...\n");
    bridge_ = std::make_unique<bridge>(num_harts, xlen, vlen, loc_);
    bridge_->reset();
    if (FLAGS_rvfi_log) {
      log(cvm::NONE, "Instr Cycle Hart Mode PC Opcode\n");
    }
    count_ = 0;
  }

  // initialize metrics
  initialize_perf();
}

void rvfi::process(const rv_tester_transactions::cosim::m_rvfi& m_rvfi) {
  // Construct rv_instr_t and send to bridge
  rv_instr_t instr;
  make_instr(m_rvfi, instr);
  print_instr(instr);
  enter_debug_mode(instr);
  send_instr(instr);
  exit_debug_mode(instr);
  // Clear state
  intr_ = false;
  excp_ = false;

  if (FLAGS_perf)
    collect_perf(m_rvfi);
}

void rvfi::process(const rv_tester_transactions::cosim::m_trap& m_trap) {
  if ((m_trap.cause >> 63) & 0x1) {
    intr_ = true;
    icause_ = (m_trap.cause & 0x3f);
  } else {
    excp_ = true;
    ecause_ = (m_trap.cause & 0xff);
  }
}

void rvfi::process(const rv_tester_transactions::cosim::m_intr& m_intr) {
  if (!FLAGS_cosim)
    return;

  rv_intr_t intr;
  intr.cycle = m_intr.cycle;
  intr.mip_posedge = m_intr.mip_posedge;
  intr.mip = m_intr.mip;
  intr.seip_posedge = m_intr.seip_posedge;
  intr.seip_negedge = m_intr.seip_negedge;
  intr.seip = m_intr.seip;

  bridge_->process_dut_interrupt(0, intr);
  if (FLAGS_rvfi_log) {
    log(cvm::NONE, "#{} {} 0 (mip:{:#x} seip:{})\n", count_, intr.cycle, intr.mip, intr.seip);
  }
}

void rvfi::process(const rv_tester_transactions::cosim::m_debug&) {

}

void rvfi::make_instr(const rv_tester_transactions::cosim::m_rvfi& m_rvfi, rv_instr_t& instr) {

  static bool started = true;
  if (started) {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << "start time: " << std::ctime(&now) << std::endl;
    started = false;
  }


  // Metadata
  instr.valid = true;
  instr.hart = 0;
  instr.cycle = m_rvfi.cycle;
  instr.id = ++count_;
  instr.tag = m_rvfi.order;
  instr.opcode = m_rvfi.insn;
  instr.priv = m_rvfi.mode;
  instr.trap = m_rvfi.trap || intr_ || excp_;
  instr.intr = intr_;
  instr.excp = excp_;
  instr.icause = icause_;
  instr.ecause = ecause_;

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

  // tlb
  instr.mem_va = m_rvfi.mem_addr;
  instr.mem_pa = m_rvfi.mem_paddr;

  // Mem reads
  instr.mem_read.valid = (m_rvfi.mem_rmask != 0);
  auto [raddr, rdata, rsize] = get_mem_attributes(m_rvfi.mem_addr, m_rvfi.mem_rmask, m_rvfi.mem_rdata);
  instr.mem_read.va = raddr;
  instr.mem_read.pa = raddr;
  instr.mem_read.data = rdata;
  instr.mem_read.size = rsize;

  // Mem writes
  instr.mem_write.valid = (m_rvfi.mem_wmask != 0);
  auto [waddr, wdata, wsize] = get_mem_attributes(m_rvfi.mem_addr, m_rvfi.mem_wmask, m_rvfi.mem_wdata);
  instr.mem_write.va = waddr;
  instr.mem_write.pa = waddr;
  instr.mem_write.data = wdata;
  instr.mem_write.size = wsize;
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
  log(cvm::NONE, "#{} {} {} {} {:016x} {:08x}", instr.id, instr.cycle, instr.hart, instr.priv, instr.pc.pc_rdata,
      instr.opcode);

  if (instr.gpr.valid)
    log(cvm::NONE, " r {:016x} {:016x}", instr.gpr.rd_addr, instr.gpr.rd_wdata);

  if (instr.fpr.valid)
    log(cvm::NONE, " f {:016x} {:016x}", instr.fpr.frd_addr, instr.fpr.frd_wdata);

  if (instr.mem_write.valid) {
    log(cvm::NONE, " m {:016x} {:016x}", instr.mem_write.va, instr.mem_write.data);
    log(cvm::NONE, " [{:#x}:{:#x}]", instr.mem_write.va, instr.mem_write.pa);
  }

  if (instr.mem_read.valid)
    log(cvm::NONE, " [{:#x}:{:#x}]", instr.mem_read.va, instr.mem_read.pa);

  if (instr.intr)
    log(cvm::NONE, " (interrupt:{})", instr.icause);

  if (instr.excp)
    log(cvm::NONE, " (exception:{})", instr.ecause);

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

void rvfi::initialize_perf() {
  if (FLAGS_perf and not FLAGS_load.empty()) {
    // initialize metrics
    char buffer_start[128]; char buffer_end[128];
    std::string perf_start, perf_end;
    FILE* pipe_start = popen(("nm " + FLAGS_load + " | grep __perf_start").c_str(), "r");
    FILE* pipe_end = popen(("nm " + FLAGS_load + " | grep __perf_end").c_str(), "r");
    try {
      while (fgets(buffer_start, sizeof(buffer_start), pipe_start) != NULL)
        perf_start += buffer_start;

      while (fgets(buffer_end, sizeof(buffer_end), pipe_end) != NULL)
        perf_end += buffer_end;

      int pos = perf_start.find(" ");
      perf_start_pc = std::strtoll(perf_start.substr(0, pos).c_str(), nullptr, 16);
      pos = perf_end.find(" ");
      perf_end_pc = std::strtoll(perf_end.substr(0, pos).c_str(), nullptr, 16);

    } catch (...) {
      pclose(pipe_start);
      pclose(pipe_end);
      return;
    }

    pclose(pipe_start);
    pclose(pipe_end);
    perf_ok = true;
  }
}

void rvfi::collect_perf(const rv_tester_transactions::cosim::m_rvfi& m_rvfi) {
  if (perf_ok) {
    if (perf_start_pc == uint64_t(m_rvfi.pc_rdata))
      perf_start_cycle = m_rvfi.cycle;
    if (perf_end_pc == uint64_t(m_rvfi.pc_rdata))
      perf_end_cycle = m_rvfi.cycle;

    if (perf_start_cycle)
      perf_instrs++;
  }
}

void rvfi::report_perf() {
  if (perf_ok) {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"perf_start_pc\": \"{}\"}}\n", perf_start_pc);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"perf_end_pc\": \"{}\"}}\n", perf_end_pc);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"perf_start_cycle\": \"{}\"}}\n", perf_start_cycle);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"perf_end_cycle\": \"{}\"}}\n", perf_end_cycle);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"perf_cycles\": \"{}\"}}\n", perf_end_cycle - perf_start_cycle);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"perf_instrs\": \"{}\"}}\n", perf_instrs);
  }
}

extern "C" {
    void cosim_set_scope(cvm::topology::loc_t loc) {
      svScope scope = svGetScope();
      cvm::registry::messenger.signal<svScope>(loc, scope);
    }
}
