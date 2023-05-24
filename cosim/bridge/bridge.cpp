// Licensed under the Apache License, Version 2.0, see LICENSE.TT for details

#include "bridge.h"
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/topology.hpp"
#include "sysmod/htif/htif.h"

#include <iostream>         // cout
#include <cstring>          // strlen
#include <sstream>          // stringstream
#include <thread>           // std::this_thread::sleep_for
#include <chrono>           // std::chrono::seconds
#include <cstdlib>          // system
#include <vector>
#include <fmt/format.h>

// Plusargs
DECLARE_string(load);
DECLARE_string(hex);
DECLARE_string(eot);

DEFINE_bool(cosim_tracer, true, "Enable bridge trace prints");
DEFINE_string(bootrom_path, "", "Path to bootrom object file");
DEFINE_string(whisper_path, "", "Path to whisper executable");
DEFINE_string(whisper_json_path, "", "Path to whisper json config");
DEFINE_bool(cosim_resynch, false, "Resynch whisper with dut state on every instruction");
DEFINE_string(cosim_resynch_instr, "", "List of instruction mnemonics to resynch whisper with dut state");
DEFINE_string(cosim_resynch_prev_instr, "", "List of instruction mnemonics to resynch whisper with dut state");
DEFINE_bool(lrsc_resynch, false, "Resynch whisper with dut state on LRSC fail condition");
DEFINE_bool(retire_ucode_trap, true, "DUT indicates retire on a trap after executing the ucode trap handler");
DEFINE_bool(mcm, false, "Enable memory consistency checker");
DEFINE_int32(max_cycle, 1000000, "Max cycle limit to terminate the sim");
DEFINE_int32(debug_excp_mcause, 24, "MCAUSE value for debug exception");
DEFINE_int32(max_stall_cycle, 50000, "Max stall cycle limit to terminate the sim");
DEFINE_bool(translation_check, false, "Do VA-PA translation check");
DEFINE_bool(emulate_debug_mode, false, "Emulate debug mode by forcing whisper to be in sync with DUT");
DEFINE_bool(delay_satp_update, false, "Delay satp update till next sfence.vma");
DEFINE_bool(cov, false, "Enable Arch coverage");
DEFINE_string(archsample_lib_path, "", "Path to libarchsample.so");
DEFINE_bool(metrics, true, "Enable printing metrics in log file");
DEFINE_uint32(pend_intr_threshold, 32, "Number of instructions allowed to retire before a pending interrupt should be taken");
DEFINE_bool(whisper_log, true, "Enable whisper logging to iss_cosim.log and iss_cmd.log");
DEFINE_bool(whisper_stdin_null, false, "Redirect whisoer stdin to null");
DEFINE_bool(whisper_stdout_null, false, "Redirect whisoer stdout to null");
DEFINE_bool(whisper_clint, false, "Set clint addr in whisper command");
DEFINE_bool(whisper_tohost, true, "Set tohost addr in whisper command");
DEFINE_bool(whisper_fromhost, true, "Set fromhost addr in whisper command");
DEFINE_string(whisper_client, "socket", "Select whisper client to communicate - socket, shm (shared mem), or library");
DEFINE_int32(whisper_connect_timeout_ms, 10000, "Set whisper connect timeout in milliseconds");

// Constructor
bridge::bridge(int num_harts, int xlen, int vlen, cvm::topology::loc_t loc)
  : log("cosim.log"),
    num_harts_(num_harts),
    xlen_(xlen),
    vlen_(vlen),
    loc_(loc),
    cac_(CacCore(num_harts)),
    archcov(ArchSample(num_harts))
{
  cvm::registry::messenger.connect<htif::terminate_t>(loc_, [&](htif::terminate_t t) {
      return this->final_phase();
  });
}

// Destructor
bridge::~bridge() {
  report_metrics();
  client_->whisperQuit();
}

bool bridge::whisper_connect(std::string cmd, int timeout) {

  std::cout << "Cosim whisper command: " << cmd << "\n";
  // Used for gperftools to collect profiling data on whisper and output it to ./whisper.prof
  setenv("CPUPROFILE", "whisper.prof", 1);
  int status = system(cmd.c_str());
  if (status != 0)
    cvm::log(cvm::NONE, "Error: Whisper command failed with non-zero return code {}", status);

  auto start = std::chrono::high_resolution_clock::now();
  while (true) {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // need to connect to archsample before whisper connection (whisper loads tracerlib first)
    if (FLAGS_cov) {
      archcov.reset();
    }

    int result;
    if (FLAGS_whisper_client == "lib")
      result = client_->whisperConnect(FLAGS_whisper_json_path.c_str());
    else
      result = client_->whisperConnect("whisper_connect");

    if (result >= 0) {
      cvm::log(cvm::NONE, "Whisper connect succeeded in {} ms.\n", duration);
      return true;
    }
    else if (duration > timeout) {
      cvm::log(cvm::ERROR, "Error: Whisper connect failed. Stopping after {} ms.\n", duration);
      return false;
    }
    std::this_thread::sleep_for (std::chrono::milliseconds(20));
  }
}

void bridge::reset() {

  memmap::get(memmap_);

  cac_.reset();
  cac_.configureVlen(vlen_);

  if (FLAGS_whisper_client == "socket") {
    client_ = std::make_unique<whisperClientSocket>();
  } else if (FLAGS_whisper_client == "shm") {
    client_ = std::make_unique<whisperClientShm>();
  } else if (FLAGS_whisper_client == "lib") {
    std::string traceFile = FLAGS_whisper_log ? "iss_cosim.log" : "";
    std::string commandLog = FLAGS_whisper_log ? "iss_cmd.log" : "";
    client_ = std::make_unique<whisperClientLib<uint64_t>>(traceFile, commandLog, FLAGS_load);
  } else {
    cvm::log(cvm::ERROR, "Error: Invalid option passed for +whisper_client. Should be one of - socket, shm.");
    return;
  }

  whisper_connect(get_whisper_cmd(), FLAGS_whisper_connect_timeout_ms);

  bool valid;
  client_->whisperReset(0, valid);
}

// Whisper command options
std::string bridge::get_whisper_cmd() {
  // Validate flags
  if ((FLAGS_load == "") && (FLAGS_hex == "")) {
    cvm::log(cvm::ERROR, "Error: Need to provide at least one of +load <elf> or +hex <hex>\n");
    return {};
  }

  if ((FLAGS_whisper_path == "") || (FLAGS_whisper_json_path == "")) {
    cvm::log(cvm::ERROR, "Error: +whisper_path or +whisper_json_path cannot be empty\n");
    return {};
  }

  // Command components
  std::string harts = " --harts " + std::to_string(num_harts_);
  std::string config = " --configfile " + FLAGS_whisper_json_path;
  std::string trace = " --traceload --traceptw";
  std::string out_log = FLAGS_whisper_log ? " --logfile iss_cosim.log" : "";
  std::string cmd_log = FLAGS_whisper_log ? " --commandlog iss_cmd.log" : "";
  std::string std_out = FLAGS_whisper_stdout_null ? " --stdout /dev/null" : "";
  std::string std_in = FLAGS_whisper_stdin_null ? " --stdin /dev/null" : "";
  std::string client = (FLAGS_whisper_client == "shm") ? " --shm" : "";
  std::string mcm = FLAGS_mcm ? " --mcm --mcmls 64" : "";
  std::string clint = FLAGS_whisper_clint ? " --clint " + memmap_.at("clint").base_str : "";
  std::string htif = memmap_.at("htif").base_str;
  std::string tohost = FLAGS_whisper_tohost ? " --tohost " + htif : "";
  std::string fromhost = FLAGS_whisper_fromhost ? " --fromhost " + htif.replace(htif.size() - 1, 1, "8") : "";
  std::string test = (FLAGS_load != "") ? FLAGS_load : ("--hex " + FLAGS_hex);

  std::string cmd = FLAGS_whisper_path + " " + test + " " + FLAGS_bootrom_path +
    harts + config + trace + out_log + cmd_log + std_out + std_in + client +
    clint + tohost + fromhost + " --raw --server whisper_connect &";

  if (FLAGS_cov){
    std::string cov_cmd = " --tracerlib " + FLAGS_archsample_lib_path + ":tracer_ext ";
    size_t raw_pos = cmd.find("--raw");
    cmd.insert(raw_pos, cov_cmd);
  }

  if (FLAGS_whisper_client == "lib")
    cmd = "echo 'using whisper library'";

  return cmd;
}

// DUT interface callback: Instruction Retire
void bridge::process_dut_instr_retire(hart_id_t hart, rv_instr_t& d) {
  step_[hart] = cac_.getStep(hart) + 1;

  // Update cac with dut state
  update_dut_state(hart, d);

  whisper_state_t w {
    .tag = d.tag,
    .time = d.cycle
  };

  // Handle pre-step condition - Debug
  if (debug_mode_) {
    if (FLAGS_emulate_debug_mode) {
      handle_debug(hart, d, w);
    } else {
      return;
    }
  }

  // Handle pre-step condition - Interrupts
  check_interrupt(hart);
  handle_interrupt(hart, d, w);

  // Step whisper
  w_.clear();
  step(hart, w);

  // Update cac with whisper state
  update_whisper_state(hart, w);

  // Handle post-step conditions
  handle_exception(hart, d, w);
  handle_satp(hart, d, w);

  // Check dut vs whisper
  cac_.step(hart);

  // Resynch whisper with dut state if needed
  // to continue without failing
  if (does_instr_match_resynch_list(w) ||
      does_prev_instr_match_resynch_list(pw_[hart]) ||
      does_instr_match_resynch_condition(d, w)) {
    resynch(hart, d);
    cac_.resetStatus(hart);
  }

  // FIXME Temporarily disable FP checking
  if (w_.fpr.valid) {
    cac_.resetStatus(hart);
  }

  // Save whisper state
  ppw_[hart] = pw_[hart];
  pw_[hart] = w;

  // Error on mismatch
  if (!cac_.getStatus(hart)) {
    if (FLAGS_cosim_resynch) {
      if (FLAGS_cosim_tracer) {
        print_instr(hart, w);
        log(cvm::MEDIUM, "{}", cac_.getStatusStr(hart));
      }
      resynch(hart, d);
      cac_.resetStatus(hart);
    } else {
      std::string instr = w.disasm.substr(0, w.disasm.find(' '));
      print_instr(hart, w);
      cvm::log(cvm::NONE, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, ChangeCount={}, PC={:#x}, Opcode={:#x}, Disasm={}]\n",
        w.time, step_[hart], hart, w.priv_mode, w.tag, w.change_count, w.pc, w.opcode, w.disasm);
      cvm::log(cvm::NONE, "{}", cac_.getStatusStr(hart));
      cvm::log(cvm::ERROR, "Error: Core Arch Checker Mismatch - {}\n", instr);
      return;
    }
  }
  else {
      log(cvm::HIGH, "{}", cac_.getStatusStr(hart));
  }

  // TLB checks
  translation_check(hart, d, w);

  // coverage
  if (FLAGS_cov){
    archcov.coverage_sample(hart, step_[hart] - 1, w);
  }
}

void bridge::update_dut_state(hart_id_t hart, rv_instr_t& d) {
  update_pc(hart, src_t::dut, d.pc.pc_rdata);
  //TODO:update_priv(hart, src_t::dut, d.priv);
  if (d.gpr.valid || d.fpr.valid || d.vr.valid) {
    update_regs(hart, d);
  }
  if (d.mem_write.valid) {
    update_mem(hart, d);
  }
}

void bridge::handle_debug(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {
  return;
}

void bridge::handle_interrupt(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {

  if (!d.intr)
    return;

  // Poke mip before invoking whisper step
  poke_pend_interrupt(hart, w.time);

  if (FLAGS_cosim_tracer) {
    log(cvm::MEDIUM, "<{}> Interrupt taken. cause: [{}]\n", w.time, d.icause);
  }

  if (FLAGS_retire_ucode_trap)
    return;

  step(hart, w);
  if (FLAGS_cosim_tracer) {
    log(cvm::MEDIUM, "<{}> Whisper Step #{}: Extra step due to interrupt\n", w.time, step_[hart]);
  }
}

void bridge::handle_exception(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {

  if (!d.excp)
    return;

  bool custom_nonspec_resync = (d.ecause == 28);
  if (custom_nonspec_resync) {
    log(cvm::MEDIUM, "<{}> Special custom exception detected: NONSPEC_RESYNC\n", d.cycle);
    return;
  }

  if (!w.trap && !ecall_ && !FLAGS_cosim_resynch) {
    print_instr(hart, w);
    cvm::log(cvm::ERROR, "Error: DUT took exception, Whisper did not. cause:[{}]\n", d.ecause);
    return;
  }

  // Special case - ecall - No extra step
  if (is_ecall(w)) {
    ecall_ = true;
    return;
  } else {
    ecall_ = false;
  }

  // If resynch, poke CSR values to whisper
  if (FLAGS_cosim_resynch) {
    for (auto& c : w_.csr) {
      if (FLAGS_cosim_tracer) {
        log(cvm::HIGH, "<{}> Whisper Step #{}: Resynch: C{:#x}={:#x}\n", d.cycle, step_[hart],
          c.csr_addr, c.csr_wdata);
      }
      bool valid;
      if (!client_->whisperPoke(hart, d.cycle, 'c', c.csr_addr, c.csr_wdata, valid)) {
        cvm::log(cvm::ERROR, "Error: Failed to resynch CSR values\n");
        return;
      }
    }
  }

  // Print exception info
  if (FLAGS_cosim_tracer) {
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
  if (FLAGS_cosim_tracer) {
    log(cvm::MEDIUM, "<{}> Whisper Step #{}: Extra step due to exception\n", w.time, step_[hart]);
  }
  update_whisper_state(hart,w);
}

void bridge::handle_satp(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {
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

        if (FLAGS_cosim_tracer) {
          log(cvm::MEDIUM, "<{}> Whisper Step #{}: SATP write, don't apply till sfence.vma\n", w.time, step_[hart]);
        }
        bool valid = false;
        if (!client_->whisperPoke(hart, d.cycle, 'c', 0x180, satp_, valid)) {
          cvm::log(cvm::ERROR, "Error: Failed to poke SATP\n");
          return;
        }
      }
    }
  }

  if (w.disasm.find("sfence.vma") != std::string::npos) {
    if (satp_ == new_satp_)
      return;

    satp_ = new_satp_;

    if (FLAGS_cosim_tracer) {
      log(cvm::MEDIUM, "<{}> Whisper Step #{}: sfence.vma, apply SATP write\n", w.time, step_[hart]);
    }
    bool valid = false;
    if (!client_->whisperPoke(hart, w.time, 'c', 0x180, new_satp_, valid)) {
      cvm::log(cvm::ERROR, "Error: Failed to poke new SATP\n");
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

  w_.pc.valid = true;
  w_.pc.pc_rdata = w.pc;
  update_pc(hart, src_t::whisper, w.pc);

  //TODO:update_priv(hart, src_t::whisper, w.priv_mode);

  for (auto i = 0u; i < w.change_count; i++) {
    if (!client_->whisperChange(hart, w.resource, w.address, w.value,
        w.valid)) {
      cvm::log(cvm::ERROR, "Error: Failed to get whisper changes\n");
      return;
    }
    if (FLAGS_cosim_tracer) {
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
}

// Print functions
void bridge::print_instr(hart_id_t hart, const whisper_state_t& w) {
  log(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, ChangeCount={}, PC={:#x}, Opcode={:#x}, Disasm={}]\n",
    w.time, step_[hart], hart, w.priv_mode, w.tag, w.change_count, w.pc, w.opcode, w.disasm);
}

void bridge::print_resource(hart_id_t hart, const whisper_state_t& w) {
  log(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, Resource={}, Addr={:#x}, Data={:#x}]\n",
    w.time, step_[hart], hart, w.priv_mode, w.tag, (char)w.resource, w.address, w.value);
}

void bridge::step(hart_id_t hart, whisper_state_t& w) {
  if (!client_->whisperStep(hart, w.time, w.tag,  w.pc, w.opcode, w.change_count, w.disasm,
      w.priv_mode, w.fp_flags, w.trap, w.stop)) {
    cvm::log(cvm::ERROR, "Error: Failed to step whisper\n");
    return;
  }

  // Print instruction
  if (FLAGS_cosim_tracer) {
    print_instr(hart, w);
  }
}

// Push DUT register state to cac
void bridge::update_regs(hart_id_t hart, const rv_instr_t& d) {
  // GPR
  if (d.gpr.valid) {
    update_regs(hart, src_t::dut, resource_t::int_reg, d.gpr.rd_addr, {d.gpr.rd_wdata});
  }
  // FPR
  if (d.fpr.valid) {
    update_regs(hart, src_t::dut, resource_t::fp_reg, d.fpr.frd_addr, {d.fpr.frd_wdata});
  }
  // VR
  if (d.vr.valid) {
    update_regs(hart, src_t::dut, resource_t::vec_reg, d.vr.vrd_addr, {d.vr.vrd_wdata, d.vr.vrd_wdata + (vlen_/64)});
  }
}

// Push DUT mem state to cac
void bridge::update_mem(hart_id_t hart, rv_instr_t& d) {
}

// Push whisper register state to cac
void bridge::update_regs(hart_id_t hart, const whisper_state_t& w) {
  // Register changes - r, f, v,
  //TODO:size8BytesT dword_vec_array [vlen_/64] = {0};
  //TODO:uint32_t entries = vlen_/64;

  switch(w.resource) {
    case 'r':
      update_regs(hart, src_t::whisper, resource_t::int_reg, w.address, {w.value});
      break;
    case 'f':
      update_regs(hart, src_t::whisper, resource_t::fp_reg, w.address, {w.value});
      break;
    case 'v':
      //TODO:dword_vec_array [i % entries] = w.value;
      //TODO:if ((i % entries) == (entries - 1))
      //TODO:  update_regs(hart, src_t::whisper, resource_t::vec_reg, w.address, dword_vec_array);
      break;
    default:
      break;
  }
}

// Utility functions
void bridge::update_pc(hart_id_t hart, src_t src, uint64_t data) {
  if (src == src_t::dut) {
    cac_.updateRegister(hart, CAC_STATE_PC_ID, {data});
  } else {
    cac_.updateRefRegister(hart, CAC_STATE_PC_ID, {data});
  }
}

//TODO:void bridge::update_priv(hart_id_t hart, src_t src, uint32_t data) {
//TODO:  size8BytesT dword_array [1] = {data};
//TODO:  if (src == src_t::dut) {
//TODO:    cac_.updateRegister(hart, CAC_STATE_PRIV_MODE, dword_array);
//TODO:  } else {
//TODO:    cac_.updateRefRegister(hart, CAC_STATE_PRIV_MODE, dword_array);
//TODO:  }
//TODO:}

void bridge::update_regs(hart_id_t hart, src_t src, resource_t resource, uint64_t addr, const std::vector<size8BytesT>&& dword_vec) {
  if (src == src_t::dut) {
    if ((resource == resource_t::int_reg) && (addr == 0x0))
      return;
    cac_.updateRegister(hart, resource, addr, std::move(dword_vec));
  } else {
    cac_.updateRefRegister(hart, resource, addr, std::move(dword_vec));
  }
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
  if (mhpm_counter_read(w)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[mhpm_counter_read]\n", w.time);
    return true;
  }
  if (FLAGS_lrsc_resynch && lrsc_fail(w)) {
    log(cvm::MEDIUM, "<{}> Resynch: Reason=[lrsc_fail]\n", w.time);
    return true;
  }

  return false;
}

bool bridge::clint_read(const rv_instr_t& d) {
  if (d.mem_read.pa >= memmap_.at("clint").base &&
      d.mem_read.pa < memmap_.at("clint").end)
    return true;
  return false;
}

bool bridge::htif_read(const rv_instr_t& d) {
  if (d.mem_read.valid && (d.mem_read.pa == (memmap_.at("htif").base + 8)))
    return true;
  return false;
}

bool bridge::mhpm_counter_read(const whisper_state_t& w) {
  if (w.disasm.find("mhpmcounter") != std::string::npos)
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
    if (FLAGS_cosim_tracer) {
      log(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: PC={:#x}\n", d.cycle, step_[hart], d.pc.pc_rdata);
    }
    if (!client_->whisperPoke(hart, d.cycle, 'p', 0, d.pc.pc_rdata, valid)) {
      cvm::log(cvm::ERROR, "Error: Failed to resynch PC\n");
      return;
    }
  }

  if (d.gpr.valid) {
    if (FLAGS_cosim_tracer) {
      log(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: X{}={:#x}\n", d.cycle, step_[hart], d.gpr.rd_addr,
        d.gpr.rd_wdata);
    }
    if (!client_->whisperPoke(hart, d.cycle, 'r', d.gpr.rd_addr, d.gpr.rd_wdata, valid)) {
      cvm::log(cvm::ERROR, "Error: Failed to resynch GPR\n");
      return;
    }
  }

  if (d.fpr.valid) {
    if (FLAGS_cosim_tracer) {
      log(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: F{}={:#x}\n", d.cycle, step_[hart], d.fpr.frd_addr,
        d.fpr.frd_wdata);
    }
    if (!client_->whisperPoke(hart, d.cycle, 'f', d.fpr.frd_addr, d.fpr.frd_wdata, valid)) {
      cvm::log(cvm::ERROR, "Error: Failed to resynch FP\n");
      return;
    }
  }

  if (d.mem_write.valid) {
    uint64_t pa = translate(hart, d.mem_write.va, w_.priv, memclass_t::write);
    if (FLAGS_cosim_tracer) {
      log(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: M[{:#x}]={:#x}\n", d.cycle, step_[hart], pa,
        d.mem_write.data);
    }
    if (!client_->whisperPoke(hart, d.cycle, 'm', pa, d.mem_write.data, valid)) {
      cvm::log(cvm::ERROR, "Error: Failed to resynch memory\n");
      return;
    }
  }
}

// Process mem accesses - load resolves
void bridge::process_dut_mem_read(hart_id_t hart, mem_t& m) {
  unsigned size_in_bytes = 1 << m.size;
  bool internal = false;
  bool valid = false;
  if (!client_->whisperMcmRead(hart, m.cycle, m.tag, m.pa, size_in_bytes, m.data, internal, valid)) {
    cvm::log(cvm::ERROR, "Error: Failed mcm load resolve\n");
    return;
  }
}

// Process mem accesses - store inserts
void bridge::process_dut_mb_insert(hart_id_t hart, mem_t& m) {
  unsigned size_in_bytes = 1 << m.size;
  bool valid = false;

  if (!client_->whisperMcmInsert(hart, m.cycle, m.tag, m.pa, size_in_bytes, m.data, valid)) {
    cvm::log(cvm::ERROR, "Error: Failed mcm store insert\n");
    return;
  }

  // Collect metrics
  num_stores_++;
}

// Process mem accesses - store drains
void bridge::process_dut_mb_drain(hart_id_t hart, mem_cl_t& m) {
  unsigned size_in_bytes = 64;

  uint64_t addr = (m.addr >> 6) << 6;
  char data[64] = {0};
  for (unsigned i=0; i<size_in_bytes; i++) {
    data[i] = (char)((m.data >> (i*8)) & std::bitset<512>(0xff)).to_ulong();
  }

  bool valid = false;
  if (!client_->whisperMcmWrite(hart, m.cycle, addr, size_in_bytes, data, m.mask, valid)) {
    cvm::log(cvm::ERROR, "Error: Failed mcm store drain\n");
    return;
  }
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
    cvm::log(cvm::ERROR, "Error: Failed VA translation\n");
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
    cvm::log(cvm::NONE, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, PC={:#x}, VA={:#x}, RTL-PA={:#x}, ISS-PA={:#x}]\n", w.time, step_[hart]-1, hart, w.priv_mode, w.tag, w.pc, d.mem_va, d.mem_pa, pa);
    cvm::log(cvm::ERROR, "Error: PA MISMATCH !! :\n");
    return;
  }
  else {
    log(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, PC={:#x}, VA={:#x}, PA={:#x}]\n", w.time, step_[hart]-1, hart, w.priv_mode, w.tag, w.pc, d.mem_va, pa);
  }

}

// Interrupts
void bridge::process_dut_interrupt(hart_id_t hart, rv_intr_t& i) {
  if (i.mip_posedge) {
    log(cvm::MEDIUM, "<{}> Interrupt pin(s) asserted. MipEncodedValue: {:#x}\n", i.cycle, i.mip);
    is_intr_pend_[hart] = true;
    pend_intr_[hart] = i.mip;
    pend_intr_count_[hart] = 0;
  } else {
    log(cvm::MEDIUM, "<{}> Interrupt pin(s) deasserted. MipEncodedValue: {:#x}\n", i.cycle, i.mip);
    poke_interrupt(hart, i.cycle, i.mip);
  }

  if (i.seip_posedge) {
    log(cvm::MEDIUM, "<{}> Seip pin asserted\n", i.cycle);
    is_seip_pend_[hart] = true;
  } else if (i.seip_negedge) {
    log(cvm::MEDIUM, "<{}> Seip pin deasserted\n", i.cycle);
    poke_seip(hart, false);
  }
}

void bridge::check_interrupt(hart_id_t hart) {
  if (!is_intr_pend_[hart])
    return;

  bool should_be_taken;
  uint64_t eff_mip = pend_intr_[hart] | ((uint64_t)is_seip_pend_[hart] << 9);
  if (!client_->whisperCheckInterrupt(hart, eff_mip, should_be_taken)) {
    cvm::log(cvm::ERROR, "Error: Failed whisper API call - whisperCheckInterrupt\n");
    return;
  }

  if (should_be_taken) {
    pend_intr_count_[hart]++;
  }

  if (pend_intr_count_[hart] > FLAGS_pend_intr_threshold) {
    cvm::log(cvm::ERROR, "Error: Hart {}: Interrupt kept pending for pend_intr_threshold({}) instructions\n",
      hart, FLAGS_pend_intr_threshold);
    return;
  }
}

// Poke the pending interrupt(s) at DUT taken boundary
void bridge::poke_pend_interrupt(hart_id_t hart, uint64_t time) {
  if (is_intr_pend_[hart]) {
    poke_interrupt(hart, time, pend_intr_[hart]);
    is_intr_pend_[hart] = false;
    pend_intr_count_[hart] = 0;
  }

  if (is_seip_pend_[hart]) {
    poke_seip(hart, true);
    is_seip_pend_[hart] = false;
    pend_intr_count_[hart] = 0;
  }
}

void bridge::poke_interrupt(hart_id_t hart, uint64_t time, uint64_t mip) {
  bool valid;

  // Peek old mip
  uint64_t old_mip;
  if (!client_->whisperPeek(hart, 'c', 0x344, old_mip, valid)) {
    cvm::log(cvm::ERROR, "Error: Failed to peek mip csr");
    return;
  }

  // Poke new mip = mask(old mip, stip) | mip
  uint64_t new_mip = (old_mip & 0x20) | mip;
  log(cvm::MEDIUM, "<{}> Mip poked. MipEncodedValue: {:#x}\n", time, new_mip);
  if (!client_->whisperPoke(hart, time, 'c', 0x344, new_mip, valid)) {
    cvm::log(cvm::ERROR, "Error: Failed to poke mip csr");
    return;
  }
}

void bridge::poke_seip(hart_id_t hart, bool val) {
  if (!client_->whisperSetSeiPin(hart, (uint64_t)val)) {
    cvm::log(cvm::ERROR, "Error: Failed to poke seip");
    return;
  }
}

// Debug Mode
void bridge::enter_debug_mode(rv_debug_t& d) {

  log(cvm::NONE, "<{}> Enter debug mode\n", d.cycle);
  debug_mode_ = true;
  if (!client_->whisperEnterDebug()) {
    cvm::log(cvm::ERROR, "Error: Failed to enter debug mode\n");
    return;
  }
  //whisper: if debug_exc -> poke mcause with 24<configurable cmdline>
  // Poke mip before invoking whisper step
  bool valid = false;
  uint64_t mcause = 0x342;
  uint64_t cause = FLAGS_debug_excp_mcause; //24 for cva6
  if (!client_->whisperPoke(d.hart, d.cycle, 'c', mcause, cause, valid)) {
    cvm::log(cvm::ERROR, "Error: Failed to poke mcause\n");
    return;
  }else{
    std::cout <<"whisper poke mcause with "<<FLAGS_debug_excp_mcause<<"\n";
  }
}

void bridge::exit_debug_mode(rv_debug_t& d) {
  log(cvm::NONE, "<{}> Exit debug mode\n", d.cycle);
  debug_mode_ = false;
  if (!client_->whisperExitDebug()) {
    cvm::log(cvm::ERROR, "Error: Failed to exit debug mode\n");
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

  for (int h = 0; h < num_harts_; h++) {
    const auto& prev_whisp_state = pw_[h];
    const auto& prev_prev_whisp_state = ppw_[h];
    const int num_instructions = cac_.getStep(h);
    const auto& num_cycles = prev_whisp_state.time;
    const double ipc = static_cast<double>(num_instructions) / static_cast<double>(num_cycles);
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

    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_num_instructions\": {}}}\n", h, num_instructions);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_num_cycles\": {}}}\n", h, num_cycles);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_ipc\": {:.2f}}}\n", h, ipc);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_instr\": \"{}\"}}\n", h, instr);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_mode\": {}}}\n", h, mode);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_trap\": {}}}\n", h, trap);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_num_dest\": {}}}\n", h, num_dest);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_dest\": \"{}\"}}\n", h, dest);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_dest_addr\": \"{}\"}}\n", h, dest_addr);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_dest_data\": \"{}\"}}\n", h, dest_data);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_prev_instr\": \"{}\"}}\n", h, prev_instr);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_prev_mode\": {}}}\n", h, prev_mode);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_prev_trap\": {}}}\n", h, prev_trap);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_prev_num_dest\": {}}}\n", h, prev_num_dest);

    for (auto& csr : csrs) {
      uint64_t csr_data;
      bool valid;
      if (!client_->whisperPeek(h, 'c', csr.address, csr_data, valid)) {
        cvm::log(cvm::ERROR, "Error: Failed to peek CSR values\n");
      }
      cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_csr_{}\": \"0x{:x}\"}}\n", h, csr.name, csr_data);
    }
  }
}
