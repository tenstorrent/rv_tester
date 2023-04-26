#include "rvfi.h"
#include "cvm/plusargs.hpp"
#include "cvm/bitmanip.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/registry.hpp"
#include "sysmod/htif/htif.h"

#include <iostream>

DEFINE_bool(rvfi, true, "Enable rvfi logging");
DEFINE_bool(cosim, true, "Enable cosim checking");
DEFINE_bool(perf, false, "Enable core performance metrics");
DECLARE_string(load);

DEFINE_uint64(debug_entry_pc, 0x800, "Debug Mode entry PC");
DEFINE_uint64(debug_exit_pc, 0x860, "Debug Mode exit PC");

REGISTRY_register(rvfi, PLATFORM, 0);

extern "C" {
  void cosim_terminate();
}

rvfi::rvfi(cvm::topology::loc_t loc, unsigned id)
  : log("dut_rvfi.log"), loc_(loc) {
  init();

  cvm::registry::messenger.connect<scope_t>(
    loc_,
    [&](scope_t s) { return this->set_scope(s.scope); });

  cvm::registry::messenger.connect<htif::terminate_t>(
    loc_,
    [&](htif::terminate_t t) { return this->report_perf(); });

  connect<
    cosim_transactions::m_rvfi,
    cosim_transactions::m_trap,
    cosim_transactions::m_intr,
    cosim_transactions::m_debug
  >(loc);
}

rvfi::~rvfi() {
}

void rvfi::init() {
  bot_ = std::make_unique<bot>();;
  eot_ = std::make_unique<eot>(loc_);;

  if (FLAGS_cosim) {
    cvm::log(cvm::MEDIUM, "[RVFI] Constructing bridge...\n");
    bridge_ = std::make_unique<bridge>(num_harts, xlen, vlen, loc_);
    bridge_->reset();
    log(cvm::NONE, "Instr Cycle Hart Mode PC Opcode\n");
    count_ = 0;
  }

  // initialize metrics
  initialize_perf();
}

void rvfi::process(const cosim_transactions::m_rvfi& m_rvfi) {
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

void rvfi::process(const cosim_transactions::m_trap& m_trap) {
  if ((m_trap.cause >> 63) & 0x1) {
    intr_ = true;
    icause_ = (m_trap.cause & 0x3f);
  } else {
    excp_ = true;
    ecause_ = (m_trap.cause & 0xff);
  }
}

void rvfi::process(const cosim_transactions::m_intr& m_intr) {
   uint64_t cause = (m_intr.timer << 7) | (m_intr.ipi << 3) | (m_intr.external << 11);

   if (!m_intr.pos_edge)
     //bridge_->deassert_interrupt(cause);

  if (!FLAGS_rvfi)
    return;

   if (m_intr.pos_edge) {
     log(cvm::NONE, "#{} {} 0 (assert interrupt:{})\n", m_intr.cycle, count_, cause);
   } else {
     log(cvm::NONE, "#{} {} 0 (deassert interrupt:{})\n", m_intr.cycle, count_, cause);
   }
}

void rvfi::process(const cosim_transactions::m_debug& m_debug) {
  if (!FLAGS_rvfi)
    return;

}

void rvfi::make_instr(const cosim_transactions::m_rvfi& m_rvfi, rv_instr_t& instr) {

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
  if (!FLAGS_rvfi)
    return;

  log(cvm::NONE, "#{} {} {} {} {:016x} {:08x}", instr.id, instr.cycle, instr.hart, instr.priv, instr.pc.pc_rdata,
      instr.opcode);

  if (instr.gpr.valid)
    log(cvm::NONE, " r {:016x} {:016x}", instr.gpr.rd_addr, instr.gpr.rd_wdata);

  if (instr.mem_write.valid)
    log(cvm::NONE, " m {:016x} {:016x}", instr.mem_write.va, instr.mem_write.data);

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

    log(cvm::NONE, "#{} {} 0 (enter debug mode)\n", count_, debug.cycle);

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

    log(cvm::NONE, "#{} {} 0 (exit debug mode)\n", count_, debug.cycle);

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

void rvfi::collect_perf(const cosim_transactions::m_rvfi& m_rvfi) {
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
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"perf_cycles\": \"{}\"}}\n", perf_end_cycle - perf_start_cycle);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"perf_instrs\": \"{}\"}}\n", perf_instrs);
  }
}

extern "C" {
    void cosim_set_scope(cvm::topology::loc_t loc) {
      svScope scope = svGetScope();
      cvm::registry::messenger.signal<rvfi::scope_t>(loc, {scope});
    }
}
