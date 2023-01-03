#include "rvfi.h"
#include "cvm/plusargs.hpp"
#include "cvm/bitmanip.hpp"

#include <iostream>

DEFINE_bool(rvfi, true, "Enable rvfi logging");
DEFINE_bool(cosim, false, "Enable cosim checking");

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

  if (!FLAGS_rvfi)
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
  instr.gpr.valid = (m_rvfi.rd_addr != 0);
  instr.gpr.rd_addr = m_rvfi.rd_addr;
  instr.gpr.rd_wdata = m_rvfi.rd_wdata;

  // Mem reads
  instr.mem_read.valid = (m_rvfi.mem_rmask != 0);
  auto [raddr, rdata, rsize] = get_mem_attributes(m_rvfi.mem_addr, m_rvfi.mem_rmask, m_rvfi.mem_rdata);
  instr.mem_read.addr = raddr;
  instr.mem_read.data = rdata;
  instr.mem_read.size = rsize;

  // Mem writes
  instr.mem_write.valid = (m_rvfi.mem_wmask != 0);
  auto [waddr, wdata, wsize] = get_mem_attributes(m_rvfi.mem_addr, m_rvfi.mem_wmask, m_rvfi.mem_wdata);
  instr.mem_write.addr = waddr;
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
