// vim: ft=c et ts=2 sw=0 sts

#include <iostream>
#include <fstream>
#include <cstring>
#include <cassert>
#include <cstdint>
#include <atomic>
#include <vector>
#include <thread>
#include <string>
#include <dlfcn.h>
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"

#include "whisper_client.h"
#include "rvfi_plusargs.h"
#include "sysmod_plusargs.h"
#include "bridge_plusargs.h"
#include "eot_plusargs.h"
#include "cosim/utils/util.h"
#include "rv_tester_plusargs.h"
#include "rv_tester_structs.h"
#include "cvm/registry.hpp"
#include "nlohmann/json.hpp"

DEFINE_uint64(resetpc, 0x80000000, "Reset PC");
DEFINE_uint64(resetpcfw, 0xC0040000, "Reset firmware PC");
DEFINE_string(whisper_instr_lines, "", "Write instr cache line addresses used in test to a file");
DEFINE_string(whisper_data_lines, "", "Write data cache line addresses used in test to a file");
DEFINE_bool(whisper_csv_log, false, "Make whisper use a csv trace.");
DEFINE_int32(whisper_tlb_size, -1, "Specify whisper tlb size");
DEFINE_string(isa, "", "Override isa spec");
DEFINE_bool(whisper_log, true, "Enable whisper logging to iss_cosim.log and iss_cmd.log");
DEFINE_bool(whisper_cosim_log, false, "Enable whisper logging to iss_cosim.log");
DEFINE_bool(whisper_cmd_log, false, "Enable whisper logging to iss_cmd.log");
DEFINE_string(whisper_stdin, "", "Redirect whisper stdin");
DEFINE_string(whisper_stdout, "", "Redirect whisper stdout");
DEFINE_string(whisper_stderr, "", "Redirect whisper stderr");
DEFINE_string(whisper_json_path, "", "Path to whisper json config");
DEFINE_uint32(whisper_deterministic, 100, "Equivalent to Whisper's deterministic");
DEFINE_uint64(nmi_vec, 0, "NMI handler PC");
DEFINE_uint64(nme_vec, 0, "NMI exception handler PC");
DEFINE_bool(ppo, true, "Enable ppo checks");
DEFINE_bool(traceptw, true, "Enable page table walk tracing");
DEFINE_bool(whisper_auto_increment_timer, false, "Enable whisper auto_increment_timer");
DEFINE_uint64(whisper_aclint_time_adjust, 0, "Set aclint adjust time compare offset");
DEFINE_bool(whisper_vmvr_ignore_vill, false, "Enable whisper vmvr_ignore_vill flag");
DEFINE_string(whisper_loadfrom, "", "Path to whisper Snapshot");
DEFINE_bool(savepoint_en, false, "savepoint_en");
DEFINE_uint32(derr_interrupt_num_override, 0, "DERR interrupt number which can be set dynamically based on chicken bits");
DEFINE_uint32(derr_interrupt_num_default, 23, "DERR interrupt default number");
#include "iss_utils.h"

REGISTRY_register(whisperClient<uint64_t>, TOP.PLATFORM.WHISPER_CLIENT, 0);

extern void (*__tracerExtension)(void*);

uint64_t
getNmiPc() {
  if (FLAGS_nmi_vec != 0) {
    return FLAGS_nmi_vec;
  } else {
    std::string cmd = "nm " + FLAGS_load + " " + FLAGS_bootrom_path + " | grep -w nmivec";
    std::string result = cosim_util::exec(cmd.c_str());
    std::string addr_str = result.substr(0, 16);
    uint64_t nmivec_addr_ = 0;

    try {
      nmivec_addr_ = std::stoul(addr_str, nullptr, 16);
    } catch (...) {
      std::cout << "Warn: No nmivec symbol in elf\n";
    }
    return nmivec_addr_;
  }
}

uint64_t
getNmiExceptionPc() {
  if (FLAGS_nme_vec != 0) {
    return FLAGS_nme_vec;
  } else {
    std::string cmd = "nm " + FLAGS_load + " " + FLAGS_bootrom_path + " | grep -w nmevec";
    std::string result = cosim_util::exec(cmd.c_str());
    std::string addr_str = result.substr(0, 16);
    uint64_t nmevec_addr_ = 0;

    try {
      nmevec_addr_ = std::stoul(addr_str, nullptr, 16);
    } catch (...) {
      std::cout << "Warn: No nmevec symbol in elf\n";
    }
    return nmevec_addr_;
  }
}

template <typename URV>
whisperClient<URV>::whisperClient(cvm::topology::loc_t loc, unsigned) : loc_(loc) {
  cvm::log(cvm::MEDIUM, "[whisperClient] initializing whisperClient\n");

  ncores_ = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  std::string traceFile = (FLAGS_whisper_log || FLAGS_whisper_cosim_log) ? "iss_cosim.log" : "";
  std::string commandLog = (FLAGS_whisper_log || FLAGS_whisper_cmd_log) ? "iss_cmd.log" : "";

  traceFile_ = traceFile.empty() ? nullptr : fopen(traceFile.c_str(), "w");
  commandLog_ = commandLog.empty() ? nullptr : fopen(commandLog.c_str(), "w");
}

template <typename URV>
void whisperClient<URV>::configure() {
  cvm::registry::messenger.procedure<iss_select_rand_RPC>(loc_, [this](uint32_t hart = 0) { return this->get_iss_select(hart); });
  cvm::registry::messenger.procedure<whisperConnectRPC>(loc_, [this]() { return this->whisperConnect(); });
  cvm::registry::messenger.procedure<whisperConnectedRPC>(loc_, [this]() { return this->whisperConnected(); });
  cvm::registry::messenger.procedure<whisperStepRPC>(loc_, [this](int hart, uint64_t time, uint64_t instrTag, uint64_t& pc, uint32_t& instruction, unsigned& changeCount, std::string& disasm, uint32_t& privMode, uint32_t& fpFlags, bool& hasTrap, bool& hasStop, bool& isLoad, bool& isCancelled, bool& valid) { return this->whisperStep(hart, time, instrTag, pc, instruction, changeCount, disasm, privMode, fpFlags, hasTrap, hasStop, isLoad, isCancelled, valid); });
  cvm::registry::messenger.procedure<whisperSimpleStepRPC>(loc_, [this](int hart, uint64_t& pc, uint32_t& instruction, unsigned& changeCount) { return this->whisperSimpleStep(hart, pc, instruction, changeCount); });
  cvm::registry::messenger.procedure<whisperChangeRPC>(loc_, [this](int hart, uint32_t& resource, uint64_t& addr, uint64_t& value, bool& valid) { return this->whisperChange(hart, resource, addr, value, valid); });
  cvm::registry::messenger.procedure<whisperMcmReadRPC>(loc_, [this](int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, unsigned elemIx, unsigned field, bool cache, bool& valid) { return this->whisperMcmRead(hart, time, instrTag, addr, size, value, elemIx, field, cache, valid); });
  cvm::registry::messenger.procedure<whisperMcmVecReadRPC>(loc_, [this](int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, std::vector<uint64_t> value, unsigned elemIx, unsigned field, bool cache, bool& valid) { return this->whisperMcmVecRead(hart, time, instrTag, addr, size, value, elemIx, field, cache, valid); });
  cvm::registry::messenger.procedure<whisperMcmVecInsertRPC>(loc_, [this](int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, std::vector<uint64_t> value, unsigned elemIx, unsigned field, bool& valid) { return this->whisperMcmVecInsert(hart, time, instrTag, addr, size, value, elemIx, field, valid); });
  cvm::registry::messenger.procedure<whisperMcmInsertRPC>(loc_, [this](int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, unsigned elemIx, unsigned field, bool& valid) { return this->whisperMcmInsert(hart, time, instrTag, addr, size, value, elemIx, field, valid); });
  cvm::registry::messenger.procedure<whisperMcmVecBypassRPC>(loc_, [this](int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, std::vector<uint64_t> value, unsigned elemIx, unsigned field, bool cache, bool& valid) { return this->whisperMcmVecBypass(hart, time, instrTag, addr, size, value, elemIx, field, cache, valid); });
  cvm::registry::messenger.procedure<whisperMcmBypassRPC>(loc_, [this](int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, unsigned elemIx, unsigned field, bool cache, bool& valid) { return this->whisperMcmBypass(hart, time, instrTag, addr, size, value, elemIx, field, cache, valid); });
  cvm::registry::messenger.procedure<whisperMcmWriteRPC>(loc_, [this](int hart, uint64_t time, uint64_t addr, unsigned size, svOpenArrayHandle handle, uint64_t mask, bool error, bool& valid) { return this->whisperMcmWrite(hart, time, addr, size, handle, mask, error, valid); });
  cvm::registry::messenger.procedure<whisperMcmIFetchRPC>(loc_, [this](int hart, uint64_t time, uint64_t addr, bool& valid) { return this->whisperMcmIFetch(hart, time, addr, valid); });
  cvm::registry::messenger.procedure<whisperMcmIEvictRPC>(loc_, [this](int hart, uint64_t time, uint64_t addr, bool& valid) { return this->whisperMcmIEvict(hart, time, addr, valid); });
  cvm::registry::messenger.procedure<whisperMcmDEvictRPC>(loc_, [this](int hart, uint64_t time, uint64_t addr, bool& valid) { return this->whisperMcmDEvict(hart, time, addr, valid); });
  cvm::registry::messenger.procedure<whisperMcmDWritebackRPC>(loc_, [this](int hart, uint64_t time, uint64_t addr, bool& valid) { return this->whisperMcmDWriteback(hart, time, addr, valid); });
  // Add MCM Dfetch RPC
  cvm::registry::messenger.procedure<whisperMcmDFetchRPC>(loc_, [this](int hart, uint64_t time, uint64_t addr, bool& valid) { return this->whisperMcmDFetch(hart, time, addr, valid); });
  cvm::registry::messenger.procedure<whisperMcmEndRPC>(loc_, [this](int hart, uint64_t time, bool& valid) { return this->whisperMcmEnd(hart, time, valid); });
  cvm::registry::messenger.procedure<whisperInjectExceptionRPC>(loc_, [this](int hart, bool isLoad, uint64_t code, unsigned elemIx, uint64_t addr, bool& valid) { return this->whisperInjectException(hart, isLoad, code, elemIx, addr, valid); });
  cvm::registry::messenger.procedure<whisperPokeRPC>(loc_, [this](int hart, uint64_t time, char resource, uint64_t addr, uint64_t value, bool cache, bool skipmem, bool& valid) { return this->whisperPoke(hart, time, resource, addr, value, cache, skipmem, valid); });
  cvm::registry::messenger.procedure<whisperPokeMemRPC>(loc_, [this](int hart, uint64_t time, char resource, uint64_t addr, unsigned size, uint64_t value, bool cache, bool skipmem, bool& valid) { return this->whisperPokeMem(hart, time, resource, addr, size, value, cache, skipmem, valid); });
  cvm::registry::messenger.procedure<whisperPokeMemBatchRPC>(loc_, [this](int hart, uint64_t time, char resource, uint64_t addr, const std::vector<uint8_t>& data, bool& valid) { return this->whisperPokeMemBatch(hart, time, resource, addr, data, valid); });
  cvm::registry::messenger.procedure<whisperPeekRPC>(loc_, [this](int hart, char resource, uint64_t addr, uint64_t& value, bool& valid) { return this->whisperPeek(hart, resource, addr, value, valid); });
  cvm::registry::messenger.procedure<whisperPeekExtendedRPC>(loc_, [this](int hart, char resource, uint64_t addr, uint64_t& value, uint64_t& reply_addr, bool& valid) { return this->whisperPeek(hart, resource, addr, value, reply_addr, valid); });
  cvm::registry::messenger.procedure<whisperPeekPcRPC>(loc_, [this](int hart, uint64_t& value) { return this->whisperPeekPc(hart, value); });
  cvm::registry::messenger.procedure<whisperPeekCsrRPC>(loc_, [this](int hart, uint64_t addr, uint64_t& value, uint64_t& mask, uint64_t& reset_value, uint64_t& read_mask, bool& valid) { return this->whisperPeekCsr(hart, addr, value, mask, reset_value, read_mask, valid); });
  cvm::registry::messenger.procedure<whisperResetRPC>(loc_, [this](int hart, uint64_t addr, bool& valid) { return this->whisperReset(hart, addr, valid); });
  cvm::registry::messenger.procedure<whisperQuitRPC>(loc_, [this]() { return this->whisperQuit(); });
  cvm::registry::messenger.procedure<whisperPageTableWalkRPC>(loc_, [this](int hart, bool isInstr, bool isAddr, svOpenArrayHandle items, unsigned& itemCount, bool& valid) { return this->whisperPageTableWalk(hart, isInstr, isAddr, items, itemCount, valid); });
  cvm::registry::messenger.procedure<whisperTranslateRPC>(loc_, [this](int hart, uint64_t vaddr, bool r, bool w, bool x, bool twoStage, bool supervisor, uint64_t& paddr, bool& valid) { return this->whisperTranslate(hart, vaddr, r, w, x, twoStage, supervisor, paddr, valid); });
  cvm::registry::messenger.procedure<whisperEnterDebugRPC>(loc_, [this](int hart) { return this->whisperEnterDebug(hart); });
  cvm::registry::messenger.procedure<whisperExitDebugRPC>(loc_, [this](int hart) { return this->whisperExitDebug(hart); });
  cvm::registry::messenger.procedure<whisperCheckInterruptRPC>(loc_, [this](int hart, bool& interrupt, uint64_t& cause, bool& virt_mode) { return this->whisperCheckInterrupt(hart, interrupt, cause, virt_mode); });
  cvm::registry::messenger.procedure<whisperGetSeiPinRPC>(loc_, [this](int hart, uint64_t& value) { return this->whisperGetSeiPin(hart, value); });
  cvm::registry::messenger.procedure<whisperCancelLrRPC>(loc_, [this](int hart, bool& valid) { return this->whisperCancelLr(hart, valid); });
  cvm::registry::messenger.procedure<whisperPeekGprRPC>(loc_, [this](int hart, uint64_t addr, uint64_t& value) { return this->whisperPeekGpr(hart, addr, value); });
  cvm::registry::messenger.procedure<whisperPeekFprRPC>(loc_, [this](int hart, uint64_t addr, uint64_t& value) { return this->whisperPeekFpr(hart, addr, value); });
  cvm::registry::messenger.procedure<whisperPeekVprRPC>(loc_, [this](int hart, uint64_t addr, std::array<std::uint8_t, 32>& value) { return this->whisperPeekVpr(hart, addr, value); });
  cvm::registry::messenger.procedure<whisperGetLastLdStAddressRPC>(loc_, [this](int hart, uint64_t& pa) { return this->whisperGetLastLdStAddress(hart, pa); });
  cvm::registry::messenger.procedure<whisperNmiRPC>(loc_, [this](int hart, uint64_t time, uint64_t cause) { return this->whisperNmi(hart, time, cause); });
  cvm::registry::messenger.procedure<whisperClearNmiRPC>(loc_, [this](int hart, uint64_t time) { return this->whisperClearNmi(hart, time); });
  cvm::registry::messenger.procedure<whisperClearNmiCauseRPC>(loc_, [this](int hart, uint64_t time, uint64_t cause) { return this->whisperClearNmiCause(hart, time, cause); });
  cvm::registry::messenger.procedure<whisperSnapshotSaveRPC>(loc_, [this]() { return this->whisperSnapshotSave(); });
  cvm::registry::messenger.procedure<whisperMcmSkipReadDataCheckRPC>(loc_, [this](uint64_t addr, unsigned size, bool enable) { return this->whisperMcmSkipReadDataCheck(addr, size, enable); });
}

template <typename URV>
bool whisperClient<URV>::constructSystem(std::shared_ptr<WdRiscv::Session<URV>>& session, std::shared_ptr<WdRiscv::System<URV>>& system, WdRiscv::Args& args, uint16_t ncores, bool standalone, std::string logfile) {
  std::vector<std::string> args_str = {"whisper"};
  auto config_file = overrideWhisperJson(standalone);
  cvm::log(cvm::HIGH, "Whisper config changed to: {}\n", config_file);
  args_str.insert(args_str.end(), {"--config", config_file});
  args_str.insert(args_str.end(), {"--cores", std::to_string(ncores)});
  args_str.insert(args_str.end(), {"--nmivec", std::to_string(getNmiPc())});
  args_str.insert(args_str.end(), {"--nmevec", std::to_string(getNmiExceptionPc())});

  if (!standalone && FLAGS_savepoint_en) // Cosim capture
    args_str.insert(args_str.end(), "--hintops");
  if (!standalone && (FLAGS_whisper_loadfrom != ""))
    args_str.insert(args_str.end(), {"--loadfrom", FLAGS_whisper_loadfrom, "--elfaftersnap"});

  if (FLAGS_cplfw && FLAGS_cplfw_path != "")
    args_str.push_back(FLAGS_cplfw_path);
  if (FLAGS_bootrom && FLAGS_bootrom_path != "")
    args_str.push_back(FLAGS_bootrom_path);
  if (FLAGS_debugrom && FLAGS_debugrom_path != "")
    args_str.push_back(FLAGS_debugrom_path);
  if (FLAGS_load != "")
    args_str.push_back(FLAGS_load);
  if (FLAGS_traceptw)
    args_str.push_back("--traceptw");
  if (FLAGS_whisper_csv_log)
    args_str.push_back("--csv");
  if (FLAGS_hex != "") {
    auto hex_files = cosim_util::split_string(FLAGS_hex, ",");
    for (const auto& hex : hex_files)
      args_str.insert(args_str.end(), {"--hex", hex});
  }
  if (FLAGS_load_lz4 != "") {
    auto lz4_files = cosim_util::split_string(FLAGS_load_lz4, ",");
    for (const auto& lz4 : lz4_files)
      args_str.insert(args_str.end(), {"--lz4", lz4});
  }
  if (FLAGS_load_bin != "") {
    auto bin_files = cosim_util::split_string(FLAGS_load_bin, ",");
    for (const auto& bin : bin_files)
      args_str.insert(args_str.end(), {"--binary", bin});
  }
  if (FLAGS_isa != "")
    args_str.insert(args_str.end(), {"--isa", FLAGS_isa});

  if (FLAGS_whisper_stderr != "")
    args_str.insert(args_str.end(), {"--stderr", FLAGS_whisper_stderr});
  else if (!standalone)
    args_str.insert(args_str.end(), {"--stderr", "/dev/null"});

  if (FLAGS_whisper_stdout != "")
    args_str.insert(args_str.end(), {"--stdout", FLAGS_whisper_stdout});
  else if (!standalone)
    args_str.insert(args_str.end(), {"--stdout", "/dev/null"});

  if (FLAGS_whisper_stdin != "")
    args_str.insert(args_str.end(), {"--stdin", FLAGS_whisper_stdin});
  else if (!standalone)
    args_str.insert(args_str.end(), {"--stdin", "/dev/null"});

  if (FLAGS_stee_secure_region != "")
    args_str.insert(args_str.end(), {"--steesr", FLAGS_stee_secure_region});

  if (!standalone && FLAGS_eot_mem_check && FLAGS_whisper_data_lines == "")
    FLAGS_whisper_data_lines = "whisper_data_lines.log";
  if (FLAGS_whisper_data_lines != "")
    args_str.insert(args_str.end(), {"--datalines", FLAGS_whisper_data_lines});
  if (FLAGS_whisper_instr_lines != "")
    args_str.insert(args_str.end(), {"--instrlines", FLAGS_whisper_instr_lines});

  if (FLAGS_whisper_tlb_size >= 0 ||
      FLAGS_whisper_tlb_size > ncores)
    args_str.insert(args_str.end(), {"--tlbsize", std::to_string(FLAGS_whisper_tlb_size)});

  auto resetpc = FLAGS_resetpc;
  if (FLAGS_cplfw)
    resetpc = FLAGS_resetpcfw;
  // Remove last term?
  if (standalone || (FLAGS_whisper_loadfrom == ""))
    args_str.insert(args_str.end(), {"--startpc", std::to_string(resetpc)});

  if (standalone) {
    if (logfile != "")
      args_str.insert(args_str.end(), {"--logfile", logfile});
    if (FLAGS_preload)
      args_str.insert(args_str.end(), {"--initstate", "preload_0.csv"});
    if (FLAGS_max_instr)
      args_str.insert(args_str.end(), {"--maxinst", std::to_string(FLAGS_max_instr)});
    if (FLAGS_tohost)
      args_str.insert(args_str.end(), {"--tohost", std::to_string(FLAGS_tohost)});
    if (FLAGS_eot != "tohost_all")
      args_str.push_back("--quitany");
    if (ncores > 1)
      args_str.insert(args_str.end(), {"--deterministic", std::to_string(FLAGS_whisper_deterministic)});
  } else {
    if (FLAGS_mcm) {
      args_str.push_back("--mcm");
      if (!FLAGS_cache_model_en)
        args_str.push_back("--dismcmcache");
      if (!FLAGS_ppo)
        args_str.push_back("--noppo");
    } else {
      args_str.push_back("--dismcmcache");
    }
  }
  std::string string_ = "";
  for (auto& i : args_str)
    string_ = string_ + i + " ";
  cvm::log(cvm::MEDIUM, "(Equivalent) Whisper raw command: {}\n", string_);

  args.parseCmdLineArgs(args_str);
  WdRiscv::HartConfig config;
  if (!config.loadConfigFile(config_file.c_str()))
    return false;

  session = std::make_shared<WdRiscv::Session<URV>>();
  if (session == nullptr)
    return false;
  system = session->defineSystem(args, config);
  if (system == nullptr)
    return false;
  bool ok = session->configureSystem(args, config);
  if (ok && standalone) {                     // remove this block and integrate with override json
    system->setAplicAutoForwardViaMsi(false); // currently, these dont have a mapping in whisper json
    for (unsigned i = 0; i < system->hartCount(); ++i) {
      WdRiscv::Hart<URV>* hart = system->ithHart(i).get();
      hart->setAclintDeliverInterrupts(false);
    }
  }
  return ok;
}

template <typename URV>
int whisperClient<URV>::whisperStandalone() {
  bool ok = session_->run(args_);
  ok = ok && session_->cleanup(args_);
  if (!ok) {
    cvm::log(cvm::ERROR, "Error: Test failed on Standalone Whisper, stopping simulation\n");
    return 1;
  } else {
    for (unsigned i = 0; i < system_->hartCount(); ++i) {
      auto instr_count = system_->ithHart(i).get()->getInstructionCount();
      if (instr_count >= FLAGS_max_instr) {
        cvm::log(cvm::ERROR, "Error: Test reached max instr on standalone Whisper, stopping simulation\n");
        return 1;
      }
    }
  }
  return 0;
}

template <typename URV>
int whisperClient<URV>::whisperConnect() {
  if (FLAGS_preload && FLAGS_standalone && ncores_ > 1)
    cvm::log(cvm::ERROR, "Error: Preloading works only on single core runs and +standalone plusarg enabled\n");

  if (FLAGS_standalone) {
    cvm::log(cvm::MEDIUM, "Running Whisper standalone\n");
    args_ = WdRiscv::Args();
    if (!constructSystem(session_, system_, args_, FLAGS_num_harts, true, "iss_standalone.log"))
      cvm::log(cvm::ERROR, "Error: could not construct system\n");
    int failed = whisperStandalone();
    if (failed)
      return failed;
    failed = processStandaloneInfo();
    if (failed)
      return failed;
  }

  cvm::log(cvm::HIGH, "Construct Whisper for cosim\n");
  args_ = WdRiscv::Args();
  if (!constructSystem(session_, system_, args_, ncores_, false))
    cvm::log(cvm::ERROR, "Error: could not construct system\n");
  server_ = std::make_unique<WdRiscv::Server<URV>>(*system_);

  // Coverage setup
  if (FLAGS_cov) {
    auto soPtr = dlopen(FLAGS_archsample_lib_path.c_str(), RTLD_NOW);
    if (not soPtr)
      cvm::log(cvm::ERROR, "Error: Failed to load shared library {}\n", dlerror());

    std::string entry("tracerExtension");
    entry += sizeof(URV) == 4 ? "32" : "64";

    __tracerExtension = reinterpret_cast<void (*)(void*)>(dlsym(soPtr, entry.c_str()));
    if (not __tracerExtension)
      cvm::log(cvm::ERROR, "Error: Could not find symbol tracerExtension in {} \n", std::string(FLAGS_archsample_lib_path));
  }

  // Signal to subscribers that whisper is ready to receive cosim calls
  cvm::registry::messenger.signal<rv_tester::whisper_connected>(loc_, rv_tester::whisper_connected{});
  return 0;
}

template <typename URV>
bool whisperClient<URV>::whisperConnected() {
  return server_ != nullptr;
}

template <typename URV>
bool whisperClient<URV>::whisperCommand(const WhisperMessage& req, WhisperMessage& reply) {
  if (FLAGS_cosim && (server_ != nullptr)) {
    server_->interact(req, reply, traceFile_, commandLog_);
  }
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperPeek(int hart, char resource, uint64_t addr, uint64_t& value,
                                     bool& valid) {
  uint64_t unused_addr;
  return whisperPeek(hart, resource, addr, value, unused_addr, valid);
}

template <typename URV>
bool whisperClient<URV>::whisperPeek(int hart, char resource, uint64_t addr, uint64_t& value,
                                     uint64_t& reply_addr, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::Peek;
  req.resource = resource;
  req.address = addr & ~FLAGS_pa_mask;
  req.tag[0] = 0;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  value = reply.value;
  reply_addr = reply.address;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperPeekCsr(int hart, uint64_t addr, uint64_t& value, uint64_t& mask,
                                        uint64_t& poke_mask, uint64_t& read_mask, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::Peek;
  req.resource = 'c';
  req.address = addr;
  req.tag[0] = 0;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  value = reply.value;
  mask = reply.address;
  poke_mask = reply.time;
  read_mask = reply.instrTag;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperPeekPc(int hart, uint64_t& value) {
  req.hart = hart;
  req.type = WhisperMessageType::Peek;
  req.resource = 'p';
  req.tag[0] = 0;

  if (not whisperCommand(req, reply))
    return false;

  value = reply.value;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperPeekGpr(int hart, uint64_t addr, uint64_t& value) {
  req.hart = hart;
  req.type = WhisperMessageType::Peek;
  req.resource = 'r';
  req.address = addr;
  req.tag[0] = 0;

  if (not whisperCommand(req, reply))
    return false;

  value = reply.value;
  return true;
}
template <typename URV>
bool whisperClient<URV>::whisperPeekFpr(int hart, uint64_t addr, uint64_t& value) {
  req.hart = hart;
  req.type = WhisperMessageType::Peek;
  req.resource = 'f';
  req.address = addr;
  req.tag[0] = 0;

  if (not whisperCommand(req, reply))
    return false;

  value = reply.value;
  return true;
}
template <typename URV>
bool whisperClient<URV>::whisperPeekVpr(int hart, uint64_t addr, std::array<std::uint8_t, 32>& value) {
  req.hart = hart;
  req.type = WhisperMessageType::Peek;
  req.resource = 'v';
  req.address = addr;
  req.tag[0] = 0;

  if (not whisperCommand(req, reply))
    return false;

  for (int i = 0; i < 32; i++) {
    value[i] = reply.buffer[i];
  }
  return true;
}

// Send a whisper InjectException command. Return true on successful comunication
// and false on failure. Set valid to false if hart/resource/addr
// are invalid.
template <typename URV>
bool whisperClient<URV>::whisperInjectException(int hart, bool isLoad, uint64_t code, unsigned elemIx,
                                                uint64_t addr, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::InjectException;
  WhisperFlags wflags;
  wflags.bits.load = isLoad;
  req.flags = wflags.value;
  req.address = code;
  req.resource = elemIx;
  req.value = addr;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

// Send a whisper poke command. Retrun true on successful comunication
// and false on failure. Set valid to false if hart/resource/addr
// are invalid.
template <typename URV>
bool whisperClient<URV>::whisperPoke(int hart, uint64_t time, char resource, uint64_t addr, uint64_t value, bool cache, bool skipmem,
                                     bool& valid) {
  WhisperFlags wflags;
  wflags.bits.cache = cache & FLAGS_cache_model_en;
  wflags.bits.skipMem = skipmem & FLAGS_cache_model_en;
  req.flags = wflags.value;
  req.hart = hart;
  req.type = WhisperMessageType::Poke;
  req.resource = resource;
  req.address = addr;
  req.value = value;
  req.time = time;
  req.tag[0] = 0;

  // cvm::log(cvm::MEDIUM, "Poke address : {:#x}, cache flag : {}, skipMem flag: {} DATA: {:#x}\n",addr, cache, skipmem, value);

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

// Specialized poke for memory that accepts size argument.
template <typename URV>
bool whisperClient<URV>::whisperPokeMem(int hart, uint64_t time, char resource, uint64_t addr, unsigned size,
                                        uint64_t value, bool cache, bool skipmem, bool& valid) {
  WhisperFlags wflags;
  wflags.bits.cache = cache;
  wflags.bits.skipMem = skipmem;
  req.flags = wflags.value;
  req.hart = hart;
  req.type = WhisperMessageType::Poke;
  req.resource = resource;
  req.address = addr & ~FLAGS_pa_mask;
  req.value = value;
  req.time = time;
  req.size = size;
  req.tag[0] = 0;

  cvm::log(cvm::FULL, "Poke Mem address : {:#x}, cache flag : {}, skipMem flag : {}, size : {}, DATA : {:#X}\n", addr, cache, skipmem, size, value);

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperPokeMemBatch(int hart, uint64_t time, char resource, uint64_t addr,
                                             const std::vector<uint8_t>& data, bool& valid) {
  valid = true;

  // Process data in 8-byte (uint64_t) chunks
  for (size_t i = 0; i < data.size(); i += 8) {
    uint64_t value = 0;
    unsigned chunk_size = std::min(static_cast<size_t>(8), data.size() - i);

    // Pack bytes into uint64_t (little-endian)
    for (unsigned j = 0; j < chunk_size; ++j) {
      value |= static_cast<uint64_t>(data[i + j]) << (j * 8);
    }

    bool chunk_valid = true;
    if (!whisperPokeMem(hart, time, resource, addr + i, chunk_size, value, false, false, chunk_valid)) {
      // #FIXME : Currently cache and skip mem flag are marked as false - fix while enabling MCM caches
      valid = false;
      return false;
    }

    if (!chunk_valid) {
      valid = false;
    }
  }

  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperStep(int hart, uint64_t time, uint64_t instrTag, uint64_t& pc,
                                     uint32_t& instruction, unsigned& changeCount,
                                     std::string& disasm, uint32_t& privMode,
                                     uint32_t& fpFlags, bool& hasTrap, bool& hasStop, bool& isLoad, bool& isCancelled, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::Step;
  req.instrTag = instrTag;
  req.time = time;

  if (not whisperCommand(req, reply))
    return false;

  pc = reply.address;
  instruction = reply.resource;
  changeCount = reply.value;

  // Recover privilege mode (2 bits), fpFlags (5 bits), and trap (1
  // bit) from flags field.
  WhisperFlags wflags(reply.flags);
  unsigned mode = wflags.bits.privMode;
  unsigned flags = wflags.bits.fpFlags;
  unsigned trap = wflags.bits.trap;
  unsigned stop = wflags.bits.stop;
  unsigned virt = wflags.bits.virt;
  unsigned debug = wflags.bits.debug;
  unsigned load = wflags.bits.load;
  unsigned cancelled = wflags.bits.cancelled;

  privMode = debug ? 6 : mode | (virt << 3);
  fpFlags = flags;
  hasTrap = trap;
  hasStop = stop;
  isLoad = load;
  isCancelled = cancelled;
  reply.buffer[reply.buffer.size() - 1] = '\0';
  disasm = reply.buffer.data();

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

// Copied from chuang's LSTB Whisper Client
template <typename URV>
bool whisperClient<URV>::whisperSimpleStep(int hart, uint64_t& pc, uint32_t& instruction, unsigned& changeCount) {
  req.type = WhisperMessageType::Step;
  req.hart = hart;
  req.time = 0;
  req.instrTag = 0;

  WhisperMessage reply;
  if (not whisperCommand(req, reply))
    return false;

  pc = reply.address;
  instruction = reply.resource;
  changeCount = reply.value;

  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperChange(int hart, uint32_t& resource, uint64_t& addr, uint64_t& value,
                                       bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::Change;

  if (not whisperCommand(req, reply))
    return false;

  resource = reply.resource;
  addr = reply.address;
  value = reply.value;
  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperGetLastLdStAddress(int hart, uint64_t& value) {
  req.hart = hart;
  req.type = WhisperMessageType::Peek;
  req.resource = 's';
  req.address = WhisperSpecialResource::LastLdStAddress;
  req.tag[0] = 0;

  WhisperMessage reply;
  if (not whisperCommand(req, reply))
    return false;
  value = reply.value;
  return true;
}

std::vector<uint8_t> convert_to_byte_array(const std::vector<uint64_t>& dword_array) {
  const uint8_t* begin = reinterpret_cast<const uint8_t*>(dword_array.data());
  const uint8_t* end = begin + dword_array.size() * sizeof(uint64_t);
  std::vector<uint8_t> result(begin, end);
  // std::reverse(result.begin(), result.end());
  return result;
}

template <typename URV>
bool whisperClient<URV>::whisperMcmRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
                                        unsigned size, uint64_t value, unsigned elemIx,
                                        unsigned field, bool cache, bool& valid) {
  WhisperFlags wflags;
  wflags.bits.cache = cache;
  req.flags = wflags.value;
  req.type = WhisperMessageType::McmRead;
  req.hart = hart;
  req.time = time;
  req.value = value;
  req.size = size;
  req.address = addr & ~FLAGS_pa_mask;
  req.instrTag = instrTag;
  req.resource = (elemIx << 16) | (field & 0xffff); // Pack elemIx and field into resource.

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperMcmVecRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
                                           unsigned size, std::vector<uint64_t> value,
                                           unsigned elemIx, unsigned field, bool cache, bool& valid) {
  std::vector<uint8_t> byte_value = convert_to_byte_array(value);

  if (size <= 8) {
    uint64_t u64 = 0;
    for (unsigned i = 0; i < size; ++i) {
      uint8_t byte = byte_value[i];
      u64 = (u64 << 8) | byte;
    }
    return whisperMcmRead(hart, time, instrTag, addr, size, value[0], elemIx, field, cache, valid);
  }

  WhisperFlags wflags;
  wflags.bits.cache = cache;
  req.flags = wflags.value;
  req.hart = hart;
  req.type = WhisperMessageType::McmRead;
  req.time = time;
  req.instrTag = instrTag;
  req.address = addr & ~FLAGS_pa_mask;
  req.size = size;                                  // Total size in bytes
  req.resource = (elemIx << 16) | (field & 0xffff); // Pack elemIx and field into resource.

  for (unsigned i = 0; i < size; ++i) {
    req.buffer.at(i) = byte_value[i];
  }

  WhisperMessage reply;
  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperMcmVecInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
                                             unsigned size, std::vector<uint64_t> value, unsigned elemIx, unsigned field, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::McmInsert;
  req.time = time;
  req.instrTag = instrTag;
  req.address = addr & ~FLAGS_pa_mask;
  req.size = size;                                  // Total size in bytes
  req.resource = (elemIx << 16) | (field & 0xffff); // Pack elemIx and field into resource.

  std::vector<uint8_t> byte_value = convert_to_byte_array(value);

  if (size <= 8) {
    uint64_t u64 = 0;
    for (unsigned i = 0; i < size; ++i) {
      uint8_t byte = byte_value[i];
      u64 = (u64 << 8) | byte;
    }
    return whisperMcmInsert(hart, time, instrTag, (addr & ~FLAGS_pa_mask), size, value[0], elemIx, field, valid);
  }

  for (unsigned i = 0; i < size; ++i) {
    req.buffer.at(i) = byte_value[i];
  }

  WhisperMessage reply;
  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperMcmInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
                                          unsigned size, uint64_t value, unsigned elemIx, unsigned field, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::McmInsert;
  req.time = time;
  req.instrTag = instrTag;
  req.address = addr & ~FLAGS_pa_mask;
  req.value = value;
  req.size = size;
  req.resource = (elemIx << 16) | (field & 0xffff); // Pack elemIx and field into resource.

  if (size > 8) {
    // Inserts with size larger than 8 should use the vector interface to pass
    // the vector data. Here we accept size larger than 8 if the data is zero
    // (this maybe used for the cbo.zero instruction).
    assert(value == 0);
    req.buffer.fill(0);
  }

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperMcmVecBypass(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
                                             unsigned size, std::vector<uint64_t> value, unsigned elemIx, unsigned field, bool cache, bool& valid) {
  WhisperFlags wflags;
  wflags.bits.cache = cache;
  req.flags = wflags.value;
  req.hart = hart;
  req.type = WhisperMessageType::McmBypass;
  req.time = time;
  req.instrTag = instrTag;
  req.address = addr & ~FLAGS_pa_mask;
  req.size = size;                                  // Total size in bytes
  req.resource = (elemIx << 16) | (field & 0xffff); // Pack elemIx and field into resource.

  std::vector<uint8_t> byte_value = convert_to_byte_array(value);

  if (size <= 8) {
    uint64_t u64 = 0;
    for (unsigned i = 0; i < size; ++i) {
      uint8_t byte = byte_value[i];
      u64 = (u64 << 8) | byte;
    }
    return whisperMcmBypass(hart, time, instrTag, (addr & ~FLAGS_pa_mask), size, value[0], elemIx, field, cache, valid);
  }

  for (unsigned i = 0; i < size; ++i) {
    req.buffer.at(i) = byte_value[i];
  }

  WhisperMessage reply;
  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperMcmBypass(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
                                          unsigned size, uint64_t value, unsigned elemIx, unsigned field, bool cache, bool& valid) {
  WhisperFlags wflags;
  wflags.bits.cache = cache;
  req.flags = wflags.value;
  req.hart = hart;
  req.type = WhisperMessageType::McmBypass;
  req.time = time;
  req.instrTag = instrTag;
  req.address = addr & ~FLAGS_pa_mask;
  req.value = value;
  req.size = size;
  req.resource = (elemIx << 16) | (field & 0xffff); // Pack elemIx and field into resource.

  if (size > 8) {
    // Bypasses with size larger than 8 should use the vector interface to pass
    // the vector data. Here we accept size larger than 8 if the data is zero
    // (this maybe used for the cbo.zero instruction).
    assert(value == 0);
    req.buffer.fill(0);
  }

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperMcmWrite(int hart, uint64_t time, uint64_t addr,
                                         unsigned size, svOpenArrayHandle handle, uint64_t mask, bool error, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::McmWrite;
  req.time = time;
  req.address = addr & ~FLAGS_pa_mask;
  req.size = size;
  req.flags = 1;
  req.flags |= (error << 1);
  for (uint8_t i = 0; i < req.size / 8; ++i)
    req.tag[i] = (uint8_t)((mask >> (i * 8)) & 0xff);

  if (req.size > req.buffer.size()) {
    cvm::log(cvm::ERROR, "Error: whisperMcmWrite: write size {} too large\n", req.size);
    valid = false;
    return true;
  }

  uint8_t* data = reinterpret_cast<uint8_t*>(handle);
  for (unsigned i = 0; i < size; ++i)
    req.buffer[i] = data[i];

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperMcmIFetch(int hart, uint64_t time, uint64_t addr, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::McmIFetch;
  req.time = time;
  req.address = addr;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperMcmSkipReadDataCheck(uint64_t addr, unsigned size, bool enable) {
  req.value = enable;
  req.type = WhisperMessageType::McmSkipReadChk;
  req.size = size;
  req.address = addr;

  if (not whisperCommand(req, reply))
    return false;

  return true;
}
// Creating a Remote Procedural Call for skip Read Data check

template <typename URV>
bool whisperClient<URV>::whisperMcmIEvict(int hart, uint64_t time, uint64_t addr, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::McmIEvict;
  req.time = time;
  req.address = addr;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

// Remote Procedural Call for MCM Devict
template <typename URV>
bool whisperClient<URV>::whisperMcmDEvict(int hart, uint64_t time, uint64_t addr, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::McmDEvict;
  req.time = time;
  req.address = addr & ~FLAGS_pa_mask;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

// Remote Procedural Call for MCM Devict
template <typename URV>
bool whisperClient<URV>::whisperMcmDWriteback(int hart, uint64_t time, uint64_t addr, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::McmDWriteback;
  req.time = time;
  req.size = 0; // currently not sending data as a a part of writeback
  req.buffer.fill(0);
  req.value = 0;
  req.address = addr & ~FLAGS_pa_mask;

  if (not whisperCommand(req, reply)) {
    return false;
  }

  valid = reply.type != WhisperMessageType::Invalid;
  cvm::log(cvm::FULL, "valid : {}\n", valid);
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperMcmDFetch(int hart, uint64_t time, uint64_t addr, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::McmDFetch;
  req.time = time;
  req.address = addr & ~FLAGS_pa_mask;

  if (not whisperCommand(req, reply)) {
    return false;
  }

  valid = reply.type != WhisperMessageType::Invalid;
  cvm::log(cvm::FULL, "dfetch valid : {}\n", valid);
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperMcmEnd(int hart, uint64_t time, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::McmEnd;
  req.time = time;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperTranslate(int hart, uint64_t vaddr, bool r, bool w, bool x, bool twoStage,
                                          bool supervisor, uint64_t& paddr, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::Translate;
  req.address = vaddr;
  req.flags = 0;

  if (r)
    req.flags |= 1;
  else if (w)
    req.flags |= 2;
  else if (x)
    req.flags |= 4;

  if (supervisor)
    req.flags |= 8;
  if (twoStage)
    req.flags |= 16;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  if (valid)
    paddr = reply.address;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperCancelLr(int hart, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::CancelLr;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperReset(int hart, uint64_t addr, bool& valid) {
  req.hart = hart;
  req.type = WhisperMessageType::Reset;
  req.address = addr;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperQuit() {
  WhisperMessage req(0 /*hart*/, WhisperMessageType::Quit); // Any hart will do
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperPageTableWalk(int, bool, bool,
                                              svOpenArrayHandle, unsigned&, bool&) {
  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperEnterDebug(int hart) {
  std::cout << "Whisper client Enter Debug\n";
  req.hart = hart;
  req.type = WhisperMessageType::EnterDebug;

  if (not whisperCommand(req, reply))
    return false;

  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperExitDebug(int hart) {
  std::cout << "Whisper client Exit Debug\n";
  req.hart = hart;
  req.type = WhisperMessageType::ExitDebug;

  if (not whisperCommand(req, reply))
    return false;

  return true;
}

// Send a whisper check_interrupt command. Return true on successful communication
// and false on failure. Set interrupt to true/false if interrupt is/is-not
// possible assuming the MIP CSR has the given mip value.
template <typename URV>
bool whisperClient<URV>::whisperCheckInterrupt(int hart, bool& interrupt, uint64_t& cause, bool& virt_mode) {
  req.hart = hart;
  req.type = WhisperMessageType::CheckInterrupt;

  WhisperMessage reply;

  if (not whisperCommand(req, reply))
    return false;

  interrupt = reply.flags & 1;
  virt_mode = (reply.flags & 2) != 0;
  cause = reply.value;

  return true;
}

// Set the supervisor mode external interrupt pin to the given
// value (0 or 1). Whisper will consider either the SEIP bit in MIP or the
// the external pin value set by this method for taking a supervisor
// external interrupt (assuming that interrupt is enabled).
template <typename URV>
bool whisperClient<URV>::whisperGetSeiPin(int hart, uint64_t& value) {
  req.hart = hart;
  req.type = WhisperMessageType::Peek;
  req.resource = 's';
  req.address = WhisperSpecialResource::Seipin;
  req.tag[0] = 0;

  WhisperMessage reply;

  if (not whisperCommand(req, reply))
    return false;

  value = reply.value;

  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperNmi(int hart, uint64_t time, uint64_t cause) {
  req.hart = hart;
  req.type = WhisperMessageType::Nmi;
  req.time = time;
  req.value = cause;

  if (not whisperCommand(req, reply))
    return false;

  return true;
}

template <typename URV>
bool whisperClient<URV>::whisperClearNmi(int hart, uint64_t time) {
  req.hart = hart;
  req.type = WhisperMessageType::ClearNmi;
  req.flags = 1; // Clear all.
  req.time = time;

  return whisperCommand(req, reply);
}

template <typename URV>
bool whisperClient<URV>::whisperClearNmiCause(int hart, uint64_t time, uint64_t cause) {
  req.hart = hart;
  req.type = WhisperMessageType::ClearNmi;
  req.flags = 0; // Clear one.
  req.value = cause;
  req.time = time;

  return whisperCommand(req, reply);
}

// Static function for whisper JSON override
template <typename URV>
std::string
whisperClient<URV>::overrideWhisperJson(bool standalone) {
  nlohmann::json j;
  try {
    std::ifstream ifs(FLAGS_whisper_json_path);
    j = nlohmann::json::parse(ifs);
    ifs.close();
  } catch (...) {
    cvm::log(cvm::ERROR, "Error: Unable to parse whisper json from +whisper_json_path\n");
  }

  if (standalone)
    j["wfi_timeout"] = 0;

  if (!standalone)
    j["auto_increment_timer"] = FLAGS_whisper_auto_increment_timer;

  if (j.contains("aclint"))
    if (!standalone)
      j["aclint"]["time_adjust"] = FLAGS_whisper_aclint_time_adjust;

  if (FLAGS_whisper_vmvr_ignore_vill)
    j["vector"]["vmvr_ignore_vill"] = true;

  if (FLAGS_derr_interrupt_num_override && (FLAGS_derr_interrupt_num_override != FLAGS_derr_interrupt_num_default)) {
    if (j.contains("csr") and j["csr"].contains("mie") and j["csr"]["mie"].contains("mask")) {
      auto data = ((std::stoull(std::string(j["csr"]["mie"]["mask"]), nullptr, 0)) | (1ull << FLAGS_derr_interrupt_num_override)) & ~(1ull << FLAGS_derr_interrupt_num_default);
      j["csr"]["mie"]["mask"] = fmt::format("{:#x}", data);
    }
    if (j.contains("csr") and j["csr"].contains("mip") and j["csr"]["mip"].contains("mask")) {
      auto data = ((std::stoull(std::string(j["csr"]["mip"]["mask"]), nullptr, 0)) | (1ull << FLAGS_derr_interrupt_num_override)) & ~(1ull << FLAGS_derr_interrupt_num_default);
      j["csr"]["mip"]["mask"] = fmt::format("{:#x}", data);
    }
  }

  std::string whisper_override_json = standalone ? "whisper_standalone.json" : "whisper_cosim.json";
  std::ofstream o(whisper_override_json);
  o << std::setw(4) << j << std::endl;
  o.close();
  return whisper_override_json;
}

template <typename URV>
bool whisperClient<URV>::whisperSnapshotSave() {
  system_->saveSnapshot("snapshot0");
  return true;
}

template class whisperClient<uint32_t>;
template class whisperClient<uint64_t>;
