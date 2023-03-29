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

// Plusargs
DEFINE_bool(cosim_tracer, true, "Enable bridge trace prints");
DECLARE_string(load);
DECLARE_string(hex);
DECLARE_string(eot);
DEFINE_string(bootrom_path, "", "Path to bootrom object file");
DEFINE_string(whisper_path, "", "Path to whisper executable");
DEFINE_string(whisper_json_path, "", "Path to whisper json config");
DEFINE_bool(cosim_resynch, false, "Resynch whisper with dut state on every instruction");
DEFINE_string(cosim_resynch_instr, "", "List of instruction mnemonics to resynch whisper with dut state");
DEFINE_string(cosim_resynch_prev_instr, "", "List of instruction mnemonics to resynch whisper with dut state");
DEFINE_bool(lrsc_resynch, false, "Resynch whisper with dut state on LRSC fail condition");
DEFINE_bool(retire_ucode_trap, true, "DUT indicates retire on a trap after executing the ucode trap handler");
DEFINE_bool(mcm, false, "Enable memory consistency checker");
DEFINE_int32(max_instr, 100000, "Max instruction limit to terminate the sim");
DEFINE_int32(max_cycle, 1000000, "Max cycle limit to terminate the sim");
DEFINE_int32(debug_excp_mcause, 24, "MCAUSE value for debug exception");
DEFINE_int32(max_stall_cycle, 50000, "Max stall cycle limit to terminate the sim");
DEFINE_bool(translation_check, false, "Do VA-PA translation check");
DEFINE_bool(emulate_debug_mode, false, "Emulate debug mode by forcing whisper to be in sync with DUT");
DEFINE_bool(delay_satp_update, false, "Delay satp update till next sfence.vma");
DEFINE_bool(whisper_stdin_null, false, "Redirect whisoer stdin to null");
DEFINE_bool(whisper_stdout_null, false, "Redirect whisoer stdout to null");
DEFINE_bool(whisper_clint, false, "Set clint addr in whisper command");
DEFINE_bool(whisper_tohost, true, "Set tohost addr in whisper command");
DEFINE_bool(whisper_fromhost, true, "Set fromhost addr in whisper command");
DEFINE_string(whisper_client, "socket", "Select whisper client to communicate - socket, or shm (shared mem)");
DEFINE_int32(whisper_connect_timeout_ms, 10000, "Set whisper connect timeout in milliseconds");
DEFINE_bool(cov, false, "Enable Arch coverage");
DEFINE_string(archsample_lib_path, "", "Path to libarchsample.so");

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
}

// Destructor
bridge::~bridge() {
  client_->whisperQuit();
}

bool bridge::whisper_connect(std::string cmd, int timeout) {

  std::cout << "Cosim whisper command: " << cmd << "\n";
  system(cmd.c_str());

  auto start = std::chrono::high_resolution_clock::now();
  while (true) {
    std::this_thread::sleep_for (std::chrono::seconds(5));
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // need to connect to archsample before whisper connection (whisper loads tracerlib first)
    if (FLAGS_cov) {
      archcov.reset();
    }

    if (client_->whisperConnect("whisper_connect") >= 0) {
      std::cout << "Whisper connect succeeded in " << std::dec << duration << " ms\n";
      return true;
    }
    else if (duration > timeout) {
      std::cout << "Error: Whisper connect failed. Stopping after " << duration << " ms.\n";
      return false;
    }
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
  } else {
    cvm::log(cvm::NONE, "Error: Invalid option passed for +whisper_client. Should be one of {socket, shm}.");
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
    return;
  }

  if (!whisper_connect(get_whisper_cmd(), FLAGS_whisper_connect_timeout_ms)) {
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
    return;
  }

  bool valid;
  client_->whisperReset(0, valid);
}

// Whisper command options
std::string bridge::get_whisper_cmd() {
  // Validate flags
  if ((FLAGS_load == "") && (FLAGS_hex == "")) {
    cvm::log(cvm::NONE, "Error: Need to provide at least one of +load <elf> or +hex <hex>\n");
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
    return {};
  }

  if ((FLAGS_whisper_path == "") || (FLAGS_whisper_json_path == "")) {
    cvm::log(cvm::NONE, "Error: +whisper_path or +whisper_json_path cannot be empty\n");
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
    return {};
  }

  // Command components
  std::string harts = " --harts " + std::to_string(num_harts_);
  std::string config = " --configfile " + FLAGS_whisper_json_path;
  std::string trace = " --traceload --traceptw";
  std::string out_log = " --logfile iss_cosim.log";
  std::string cmd_log = " --commandlog iss_cmd.log";
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

  return cmd;
}

// DUT interface callback: Instruction Retire
void bridge::process_dut_instr_retire(hart_id_t hart, rv_instr_t& d) {
  // Update cac with dut state
  update_dut_state(hart, d);

  whisper_state_t w;
  w.time = d.cycle;
  w.tag = d.tag;

  // Handle pre-step conditions
  if (debug_mode_) {
    if (FLAGS_emulate_debug_mode) {
      handle_debug(hart, d, w);
    } else {
      return;
    }
  }
  handle_interrupt(hart, d, w);
  handle_wfi(hart, d, w);

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
      does_prev_instr_match_resynch_list(pw_) ||
      does_instr_match_resynch_condition(d, w)) {
    resynch(hart, d);
    cac_.resetStatus(hart);
  }

  // FIXME Temporarily disable FP checking
  if (w_.fpr.valid) {
    cac_.resetStatus(hart);
  }

  // Save whisper state
  pw_ = w;

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
      print_instr(hart, w);
      cvm::log(cvm::NONE, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, ChangeCount={}, PC={:#x}, Opcode={:#x}, Disasm={}]\n",
        w.time, cac_.getStep(hart), hart, w.priv_mode, w.tag, w.change_count, w.pc, w.opcode, w.buffer);
      cvm::log(cvm::NONE, "{}", cac_.getStatusStr(hart));
      cvm::log(cvm::NONE, "Error: Core Arch Checker Mismatch\n");
      cvm::registry::messenger.signal<terminate_t>(loc_, {false});
      return;
    }
  }
  else {
      log(cvm::HIGH, "{}", cac_.getStatusStr(hart));
  }

  // End test on max_instr
  if (cac_.getStep(hart) > FLAGS_max_instr) {
    if (FLAGS_eot == "max_instr") {
      cvm::log(cvm::NONE, "<{}> Stop condition detected: +eot=max_instr +max_instr={}\n", w.time, FLAGS_max_instr);

      auto location = cvm::topology::get("platform", 0);
      cvm::registry::registry::messenger.signal<terminate_t>(location, {false});
      return;
    } else {
      print_instr(hart, w);
      cvm::log(cvm::NONE, "Error: max_instr limit reached: {}\n", FLAGS_max_instr);
      cvm::registry::messenger.signal<terminate_t>(loc_, {false});
      return;
    }
  }

  // TLB checks
  translation_check(hart, d, w);

  // coverage
  if (FLAGS_cov){
    archcov.coverage_sample(hart, (cac_.getStep(hart)-1), w);
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

  uint64_t mip = 0x344;

  // Clear mip one step after it's set
  if (intr_in_progress_) {
    intr_in_progress_ = false;
    bool valid = false;
    if (!client_->whisperPoke(hart, 'c', mip, 0, valid)) {
      cvm::log(cvm::NONE, "Error: failed to clear interrupt\n");
      cvm::registry::messenger.signal<terminate_t>(loc_, {false});
      return;
    }
  }

  if (!d.intr)
    return;

  if (FLAGS_cosim_tracer) {
    log(cvm::MEDIUM, "<{}> Interrupt detected. cause: [{}]\n", w.time, d.icause);
  }

  // Poke mip before invoking whisper step
  bool valid = false;
  uint64_t cause = (1ull << d.icause);
  if (!client_->whisperPoke(hart, 'c', mip, cause, valid)) {
    cvm::log(cvm::NONE, "Error: failed to poke interrupt\n");
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
    return;
  }

  intr_in_progress_ = true;

  if (FLAGS_retire_ucode_trap)
    return;

  step(hart, w);
  if (FLAGS_cosim_tracer) {
    log(cvm::MEDIUM, "<{}> Whisper Step #{}: Extra step due to interrupt\n", w.time, cac_.getStep(hart));
  }
}

void bridge::handle_exception(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {

  if (!d.excp)
    return;

  bool custom_nonspec_resync = (d.ecause == 28);
  if (custom_nonspec_resync) {
    cvm::log(cvm::MEDIUM, "<{}> Special custom exception detected: NONSPEC_RESYNC\n", d.cycle);
    return;
  }

  if (!w.trap && !ecall_ && !FLAGS_cosim_resynch) {
    print_instr(hart, w);
    cvm::log(cvm::NONE, "Error: DUT took exception, Whisper did not. cause:[{}]\n", d.ecause);
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
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
        log(cvm::HIGH, "<{}> Whisper Step #{}: Resynch: C{:#x}={:#x}\n", d.cycle, cac_.getStep(hart),
          c.csr_addr, c.csr_wdata);
      }
      bool valid;
      if (!client_->whisperPoke(hart, 'c', c.csr_addr, c.csr_wdata, valid)) {
        cvm::log(cvm::NONE, "Error: Failed to resync CSR values\n");
        cvm::registry::messenger.signal<terminate_t>(loc_, {false});
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
    log(cvm::MEDIUM, "<{}> Whisper Step #{}: Extra step due to exception\n", w.time, cac_.getStep(hart));
  }
  update_whisper_state(hart,w);
}

void bridge::handle_satp(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {
  if (!FLAGS_delay_satp_update)
    return;

  // Save satp updates and apply only when sfence.vma is seen
  std::string disasm(w.buffer);
  if (disasm.find("satp") != std::string::npos) {
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
          log(cvm::MEDIUM, "<{}> Whisper Step #{}: SATP write, don't apply till sfence.vma\n", w.time, cac_.getStep(hart));
        }
        bool valid = false;
        if (!client_->whisperPoke(hart, 'c', 0x180, satp_, valid)) {
          cvm::log(cvm::NONE, "Error: failed to poke SATP\n");
          cvm::registry::messenger.signal<terminate_t>(loc_, {false});
          return;
        }
      }
    }
  }

  if (disasm.find("sfence.vma") != std::string::npos) {
    if (satp_ == new_satp_)
      return;

    satp_ = new_satp_;

    if (FLAGS_cosim_tracer) {
      log(cvm::MEDIUM, "<{}> Whisper Step #{}: sfence.vma, apply SATP write\n", w.time, cac_.getStep(hart));
    }
    bool valid = false;
    if (!client_->whisperPoke(hart, 'c', 0x180, new_satp_, valid)) {
      cvm::log(cvm::NONE, "Error: failed to poke new SATP\n");
      cvm::registry::messenger.signal<terminate_t>(loc_, {false});
      return;
    }
  }
}

void bridge::handle_wfi(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {
  // Step whisper an extra time on wfi to synch with dut jump to handler
  std::string disasm(w.buffer);
  if (disasm.find("wfi") != std::string::npos) {
    step(hart, w);
    if (FLAGS_cosim_tracer) {
      log(cvm::MEDIUM, "<{}> Whisper Step #{}: Extra step due to wfi\n", w.time, cac_.getStep(hart));
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
      cvm::log(cvm::NONE, "Error: Failed to get whisper changes\n");
      cvm::registry::messenger.signal<terminate_t>(loc_, {false});
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
    w.time, cac_.getStep(hart), hart, w.priv_mode, w.tag, w.change_count, w.pc, w.opcode, w.buffer);
}

void bridge::print_resource(hart_id_t hart, const whisper_state_t& w) {
  log(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, Resource={}, Addr={:#x}, Data={:#x}]\n",
    w.time, cac_.getStep(hart), hart, w.priv_mode, w.tag, (char)w.resource, w.address, w.value);
}

void bridge::step(hart_id_t hart, whisper_state_t& w) {
  if (!client_->whisperStep(hart, w.time, w.tag,  w.pc, w.opcode, w.change_count, w.buffer, w.buffer_size,
      w.priv_mode, w.fp_flags, w.trap, w.stop)) {
    cvm::log(cvm::NONE, "Error: Failed to step whisper\n");
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
    return;
  }
  if (FLAGS_cosim_tracer) {
    print_instr(hart, w);
  }
}

// Push DUT register state to cac
void bridge::update_regs(hart_id_t hart, const rv_instr_t& d) {
  // GPR
  if (d.gpr.valid) {
    size8BytesT dword_array [1] = {d.gpr.rd_wdata};
    update_regs(hart, src_t::dut, resource_t::int_reg, d.gpr.rd_addr, dword_array);
  }
  // FPR
  if (d.fpr.valid) {
    size8BytesT dword_array [1] = {d.fpr.frd_wdata};
    update_regs(hart, src_t::dut, resource_t::fp_reg, d.fpr.frd_addr, dword_array);
  }
  // VR
  if (d.vr.valid) {
    size8BytesT dword_array [vlen_/64] = {0};
    for (int i = 0; i< vlen_/64; i++) {
      dword_array[i] = d.vr.vrd_wdata[i];
    }
    update_regs(hart, src_t::dut, resource_t::vec_reg, d.vr.vrd_addr, dword_array);
  }
}

// Push DUT mem state to cac
void bridge::update_mem(hart_id_t hart, rv_instr_t& d) {
}

// Push whisper register state to cac
void bridge::update_regs(hart_id_t hart, const whisper_state_t& w) {
  // Register changes - r, f, v,
  size8BytesT dword_array [1] = {0};
  //TODO:size8BytesT dword_vec_array [vlen_/64] = {0};
  //TODO:uint32_t entries = vlen_/64;

  switch(w.resource) {
    case 'r':
      dword_array [0] = w.value;
      update_regs(hart, src_t::whisper, resource_t::int_reg, w.address, dword_array);
      break;
    case 'f':
      dword_array [0] = w.value;
      update_regs(hart, src_t::whisper, resource_t::fp_reg, w.address, dword_array);
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
  size8BytesT dword_array [1] = {data};
  if (src == src_t::dut) {
    cac_.updateRegister(hart, CAC_STATE_PC_ID, dword_array);
  } else {
    cac_.updateRefRegister(hart, CAC_STATE_PC_ID, dword_array);
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

void bridge::update_regs(hart_id_t hart, src_t src, resource_t resource, uint64_t addr, size8BytesT dword_array[]) {
  if (src == src_t::dut) {
    if ((resource == resource_t::int_reg) && (addr == 0x0))
      return;
    cac_.updateRegister(hart, resource, addr, dword_array);
  } else {
    cac_.updateRefRegister(hart, resource, addr, dword_array);
  }
}

bool bridge::is_ecall(const whisper_state_t& w) {
  std::string disasm(w.buffer);
  if (disasm.find("ecall") != std::string::npos)
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
    log(cvm::HIGH, "<{}> Resynch: Reason=[clint_read]\n", w.time);
    return true;
  }
  // Case #2
  if (htif_read(d)) {
    log(cvm::HIGH, "<{}> Resynch: Reason=[htif_read]\n", w.time);
    return true;
  }
  // Case #3
  if (mhpm_counter_read(w)) {
    log(cvm::HIGH, "<{}> Resynch: Reason=[mhpm_counter_read]\n", w.time);
    return true;
  }
  if (FLAGS_lrsc_resynch && lrsc_fail(w)) {
    log(cvm::HIGH, "<{}> Resynch: Reason=[lrsc_fail]\n", w.time);
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
  if (d.mem_read.pa >= memmap_.at("htif").base &&
      d.mem_read.pa < memmap_.at("htif").end)
    return true;
  return false;
}

bool bridge::mhpm_counter_read(const whisper_state_t& w) {
  std::string disasm(w.buffer);
  if (disasm.find("mhpmcounter") != std::string::npos)
    return true;
  return false;
}

bool bridge::lrsc_fail(const whisper_state_t& w) {
  std::string disasm(w.buffer);
  if ((disasm.find("sc.w") != std::string::npos) ||
      (disasm.find("sc.d") != std::string::npos)) {
    uint64_t fail_code = 1;
    if (w_.gpr.rd_wdata == fail_code)
      return true;
  }
  return false;
}

bool bridge::does_instr_match_resynch_list(const whisper_state_t& w) {
  if (FLAGS_cosim_resynch_instr == "")
    return false;

  std::string disasm(w.buffer);
  std::stringstream ss(FLAGS_cosim_resynch_instr);

  while(ss.good()) {
    std::string instr;
    std::getline(ss, instr, ',' );

    if (disasm.find(instr) != std::string::npos) {
      log(cvm::HIGH, "<{}> Resynch: Reason=[+cosim_resynch_instr={} for instr={}]\n", w.time, FLAGS_cosim_resynch_instr, instr);
      return true;
    }
  }
  return false;
}

bool bridge::does_prev_instr_match_resynch_list(const whisper_state_t& w) {
  if (FLAGS_cosim_resynch_prev_instr == "")
    return false;

  std::string disasm(w.buffer);
  std::stringstream ss(FLAGS_cosim_resynch_prev_instr);

  while(ss.good()) {
    std::string instr;
    std::getline(ss, instr, ',' );

    if (disasm.find(instr) != std::string::npos) {
      log(cvm::HIGH, "<{}> Resynch: Reason=[+cosim_resynch_prev_instr={} for instr={}]\n", w.time, FLAGS_cosim_resynch_prev_instr, instr);
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
      log(cvm::HIGH, "<{}> Whisper Step #{}: Resynch: PC={:#x}\n", d.cycle, cac_.getStep(hart), d.pc.pc_rdata);
    }
    if (!client_->whisperPoke(hart, 'p', 0, d.pc.pc_rdata, valid)) {
      cvm::log(cvm::NONE, "Error: Failed to resync PC\n");
      cvm::registry::messenger.signal<terminate_t>(loc_, {false});
      return;
    }
  }

  if (d.gpr.valid) {
    if (FLAGS_cosim_tracer) {
      log(cvm::HIGH, "<{}> Whisper Step #{}: Resynch: X{}={:#x}\n", d.cycle, cac_.getStep(hart), d.gpr.rd_addr,
        d.gpr.rd_wdata);
    }
    if (!client_->whisperPoke(hart, 'r', d.gpr.rd_addr, d.gpr.rd_wdata, valid)) {
      cvm::log(cvm::NONE, "Error: Failed to resync GPR\n");
      cvm::registry::messenger.signal<terminate_t>(loc_, {false});
      return;
    }
  }

  if (d.fpr.valid) {
    if (FLAGS_cosim_tracer) {
      log(cvm::HIGH, "<{}> Whisper Step #{}: Resynch: F{}={:#x}\n", d.cycle, cac_.getStep(hart), d.fpr.frd_addr,
        d.fpr.frd_wdata);
    }
    if (!client_->whisperPoke(hart, 'f', d.fpr.frd_addr, d.fpr.frd_wdata, valid)) {
      cvm::log(cvm::NONE, "Error: Failed to resync FP\n");
      cvm::registry::messenger.signal<terminate_t>(loc_, {false});
      return;
    }
  }

  if (d.mem_write.valid) {
    uint64_t pa = translate(hart, d.mem_write.va, w_.priv, memclass_t::write);
    if (FLAGS_cosim_tracer) {
      log(cvm::HIGH, "<{}> Whisper Step #{}: Resynch: M[{:#x}]={:#x}\n", d.cycle, cac_.getStep(hart), pa,
        d.mem_write.data);
    }
    if (!client_->whisperPoke(hart, 'm', pa, d.mem_write.data, valid)) {
      cvm::log(cvm::NONE, "Error: Failed to resync memory\n");
      cvm::registry::messenger.signal<terminate_t>(loc_, {false});
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
    cvm::log(cvm::NONE, "Error: Failed mcm load resolve\n");
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
    return;
  }
}

// Process mem accesses - store inserts
void bridge::process_dut_mb_insert(hart_id_t hart, mem_t& m) {
  unsigned size_in_bytes = 1 << m.size;
  bool valid = false;
  if (!client_->whisperMcmInsert(hart, m.cycle, m.tag, m.pa, size_in_bytes, m.data, valid)) {
    cvm::log(cvm::NONE, "Error: Failed mcm store insert\n");
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
    return;
  }
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
    cvm::log(cvm::NONE, "Error: Failed mcm store drain\n");
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
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
    cvm::log(cvm::NONE, "Error: Failed VA translation\n");
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
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
    cvm::log(cvm::NONE, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, PC={:#x}, VA={:#x}, RTL-PA={:#x}, ISS-PA={:#x}]\n", w.time, (cac_.getStep(hart)-1), hart, w.priv_mode, w.tag, w.pc, d.mem_va, d.mem_pa, pa);
    cvm::log(cvm::NONE, "Error: PA MISMATCH !! :\n");
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
    return;
  }
  else {
    log(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, PC={:#x}, VA={:#x}, PA={:#x}]\n", w.time, (cac_.getStep(hart)-1), hart, w.priv_mode, w.tag, w.pc, d.mem_va, pa);
  }

}

// Debug Mode
void bridge::enter_debug_mode(rv_debug_t& d) {

  log(cvm::NONE, "<{}> Enter debug mode\n", d.cycle);
  debug_mode_ = true;
  if (!client_->whisperEnterDebug()) {
    cvm::log(cvm::NONE, "Error: Failed to enter debug mode\n");
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
    return;
  }
  //whisper: if debug_exc -> poke mcause with 24<configurable cmdline>
  // Poke mip before invoking whisper step
  bool valid = false;
  uint64_t mcause = 0x342;
  uint64_t cause = FLAGS_debug_excp_mcause; //24 for cva6
  if (!client_->whisperPoke(d.hart, 'c', mcause, cause, valid)) {
    cvm::log(cvm::NONE, "Error: Failed to poke mcause\n");
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
    return;
  }else{
    std::cout <<"whisper poke mcause with "<<FLAGS_debug_excp_mcause<<"\n";
  }
}

void bridge::exit_debug_mode(rv_debug_t& d) {
  log(cvm::NONE, "<{}> Exit debug mode\n", d.cycle);
  debug_mode_ = false;
  if (!client_->whisperExitDebug()) {
    cvm::log(cvm::NONE, "Error: Failed to exit debug mode\n");
    cvm::registry::messenger.signal<terminate_t>(loc_, {false});
    return;
  }
}
