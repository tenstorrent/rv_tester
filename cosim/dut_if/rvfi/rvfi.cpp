#include "rvfi.h"
#include "cvm/plusargs.hpp"

#include <iostream>

DEFINE_bool(cosim, false, "Enable cosim checking");
DEFINE_bool(rvfi_tracer, false, "Enable rvfi trace prints");

rvfi::rvfi() : log("dut_rvfi.log") {
  if (FLAGS_cosim) {
    init();
  }

  connect<
    transactions::m_rvfi,
    transactions::m_trap,
    transactions::m_intr
  >();
}

void rvfi::init() {
  cvm::log(cvm::MEDIUM, "[RVFI] Constructing bridge...\n");
  bridge_ = std::make_unique<bridge>(num_harts, xlen, vlen);
}

void rvfi::reset() {
  if (!FLAGS_cosim)
    return;

  cvm::log(cvm::MEDIUM, "[RVFI] Hard reset for test chaining...\n");
  bridge_->reset();

  log(cvm::NONE, "Instr Cycle Hart Mode PC Opcode\n");

  count_ = 0;
}

void rvfi::process(const transactions::m_rvfi& m_rvfi) {
  // Construct rv_instr_t and send to bridge
  rv_instr_t instr;
  make_instr(m_rvfi, instr);
  print_instr(instr);
  send_instr(instr);

  // Clear state
  intr_ = false;
  excp_ = false;
}

void rvfi::process(const transactions::m_trap& m_trap) {
  if ((m_trap.cause >> 63) & 0x1) {
    intr_ = true;
    icause_ = (m_trap.cause & 0x3f);
  } else {
    excp_ = true;
    ecause_ = (m_trap.cause & 0xff);
  }
}

void rvfi::process(const transactions::m_intr& m_intr) {
   uint64_t cause = (m_intr.timer << 7) | (m_intr.ipi << 3) | (m_intr.external << 11);

   if (!m_intr.pos_edge)
     //bridge_->deassert_interrupt(cause);

  if (!FLAGS_rvfi_tracer)
    return;
   
   if (m_intr.pos_edge) {
     log(cvm::NONE, "#{} {} 0 (assert interrupt:{})", m_intr.cycle, count_, cause);
   } else {
     log(cvm::NONE, "#{} {} 0 (deassert interrupt:{})", m_intr.cycle, count_, cause);
   }
}

void rvfi::make_instr(const transactions::m_rvfi& m_rvfi, rv_instr_t& instr) {

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
  instr.gpr.valid = true;
  instr.gpr.rd_addr = m_rvfi.rd_addr;
  instr.gpr.rd_wdata = m_rvfi.rd_wdata;

  // Mem reads
  mem_t m;
  m.addr = m_rvfi.mem_addr;
  //m.size = m_rvfi.mem_rmask;
  m.data = m_rvfi.mem_rdata;
  instr.mem_read.push_back(m);
}

void rvfi::print_instr(rv_instr_t& instr) {
  if (!FLAGS_rvfi_tracer)
    return;

  log(cvm::NONE, "#{} {} {} {} {:016x} {:08x}", instr.id, instr.cycle, instr.hart, instr.priv, instr.pc.pc_rdata, 
      instr.opcode); 

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
