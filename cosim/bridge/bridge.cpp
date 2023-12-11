// Licensed under the Apache License, Version 2.0, see LICENSE.TT for details

#include "bridge.h"
#include "util.h"
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/topology.hpp"
#include "src/cac_lib.h"
#include "sysmod/htif/htif.h"
#include "whisper_client_decl.h"
#include "whisper_decoder.h"
#include "rv_tester/rv_tester_plusargs.h"

#include <cstring>          // strlen
#include <sstream>          // stringstream
#include <thread>           // std::this_thread::sleep_for
#include <chrono>           // std::chrono::seconds
#include <cstdlib>          // system
#include <vector>
#include <fmt/format.h>

// Plusargs
DECLARE_string(bootrom_path);
DECLARE_string(load);
DECLARE_string(hex);
DECLARE_uint64(debug_entry_pc);
DECLARE_uint64(debug_exit_pc);
DECLARE_uint64(hart_enable_mask);

DEFINE_bool(bridge_log, true, "Enable bridge logging");
DEFINE_string(whisper_json_path, "", "Path to whisper json config");
DEFINE_bool(cosim_resynch, false, "Resynch whisper with dut state on every instruction");
DEFINE_string(cosim_resynch_instr, "", "List of instruction mnemonics to resynch whisper with dut state");
DEFINE_string(cosim_resynch_prev_instr, "", "List of instruction mnemonics to resynch whisper with dut state");
DEFINE_bool(lrsc_resynch, true, "Resynch whisper with dut state on LRSC fail condition");
DEFINE_bool(retire_ucode_trap, true, "DUT indicates retire on a trap after executing the ucode trap handler");
DEFINE_bool(gpr_check, true, "Enable cosim checks on gprs");
DEFINE_bool(fpr_check, true, "Enable cosim checks on fprs");
DEFINE_bool(vec_check, true, "Enable cosim checks on vector regs");
DEFINE_bool(csr_check, false, "Enable cosim checks on csrs");
DEFINE_uint64(max_cycle, 1000000, "Max cycle limit to terminate the sim");
DEFINE_int32(debug_excp_mcause, 24, "MCAUSE value for debug exception");
DEFINE_bool(translation_check, false, "Do VA-PA translation check");
DEFINE_bool(emulate_debug_mode, true, "Emulate debug mode by forcing whisper to be in sync with DUT");
DEFINE_bool(delay_satp_update, false, "Delay satp update till next sfence.vma");
DEFINE_bool(cov, false, "Enable Arch coverage");
DEFINE_string(archsample_lib_path, "", "Path to libarchsample.so");
DEFINE_bool(standalone, true, "Enable whisper standalone run at beginning of sim");
DEFINE_bool(metrics, true, "Enable printing metrics in log file");
DEFINE_uint32(max_pend_intr_age, 128, "Number of instructions allowed to retire before a pending interrupt should be taken");
DEFINE_bool(whisper_log, true, "Enable whisper logging to iss_cosim.log and iss_cmd.log");
DEFINE_bool(whisper_stdin_null, false, "Redirect whisoer stdin to null");
DEFINE_bool(whisper_stdout_null, false, "Redirect whisoer stdout to null");

std::shared_ptr<whisperClient<uint64_t>> client_;
//std::unique_ptr<whisperClient<uint64_t>> client_;
// Constructor
bridge::bridge(int num_harts, int xlen, int vlen, cvm::topology::loc_t loc, unsigned id)
  : log("h" + std::to_string(id) + "_bridge.log"),
    loc_(loc),
    id_(id),
    num_harts_(num_harts),
    xlen_(xlen),
    vlen_(vlen),
    cac_(CacCore(num_harts)),
    csr_cac_(CacCore(num_harts))
{
    std::string traceFile = FLAGS_whisper_log ? "iss_cosim.log" : "";
    std::string commandLog = FLAGS_whisper_log ? "iss_cmd.log" : "";
    client_ = std::make_shared<whisperClient<uint64_t>>(traceFile, commandLog);
}

// Destructor
bridge::~bridge() {
  report_metrics();
  client_->whisperQuit();
}

bool bridge::whisper_connect() {
  return (client_->whisperConnect(num_harts_) == 0);
}

void bridge::reset() {

  memmap::get(memmap_);

  cac_.Reset();
  assert(cac_.SetVlen(vlen_));

  whisper_connect();

  bool valid;
  client_->whisperReset(0, valid);

  // Init csr reset values in cac
  csr_init();

  // Write hart enable mask to boot mem
  if (!client_->whisperPoke(id_, 0, 'm', memmap_.at("boot").base + 0x9000, FLAGS_hart_enable_mask, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke boot memory\n", id_);
    return;
  }
}

void bridge::csr_init() {
  bool valid;
  uint64_t data, mask, poke_mask;
  for (const auto& csr: csrs) {
    if (!client_->whisperPeekCsr(id_, csr.address, data, mask, poke_mask, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek csr\n", id_);
    }
    size_8_bytes_t cac_mask = 0xffffffffffffffff;
    update_csr(id_, src_t::dut, csr.address, data, cac_mask);
    update_csr(id_, src_t::iss, csr.address, data, cac_mask);
    csr_cac_.Step(id_);
  }
}

// DUT interface callback: Instruction Retire
void bridge::process_dut_instr_retire(hart_id_t hart, rv_instr_t& d) {

  whisper_state_t w {
    .tag = d.tag,
    .time = d.cycle
  };

  // Handle pre-step condition - Debug
  if (debug_mode_) {
    if (FLAGS_emulate_debug_mode) {
      process_debug_pre_step(hart, d, w);
    } else {
      return;
    }
  }

  // Handle pre-step condition - Interrupts
  process_interrupt_pre_step(hart, d, w);

  // Step whisper
  w_.clear();
  step(hart, w);

  // Update cac with whisper state
  update_whisper_state(hart, w);

  // Update cac with dut state
  update_dut_state(hart, d);

  // Handle post-step conditions
  process_interrupt_post_step(hart, d, w);
  //if(!debug_mode_){
  process_exception_post_step(hart, d, w);
  //}
  process_satp_write_post_step(hart, d, w);

  // Check dut vs whisper
  if(!excp_in_debug_mode){
    cac_.Step(hart);
  }else{
    cac_.ResetStatus(hart);
    return;
  }

  // Increment step count
  step_++;

  // Error on mismatch
  if (!cac_.GetStatus(hart)) {
    cac_.ResetStatus(hart);
    if (FLAGS_cosim_resynch) {
      if (FLAGS_bridge_log) {
        print_instr(hart, w);
        log(cvm::MEDIUM, "{}", cac_.GetStatusStr(hart));
      }
      resynch(hart, d);
    } else {
      std::string instr = cosim_util::get_nth_word(w.disasm, 1);
      std::string resource = cac_.GetResourceStr(hart) == "PC" ? " :PC" : "";
      if (instr.substr(0,3) == "csr")
        instr = "csr:" + cosim_util::get_nth_word(w.disasm, 3);
      // Resynch whisper with dut state if needed
      // to continue without failing
      if (does_instr_match_resynch_list(d, instr) ||
          does_instr_match_resynch_condition(d, instr)) {
        resynch(hart, d);
        cac_.ResetStatus(hart);
      } else {
        print_instr_stdout(hart, w);
        cvm::log(cvm::NONE, "{}", cac_.GetStatusStr(hart));
        cvm::log(cvm::ERROR, "Error: Hart {}: Core Arch Checker Mismatch{} - {}\n", hart, resource,  instr);
        return;
      }
    }
  }
  else {
      log(cvm::HIGH, "{}", cac_.GetStatusStr(hart));
  }

  // Save whisper state
  ppw_ = pw_;
  pw_ = w;
  prev_mip_ = mip_;

  // TLB checks
  translation_check(hart, d, w);
}

void bridge::process_dut_csr_hw_update(hart_id_t hart, csr_t& c) {
  // MIP updates handled in process_dut_interrupt
  if (c.csr_addr == 0x344)
    return;

  size_8_bytes_t mask = c.csr_wmask & static_cast<size_8_bytes_t>(get_csr_poke_mask(hart, c.csr_addr));
  update_csr(hart, src_t::dut, c.csr_addr, c.csr_wdata, mask);
}

void bridge::process_dut_instr_group_retire(hart_id_t hart, rv_instr_group_t& d) {
  if (!FLAGS_csr_check)
    return;

  // Step csr cac
  csr_cac_.Step(hart);

  if (resynch_csr_) {
    csr_cac_.ResetStatus(hart);
    resynch_csr_ = false;
  }

  // Error on mismatch
  if (!csr_cac_.GetStatus(hart)) {
    std::string csr = get_csr_name(csr_cac_.GetResourceStr(hart).substr(2));
    csr_cac_.ResetStatus(hart);
    if (csr == "mip") {
      // do nothing
    } else if (FLAGS_cosim_resynch) {
      resynch(hart, d);
    } else {
      for (auto & i : d.instrs)
        print_instr_stdout(hart, i);
      cvm::log(cvm::NONE, "{}", csr_cac_.GetStatusStr(hart));
      cvm::log(cvm::ERROR, "Error: Hart {}: CSR Mismatch - {}\n", hart, csr);
      return;
    }
  }
  else {
      log(cvm::HIGH, "{}", csr_cac_.GetStatusStr(hart));
  }

}

void bridge::update_dut_state(hart_id_t hart, rv_instr_t& d) {
  update_pc(hart, src_t::dut, d.pc.pc_rdata);
  if (!d.comp && !d.ucode) {
    update_insn(hart, src_t::dut, d.opcode);
  }
  if (d.gpr.valid || d.fpr.valid || d.vr.valid || !d.csr.empty()) {
    update_regs(hart, d);
  }
  if (d.mem_write.valid) {
    update_mem(hart, d);
  }
}

void bridge::process_debug_pre_step(hart_id_t hart, const rv_instr_t& instr, whisper_state_t& ) {
    bool valid;
    if (!client_->whisperPoke(hart, 0, 'm', instr.pc.pc_rdata, instr.opcode, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke memory\n", hart);
      return;
    }
  return;
}

void bridge::process_interrupt_pre_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {

  if (!mip_)
    return;

  bool w_intr;
  uint64_t w_cause;
  whisper_check_interrupt(hart, mip_, w_intr, w_cause);

  if (!d.intr && !w_intr)
    return;

  if (!d.intr && w_intr) {
    intr_age_[w_cause]++;
    log(cvm::HIGH, "<{}> intr_age_[{}][{}]++={}\n", w.time, hart, w_cause, intr_age_[w_cause]);

    // Check that interrupt age is not beyond threshold
    if ((intr_age_[w_cause] > FLAGS_max_pend_intr_age) && !FLAGS_cosim_resynch) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Whisper wants to take interrupt, DUT does not. cause: [{}], timeout: [{}] retires\n",
        hart, w_cause, FLAGS_max_pend_intr_age);
    }
    return;
  }

  if (FLAGS_bridge_log) {
    log(cvm::MEDIUM, "<{}> Interrupt taken by DUT. dcause:[{}] wcause:[{}]\n", w.time, d.icause, w_cause);
  }

  if (d.intr && !w_intr && !FLAGS_cosim_resynch) {
    whisper_check_interrupt(hart, prev_mip_, w_intr, w_cause);
    if (w_intr && (w_cause == d.icause)) {
      log(cvm::MEDIUM, "<{}> DUT took interrupt, Whisper did not. cause:[{}] (Missing modeling: Resynch and keep going)\n", w.time, d.icause);
      poke_mip(hart, w.time, (uint64_t)1 << d.icause);
    } else {
      cvm::log(cvm::ERROR, "Error: Hart {}: DUT took interrupt, Whisper did not. cause:[{}]\n", hart, d.icause);
    }
    return;
  }

  // If DUT took different older interrupt due to timing, get whisper to match
  // Undefer matching interrupt
  if (d.icause != w_cause && intr_age_[d.icause] >= intr_age_[w_cause]) {
    log(cvm::MEDIUM, "<{}> DUT vs Whisper interrupt cause mismatch [{},{}] age [{},{}] (Missing modeling: Resynch and keep going)\n",
      w.time, d.icause, w_cause, intr_age_[d.icause], intr_age_[w_cause]);
    whisper_defer_interrupt(hart, w.time, mip_ & ~((uint64_t)1 << d.icause));
    return;
  }

  // Undefer all interrupts
  if (deferred_intr_) {
    whisper_defer_interrupt(hart, w.time, 0);
    deferred_intr_ = false;
  }

  if (FLAGS_retire_ucode_trap)
    return;

  step(hart, w);
  if (FLAGS_bridge_log) {
    log(cvm::MEDIUM, "<{}> Whisper Step #{}: Extra step due to interrupt\n", w.time, step_);
  }
}

void bridge::process_interrupt_post_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {

  if (!d.intr && !w_.intr)
    return;

  intr_age_[w_.icause] = 0;

  // If interrupt asserted via csr write, we don't need to defer
  // DUT is expected to take at retire boundary if whisper takes the undeferred interrupt
  if (w_.intr && !d.intr && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    cvm::log(cvm::ERROR, "Error: Hart {}: Whisper took interrupt, DUT did not. cause:[{}]\n", hart, w_.icause);
    return;
  }

  if (d.intr && !w_.intr && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    cvm::log(cvm::ERROR, "Error: Hart {}: DUT took interrupt, Whisper did not. cause:[{}]\n", hart, d.icause);
    return;
  }

  // DUT cause should match whisper cause
  if ((d.icause != w_.icause) && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    cvm::log(cvm::ERROR, "Error: Hart {}: DUT vs Whisper interrupt cause mismatch [dut:{},whisper:{}]\n", hart, d.icause, w_.icause);
    return;
  }
}

void bridge::process_exception_post_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {

  if (!d.excp && !w_.excp)
    return;

  bool custom_nonspec_resync = (d.excp && (d.ecause == 28));
  if (custom_nonspec_resync) {
    log(cvm::MEDIUM, "<{}> Special custom exception detected: NONSPEC_RESYNC\n", d.cycle);
    return;
  }
  
  if(debug_mode_ && FLAGS_emulate_debug_mode && (d.excp )){
    excp_in_debug_mode = true;
    return;
  }else{
    excp_in_debug_mode = false;
  }
  
  if (d.excp && !w_.excp && !ecall_ && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    cvm::log(cvm::ERROR, "Error: Hart {}: DUT took exception, Whisper did not. cause:[{}]\n", hart, d.ecause);
    return;
  }
  
  if (w_.excp && !d.excp && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    cvm::log(cvm::ERROR, "Error: Hart {}: Whisper took exception, DUT did not. cause:[{}]\n", hart, w_.ecause);
    return;
  }

  // Special case - ecall - No extra step
  if (is_ecall(w)) {
    ecall_ = true;
    return;
  } else {
    ecall_ = false;
  }

  // If resynch, poke CSR values to whisper/
  if (FLAGS_cosim_resynch) {
    for (auto& c : w_.csr) {
      if (FLAGS_bridge_log) {
        log(cvm::HIGH, "<{}> Whisper Step #{}: Resynch: C{:#x}={:#x}\n", d.cycle, step_,
          c.csr_addr, c.csr_wdata);
      }
      bool valid;
      if (!client_->whisperPoke(hart, d.cycle, 'c', c.csr_addr, c.csr_wdata, valid)) {
        cvm::log(cvm::ERROR, "Error: Hart {}: Failed to resynch CSR values\n", hart);
        return;
      }
    }
  }

  // Print exception info
  if (FLAGS_bridge_log) {
    print_instr(hart, w);
    log(cvm::MEDIUM, "<{}> Exception detected. csrs:[", w.time);
    for (auto& c : w_.csr) {
      log(cvm::MEDIUM, "{:#x}={:#x},", c.csr_addr, c.csr_wdata);
    }
    log(cvm::MEDIUM, "]\n");
  }

  // If DUT indicates retire on ucode trap handler, extra step not needed
  if (FLAGS_retire_ucode_trap)
    return;

  step(hart, w);
  if (FLAGS_bridge_log) {
    log(cvm::MEDIUM, "<{}> Whisper Step #{}: Extra step due to exception\n", w.time, step_);
  }
  update_whisper_state(hart,w);
}

void bridge::process_satp_write_post_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {
  if (!FLAGS_delay_satp_update)
    return;

  // Save satp updates and apply only when sfence.vma is seen
  if (w.disasm.find("satp") != std::string::npos) {
    for (auto& c : w_.csr) {
      if (c.csr_addr == 0x180) {
        new_satp_ = c.csr_wdata;

        uint16_t new_mode_asid = (new_satp_ >> 44) & 0xffff;
        uint16_t mode_asid = (satp_ >> 44) & 0xffff;
        if (new_mode_asid != mode_asid) {
          satp_ = new_satp_;
          return;
        }

        if (FLAGS_bridge_log) {
          log(cvm::MEDIUM, "<{}> Whisper Step #{}: SATP write, don't apply till sfence.vma\n", w.time, step_);
        }
        bool valid = false;
        if (!client_->whisperPoke(hart, d.cycle, 'c', 0x180, satp_, valid)) {
          cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke SATP\n", hart);
          return;
        }
      }
    }
  }

  if (w.disasm.find("sfence.vma") != std::string::npos) {
    if (satp_ == new_satp_)
      return;

    satp_ = new_satp_;

    if (FLAGS_bridge_log) {
      log(cvm::MEDIUM, "<{}> Whisper Step #{}: sfence.vma, apply SATP write\n", w.time, step_);
    }
    bool valid = false;
    if (!client_->whisperPoke(hart, w.time, 'c', 0x180, new_satp_, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke new SATP\n", hart);
      return;
    }
  }
}

void bridge::update_whisper_state(hart_id_t hart, whisper_state_t& w) {

  w_.valid = true;
  w_.cycle = w.time;
  w_.tag = w.tag;
  w_.priv = w.priv_mode;
  w_.opcode = w.opcode;
  w_.trap = w.trap;
  w_.comp = ((w.opcode & 0x3) == 0x1) || ((w.opcode & 0x3) == 0x2);
  w_.ucode = ((w.opcode & 0x3f) == 0x73) | w.trap; // system opcode
  
  w_.pc.valid = true;
  w_.pc.pc_rdata = w.pc;
  update_pc(hart, src_t::iss, w.pc);

  if (!w_.comp && !w_.ucode)
    update_insn(hart, src_t::iss, w.opcode);

  for (auto i = 0u; i < w.change_count; i++) {
    if (!client_->whisperChange(hart, w.resource, w.address, w.value,
        w.valid)) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Failed to get whisper changes\n", hart);
      return;
    }
    if (FLAGS_bridge_log) {
      print_resource(hart, w);
    }
    // Populate w_ with bridge_if struct
    if (w.resource == 'r') {
      w_.gpr.valid = true;
      w_.gpr.rd_addr = w.address;
      w_.gpr.rd_wdata = w.value;
      update_regs(hart, w);
    }
    if (w.resource == 'f') {
      w_.fpr.valid = true;
      w_.fpr.frd_addr = w.address;
      w_.fpr.frd_wdata = w.value;
      update_regs(hart, w);
    }
    if (w.resource == 'v') {
      w_.vr.valid = true;
      w_.vr.vrd_addr = w.address;
      // w_.vr.vrd_wdata = w.value;
      update_regs(hart, w, i);
    }
    if (w.resource == 'c') {
      csr_t c;
      c.valid = true;
      c.csr_addr = w.address;
      c.csr_wdata = w.value;
      w_.csr.push_back(c);
      update_regs(hart, w);
    }
    if (w.resource == 'm') {
      w_.mem_write.valid = true;
      w_.mem_write.va = w.address;
      w_.mem_write.data = w.value;
    }
  }

  // Interrupts/Exceptions
  if (w_.trap) {
    uint64_t cause = 0;
    for (auto& c : w_.csr) {
      if ((c.csr_addr == 0x342) || (c.csr_addr == 0x142))
        cause = c.csr_wdata;
    }
    if ((cause >> 63) & 0x1) {
      w_.intr = true;
      w_.icause = (cause & 0x3f);
    } else {
      w_.excp = true;
      w_.ecause = (cause & 0xff);
    }
  }

}

// Print functions
void bridge::print_instr_stdout(hart_id_t hart, const rv_instr_t& d) {
  cvm::log(cvm::MEDIUM, "<{}> Instr Group Step #{}: [Hart={}, Mode={}, Tag={}, Trap={}, PC={:#x}, Opcode={:#x}, Disasm={}]\n",
    d.cycle, step_, hart, d.priv, d.tag, d.trap, d.pc.pc_rdata, d.opcode, d.disasm);
}

void bridge::print_instr(hart_id_t hart, const whisper_state_t& w) {
  log(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, Trap={}, ChangeCount={}, PC={:#x}, Opcode={:#x}, Disasm={}]\n",
    w.time, step_, hart, w.priv_mode, w.tag, w.trap, w.change_count, w.pc, w.opcode, w.disasm);
}

void bridge::print_instr_stdout(hart_id_t hart, const whisper_state_t& w) {
  cvm::log(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, Trap={}, ChangeCount={}, PC={:#x}, Opcode={:#x}, Disasm={}]\n",
    w.time, step_, hart, w.priv_mode, w.tag, w.trap, w.change_count, w.pc, w.opcode, w.disasm);
}

void bridge::print_resource(hart_id_t hart, const whisper_state_t& w) {
  log(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, Resource={}, Addr={:#x}, Data={:#x}]\n",
    w.time, step_, hart, w.priv_mode, w.tag, (char)w.resource, w.address, w.value);
}

void bridge::step(hart_id_t hart, whisper_state_t& w) {
  if (!client_->whisperStep(hart, w.time, w.tag,  w.pc, w.opcode, w.change_count, w.disasm,
      w.priv_mode, w.fp_flags, w.trap, w.stop)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to step whisper\n", hart);
    return;
  }

  // Print instruction
  if (FLAGS_bridge_log) {
    print_instr(hart, w);
  }
}

std::vector<cac::size_8_bytes_t> create_dword_vec(const std::bitset<256>& input) {
    // Calculate the number of 8-byte chunks needed for the 256-bit input
    size_t num_chunks = (256 + 63) / 64; // Round up division

    // Create a vector to store the chunks
    std::vector<cac::size_8_bytes_t> dword_vec(num_chunks);

    // Convert and store each chunk of 64 bits (8 bytes)
    for (size_t i = 0; i < num_chunks; ++i) {
        cac::size_8_bytes_t chunk = 0;
        for (size_t j = 0; j < 64; ++j) {
            size_t bit_index = i * 64 + j;
            if (bit_index < 256 && input[bit_index]) {
                chunk |= (cac::size_8_bytes_t(1) << j);
            }
        }
        dword_vec[i] = chunk;
    }

    return dword_vec;
}

// Push DUT register state to cac
void bridge::update_regs(hart_id_t hart, const rv_instr_t& d) {
  // GPR
  if (FLAGS_gpr_check && d.gpr.valid) {
    update_regs(hart, src_t::dut, resource_t::int_reg, d.gpr.rd_addr, {d.gpr.rd_wdata});
  }
  // FPR
  if (FLAGS_fpr_check && d.fpr.valid) {
    update_regs(hart, src_t::dut, resource_t::fp_reg, d.fpr.frd_addr, {d.fpr.frd_wdata});
  }
  // VR
  if (FLAGS_vec_check && d.vr.valid) {
    update_regs(hart, src_t::dut, resource_t::vec_reg, d.vr.vrd_addr, create_dword_vec(d.vr.vrd_wdata));
  }
  // CSR
  for (auto & c : d.csr) {
    size_8_bytes_t mask = c.csr_wmask & get_csr_mask(hart, c.csr_addr);
    uint64_t data = modify_csr_data(hart, c.csr_addr, c.csr_wdata);
    update_csr(hart, src_t::dut, c.csr_addr, data, mask);
    if (c.csr_addr == 0x344) {
      mip_ = get_csr(hart, src_t::dut, 0x344);
      log(cvm::MEDIUM, "<{}> Zicsr write based interrupt: mip {:#x} mask {:#x}\n", c.cycle, data, mask);
    }
  }
}

// Push DUT mem state to cac
void bridge::update_mem(hart_id_t, rv_instr_t&) {
}

std::bitset<256> create_bitset(cac::size_8_bytes_t dword_vec_array [vlen/64]) {
    std::bitset<256> result;

    // Iterate through the array and concatenate each value to the result bitset
    for (size_t i = 0; i < vlen/64; ++i) {
        for (size_t j = 0; j < 64; ++j) {
            size_t bit_index = i * 64 + j;
            bool bit_value = (dword_vec_array[i] & (cac::size_8_bytes_t(1) << j)) != 0;
            result[bit_index] = bit_value;
        }
    }

    return result;
}

// Push whisper register state to cac
void bridge::update_regs(hart_id_t hart, const whisper_state_t& w, uint32_t vec_slice_index) {
  // Register changes - r, f, v,
  // size_8_bytes_t dword_vec_array [vlen/64] = {0};
  uint32_t vec_slices = vlen/64;

  switch(w.resource) {
    case 'r':
      if (FLAGS_gpr_check)
        update_regs(hart, src_t::iss, resource_t::int_reg, w.address, {w.value});
      break;
    case 'f':
      if (FLAGS_fpr_check)
        update_regs(hart, src_t::iss, resource_t::fp_reg, w.address, {w.value});
      break;
    case 'v':
      if (FLAGS_vec_check){
        dword_vec_array [vec_slice_index % vec_slices] = w.value;        
        if ((vec_slice_index % vec_slices) == (vec_slices - 1)){
          update_regs(hart, src_t::iss, resource_t::vec_reg, w.address, std::vector<size_8_bytes_t>(dword_vec_array, dword_vec_array + sizeof(dword_vec_array)/sizeof(dword_vec_array[0])));
          w_.vr.vrd_wdata = create_bitset(dword_vec_array);
        }
      }
      break;
    case 'c':
      update_csr(hart, src_t::iss, w.address, w.value);
      break;
    default:
      break;
  }
}

// Utility functions
void bridge::update_pc(hart_id_t hart, src_t src, uint64_t data) {
  resource_id_t pc = resource_id_t{
    .resource = resource_t::pc_reg,
    .offset = 0
  };
  assert(cac_.UpdateResource(hart, src, pc, std::move(cac::CreateBitVec<uint64_t>(data))));
}

void bridge::update_insn(hart_id_t hart, src_t src, uint32_t data) {
  resource_id_t insn = resource_id_t{
    .resource = resource_t::insn_bytes,
    .offset = 0
  };
  assert(cac_.UpdateResource(hart, src, insn, std::move(cac::CreateBitVec<uint64_t>(data))));
}

void bridge::update_regs(hart_id_t hart, src_t src, resource_t resource, uint64_t addr, const std::vector<size_8_bytes_t>&& dword_vec) {
  if ((src == src_t::dut) && (resource == resource_t::int_reg) && (addr == 0)) {
    return;
  }
  resource_id_t rid = resource_id_t{
    .resource = resource,
    .offset = addr
  };
  assert(cac_.UpdateResource(hart, src, rid, std::move(cac::CreateBitVec<size_8_bytes_t>(dword_vec))));
}

bool bridge::is_ecall(const whisper_state_t& w) {
  if (w.disasm.find("ecall") != std::string::npos)
    return true;

  // FIXME Temp workaround to detect ecall
  for (auto& c : w_.csr) {
    if (c.csr_addr == 0x342 && c.csr_wdata == 0x9)
      return true;
  }
  return false;
}

bool bridge::does_instr_match_resynch_condition(const rv_instr_t& d, const std::string& instr) {
  // Case #1
  if (clint_read(d)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[clint_read]\n", d.cycle);
    return true;
  }
  // Case #2
  if (htif_read(d)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[htif_read]\n", d.cycle);
    return true;
  }
  // Case #3
  if (hpm_counter_read(instr)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[hpm_counter_read]\n", d.cycle);
    return true;
  }
  // Case #4
  if (FLAGS_lrsc_resynch && lrsc_fail(d, instr)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[lrsc_fail]\n", d.cycle);
    return true;
  }
  // Case #5
  if (debug_mem_access(d)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[debug mem access]\n", d.cycle);
    return true;
  }
  // Case #6
  if (boot_read(d)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[boot_read]\n", d.cycle);
    return true;
  }
  // Case #7
  if (mip_mismatch(instr)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[mip_mismatch]\n", d.cycle);
    return true;
  }
  return false;
}

bool bridge::clint_read(const rv_instr_t& d) {
  if (d.mem_read.valid &&
      d.mem_read.pa >= memmap_.at("clint").base &&
      d.mem_read.pa < memmap_.at("clint").end)
    return true;
  return false;
}

bool bridge::boot_read(const rv_instr_t& d) {
  if (d.mem_read.valid &&
      d.mem_read.pa >= memmap_.at("boot").base &&
      d.mem_read.pa < memmap_.at("boot").end)
    return true;
  return false;
}

bool bridge::mip_mismatch(const std::string& instr) {
  if ((instr.find("mip") != std::string::npos) &&
      (mip_ != prev_mip_))
    return true;
  return false;
}

bool bridge::debug_mem_access(const rv_instr_t& d){
  if (d.mem_read.valid && debug_mode_ &&
      ((d.mem_read.pa < FLAGS_debug_entry_pc) ||
      ((d.mem_read.pa > FLAGS_debug_exit_pc) && (d.mem_read.pa <=0x1000)))
      )
    return true;
  return false;
}

bool bridge::htif_read(const rv_instr_t& d) {
  if (d.mem_read.valid &&
      d.mem_read.pa >= (memmap_.at("htif").base) &&
      d.mem_read.pa < (memmap_.at("htif").end))
    return true;
  return false;
}

bool bridge::hpm_counter_read(const std::string& instr) {
  if ((instr.find("hpmcounter") != std::string::npos) ||
      (instr.find("instret") != std::string::npos) ||
      (instr.find("time") != std::string::npos) ||
      (instr.find("cycle") != std::string::npos))
    return true;
  return false;
}

bool bridge::lrsc_fail(const rv_instr_t& d, const std::string& instr) {
  if ((instr.find("sc.w") != std::string::npos) ||
      (instr.find("sc.d") != std::string::npos)) {
    uint64_t fail_code = 1;
    if (d.gpr.rd_wdata == fail_code) {
      bool valid;
      if (!client_->whisperCancelLr(id_, valid)) {
        cvm::log(cvm::ERROR, "Error: Hart {}: Failed to CancelLr\n", id_);
      }
      return true;
    }
  }
  return false;
}

bool bridge::does_instr_match_resynch_list(const rv_instr_t& d, const std::string& instr) {
  if (FLAGS_cosim_resynch_instr == "")
    return false;

  std::stringstream ss(FLAGS_cosim_resynch_instr);

  while(ss.good()) {
    std::string s;
    std::getline(ss, s, ',' );

    if (instr.find(s) != std::string::npos) {
      log(cvm::MEDIUM, "<{}> Resynch: Reason=[+cosim_resynch_instr={} for instr={}]\n", d.cycle, FLAGS_cosim_resynch_instr, instr);
      return true;
    }
  }
  return false;
}

// Poke resources in whisper
void bridge::resynch(hart_id_t hart, const rv_instr_t& d) {
  bool valid = false;

  if (d.pc.pc_rdata != w_.pc.pc_rdata) {
    if (FLAGS_bridge_log) {
      log(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: PC={:#x}\n", d.cycle, step_, d.pc.pc_rdata);
    }
    if (!client_->whisperPoke(hart, d.cycle, 'p', 0, d.pc.pc_rdata, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Failed to resynch PC\n", hart);
      return;
    }
  }

  if (d.gpr.valid) {
    if (FLAGS_bridge_log) {
      log(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: X{}={:#x}\n", d.cycle, step_, d.gpr.rd_addr,
        d.gpr.rd_wdata);
    }
    if (!client_->whisperPoke(hart, d.cycle, 'r', d.gpr.rd_addr, d.gpr.rd_wdata, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Failed to resynch GPR\n", hart);
      return;
    }
  }

  if (d.fpr.valid) {
    if (FLAGS_bridge_log) {
      log(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: F{}={:#x}\n", d.cycle, step_, d.fpr.frd_addr,
        d.fpr.frd_wdata);
    }
    if (!client_->whisperPoke(hart, d.cycle, 'f', d.fpr.frd_addr, d.fpr.frd_wdata, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Failed to resynch FP\n", hart);
      return;
    }
  }

  if (d.mem_write.valid) {
    uint64_t pa = translate(hart, d.mem_write.va, w_.priv, memclass_t::write);
    if (FLAGS_bridge_log) {
      log(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: M[{:#x}]={:#x}\n", d.cycle, step_, pa,
        d.mem_write.data);
    }
    if (!client_->whisperPoke(hart, d.cycle, 'm', pa, d.mem_write.data, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Failed to resynch memory\n", hart);
      return;
    }
  }

  for (auto& csr : d.csr) {
    if (csr.valid) {
      if (FLAGS_bridge_log) {
        log(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: C[{:#x}]={:#x}\n", d.cycle, step_, csr.csr_addr,
          csr.csr_wdata);
      }
      resynch_csr_ = true;
      if (!client_->whisperPoke(hart, d.cycle, 'c', csr.csr_addr, csr.csr_wdata, valid)) {
        cvm::log(cvm::ERROR, "Error: Hart {}: Failed to resynch CSRs\n", hart);
        return;
      }
    }
  }
}

void bridge::resynch(hart_id_t hart, const rv_instr_group_t& d) {
  bool valid = false;
  for (auto& csr : d.csrs) {
    if (csr.valid) {
      if (FLAGS_bridge_log) {
        log(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: C[{:#x}]={:#x}\n", d.cycle, step_, csr.csr_addr,
          csr.csr_wdata);
      }
      if (!client_->whisperPoke(hart, d.cycle, 'c', csr.csr_addr, csr.csr_wdata, valid)) {
        cvm::log(cvm::ERROR, "Error: Hart {}: Failed to resynch CSRs\n", hart);
        return;
      }
    }
  }
}

// Process mem accesses - load resolves
void bridge::process_dut_mcm_read(hart_id_t hart, mem_t& m) {
  bool valid = false;
  if (!client_->whisperMcmRead(hart, m.cycle, m.tag, m.pa, m.size, m.data, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed mcm load resolve\n", hart);
    return;
  }
  log(cvm::HIGH, "<{}> mcm_read [valid={}, tag={}, addr={:#x}, size={}, data={:#x}]\n",
    m.cycle, valid, m.tag, m.pa, m.size, m.data);
}

// Process mem accesses - store inserts
void bridge::process_dut_mcm_insert(hart_id_t hart, mem_t& m) {
  bool valid = false;

  if (!client_->whisperMcmInsert(hart, m.cycle, m.tag, m.pa, m.size, m.data, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed mcm store insert\n", hart);
    return;
  }
  log(cvm::HIGH, "<{}> mcm_insert [valid={}, tag={}, addr={:#x}, size={}, data={:#x}]\n",
    m.cycle, valid, m.tag, m.pa, m.size, m.data);

  // Collect metrics
  num_stores_++;
}

// Process mem accesses - store bypass_writes
void bridge::process_dut_mcm_bypass(hart_id_t hart, mem_t& m) {
  bool valid = false;

  if (!client_->whisperMcmBypass(hart, m.cycle, m.tag, m.pa, m.size, m.data, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed mcm store bypass\n", hart);
    return;
  }
  log(cvm::HIGH, "<{}> mcm_bypass [valid={}, tag={}, addr={:#x}, size={}, data={:#x}]\n",
    m.cycle, valid, m.tag, m.pa, m.size, m.data);

  // Collect metrics
  num_stores_++;
}

// Process mem accesses - store drains
void bridge::process_dut_mcm_write(hart_id_t hart, mem_cl_t& m) {
  uint8_t data[64] = {0};
  for (unsigned i=0; i<64; i++) {
    data[i] = (uint8_t)((m.data >> (i*8)) & std::bitset<512>(0xff)).to_ulong();
  }

  bool valid = false;
  if (!client_->whisperMcmWrite(hart, m.cycle, m.pa, 64, data, m.mask, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed mcm store drain\n", hart);
    return;
  }
  log(cvm::HIGH, "<{}> mcm_write [valid={}, addr={:#x}, mask={:016x}, data=",
    m.cycle, valid, m.pa, m.mask);
  for (int i=63; i>=0; i--)
    log(cvm::HIGH, "{:02x}", data[i]);
  log(cvm::HIGH, "]\n");
}

uint64_t bridge::translate(hart_id_t hart, uint64_t va, uint8_t priv, memclass_t memclass) {
  uint64_t pa = va;

  if (priv == 0x3)
    return pa;

  bool valid;
  bool r = (memclass == memclass_t::read);
  bool w = (memclass == memclass_t::write);
  bool x = (memclass == memclass_t::fetch);
  bool sup = (priv == 0x1);

  if (!client_->whisperTranslate(hart, va, r, w, x, sup, pa, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed VA translation\n", hart);
  }

  return pa;
}

// LS Translation check
void bridge::translation_check(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w){

  if (!FLAGS_translation_check)
    return;

  if (d.mem_va == 0)
  return;

  uint64_t va = d.mem_va;
  uint64_t bit57 = va & (1ull << 56);
  va &= ((1ull << 57) - 1);             // Clear all bits to the left of 57th bit
  if (bit57) {  va |= (~0ull) << 57; } // sign extend the 57th bit to [63:58]

  uint64_t pa = translate(hart, va, w.priv_mode, memclass_t::read);
  if (pa != d.mem_pa){
    cvm::log(cvm::NONE, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, PC={:#x}, VA={:#x}, RTL-PA={:#x}, ISS-PA={:#x}]\n", w.time, step_-1, hart, w.priv_mode, w.tag, w.pc, d.mem_va, d.mem_pa, pa);
    cvm::log(cvm::ERROR, "Error: Hart {}: PA MISMATCH !! :\n", hart);
    return;
  }
  else {
    log(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, PC={:#x}, VA={:#x}, PA={:#x}]\n", w.time, step_-1, hart, w.priv_mode, w.tag, w.pc, d.mem_va, pa);
  }

}

// Interrupts
void bridge::process_dut_interrupt(hart_id_t hart, rv_intr_t& i) {
  log(cvm::MEDIUM, "<{}> Wired interrupt: mip {:#x} mask {:#x}\n", i.cycle, i.mip, i.mip_mask);
  
  // Update dut mip
  size_8_bytes_t mask = i.mip_mask;
  update_csr(hart, src_t::dut, 0x344, i.mip, mask);

  // Poke whisper mip for sw/timer interrupts, not for external
  mip_ = get_csr(hart, src_t::dut, 0x344);
  if (mask & ~0x800) {
    // Ideally, would have liked to poke mip with a mask
    // Since we can't, doing a rmw instead
    poke_mip(hart, i.cycle, mip_ & ~0x800);
    // Defer interrupt when the pin is asserted
    // Undefer once it's taken
    if (i.mip_assert) {
      bool w_intr;
      uint64_t w_cause;
      whisper_check_interrupt(hart, mip_, w_intr, w_cause);
      if (w_intr) {
        deferred_intr_ = true;
        whisper_defer_interrupt(hart, i.cycle, i.mip);
      }
    }
  }
}

void bridge::process_dut_imsic_interrupt(hart_id_t hart, mem_t& m) {
  log(cvm::MEDIUM, "<{}> IMSIC interrupt: [addr={:#x} data={:#x}]\n", m.cycle, m.pa, m.data);

  // Poke imsic write into whisper memory
  bool valid;
  if (!client_->whisperPoke(hart, m.cycle, 'm', m.pa, m.data, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke memory\n", hart);
    return;
  }

  // Peek mip to check if expected to be taken
  if (!client_->whisperPeek(hart, 'c', 0x344, iss_mip_, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek mip\n", hart);
    return;
  }
  log(cvm::MEDIUM, "<{}> Whisper mip: {:#x}\n", m.cycle, iss_mip_);
}

void bridge::whisper_defer_interrupt(hart_id_t hart, uint64_t cycle, uint64_t mip) {
  log(cvm::HIGH, "<{}> Interrupt defer mip status {:#x}\n", cycle, mip);
  bool valid;
  if (!client_->whisperPoke(hart, cycle, 's', WhisperSpecialResource::DeferredInterrupts, mip, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke DeferredInterrupts\n", hart);
    return;
  }
}

void bridge::whisper_check_interrupt(hart_id_t hart, uint64_t mip, bool& taken, uint64_t& cause) {
  if (!client_->whisperCheckInterrupt(hart, mip, taken, cause)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed whisper API call - whisperCheckInterrupt\n", hart);
    return;
  }
}

void bridge::poke_mip(hart_id_t hart, uint64_t time, uint64_t mip) {
  bool valid;
  if (!client_->whisperPoke(hart, time, 'c', 0x344, mip, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke mip csr\n", hart);
    return;
  }
  log(cvm::MEDIUM, "<{}> Whisper poke: mip: {:#x}\n", time, mip);
}

void bridge::poke_seip(hart_id_t hart, uint64_t time, bool val) {
  log(cvm::MEDIUM, "<{}> Whisper poke: seip: {}\n", time, val);
  if (!client_->whisperSetSeiPin(hart, (uint64_t)val)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke seip\n", hart);
    return;
  }
}

// Debug Mode
void bridge::enter_debug_mode(rv_debug_t& d) {
   uint64_t debugROM[14] = {
    0x7b2000737b202473,
    0x10802823f1402473,
    0xaa5ff06f7b202473,
    0x1000242300100073,
    0x7b20247310002c23,
    0xfddff06ffc0414e3,
    0x0024741340044403,
    0xf140247302041263,
    0x0014741340044403,
    0x10802023f1402473,
    0x7b2410730ff0000f,
    0x000000130380006f,
    0x000000130580006f,
    0x000000130180006f
   };
  log(cvm::NONE, "<{}> Enter debug mode\n", d.cycle);
  debug_mode_ = true;
  if (!client_->whisperEnterDebug()) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to enter debug mode\n", id_);
    return;
  }
 
  bool valid;
 for(int i=0;i<14;i++){
    
    uint64_t debugROM_loc = FLAGS_debug_entry_pc + i*8;
    
    if (!client_->whisperPoke(0, 0, 'm', debugROM_loc,debugROM[i] , valid)) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke debug memory\n", 0);
      return;
    }
 }

}

void bridge::exit_debug_mode(rv_debug_t& d) {
  log(cvm::NONE, "<{}> Exit debug mode\n", d.cycle);
  debug_mode_ = false;
  if (!client_->whisperExitDebug()) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to exit debug mode\n", id_);
    return;
  }
}

uint64_t bridge::modify_csr_data(hart_id_t hart, uint64_t addr, uint64_t data) {
  uint64_t result = data;
  // pmpaddr
  // Spec section...
  if (addr >= 0x3B0 && addr < 0x3C0) {
    bool valid;
    uint64_t pmpcfg, mask, reset;
    client_->whisperPeekCsr(hart, addr - 16, pmpcfg, mask, reset, valid);
    if((pmpcfg >> 4) & 0x1) {
      result = data | 0x1ff;
    } else {
      result = data & 0xfffffffffffffc00;
    }
  }
  return result;
}

bool bridge::is_custom_csr(uint64_t addr) {
  return ((addr >= 0x5C0 && addr <= 0x5FF) ||
          (addr >= 0x6C0 && addr <= 0x6FF) ||
          (addr >= 0x7C0 && addr <= 0x7FF) ||
          (addr >= 0x800 && addr <= 0x8FF) ||
          (addr >= 0x9C0 && addr <= 0x9FF) ||
          (addr >= 0xAC0 && addr <= 0xAFF) ||
          (addr >= 0xBC0 && addr <= 0xBFF));
}

bool bridge::is_supported_csr(uint64_t addr) {
  return (addr >= 0x7E0 && addr <= 0x7EF); // pmacfg0-15
}

void bridge::update_csr(hart_id_t hart, src_t src, uint64_t addr, uint64_t data, cac::optional_const_ref<size_8_bytes_t> mask_ref) {
  if (is_custom_csr(addr) && !is_supported_csr(addr))
    return;

  resource_id_t csr_resource = resource_id_t{
    .resource = resource_t::csr_reg,
    .offset = addr
  };
  cac::mask_t mask = cac::CreateBitVec<size_8_bytes_t>(std::numeric_limits<size_8_bytes_t>::max());
  if (mask_ref != std::nullopt) {
    mask = cac::CreateBitVec<size_8_bytes_t>(mask_ref.value());
  }
  assert(csr_cac_.UpdateResource(hart, src, csr_resource, std::move(cac::CreateBitVec<size_8_bytes_t>({data})), mask));
}

uint64_t bridge::get_csr(hart_id_t hart, src_t src, uint64_t addr) {
  std::vector<bool> bool_vec;
  std::vector<uint64_t> dword_vec;
  resource_id_t csr_resource = resource_id_t{
    .resource = resource_t::csr_reg,
    .offset = addr
  };
  assert(csr_cac_.GetResource(hart, src, csr_resource, bool_vec));
  dword_vec = cac::CreateSizedVec<uint64_t>(bool_vec);
  return dword_vec[0];
}

uint64_t bridge::get_csr_mask(hart_id_t hart, uint64_t addr) {
  bool valid;
  uint64_t data, mask, poke_mask;
  if (!client_->whisperPeekCsr(hart, addr, data, mask, poke_mask, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek csr\n", hart);
  }
  return mask;
}

uint64_t bridge::get_csr_poke_mask(hart_id_t hart, uint64_t addr) {
  bool valid;
  uint64_t data, mask, poke_mask;
  if (!client_->whisperPeekCsr(hart, addr, data, mask, poke_mask, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek csr\n", hart);
  }
  return poke_mask;
}

std::string bridge::get_csr_name(const std::string& csr_addr) {
  unsigned int addr;
  try {
    addr = std::stoul(csr_addr, nullptr, 16);
  }
  catch (...) {
    return csr_addr;
  }

  for (const auto& csr : csrs) {
    if (csr.address == addr) {
        return csr.name;
    }
  }
  return csr_addr;
}

void bridge::final_phase() {
  //report_metrics();
}

void bridge::report_metrics() {
  if (!FLAGS_metrics)
    return;

  cvm::log(cvm::NONE, "[COSIM] Report metrics...\n");

  const auto& prev_whisp_state = pw_;
  const auto& prev_prev_whisp_state = ppw_;
  const int instructions = cac_.GetStep(id_);
  const auto& cpu_cycles = prev_whisp_state.time;
  const double ipc = cpu_cycles ? static_cast<double>(instructions) / static_cast<double>(cpu_cycles) : 0.0;
  const auto& instr = prev_whisp_state.disasm;
  const auto& mode = prev_whisp_state.priv_mode;
  const auto& trap = prev_whisp_state.trap;
  const auto& num_dest = prev_whisp_state.change_count;
  bool rfcm = (prev_whisp_state.resource == 'r' || prev_whisp_state.resource == 'f' || prev_whisp_state.resource == 'c' || prev_whisp_state.resource == 'm');
  const std::string dest = (rfcm ? std::string(1, static_cast<char>(prev_whisp_state.resource)) : "none");
  const std::string dest_addr = (rfcm ? fmt::format("0x{:x}", prev_whisp_state.address) : "none");
  const std::string dest_data = (rfcm ? fmt::format("0x{:x}", prev_whisp_state.value) : "none");
  const auto& prev_instr = prev_prev_whisp_state.disasm;
  const auto& prev_mode = prev_prev_whisp_state.priv_mode;
  const auto& prev_trap = prev_prev_whisp_state.trap;
  const auto& prev_num_dest = prev_prev_whisp_state.change_count;

  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_instructions\": {}}}\n", id_, instructions);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_cpu_cycles\": {}}}\n", id_, cpu_cycles);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_ipc\": {:.2f}}}\n", id_, ipc);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_instr\": \"{}\"}}\n", id_, instr);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_mode\": {}}}\n", id_, mode);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_trap\": {}}}\n", id_, trap);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_num_dest\": {}}}\n", id_, num_dest);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_dest\": \"{}\"}}\n", id_, dest);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_dest_addr\": \"{}\"}}\n", id_, dest_addr);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_dest_data\": \"{}\"}}\n", id_, dest_data);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_prev_instr\": \"{}\"}}\n", id_, prev_instr);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_prev_mode\": {}}}\n", id_, prev_mode);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_prev_trap\": {}}}\n", id_, prev_trap);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_prev_num_dest\": {}}}\n", id_, prev_num_dest);

  // Whisper csr values
  for (auto& csr : csrs) {
    uint64_t csr_data;
    bool valid;
    if (!client_->whisperPeek(id_, 'c', csr.address, csr_data, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek CSR values\n", id_);
    }
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_iss_csr_{}\": \"0x{:x}\"}}\n", id_, csr.name, csr_data);
  }

  // DUT csr values
  for (auto& csr : csrs) {
    uint64_t csr_data = get_csr(id_, src_t::dut, csr.address);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_dut_csr_{}\": \"0x{:x}\"}}\n", id_, csr.name, csr_data);
  }

  // Step one final time to collect metrics for next instruction
  whisper_state_t w {
    .tag = prev_whisp_state.tag+1,
    .time = prev_whisp_state.time+1
  };
  step(id_, w);
  const auto& next_instr = w.disasm;
  const auto& next_mode = w.priv_mode;
  const auto& next_trap = w.trap;
  const auto& next_num_dest = w.change_count;

  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_next_instr\": \"{}\"}}\n", id_, next_instr);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_next_mode\": {}}}\n", id_, next_mode);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_next_trap\": {}}}\n", id_, next_trap);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_next_num_dest\": {}}}\n", id_, next_num_dest);
  
}
