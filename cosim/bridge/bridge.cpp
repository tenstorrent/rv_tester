// Licensed under the Apache License, Version 2.0, see LICENSE.TT for details
// vim: ft=c et ts=2 sw=0 sts

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
#include <random>

// Plusargs
DECLARE_string(bootrom_path);
DECLARE_string(cplfw_path);
DECLARE_string(load);
DECLARE_string(hex);
DECLARE_bool(mcm);
DECLARE_uint64(debug_entry_pc);
DECLARE_uint64(debug_exit_pc);
DECLARE_uint64(hart_enable_mask);
DECLARE_uint32(num_harts);
DECLARE_bool(random_intr);
DECLARE_bool(random_imsic_intr);

DEFINE_bool(bridge_log, true, "Enable bridge logging");
DEFINE_string(whisper_json_path, "", "Path to whisper json config");
DEFINE_bool(cosim_resynch, false, "Resynch whisper with dut state on every instruction");
DEFINE_string(cosim_resynch_instr, "", "List of instruction mnemonics to resynch whisper with dut state");
DEFINE_string(cosim_resynch_prev_instr, "", "List of instruction mnemonics to resynch whisper with dut state");
DEFINE_string(cosim_resynch_csr, "", "List of csr mnemonics to resynch whisper with dut state"); 
DEFINE_bool(mip_resynch, true, "Resynch whisper with dut state on mip mismatch condition");
DEFINE_bool(imsic_resynch, true, "Resynch whisper with dut state on imsic mismatch condition");
DEFINE_bool(intr_defer_spcl, true, "Defer all interrupts in special cases");
DEFINE_bool(intr_timeout_resynch, true, "Ignore whisper timeout error condition");
DEFINE_bool(retire_ucode_trap, true, "DUT indicates retire on a trap after executing the ucode trap handler");
DEFINE_bool(pc_check, true, "Enable cosim checks on pc");
DEFINE_bool(priv_check, true, "Enable cosim checks on priv mode");
DEFINE_bool(insn_check, true, "Enable cosim checks on insn bytes");
DEFINE_bool(gpr_check, true, "Enable cosim checks on gprs");
DEFINE_bool(fpr_check, true, "Enable cosim checks on fprs");
DEFINE_bool(vec_check, true, "Enable cosim checks on vector regs");
DEFINE_bool(csr_rd_check, true, "Enable cosim checks on csr reads");
DEFINE_bool(csr_wr_check, true, "Enable cosim checks on csr writes");
DEFINE_bool(memattr_check, true, "Enable cosim checks on mem attributes");
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
DEFINE_bool(whisper_cosim_log, false, "Enable whisper logging to iss_cosim.log");
DEFINE_bool(whisper_cmd_log, false, "Enable whisper logging to iss_cmd.log");
DEFINE_bool(whisper_stdin_null, false, "Redirect whisoer stdin to null");
DEFINE_bool(whisper_stdout_null, false, "Redirect whisoer stdout to null");
DEFINE_bool(preload, false, "Whisper preload");

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
    std::string traceFile  = (FLAGS_whisper_log || FLAGS_whisper_cosim_log) ? "iss_cosim.log" : "";
    std::string commandLog = (FLAGS_whisper_log || FLAGS_whisper_cmd_log  ) ? "iss_cmd.log" : "";
    cosim_resynch_csr_defaults = {

      //"htval","mtval2", // RVDE-10043
      "mtinst","htinst", // RVDE-10005
      "vstart","vxsat","vxrm","vcsr", // Unimplemented
      "sstatus","mstatus","hstatus","mie","hie","vsie","sie", // RVDE-11840
      "tselect","tdata1","tdata2","tdata3","mcontext", // Unimplemented: RVDE-7518
      "fflags","fcsr", // Unimplemented
      "menvcfg","senvcfg", // FIXME: pointer masking change
      "pma","pmp", // FIXME: Performant NC change
      "vtype", // Permanent: Vector vtype will not be implemented
      "mip","hip","vsip","hvip","sip","mireg","sireg","vsireg","mtopei","stopei","vstopei", // Permanent: Interrupts
      "hpmcounter","hpmevent","scountovf","mcycle","minstret","minstreth","dcsr" // Permanent: PMC events

    };
    std::istringstream iss(FLAGS_cosim_resynch_csr);
    std::string token;
    while (std::getline(iss, token, ',')) {
        cosim_resynch_csr_defaults.push_back(token);
    }
    client_ = std::make_shared<whisperClient<uint64_t>>(traceFile, commandLog);
    auto platform = cvm::topology::get_from_type("PLATFORM", 0);
    cvm::registry::messenger.connect<rv_tester::terminate_called>(platform, [this] (const auto& v) { return this->process(v); });
    if(FLAGS_random_intr | FLAGS_random_imsic_intr){
       FLAGS_max_cycle = 2*FLAGS_max_cycle;
       cvm::log(cvm::LOW, "Doubling max_cycles for sim run to {}\n",FLAGS_max_cycle );
    }
    int32_t nharts = cvm::topology::attr(platform, "NHARTS").second;
    if(FLAGS_max_stall_cycle < (nharts*3000 + 7000)){
        FLAGS_max_stall_cycle = nharts*3000 + 7000;
        cvm::log(cvm::LOW, "Overwriting max_stall_cycle to {} cycles\n",FLAGS_max_stall_cycle );
    }
    // Overwrite hart_enable_mask in a random fashion based on num_harts run-arg
    // Do this only when hart_enable_mask run-arg is 0x1 (default value)
    if(FLAGS_hart_enable_mask == 0x1){
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<> dis(0, nharts - 1);
      unsigned char hart_enable_mask = 0;
      for (uint32_t i = 0; i < FLAGS_num_harts; ++i) {
          int bit_position;
          do {
              bit_position = dis(gen);
          } while ((hart_enable_mask >> bit_position) & 1); // Check if the bit is already set
          hart_enable_mask |= (1 << bit_position);
      }
      FLAGS_hart_enable_mask = hart_enable_mask;
      cvm::log(cvm::LOW, "Overwriting hart_enable_mask to 0x{:x}\n", FLAGS_hart_enable_mask);
    }
}

// Destructor
bridge::~bridge() {
  report_metrics();
  client_->whisperQuit();
}

void bridge::reset() {

  memmap::get(memmap_);

  cac_.Reset();
  assert(cac_.SetVlen(vlen_));

  if (client_->whisperConnect(num_harts_) != 0) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed whisper_connect\n", id_);
    return;
  }

  bool valid;
  client_->whisperReset(0, valid);

  // Init csr reset values in cac
  csr_init();

  // Write hart enable mask to boot mem
  if (!client_->whisperPoke(id_, 0, 'm', memmap_.at("boot").base + 0x9000, FLAGS_hart_enable_mask, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke boot memory\n", id_);
    return;
  }
  cvm::registry::messenger.signal<uint64_t>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0), uint64_t(0));
  resetsstc_poke(id_,0,0x14d);
  resetsstc_poke(id_,0,0x24d);
}

void bridge::csr_init() {
  bool valid;
  uint64_t data, mask, poke_mask;
  for (const auto& csr: nonzero_reset_csrs) {
    if (!client_->whisperPeekCsr(id_, csr.address, data, mask, poke_mask, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek csr\n", id_);
    }
    size_8_bytes_t cac_mask = 0xffffffffffffffff;
    update_csr(id_, src_t::dut, csr.address, data, cac_mask);
    update_csr(id_, src_t::iss, csr.address, data, cac_mask);
    csr_cac_.Step(id_, false);
  }
}

void bridge::setsstc_poke(hart_id_t hart, uint64_t cycle, uint64_t csr) {
  bool valid;
  if (!client_->whisperPoke(hart, cycle, 'c', csr, 0, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke timecmp csr\n", id_);
    return;
  }
}
void bridge::resetsstc_poke(hart_id_t hart, uint64_t cycle, uint64_t csr) {
  bool valid;
  if (!client_->whisperPoke(hart, cycle, 'c', csr, 0xffffffff, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke timecmp csr\n", id_);
    return;
  }
}

// DUT interface callback: Instruction Retire
void bridge::process_dut_instr_retire(hart_id_t hart, rv_instr_t& d) {

  // cvm::log(cvm::NONE, "Inside the process_dut_instr_retire function\n"); 


  twoStage_ = false;

  whisper_state_t w {
    .tag = d.tag,
    .time = d.cycle
  };

  // Handle debug interrupt
  if (d.intr && (d.icause == 0)){
    return;
  }

  // Handle pre-step condition - Debug
  if (debug_mode_) {
    if (FLAGS_emulate_debug_mode) {
      pre_step_debug_poke(hart, d);
    } else {
      return;
    }
  }
  // Handle pre-step condition - Interrupts
  pre_step_interrupt_poke(hart, d, w);
  lrsc_fail_ = false;

  // Handle pre-step condition - LR/SC fail
  pre_step_lrsc_poke(hart, d);

  // Step whisper
  w_.clear();
  step(hart, w);

  // Update cac with whisper state
  update_whisper_state(hart, w);

  // Update cac with dut state
  update_dut_state(hart, d);

  arch_state(w);

  // Handle post-step conditions
  post_step_interrupt_poke(hart, d, w);
  //if(!debug_mode_){
  post_step_exception_poke(hart, d, w);
  //}
  post_step_satp_write_poke(hart, d, w);

  // Check dut vs whisper
  const auto cac_status_verbosity = cvm::HIGH;
  if (!excp_in_debug_mode) {
    cac_.Step(hart, cvm::logger::check_verbosity(cac_status_verbosity));
  } else {
    cac_.ResetStatus(hart);
    return;
  }

  // Increment step count
  step_++;

  // Save whisper state
  ppw_ = pw_;
  pw_ = w;

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
      std::string resource = cac_.GetResourceStr(hart);
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
        cvm::log(cvm::ERROR, "Error: Hart {}: Core Arch Checker Mismatch - {} - {}\n", hart, resource,  instr);
        return;
      }
    }
  }
  else {
    log(cac_status_verbosity, "{}", cac_.GetStatusStr(hart));
  }

  // Save whisper state
  prev_mip_ = mip_;
  prev_e_mip_ = e_mip_;

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
  if (!FLAGS_csr_wr_check)
    return;

  const auto cac_status_verbosity = cvm::HIGH;
  // Step csr cac
  csr_cac_.Step(hart, cvm::logger::check_verbosity(cac_status_verbosity));

  if (resynch_csr_) {
    csr_cac_.ResetStatus(hart);
    resynch_csr_ = false;
  }

  // Error on mismatch
  if (!csr_cac_.GetStatus(hart)) {
    std::string csr = get_csr_name(csr_cac_.GetResourceStr(hart).substr(2));
    csr_cac_.ResetStatus(hart);
    if (FLAGS_cosim_resynch) {
      resynch(hart, d);
    } else {
      for (const auto& token_csr : cosim_resynch_csr_defaults) {
        if (csr.find(token_csr) != std::string::npos){
          return;
        }
      }
      for (auto & i : d.instrs)
        print_instr_stdout(hart, i);
      cvm::log(cvm::NONE, "{}", csr_cac_.GetStatusStr(hart));
      cvm::log(cvm::ERROR, "Error: Hart {}: CSR Write Mismatch - {}\n", hart, csr);
      return;
    }
  }
  else {
      log(cac_status_verbosity, "{}", csr_cac_.GetStatusStr(hart));
  }

}

void bridge::update_dut_state(hart_id_t hart, rv_instr_t& d) {
  if (FLAGS_pc_check) {
    update_pc(hart, src_t::dut, d.pc.pc_rdata);
  }
  if (FLAGS_priv_check) {
    update_priv(hart, src_t::dut, d.priv);
  }
  if (FLAGS_insn_check && !d.comp && !d.ucode && !is_vector(d.disasm) && !(d.disasm.substr(0,7)=="illegal")) {
    update_insn(hart, src_t::dut, d.opcode);
  }
  if (d.gpr.valid || d.fpr.valid || !d.vr.empty() || !d.csr.empty()) {
    update_regs(hart, d);
  }
  if (FLAGS_memattr_check && d.mem_read.valid && (!is_vector(d.disasm)) && !lrsc_fail_) {
      update_mem_attr(hart, src_t::dut, d.mem_read.attr);
  }
  if (FLAGS_memattr_check && d.mem_write.valid && (!is_vector(d.disasm)) && !lrsc_fail_) {
    if(!(((d.opcode & 0x7F) == 0x2F) && (d.opcode & 0xF8000000) == 0x18000000 && ((d.opcode & 0x00000F80) == 0x0))) {
      update_mem_attr(hart, src_t::dut, d.mem_write.attr);
    }
  }
}


void bridge::pre_step_debug_poke(hart_id_t hart, const rv_instr_t& instr) {
  cvm::log(cvm::NONE, "Debug pre step poking instruction in Debug mode\n", hart); 
  bool valid;
  if (!client_->whisperPoke(hart, 0, 'm', instr.pc.pc_rdata, instr.opcode, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke memory\n", hart);
    return;
  }
  return;
}

void bridge::pre_step_lrsc_poke(hart_id_t hart, const rv_instr_t& d) {
  // https://en.wikipedia.org/wiki/Load-link/store-conditional
  if ((d.disasm.find("sc.w") != std::string::npos) ||
      (d.disasm.find("sc.d") != std::string::npos)) {
    // Check if Store-Conditional (SC) failed
    uint64_t fail_code = 1;
    if (d.mem_read.data == fail_code) {
      lrsc_fail_ = true;
      bool valid;
      // Cancel Load-Reserved (LR)
      if (!client_->whisperCancelLr(hart, valid)) {
        cvm::log(cvm::ERROR, "Error: Hart {}: Failed to CancelLr\n", hart);
      }
    }
  }
}

void bridge::pre_step_interrupt_poke(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {
// FIXME We are deferring all interrupts, if new interrupt was made possible due to execution of a csr op previously
  if (FLAGS_intr_defer_spcl) {
    if (d.disasm.find("csr") != std::string::npos) {
      bool valid;
      if (!client_->whisperPeek(hart, 's', WhisperSpecialResource::DeferredInterrupts, deferred_mip_, valid)) {
        cvm::log(cvm::ERROR, "Error: Hart {}: Failed whisper API call - whisperGetDeferredInterrupts\n", hart);
        return;
      }
      if (prev_sync_intr_) {
        log(cvm::MEDIUM, "<{}> All interrupts Defer\n", d.cycle);
        all_interrupts_defer_ = true;
        pre_csr_defermip_ = deferred_mip_;
        deferred_intr_ = true;
        defer_interrupt(hart, d.cycle, mip_);
      }
      prev_sync_intr_ = 0;
      uint64_t undeferred_mip = mip_ & ~ deferred_mip_;
      uint64_t undeferred_w_cause;
      check_interrupt(hart, undeferred_mip, pre_undeferred_intr_, undeferred_w_cause);
    }
  }

  if (!mip_ && !prev_mip_)
    return;

  bool w_intr;
  uint64_t w_cause;
  check_interrupt(hart, mip_, w_intr, w_cause);

  if (!d.intr && !w_intr)
    return;

  if (!d.intr && w_intr) {
    intr_age_[w_cause]++;
    log(cvm::HIGH, "<{}> intr_age_[{}][{}]++={}\n", w.time, hart, w_cause, intr_age_[w_cause]);

    // Check that interrupt age is not beyond threshold
    if ((intr_age_[w_cause] > FLAGS_max_pend_intr_age) && !FLAGS_cosim_resynch && !FLAGS_intr_timeout_resynch) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Whisper wants to take interrupt, DUT does not. cause: [{}], timeout: [{}] retires\n",
        hart, w_cause, FLAGS_max_pend_intr_age);
    }
    return;
  }

  if (FLAGS_bridge_log) {
    log(cvm::MEDIUM, "<{}> Interrupt taken by DUT. dcause:[{}] wcause:[{}]\n", w.time, d.icause, w_cause);
  }

  // Currently for interrupts taken to VS mode, w_cause and d.icause differ by 1
  // We will calculate next privilige mode to address cause mismatch issue and also for printing interrupt stats

  bool valid;
  uint64_t hideleg, mideleg;
  if (!client_->whisperPeek(hart, 'c', 0x303, mideleg, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek mip\n", hart);
    return;
  }
  if (!client_->whisperPeek(hart, 'c', 0x603, hideleg, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek mip\n", hart);
    return;
  }
  bool hdel = hideleg & (1ull << w_cause);
  bool mdel = mideleg & (1ull << w_cause);
  if(d.priv == 3) {intrtopriv_ = 3;}                                                 // M mode
  else if (d.priv == 1 || d.priv == 0) { intrtopriv_ = mdel ? 1 : 3;}                // HS or U mode
  else if (d.priv == 9 || d.priv == 8) { intrtopriv_ = mdel ? (hdel ? 9 : 1) : 3;}   // VS or VU mode

  if (intrtopriv_ == 9 || intrtopriv_ == 8) {w_cause--;}

  log(cvm::MEDIUM, "<{}> Interrupt to privilege {} \n", w.time, intrtopriv_);

  // Timing sensitive resynch cases
  // 1. DUT took older interrupt that deasserted before retire
  if (d.intr && !w_intr && !FLAGS_cosim_resynch) {
    check_interrupt(hart, prev_mip_, w_intr, w_cause);
    if (w_intr && (w_cause == d.icause)) {
      log(cvm::MEDIUM, "<{}> DUT took interrupt, Whisper did not. cause:[{}] (Timing sensitive mismatch: Resynch and keep going)\n", w.time, d.icause);
      poke_mip(hart, w.time, (uint64_t)1 << d.icause);
      resynch_icause_ = d.icause;
      // Undefer all interrupts
      if (deferred_intr_) {
        defer_interrupt(hart, w.time, 0);
        deferred_intr_ = false;
      }

    } else {
      cvm::log(cvm::ERROR, "Error: Hart {}: DUT took interrupt, Whisper did not. cause:[{}]\n", hart, d.icause);
    }
    return;
  }

  // 2. DUT took older interrupt but a newer one asserted before retire
  if (d.icause != w_cause) {
    check_interrupt(hart, prev_mip_, w_intr, w_cause);
    if (w_intr && (w_cause == d.icause)) {
      log(cvm::MEDIUM, "<{}> DUT vs Whisper interrupt cause mismatch [{},{}] age [{},{}] (Timing sensitive mismatch: Resynch and keep going)\n",
        w.time, d.icause, w_cause, intr_age_[d.icause], intr_age_[w_cause]);
      defer_interrupt(hart, w.time, mip_ & ~((uint64_t)1 << d.icause));
    }
    return;
  }

  // Undefer all interrupts
  if (deferred_intr_) {
    defer_interrupt(hart, w.time, 0);
    deferred_intr_ = false;
  }

  if (FLAGS_retire_ucode_trap)
    return;

  step(hart, w);
  if (FLAGS_bridge_log) {
    log(cvm::MEDIUM, "<{}> Whisper Step #{}: Extra step due to interrupt\n", w.time, step_);
  }
}

void bridge::post_step_interrupt_poke(hart_id_t hart, const rv_instr_t& d, const whisper_state_t& w) {

  if (FLAGS_intr_defer_spcl) {
    if (d.disasm.find("csr") != std::string::npos) {
       uint64_t undeferred_mip = mip_ & ~ deferred_mip_;
       uint64_t undeferred_w_cause;
       check_interrupt(hart, undeferred_mip, post_undeferred_intr_, undeferred_w_cause);
       prev_sync_intr_ = post_undeferred_intr_ && !pre_undeferred_intr_;
    }

    if (all_interrupts_defer_) {
      defer_interrupt(hart, d.cycle, pre_csr_defermip_);
      all_interrupts_defer_ = false;
    }

    if ((w.disasm.find("mret") != std::string::npos) || (w.disasm.find("sret") != std::string::npos)) {
      if(prev_mip_ != mip_) {
        check_and_defer_interrupt(hart, d.cycle, ~prev_mip_ & mip_);
      }
      prev_sync_intr_ = true; // This will waive cases when after execution of mret there exists a csr operation which needs to be interrupted.
    }

    if (w.disasm.find("vsstimecmp") != std::string::npos)  {
      if (!vstimecmppoked_) resetsstc_poke(hart,d.cycle, 0x24d); else setsstc_poke(hart,d.cycle, 0x24d);
    } else if (w.disasm.find("stimecmp") != std::string::npos) {
      if (w.priv_mode == 9) {if (!vstimecmppoked_) resetsstc_poke(hart,d.cycle, 0x24d); else setsstc_poke(hart,d.cycle, 0x24d);}
      else if (!stimecmppoked_)  resetsstc_poke(hart,d.cycle, 0x14d); else setsstc_poke(hart,d.cycle, 0x14d);
    }
  }


  if (!d.intr && !w_.intr)
    return;

  if (intr_age_[w_.icause] > max_pend_intr_age_)
    max_pend_intr_age_ = intr_age_[w_.icause]; 

  intr_age_[w_.icause] = 0;

  // If interrupt asserted via csr write, we don't need to defer
  // DUT is expected to take at retire boundary if whisper takes the undeferred interrupt
  if (w_.intr && !d.intr && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    cvm::log(cvm::ERROR, "Error: Hart {}: Whisper took interrupt, DUT did not. cause:[{}]\n", hart, w_.icause);
    return;
  }

  if (d.intr && !w_.intr && !FLAGS_cosim_resynch) {
    // If Debug mode intterupt is seen, don't flag an error, Whisper gets poked based on PC fetches
    if (d.icause == 0) 
      return;

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
  if (resynch_icause_) {
    uint64_t resynch_mip_mask, resynch_mip;
    resynch_mip_mask = (1 << resynch_icause_);
    resynch_icause_ = 0;
    peek_mip(hart, d.cycle, resynch_mip);
    resynch_mip &= ~resynch_mip_mask;
    log(cvm::MEDIUM, "<{}> Poking mip de assertion due to resynch in previous step {} \n", d.cycle, resynch_mip);
    poke_mip(hart, d.cycle, resynch_mip);
  }

  num_taken_interrupts_[intrtopriv_][w_.icause]++;
}

void bridge::post_step_exception_poke(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {

  if (!d.excp && !w_.excp)
    return;

  if (d.excp && is_custom_excp(d.ecause)) {
    log(cvm::MEDIUM, "<{}> Custom exception detected: {}\n", d.cycle, d.ecause);
    // Vector conservative mode
    if (d.ecause == 55)
      resynch(hart, d);
    return;
  }
  
  if(debug_mode_ && FLAGS_emulate_debug_mode && (d.excp )){
    excp_in_debug_mode = true;
    return;
  }else{
    excp_in_debug_mode = false;
  }
  
  log(cvm::MEDIUM, "<{}> Exception detected. dut:[{}, {}] whisper:[{}, {}]\n", w.time, d.excp, d.ecause, w_.excp, w_.ecause);

  if (d.excp && !w_.excp && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    cvm::log(cvm::ERROR, "Error: Hart {}: DUT took exception, Whisper did not. Cause: {}\n", hart,
      excp_to_string.count(static_cast<excp>(d.ecause)) ? excp_to_string.at(static_cast<excp>(d.ecause)) : std::to_string(d.ecause));
    return;
  }
  
  if (w_.excp && !d.excp && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    cvm::log(cvm::ERROR, "Error: Hart {}: Whisper took exception, DUT did not. Cause: {}\n", hart,
      excp_to_string.count(static_cast<excp>(w_.ecause)) ? excp_to_string.at(static_cast<excp>(w_.ecause)) : std::to_string(w_.ecause));
    return;
  }

  if (d.excp && w_.excp && (d.ecause != w_.ecause) && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    cvm::log(cvm::ERROR, "Error: Hart {}: DUT vs Whisper exception cause mismatch. Dut: {}, Whisper: {}\n", hart,
      excp_to_string.count(static_cast<excp>(d.ecause)) ? excp_to_string.at(static_cast<excp>(d.ecause)) : std::to_string(d.ecause),
      excp_to_string.count(static_cast<excp>(w_.ecause)) ? excp_to_string.at(static_cast<excp>(w_.ecause)) : std::to_string(w_.ecause));
    return;
  }

  num_exceptions_++;

  // If DUT indicates retire on ucode trap handler, extra step not needed
  if (FLAGS_retire_ucode_trap)
    return;

  step(hart, w);
  if (FLAGS_bridge_log) {
    log(cvm::MEDIUM, "<{}> Whisper Step #{}: Extra step due to exception\n", w.time, step_);
  }
  update_whisper_state(hart,w);
}

bool bridge::is_custom_excp(uint64_t cause) {
  return (cause >= 25 && cause <= 55);
}

void bridge::post_step_satp_write_poke(hart_id_t hart, const rv_instr_t& d, const whisper_state_t& w) {
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
  w_.comp = is_compressed(w.disasm);
  w_.ucode = is_ucode(w.disasm) || w.trap; // system opcode
  w_.mem_read.valid = w.is_load;
  w_.pc.valid = true;
  w_.pc.pc_rdata = w.pc;

  if (FLAGS_pc_check)
    update_pc(hart, src_t::iss, w.pc);

  if (FLAGS_priv_check)
    update_priv(hart, src_t::iss, w.priv_mode);

  // FIXME Instruction byte checking disabled for vectors till we find a way to
  // differentiate cracked instructions
  if (FLAGS_insn_check && !w_.comp && !w_.ucode && !is_vector(w.disasm) && !(w.disasm.substr(0,7)=="illegal"))
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
      // w_.vr.valid = true;
      // w_.vr.vrd_addr = w.address;
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

  // Mem attributes
  // Disabling mem_attr checks for vectors currently
  if (FLAGS_memattr_check && !w_.trap && !is_vector(w.disasm) && (w_.mem_read.valid || w_.mem_write.valid)) {
    bool valid; 
    uint64_t eff_mem_attr;
    if (!client_->whisperPeek(hart, 's', WhisperSpecialResource::EffMemAttr, eff_mem_attr, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Failed whisper API call - whisperEffMemAttr\n", hart);
      return;
    }
  
    update_mem_attr(hart, src_t::iss, eff_mem_attr);
  }

  // Interrupts/Exceptions
  if (w_.trap) {
    uint64_t cause = 0;
    for (auto& c : w_.csr) {
      if ((c.csr_addr == 0x342) || (c.csr_addr == 0x142) || (((w.priv_mode == 0x8) || (w.priv_mode == 0x9)) && (c.csr_addr == 0x242)))
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
      w.priv_mode, w.fp_flags, w.trap, w.stop, w.is_load)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to step whisper\n", hart);
    return;
  }

  // Print instruction
  if (FLAGS_bridge_log) {
    print_instr(hart, w);
  }
}

std::vector<bridge::size_8_bytes_t> create_dword_vec(const std::bitset<256>& input) {
    // Calculate the number of 8-byte chunks needed for the 256-bit input
    size_t num_chunks = (256 + 63) / 64; // Round up division

    // Create a vector to store the chunks
    std::vector<bridge::size_8_bytes_t> dword_vec(num_chunks);

    // Convert and store each chunk of 64 bits (8 bytes)
    for (size_t i = 0; i < num_chunks; ++i) {
        bridge::size_8_bytes_t chunk = 0;
        for (size_t j = 0; j < 64; ++j) {
            size_t bit_index = i * 64 + j;
            if (bit_index < 256 && input[bit_index]) {
                chunk |= (bridge::size_8_bytes_t(1) << j);
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
  if (FLAGS_vec_check) {
    for (auto & vr : d.vr) {
      if (vr.valid){
        update_regs(hart, src_t::dut, resource_t::vec_reg, vr.vrd_addr, create_dword_vec(vr.vrd_wdata));
      }
    }
  }

  // CSR
  for (auto & c : d.csr) {
    uint64_t data = modify_csr_data(hart, c.csr_addr, c.csr_wdata);
    size_8_bytes_t mask = modify_csr_mask(hart, c.csr_addr, c.csr_wdata, c.csr_wmask);
    if (FLAGS_csr_rd_check) {
      update_csr(hart, src_t::dut, c.csr_addr, data, mask);
      if (c.csr_addr == 0x001) update_csr(hart, src_t::dut, 0x003, data, mask); // On fflags update, update fcsr
      else if (c.csr_addr == 0x002) { // On frm update, update fcsr
        data = data << 5;
        mask = mask << 5;
        update_csr(hart, src_t::dut, 0x003, data, mask, false, false);
      }
      else if (c.csr_addr == 0x003){ // On fcsr update, update fflags,frm
          bridge::size_8_bytes_t mask_fcsr = mask;
        mask = mask_fcsr & 0x1f;
        update_csr(hart, src_t::dut, 0x001, data, mask, false, false);
        data = data >> 5;
        mask = (mask_fcsr >> 5) & 0x7;
        update_csr(hart, src_t::dut, 0x002, data, mask, false, false);
      }
      else if (c.csr_addr == 0x301){ // On misa.H update, update mideleg
        if ((c.csr_wmask >> 7) & 0x1) {
          mask = 0x1444;
          if ((c.csr_wdata >> 7) & 0x1) update_csr(hart, src_t::dut, 0x303, 0x1444, mask, false, false);
          else update_csr(hart, src_t::dut, 0x303, 0, mask, false, false);
        }
      }
    }
  }
}

// Push DUT mem attr to cac 
// Currently disabling mem_attr checks for vectors
void bridge::update_mem_attr(hart_id_t hart, src_t src, uint32_t data) {
  resource_id_t mem_attr = resource_id_t{
    .resource = resource_t::mem_attr,
    .offset = 0
  };
  // Supported sttributes - [type:11, cacheability:12]
  uint32_t masked_data = data & 0x1800;
  assert(cac_.SetResource(hart, src, mem_attr, std::move(cac::CreateBitVec<uint64_t>(masked_data))));
}

std::bitset<256> create_bitset(bridge::size_8_bytes_t dword_vec_array [vlen/64]) {
    std::bitset<256> result;

    // Iterate through the array and concatenate each value to the result bitset
    for (size_t i = 0; i < vlen/64; ++i) {
        for (size_t j = 0; j < 64; ++j) {
            size_t bit_index = i * 64 + j;
            bool bit_value = (dword_vec_array[i] & (bridge::size_8_bytes_t(1) << j)) != 0;
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
  std::vector<uint64_t> csrsupdatingmip = {0x144, 0x351, 0x251, 0x151, 0x35c, 0x25c, 0x15c, 0x30a};

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
          update_regs(hart, src_t::iss, resource_t::vec_reg, w.address, std::vector<bridge::size_8_bytes_t>(dword_vec_array, dword_vec_array + sizeof(dword_vec_array)/sizeof(dword_vec_array[0])));
          vr_t vr;
          vr.valid = true;
          vr.vrd_addr = w.address;
          vr.vrd_wdata = create_bitset(dword_vec_array);
          w_.vr.push_back(vr);
        }
      }
      break;
    case 'c':
      if (FLAGS_csr_rd_check){
        // Check if PMP entry is locked
        if (w.address >= 0x3B0 && w.address < 0x3C0) {
          bool valid;
          uint64_t pmpcfg, mask, reset;
          uint64_t i, pmp_cfg_reg, pmp_cfg_index;
          // For PMP addresses, which bits of the pmpcfgs to look for 
          i = w.address - 0x3B0;
          pmp_cfg_reg = ((i*8) / 64) * 2;
          pmp_cfg_index = (i*8) % 64;
          client_->whisperPeekCsr(hart, 0x3A0 + pmp_cfg_reg, pmpcfg, mask, reset, valid);
          if((pmpcfg >> (pmp_cfg_index + 7)) & 0x1) {
            break;
          }
        }
        update_csr(hart, src_t::iss, w.address, w.value);
      }

      if (w.address == 0x344) {
        uint64_t w_seip;
        peek_seip(hart, w.time, w_seip);
        mip_ = w.value | w_seip << 9;
        e_mip_ = mip_ & 0x1e00;
        log(cvm::MEDIUM, "<{}> Zicsr write based interrupt: mip {:#x}\n", w.time, w.value);
      }
      // Whisper is not doing recordwrite of mip if change happens to it through sip, *ireg, *topei
      for (size_t i = 0; i < csrsupdatingmip.size(); ++i) {
        if (csrsupdatingmip[i] == w.address) {
            uint64_t w_seip;
            peek_seip(hart, w.time, w_seip);
            peek_mip(hart, w.time , mip_);
            mip_ |= w_seip << 9;
            e_mip_ = mip_ & 0x1e00;
            log(cvm::MEDIUM, "<{}> Zicsr write based interrupt: shadow update to mip {:#x}\n", w.time, mip_);
            break;
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
  assert(cac_.SetResource(hart, src, pc, std::move(cac::CreateBitVec<uint64_t>(data))));
}

void bridge::update_insn(hart_id_t hart, src_t src, uint32_t data) {
  resource_id_t insn = resource_id_t{
    .resource = resource_t::insn_bytes,
    .offset = 0
  };
  assert(cac_.SetResource(hart, src, insn, std::move(cac::CreateBitVec<uint64_t>(data))));
}

void bridge::update_priv(hart_id_t hart, src_t src, uint32_t data) {
  resource_id_t priv = resource_id_t{
    .resource = resource_t::priv_mode,
    .offset = 0
  };
  assert(cac_.SetResource(hart, src, priv, std::move(cac::CreateBitVec<uint64_t>(data))));
}

void bridge::update_regs(hart_id_t hart, src_t src, resource_t resource, uint64_t addr, const std::vector<size_8_bytes_t>&& dword_vec) {
  if ((src == src_t::dut) && (resource == resource_t::int_reg) && (addr == 0)) {
    return;
  }
  resource_id_t rid = resource_id_t{
    .resource = resource,
    .offset = addr
  };
  assert(cac_.SetResource(hart, src, rid, std::move(cac::CreateBitVec<size_8_bytes_t>(dword_vec))));
}

bool bridge::disable_pa_check_vec(hart_id_t hart) {
  bool valid;
  uint64_t data, mask, poke_mask;
  uint64_t vl = 0;
  uint64_t vtype ;
  uint64_t vlmax = 0;

  if(client_->whisperPeekCsr(hart,0xc21, data, mask, poke_mask, valid)) {
  
  vtype = data & mask; // getting the vtype csr
  int sew_enc = (vtype & 0x38) >> 3; // encoded sew
  int sew;

  if (sew_enc == 0) sew = 8;
  if (sew_enc == 1) sew = 16;
  if (sew_enc == 2) sew = 32;
  if (sew_enc == 3) sew = 64;

  int vlmul_enc = (vtype & 0x7);

  if (vlmul_enc == 0) 
    vlmax = 256/sew ;
  if (vlmul_enc == 1) 
    vlmax = 512/sew ;
  if (vlmul_enc == 2) 
    vlmax = 1024/sew ;
  if (vlmul_enc == 3) 
    vlmax = 2048/sew ;
  if (vlmul_enc == 5) 
    vlmax = 256/(8*sew);
  if (vlmul_enc == 6) 
    vlmax = 256/(4*sew);
  if (vlmul_enc == 7) 
    vlmax = 256/(2*sew);

}

if(client_->whisperPeekCsr(hart,0xc20, data, mask, poke_mask, valid)) 
  vl = data & mask;

if(vl < vlmax)
  return true;  
return false;

}

void bridge::arch_state(whisper_state_t& w) {

  if (w.resource == 'c') {
      if(w.address == 0x300)
      {
          if(((w.value) & 0x20000) != 0)
            {
              mprv_ = 1;
              mpp_ = ((w.value) & 0x1800) >> 11; 
            }
            else
              mprv_ = 0;
        }
      }
  }


bool bridge::is_vector(const std::string& instr) {
  if (instr.substr(0,1) == "v")
    return true;
  return false;
}

bool bridge::is_compressed(const std::string& instr) {
  if (instr.substr(0,2) == "c.")
    return true;
  return false;
}

bool bridge::is_ucode(const std::string& instr) {
  if ((instr.find("mret") != std::string::npos) ||
      (instr.find("sret") != std::string::npos) ||
      (instr.find("ecall") != std::string::npos) ||
      (instr.find("ebreak") != std::string::npos))
    return true;
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
  if (debug_mem_access(d)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[debug mem access]\n", d.cycle);
    return true;
  }
  // Case #5
  if (boot_read(d)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[boot_read]\n", d.cycle);
    return true;
  }
  // Case #6
  if (FLAGS_mip_resynch && mip_mismatch(instr)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[mip_mismatch]\n", d.cycle);
    return true;
  }
  // Case #7
  if (FLAGS_imsic_resynch && imsic_mismatch(instr)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[imsic_mismatch]\n", d.cycle);
    return true;
  }
  // Case #8
  if (unsupported_mmr_access(d)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[mmr_access]\n", d.cycle);
    return true;
  }
  // Case #9

  if (d.intr && (d.icause == 0)){
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[Debug Mode Interrupt]\n", d.cycle);
   return true;
  }
  if (unsupported_csr_access(instr)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[unsupported_csr_access]\n", d.cycle);

    return true;
  }
  return false;
}

bool bridge::clint_read(const rv_instr_t& d) {
  if (d.mem_read.valid) {
    for (const auto& s : {"clint", "aclint"}) {
      auto it = memmap_.find(s);
      if (it != memmap_.end()) {
        if (d.mem_read.pa >= it->second.base && d.mem_read.pa < it->second.end) {
          return true;
        }
      }
    }
  }
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

bool bridge::imsic_mismatch(const std::string& instr) {
  if ((instr.find("top") != std::string::npos) &&
      (e_mip_ != prev_e_mip_))
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

bool bridge::unsupported_mmr_access(const rv_instr_t& d){
  if (d.mem_read.valid &&
      d.mem_read.pa >= mmr_lo_addr &&
      d.mem_read.pa < mmr_hi_addr)
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
      (instr.find("stimecmp") != std::string::npos) ||
      (instr.find("hpmevent") != std::string::npos) || //FIXME: poke events to whisper
      (instr.find("scountovf") != std::string::npos) ||//FIXME: poke events to whisper
      (instr.find("cycle") != std::string::npos))
    return true;
  return false;
}

bool bridge::unsupported_csr_access(const std::string& instr) {
  if ((instr.find("as_dbg_mux_sel") != std::string::npos))
    return true;
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
      cvm::log(cvm::MEDIUM, "addr {:#x} data {:#x} \n", csr.csr_addr, get_csr(hart, src_t::dut, csr.csr_addr));
      if (csr.csr_addr==0x15c || csr.csr_addr==0x25c || csr.csr_addr==0x35c) return;
      if (!client_->whisperPoke(hart, d.cycle, 'c', csr.csr_addr, get_csr(hart, src_t::dut, csr.csr_addr), valid)) {
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

// Process inst fetches
void bridge::process_dut_mcm_ifetch(hart_id_t hart, mem_t& m) {
  bool valid = false;

  if (!client_->whisperMcmIFetch(hart, m.cycle, m.pa, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed mcm ifetch\n", hart);
    return;
  }
  log(cvm::HIGH, "<{}> mcm_ifetch [valid={}, addr={:#x}]\n", m.cycle, valid, m.pa);
}

// Process inst evicts
void bridge::process_dut_mcm_ievict(hart_id_t hart, mem_t& m) {
  bool valid = false;

  if (!client_->whisperMcmIEvict(hart, m.cycle, m.pa, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed mcm ievict\n", hart);
    return;
  }
  log(cvm::HIGH, "<{}> mcm_ievict [valid={}, addr={:#x}]\n", m.cycle, valid, m.pa);
}

uint64_t bridge::translate(hart_id_t hart, uint64_t va, uint8_t priv, memclass_t memclass) {
  uint64_t pa = va;

  if (priv == 0x3)
    return pa;

  bool valid;
  bool r = (memclass == memclass_t::read);
  bool w = (memclass == memclass_t::write);
  bool x = (memclass == memclass_t::fetch);
  bool sup = ((priv & 0x11) == 0x1); // made a change here

  if((priv & 0x8) != 0)
  {
    twoStage_ = true;
  }
  else
  {
    twoStage_ = false;
  }

if (!client_->whisperTranslate(hart, va, r, w, x, twoStage_, sup, pa, valid)) {
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

  uint64_t pa;
  uint64_t va = d.mem_va;
  uint64_t bit57 = va & (1ull << 56);
  va &= ((1ull << 57) - 1);             // Clear all bits to the left of 57th bit
  if (bit57) {  va |= (~0ull) << 57; } // sign extend the 57th bit to [63:58]

if((mprv_ == 1) && w.priv_mode == 3)
  {
    pa = translate(hart, va, mpp_, memclass_t::read);
  }
  else
    pa = translate(hart, va, w.priv_mode, memclass_t::read);
  
  if (pa != d.mem_pa){
    cvm::log(cvm::NONE, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, PC={:#x}, VA={:#x}, RTL-PA={:#x}, ISS-PA={:#x}]\n", w.time, step_-1, hart, w.priv_mode, w.tag, w.pc, d.mem_va, d.mem_pa, pa);
      //cvm::log(cvm::ERROR, "Error: Hart {}: PA MISMATCH !! :\n", hart);
    if(is_vector(d.disasm) && disable_pa_check_vec(hart));
    
    else {
    cvm::log(cvm::ERROR, "Error: Hart {}: PA MISMATCH !! :\n", hart);
    }

    return;
  }
  else {
    log(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, PC={:#x}, VA={:#x}, PA={:#x}]\n", w.time, step_-1, hart, w.priv_mode, w.tag, w.pc, d.mem_va, pa);
  }

}

// Interrupts
void bridge::process_dut_interrupt(hart_id_t hart, rv_intr_t& i) {
  if (i.mip_mask & 0x1e00) {
    process_external_interrupt(hart, i);
  } 
  if (i.mip_mask & 0x20ee) {
    process_timer_sw_interrupt(hart, i);
  }
}

void bridge::process_external_interrupt(hart_id_t hart, rv_intr_t& i) {
    mip_ = (i.mip & i.mip_mask) | (mip_ & ~i.mip_mask);
    e_mip_ = mip_ & 0x1e00;
    check_and_defer_interrupt(hart, i.cycle, i.mip_assert);
  log(cvm::MEDIUM, "<{}> External interrupt: Hart {} mip {:#x} mask {:#x} assert {:#x}\n", hart, i.cycle, i.mip, i.mip_mask, i.mip_assert);
}

void bridge::process_timer_sw_interrupt(hart_id_t hart, rv_intr_t& i) {
  log(cvm::MEDIUM, "<{}> Timer/Sw interrupt: mip {:#x} mask {:#x} assert {:#x}\n", i.cycle, i.mip, i.mip_mask, i.mip_assert);

  // Ideally, would have liked to poke mip with a mask
  // Since we can't, doing a rmw instead
  uint64_t mip, mip_mask;
  // ~0x1e60: Currently wires from harness for mip.STIP and mip.VSTIP are
  // outputs of MS which only convey SSTC based writes we should avoid poking of mip from MS during menvcfg.stce=0
  mip_mask = i.mip_mask & (~0x1e60); 

  // POKE 0x14d 0x24d
  if(i.mip & i.mip_assert & 0x20) { setsstc_poke(hart, i.cycle, 0x14d); stimecmppoked_ = true; }
  if(i.mip & i.mip_assert & 0x40) { setsstc_poke(hart, i.cycle, 0x24d); vstimecmppoked_ = true;}

  if(i.mip_mask & 0x20) {
  uint64_t menvcfg;
  bool valid;
  if (!client_->whisperPeek(hart, 'c', 0x30a, menvcfg, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek mip\n", hart);
    return;
  }
  if (static_cast<int64_t>(menvcfg) < 0) mip_mask |= 0x20;
  }

  if(i.mip_mask & 0x40) {
  uint64_t menvcfg, henvcfg;
  bool valid;
  if (!client_->whisperPeek(hart, 'c', 0x30a, menvcfg, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek mip\n", hart);
    return;
  }
  if (!client_->whisperPeek(hart, 'c', 0x60a, henvcfg, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek mip\n", hart);
    return;
  }
  if (static_cast<int64_t>(menvcfg) < 0 && static_cast<int64_t>(henvcfg) < 0) mip_mask |= 0x40;
  }

  // POKE 0x14d 0x24d
  if( ~i.mip_assert & i.mip_mask & 0x20) { resetsstc_poke(hart, i.cycle, 0x14d); stimecmppoked_ = false; }
  if( ~i.mip_assert & i.mip_mask & 0x40) { resetsstc_poke(hart, i.cycle, 0x24d); vstimecmppoked_ = false; }
  peek_mip(hart, i.cycle, mip);
  mip_ = (mip & ~mip_mask) | (i.mip & mip_mask); 
  poke_mip(hart, i.cycle, mip_);

  uint64_t w_seip;
  peek_seip(hart, i.cycle, w_seip);
  mip_ |= w_seip << 9;

  // Defer interrupt only on 0->1 transition
  check_and_defer_interrupt(hart, i.cycle, i.mip_assert);
}

void bridge::process_dut_imsic_msi(hart_id_t hart, mem_t& m) {
  log(cvm::MEDIUM, "<{}> IMSIC interrupt: [addr={:#x} data={:#x}]\n", m.cycle, m.pa, m.data);

  // Poke imsic write into whisper memory
  bool valid;
  if (!client_->whisperPokeMem(hart, m.cycle, 'm', m.pa, 4, m.data, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke memory\n", hart);
    return;
  }

  // Peek mip to check if expected to be taken
  peek_mip(hart, m.cycle, mip_);
  uint64_t w_seip;
  peek_seip(hart, m.cycle, w_seip);
  mip_ |= w_seip << 9;
  e_mip_ = mip_ & 0x1e00;
  
  // Defer interrupt only on 0->1 transition
  bool meip = (e_mip_ >> 11) & 0x1;
  bool seip = (e_mip_ >> 9) & 0x1;
  bool prev_meip = (prev_e_mip_ >> 11) & 0x1;
  bool prev_seip = (prev_e_mip_ >> 9) & 0x1;
  bool meip_assert = (meip != prev_meip);
  bool seip_assert = (seip != prev_seip);
  uint64_t mip_assert = (meip_assert << 11) | (seip_assert << 9);
  check_and_defer_interrupt(hart, m.cycle, mip_assert);
}

void bridge::check_and_defer_interrupt(hart_id_t hart, uint64_t time, uint64_t mip) {
  bool w_intr;
  uint64_t w_cause;
  uint64_t deferredmip;
  bool valid;
  if (!client_->whisperPeek(hart, 's', WhisperSpecialResource::DeferredInterrupts, deferredmip, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed whisper API call - whisperGetDeferredInterrupts\n", hart);
    return;
  }
  uint64_t defer_mip = mip | deferredmip;
  check_interrupt(hart, mip, w_intr, w_cause);
  if (w_intr) {
    deferred_intr_ = true;
    defer_interrupt(hart, time, defer_mip);
  }
}

void bridge::defer_interrupt(hart_id_t hart, uint64_t cycle, uint64_t mip) {
  log(cvm::MEDIUM, "<{}> Interrupt defer mip status {:#x}\n", cycle, mip);
  bool valid;
  if (!client_->whisperPoke(hart, cycle, 's', WhisperSpecialResource::DeferredInterrupts, mip, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to poke DeferredInterrupts\n", hart);
    return;
  }
}

void bridge::check_interrupt(hart_id_t hart, uint64_t mip, bool& taken, uint64_t& cause) {
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

void bridge::peek_mip(hart_id_t hart, uint64_t time, uint64_t& mip) {
  bool valid;
  if (!client_->whisperPeek(hart, 'c', 0x344, mip, valid)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek mip\n", hart);
    return;
  }
  log(cvm::MEDIUM, "<{}> Whisper peek: mip: {:#x}\n", time, mip);
}

void bridge::peek_seip(hart_id_t hart, uint64_t time, uint64_t& val) {
  if (!client_->whisperGetSeiPin(hart, val)) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek seip\n", hart);
    return;
  }
  log(cvm::MEDIUM, "<{}> Whisper peek: seip: {}\n", time, val);
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
  cvm::log(cvm::NONE, "<{}> Enter debug mode\n", d.cycle);
  if (!debug_mode_) {
    if (!client_->whisperEnterDebug()) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Failed to enter debug mode\n", id_);
      return;
    }
  }

  debug_mode_ = true;

  bool valid;
 for(int i=13;i>=0;i--){
    
    uint64_t debugROM_loc = FLAGS_debug_entry_pc + (13-i)*8;

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
    uint64_t i, pmp_cfg_reg, pmp_cfg_index;
    // For PMP addresses, which bits of the pmpcfgs to look for 
    i = addr - 0x3B0;
    pmp_cfg_reg = ((i*8) / 64) * 2;
    pmp_cfg_index = (i*8) % 64;
    client_->whisperPeekCsr(hart, 0x3A0 + pmp_cfg_reg, pmpcfg, mask, reset, valid);
    if((pmpcfg >> (pmp_cfg_index + 4)) & 0x1) {
      result = data | 0x1ff;
    } else {
      result = data & 0xfffffffffffffc00;
    }
  }
  return result;
}

bridge::size_8_bytes_t bridge::modify_csr_mask(hart_id_t hart, uint64_t addr, uint64_t data, size_8_bytes_t mask) {
  size_8_bytes_t result = mask;
  // pmpaddr
  // Spec section...
  if (addr == 0xC20) result = mask;
  else result = mask & get_csr_mask(hart, addr);
  if (addr >= 0x3B0 && addr < 0x3C0) {
    bool valid;
    uint64_t pmpcfg, mask_iss, reset;
    uint64_t i, pmp_cfg_reg, pmp_cfg_index;
    // For PMP addresses, which bits of the pmpcfgs to look for 
    i = addr - 0x3B0;
    pmp_cfg_reg = ((i*8) / 64) * 2;
    pmp_cfg_index = (i*8) % 64;
    client_->whisperPeekCsr(hart, 0x3A0 + pmp_cfg_reg, pmpcfg, mask_iss, reset, valid);
    if((pmpcfg >> (pmp_cfg_index + 4)) & 0x1) {
      result = result | 0x1ff;
    } else {
      result = result | 0x3ff;
    }
  }
  if (addr == 0x680) {
    uint16_t mode = (data & mask) >> 60;
    constexpr uint16_t valid_modes[] = {0, 8, 9, 10};
    bool valid_mode = false;
    for (uint16_t valid_mode_value : valid_modes) {
      if (mode == valid_mode_value) {
          valid_mode = true;
          break;
      }
    }
    if (!valid_mode) {
      result = result & 0xfffffffffffffffULL;
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

void bridge::update_csr(hart_id_t hart, src_t src, uint64_t addr, uint64_t data, cac::optional_const_ref<size_8_bytes_t> mask_ref, bool shadow_csr, bool check_en) {
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
  assert(csr_cac_.SetResource(hart, src, csr_resource, std::move(cac::CreateBitVec<size_8_bytes_t>({data})), mask, check_en));

  // Also update shadow csr if applicable ex: mstatus/sstatus
  if (!shadow_csr && shadow_csrs.count(addr)) {
    auto range = shadow_csrs.equal_range(addr);
    for (auto shadow_csr = range.first; shadow_csr != range.second; ++shadow_csr) {
        size_8_bytes_t alias_mask;
      if (src == src_t::dut){
        if (mask_ref)
          alias_mask = mask_ref.value() & get_csr_poke_mask(hart, shadow_csr->second);
        else
          alias_mask = get_csr_poke_mask(hart, shadow_csr->second);
      }
      else {
        uint64_t mask, poke_mask;
        bool valid;
        if (!client_->whisperPeekCsr(hart, shadow_csr->second, data, mask, poke_mask, valid)) {
          cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek csr\n", hart);
        }
        alias_mask = get_csr_poke_mask(hart, shadow_csr->second);
      }
      update_csr(hart, src, shadow_csr->second, data, alias_mask, true);
    }
  }
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

void bridge::process(const rv_tester::terminate_called&) {
  terminated_ = true;
}

void bridge::report_metrics() {
  if (!FLAGS_metrics || !client_->whisperConnected())
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
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_exceptions\": {}}}\n", id_, num_exceptions_);
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
  cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_max_pend_intr_age\": {}}}\n", id_, max_pend_intr_age_);
  
  // Whisper csr values
  for (auto& csr : metrics_csrs) {
    uint64_t csr_data;
    bool valid;
    if (!client_->whisperPeek(id_, 'c', csr.address, csr_data, valid)) {
      cvm::log(cvm::ERROR, "Error: Hart {}: Failed to peek CSR values\n", id_);
    }
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_iss_csr_{}\": \"0x{:x}\"}}\n", id_, csr.name, csr_data);
  }

  // DUT csr values
  for (auto& csr : metrics_csrs) {
    uint64_t csr_data = get_csr(id_, src_t::dut, csr.address);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_dut_csr_{}\": \"0x{:x}\"}}\n", id_, csr.name, csr_data);
  }

  // Interrupts taken count
  for (size_t i = 0; i < num_taken_interrupts_.size(); i++) {
    for (size_t j = 0; j < num_taken_interrupts_[i].size(); j++) {
        if (num_taken_interrupts_[i][j] != 0) {
            cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_taken_interrupt_count_{}_{}\": {}}}\n", id_, intr_to_string.at(static_cast<intr>(j)), priv_to_string.at(static_cast<priv>(i)), num_taken_interrupts_[i][j]);
        }
    }
  }

  if (!terminated_) {
    // Step one final time to collect metrics for next instruction
    whisper_state_t w;
    if (FLAGS_mcm) {
      client_->whisperDisableMcm();
      w = { .tag = prev_whisp_state.tag+1, .time = prev_whisp_state.time+1 };
    }
    else {
      w = { .tag = step_+1, .time = prev_whisp_state.time+1 };
    }
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
  // Regression level metrics from hart 0
  if (id_ == 0) {
    // Average ipc
    cvm::log(cvm::NONE, "INFO_PASS_REGR_METRIC:{{\"name\": \"ipc\", \"value\": {:.2f}, \"type\": \"d\", \"action\": \"average\"}}\n", ipc);
  }
}
