#include "rvfi.h"
#include "cvm/plusargs.hpp"

#include <iostream>

DEFINE_bool(cosim, false, "Enable cosim checking");
DEFINE_bool(rvfi_tracer, false, "Enable rvfi trace prints");

rvfi::rvfi() {
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
  std::cout << "[RVFI] Constructing bridge...\n";
  bridge_ = std::make_unique<bridge>(num_harts, xlen, vlen);
}

void rvfi::reset() {
  if (FLAGS_cosim) {
    std::cout << "[RVFI] Hard reset for test chaining...\n";
    bridge_->reset();
  }
}

void rvfi::process(const transactions::m_rvfi& m_rvfi) {
  // Construct rv_instr_t and send to bridge
  rv_instr_t instr;
  make_instr(m_rvfi, instr);
  print_instr(instr);
  send_instr(instr);

  // Clear state
  intr = false;
  excp = false;
}

void rvfi::process(const transactions::m_trap& m_trap) {
  if ((m_trap.cause >> 63) & 0x1) {
    intr = true;
    icause = (m_trap.cause & 0x3f);
  } else {
    excp = true;
    ecause = (m_trap.cause & 0xff);
  }
}

void rvfi::process(const transactions::m_intr& m_intr) {
   
   uint64_t cause = (m_intr.timer << 7) | (m_intr.ipi << 3) | (m_intr.external << 11);

   if (m_intr.pos_edge) {
     std::cout << "<" << std::dec << m_intr.cycle << "> Asserting interrupt. cause: [0x" << cause << "\n";
   } else {
     std::cout << "<" << std::dec << m_intr.cycle << "> Deasserting interrupt. cause: [0x" << cause << "\n";
     //bridge_->deassert_interrupt(cause);
   }
}

void rvfi::make_instr(const transactions::m_rvfi& m_rvfi, rv_instr_t& instr) {

  // Metadata
  instr.valid = true;
  instr.cycle = m_rvfi.cycle;
  instr.tag = m_rvfi.order;
  instr.opcode = m_rvfi.insn;
  instr.priv = m_rvfi.mode;
  instr.trap = m_rvfi.trap || intr || excp;
  instr.intr = intr; 
  instr.excp = excp; 
  instr.icause = icause;
  instr.ecause = ecause;

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

  std::cout << "<" << std::dec << instr.cycle << "> DUT Step #" << instr.tag << ": ["
            << "PC=0x" << std::hex << instr.pc.pc_rdata 
            << ", Opcode=0x" << std::hex << instr.opcode
            << ", Mode=" << instr.priv
            << ", Intr=" << instr.intr << ";" << instr.icause
            << ", Excp=" << instr.excp << ";" << instr.ecause
            << "]\n";
}

void rvfi::send_instr(rv_instr_t& instr) {
  if (!FLAGS_cosim)
    return;

  bridge_->process_dut_instr_retire(0, instr);
}
