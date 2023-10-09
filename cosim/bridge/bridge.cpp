// Licensed under the Apache License, Version 2.0, see LICENSE.TT for details

#include "bridge.h"
#include "util.h"
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/topology.hpp"
#include "src/cac_lib.h"
#include "sysmod/htif/htif.h"
#include "whisper_client_decl.h"


#include <iostream>         // cout
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

DEFINE_bool(bridge_log, true, "Enable bridge logging");
DEFINE_string(whisper_json_path, "", "Path to whisper json config");
DEFINE_bool(cosim_resynch, false, "Resynch whisper with dut state on every instruction");
DEFINE_string(cosim_resynch_instr, "", "List of instruction mnemonics to resynch whisper with dut state");
DEFINE_string(cosim_resynch_prev_instr, "", "List of instruction mnemonics to resynch whisper with dut state");
DEFINE_bool(lrsc_resynch, false, "Resynch whisper with dut state on LRSC fail condition");
DEFINE_bool(retire_ucode_trap, true, "DUT indicates retire on a trap after executing the ucode trap handler");
DEFINE_bool(gpr_check, true, "Enable cosim checks on gprs");
DEFINE_bool(fpr_check, true, "Enable cosim checks on fprs");
DEFINE_bool(vec_check, true, "Enable cosim checks on vector regs");
DEFINE_bool(csr_check, false, "Enable cosim checks on csrs");
DEFINE_uint64(max_cycle, 1000000, "Max cycle limit to terminate the sim");
DEFINE_int32(debug_excp_mcause, 24, "MCAUSE value for debug exception");
DEFINE_int32(max_stall_cycle, 50000, "Max stall cycle limit to terminate the sim");
DEFINE_bool(translation_check, false, "Do VA-PA translation check");
DEFINE_bool(emulate_debug_mode, true, "Emulate debug mode by forcing whisper to be in sync with DUT");
DEFINE_bool(delay_satp_update, false, "Delay satp update till next sfence.vma");
DEFINE_bool(cov, false, "Enable Arch coverage");
DEFINE_string(archsample_lib_path, "", "Path to libarchsample.so");
DEFINE_bool(standalone, true, "Enable whisper standalone run at beginning of sim");
DEFINE_bool(metrics, true, "Enable printing metrics in log file");
DEFINE_uint32(max_pend_intr_age, 64, "Number of instructions allowed to retire before a pending interrupt should be taken");
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
    cac_(CacCore(num_harts))
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
}

// DUT interface callback: Instruction Retire
void bridge::process_dut_instr_retire(hart_id_t hart, rv_instr_t& d) {
  // Update cac with dut state
  update_dut_state(hart, d);

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

  // Resynch whisper with dut state if needed
  // to continue without failing
  if ( does_instr_match_resynch_list(w) ||
      does_prev_instr_match_resynch_list(pw_) ||
      does_instr_match_resynch_condition(d, w)) {
    resynch(hart, d);
    cac_.ResetStatus(hart);
  }

  // Save whisper state
  ppw_ = pw_;
  pw_ = w;
  prev_intr_pins_ = intr_pins_;

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
      print_instr_stdout(hart, w);
      cvm::log(cvm::NONE, "{}", cac_.GetStatusStr(hart));
      cvm::log(cvm::ERROR, "Error: Hart{}: Core Arch Checker Mismatch{} - {}\n", hart, resource,  instr);
      return;
    }
  }
  else {
      log(cvm::HIGH, "{}", cac_.GetStatusStr(hart));
  }

  // TLB checks
  translation_check(hart, d, w);
}

void bridge::update_dut_state(hart_id_t hart, rv_instr_t& d) {
  update_pc(hart, src_t::dut, d.pc.pc_rdata);
  if (!d.comp && !d.ucode) {
    update_insn(hart, src_t::dut, d.opcode);
  }
  if (d.gpr.valid || d.fpr.valid || d.vr.valid) {
    update_regs(hart, d);
  }
  if (d.mem_write.valid) {
    update_mem(hart, d);
  }
}

void bridge::process_debug_pre_step(hart_id_t hart, const rv_instr_t& instr, whisper_state_t& ) {
    bool valid;
    if (!client_->whisperPoke(hart, 0, 'm', instr.pc.pc_rdata, instr.opcode, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart{}: Failed to poke memory\n", hart);
      return;
    }
  return;
}

void bridge::process_interrupt_pre_step(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {

  // FIXME Get mip value from DUT csr update
  get_whisper_mip(hart, mip_);

  if (!intr_pins_ && !mip_ && !prev_intr_pins_)
    return;

  uint64_t w_cause;
  bool w_intr;
  get_whisper_intr_status(hart, w_intr, w_cause);

  if (!d.intr && !w_intr)
    return;

  if (!d.intr && w_intr) {
    poke_intr_defer_status(hart, w.time, intr_pins_);
    update_intr_age(hart, d);

    // Check that interrupt age is not beyond threshold
    if ((intr_age_[w_cause] > FLAGS_max_pend_intr_age) && !FLAGS_cosim_resynch) {
      cvm::log(cvm::ERROR, "Error: Hart{}: Whisper wants to take interrupt, DUT does not. cause: [{}], timeout: [{}] retires\n",
        hart, w_cause, FLAGS_max_pend_intr_age);
    }
    return;
  }

  if (FLAGS_bridge_log) {
    log(cvm::MEDIUM, "<{}> Interrupt taken by DUT. dcause:[{}] wcause:[{}]\n", w.time, d.icause, w_cause);
  }

  if (d.intr && !w_intr && !FLAGS_cosim_resynch) {
    if ((prev_intr_pins_ >> d.icause) & 0x1) {
      log(cvm::MEDIUM, "<{}> DUT took interrupt, Whisper did not. cause:[{}] - timing issue, pin deasserted after DUT observed\n", w.time, d.icause);
      poke_mip(hart, w.time, (uint64_t)1 << d.icause);
    } else {
      cvm::log(cvm::ERROR, "Error: Hart{}: DUT took interrupt, Whisper did not. cause:[{}]\n", hart, d.icause);
    }
    return;
  }

  // If DUT took different older interrupt due to timing, get whisper to match
  // Undefer matching interrupt
  if (d.icause != w_cause && intr_age_[d.icause] >= intr_age_[w_cause]) {
    log(cvm::MEDIUM, "<{}> Whisper Step #{}: DUT vs Whisper interrupt cause mismatch [{},{}] age [{},{}]\n",
      w.time, step_, d.icause, w_cause, intr_age_[d.icause], intr_age_[w_cause]);
    poke_intr_defer_status(hart, w.time, intr_pins_ & ~((uint64_t)1 << d.icause));
    return;
  }

  // Undefer all interrupts
  poke_intr_defer_status(hart, w.time, 0);

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

  // If interrupt asserted via csr write, we don't need to defer
  // DUT is expected to take at retire boundary if whisper takes the undeferred interrupt
  if (w_.intr && !d.intr && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    cvm::log(cvm::ERROR, "Error: Hart{}: Whisper took interrupt, DUT did not. cause:[{}]\n", hart, w_.icause);
    return;
  }

  // DUT cause should match whisper cause
  if ((d.icause != w_.icause) && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    cvm::log(cvm::ERROR, "Error: Hart{}: DUT vs Whisper interrupt cause mismatch [dut:{},whisper:{}]\n", hart, d.icause, w_.icause);
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
    cvm::log(cvm::ERROR, "Error: Hart{}: DUT took exception, Whisper did not. cause:[{}]\n", hart, d.ecause);
    return;
  }
  
  if (w_.excp && !d.excp && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    cvm::log(cvm::ERROR, "Error: Hart{}: Whisper took exception, DUT did not. cause:[{}]\n", hart, w_.ecause);
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
        cvm::log(cvm::ERROR, "Error: Hart{}: Failed to resynch CSR values\n", hart);
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
          cvm::log(cvm::ERROR, "Error: Hart{}: Failed to poke SATP\n", hart);
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
      cvm::log(cvm::ERROR, "Error: Hart{}: Failed to poke new SATP\n", hart);
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
      cvm::log(cvm::ERROR, "Error: Hart{}: Failed to get whisper changes\n", hart);
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
    cvm::log(cvm::ERROR, "Error: Hart{}: Failed to step whisper\n", hart);
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

bool bridge::does_instr_match_resynch_condition(const rv_instr_t& d, const whisper_state_t& w) {
  // Case #1
  if (clint_read(d)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[clint_read]\n", w.time);
    return true;
  }
  // Case #2
  if (htif_read(d)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[htif_read]\n", w.time);
    return true;
  }
  // Case #3
  if (hpm_counter_read(w)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[hpm_counter_read]\n", w.time);
    return true;
  }
  // Case #4
  if (FLAGS_lrsc_resynch && lrsc_fail(w)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[lrsc_fail]\n", w.time);
    return true;
  }
  // Case #5
  if (mip_timing_mismatch(w)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[mip_timing_mismatch]\n", w.time);
    return true;
  }
  // Case #6
  if (debug_mem_access(d)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[debug mem access]\n", w.time);
    return true;
  }
  // Case #7
  if (boot_read(d)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[boot_read]\n", w.time);
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

bool bridge::hpm_counter_read(const whisper_state_t& w) {
  if ((w.disasm.find("hpmcounter") != std::string::npos) ||
      (w.disasm.find("instret") != std::string::npos) ||
      (w.disasm.find("time") != std::string::npos) ||
      (w.disasm.find("cycle") != std::string::npos))
    return true;
  return false;
}

bool bridge::lrsc_fail(const whisper_state_t& w) {
  if ((w.disasm.find("sc.w") != std::string::npos) ||
      (w.disasm.find("sc.d") != std::string::npos)) {
    uint64_t fail_code = 1;
    if (w_.gpr.rd_wdata == fail_code)
      return true;
  }
  return false;
}

bool bridge::mip_timing_mismatch(const whisper_state_t& w) {
  if (w.disasm.find("mip") != std::string::npos) {
    if (prev_intr_pins_ != intr_pins_)
      return true;
  }
  return false;
}

bool bridge::does_instr_match_resynch_list(const whisper_state_t& w) {
  if (FLAGS_cosim_resynch_instr == "")
    return false;

  std::stringstream ss(FLAGS_cosim_resynch_instr);

  while(ss.good()) {
    std::string instr;
    std::getline(ss, instr, ',' );

    if (w.disasm.find(instr) != std::string::npos) {
      log(cvm::MEDIUM, "<{}> Resynch: Reason=[+cosim_resynch_instr={} for instr={}]\n", w.time, FLAGS_cosim_resynch_instr, instr);
      return true;
    }
  }
  return false;
}

bool bridge::does_prev_instr_match_resynch_list(const whisper_state_t& w) {
  if (FLAGS_cosim_resynch_prev_instr == "")
    return false;

  std::stringstream ss(FLAGS_cosim_resynch_prev_instr);

  while(ss.good()) {
    std::string instr;
    std::getline(ss, instr, ',' );

    if (w.disasm.find(instr) != std::string::npos) {
      log(cvm::MEDIUM, "<{}> Resynch: Reason=[+cosim_resynch_prev_instr={} for instr={}]\n", w.time, FLAGS_cosim_resynch_prev_instr, instr);
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
      cvm::log(cvm::ERROR, "Error: Hart{}: Failed to resynch PC\n", hart);
      return;
    }
  }

  if (d.gpr.valid) {
    if (FLAGS_bridge_log) {
      log(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: X{}={:#x}\n", d.cycle, step_, d.gpr.rd_addr,
        d.gpr.rd_wdata);
    }
    if (!client_->whisperPoke(hart, d.cycle, 'r', d.gpr.rd_addr, d.gpr.rd_wdata, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart{}: Failed to resynch GPR\n", hart);
      return;
    }
  }

  if (d.fpr.valid) {
    if (FLAGS_bridge_log) {
      log(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: F{}={:#x}\n", d.cycle, step_, d.fpr.frd_addr,
        d.fpr.frd_wdata);
    }
    if (!client_->whisperPoke(hart, d.cycle, 'f', d.fpr.frd_addr, d.fpr.frd_wdata, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart{}: Failed to resynch FP\n", hart);
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
      cvm::log(cvm::ERROR, "Error: Hart{}: Failed to resynch memory\n", hart);
      return;
    }
  }

  for (auto& csr : d.csr) {
    if (csr.valid) {
      if (FLAGS_bridge_log) {
        log(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: C[{:#x}]={:#x}\n", d.cycle, step_, csr.csr_addr,
          csr.csr_wdata);
      }
      if (!client_->whisperPoke(hart, d.cycle, 'c', csr.csr_addr, csr.csr_wdata, valid)) {
        cvm::log(cvm::ERROR, "Error: Hart{}: Failed to resynch CSRs\n", hart);
        return;
      }
    }
  }
}

// Process mem accesses - load resolves
void bridge::process_dut_mcm_read(hart_id_t hart, mem_t& m) {
  bool valid = false;
  if (!client_->whisperMcmRead(hart, m.cycle, m.tag, m.pa, m.size, m.data, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart{}: Failed mcm load resolve\n", hart);
    return;
  }
  log(cvm::HIGH, "<{}> mcm_read [valid={}, tag={}, addr={:#x}, size={}, data={:#x}]\n",
    m.cycle, valid, m.tag, m.pa, m.size, m.data);
}

// Process mem accesses - store inserts
void bridge::process_dut_mcm_insert(hart_id_t hart, mem_t& m) {
  bool valid = false;

  if (!client_->whisperMcmInsert(hart, m.cycle, m.tag, m.pa, m.size, m.data, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart{}: Failed mcm store insert\n", hart);
    return;
  }
  log(cvm::HIGH, "<{}> mcm_insert [valid={}, tag={}, addr={:#x}, size={}, data={:#x}]\n",
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
    cvm::log(cvm::ERROR, "Error: Hart{}: Failed mcm store drain\n", hart);
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
    cvm::log(cvm::ERROR, "Error: Hart{}: Failed VA translation\n", hart);
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
    cvm::log(cvm::ERROR, "Error: Hart{}: PA MISMATCH !! :\n", hart);
    return;
  }
  else {
    log(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, PC={:#x}, VA={:#x}, PA={:#x}]\n", w.time, step_-1, hart, w.priv_mode, w.tag, w.pc, d.mem_va, pa);
  }

}

// Interrupts
void bridge::process_dut_interrupt(hart_id_t hart, rv_intr_t& i) {
  intr_pins_ = i.mip | (i.seip << 9);
  log(cvm::MEDIUM, "<{}> Interrupt pin(s) toggled. Mip {:#x} Seip {:#x}\n", i.cycle, i.mip, i.seip);
  
  // Preserve sup bits in mip if written via csr except if pin is lowered (stip:y,seip:n,ssip:?)
  mip_ = (mip_ & 0x222) | i.mip;
  if (i.stip_negedge)
    mip_ &= 0xffdf;

  // Poke the interrupt pin values into whisper mip csr and sei pin if applicable
  poke_mip(hart, i.cycle, mip_);
  if (i.seip_posedge | i.seip_negedge)
    poke_seip(hart, i.cycle, i.seip);
}

void bridge::poke_intr_defer_status(hart_id_t hart, uint64_t cycle, uint64_t mip) {
  log(cvm::MEDIUM, "<{}> Interrupt defer mip status {:#x}\n", cycle, mip);
  bool valid;
  if (!client_->whisperPoke(hart, cycle, 's', WhisperSpecialResource::DeferredInterrupts, mip, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart{}: Failed to poke DeferredInterrupts\n", hart);
    return;
  }
}

void bridge::get_whisper_intr_status(hart_id_t hart, bool& taken, uint64_t& cause) {
  if (!client_->whisperCheckInterrupt(hart, (intr_pins_ | mip_), taken, cause)) {
    cvm::log(cvm::ERROR, "Error: Hart{}: Failed whisper API call - whisperCheckInterrupt\n", hart);
    return;
  }
}

void bridge::get_whisper_mip(hart_id_t hart, uint64_t& mip) {
  bool valid;
  if (!client_->whisperPeek(hart, 'c', 0x344, mip, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart{}: Failed to peek mip csr\n", hart);
    return;
  }
}

void bridge::update_intr_age(hart_id_t hart, const rv_instr_t& d) {
  for (int i = 0; i < 16; ++i) {
    if (((intr_pins_ | mip_) & (1ULL << i)) != 0) {
      intr_age_[i]++;
      log(cvm::HIGH, "<{}> intr_age_[{}][{}]++={}\n", d.cycle, hart, i, intr_age_[i]);
    } else {
      intr_age_[i] = 0;
      log(cvm::FULL, "<{}> intr_age_[{}][{}]=0\n", d.cycle, hart, i);
    }
  }
}

void bridge::poke_mip(hart_id_t hart, uint64_t time, uint64_t mip) {
  bool valid;
  // Peek old mip
  uint64_t old_mip;
  if (!client_->whisperPeek(hart, 'c', 0x344, old_mip, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart{}: Failed to peek mip csr\n", hart);
    return;
  }

  // Poke new mip = mask(old mip, sup e/t/s) | mip
  uint64_t new_mip = (old_mip & 0x222) | mip;
  mip_ = new_mip;

  if (!client_->whisperPoke(hart, time, 'c', 0x344, new_mip, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart{}: Failed to poke mip csr\n", hart);
    return;
  }
  log(cvm::MEDIUM, "<{}> Mip poked. Mip: {:#x}\n", time, new_mip);
}

void bridge::poke_seip(hart_id_t hart, uint64_t time, bool val) {
  log(cvm::MEDIUM, "<{}> Seip poked. Seip: {}\n", time, val);
  if (!client_->whisperSetSeiPin(hart, (uint64_t)val)) {
    cvm::log(cvm::ERROR, "Error: Hart{}: Failed to poke seip\n", hart);
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
    cvm::log(cvm::ERROR, "Error: Hart{}: Failed to enter debug mode\n", id_);
    return;
  }
 
  bool valid;
 for(int i=0;i<14;i++){
    
    uint64_t debugROM_loc = FLAGS_debug_entry_pc + i*8;
    
    if (!client_->whisperPoke(0, 0, 'm', debugROM_loc,debugROM[i] , valid)) {
      cvm::log(cvm::ERROR, "Error: Hart{}: Failed to poke debug memory\n", 0);
      return;
    }
 }

}

void bridge::exit_debug_mode(rv_debug_t& d) {
  log(cvm::NONE, "<{}> Exit debug mode\n", d.cycle);
  debug_mode_ = false;
  if (!client_->whisperExitDebug()) {
    cvm::log(cvm::ERROR, "Error: Hart{}: Failed to exit debug mode\n", id_);
    return;
  }
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

  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_instructions\": {}}}\n", id_, instructions);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_cpu_cycles\": {}}}\n", id_, cpu_cycles);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_ipc\": {:.2f}}}\n", id_, ipc);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_instr\": \"{}\"}}\n", id_, instr);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_mode\": {}}}\n", id_, mode);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_trap\": {}}}\n", id_, trap);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_num_dest\": {}}}\n", id_, num_dest);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_dest\": \"{}\"}}\n", id_, dest);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_dest_addr\": \"{}\"}}\n", id_, dest_addr);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_dest_data\": \"{}\"}}\n", id_, dest_data);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_prev_instr\": \"{}\"}}\n", id_, prev_instr);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_prev_mode\": {}}}\n", id_, prev_mode);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_prev_trap\": {}}}\n", id_, prev_trap);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_prev_num_dest\": {}}}\n", id_, prev_num_dest);

  for (auto& csr : csrs) {
    uint64_t csr_data;
    bool valid;
    if (!client_->whisperPeek(id_, 'c', csr.address, csr_data, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart{}: Failed to peek CSR values\n", id_);
    }
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_csr_{}\": \"0x{:x}\"}}\n", id_, csr.name, csr_data);
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

  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_next_instr\": \"{}\"}}\n", id_, next_instr);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_next_mode\": {}}}\n", id_, next_mode);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_next_trap\": {}}}\n", id_, next_trap);
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"h{}_next_num_dest\": {}}}\n", id_, next_num_dest);
  
}
