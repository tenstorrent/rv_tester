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
#include "HartConfig.hpp"
#include "Hart.hpp"
#include "cosim/dut_if/rvfi/rvfi_plusargs.h"
#include "sysmod/sysmod_plusargs.h"
#include "cosim/bridge/bridge_plusargs.h"
#include "cosim/utils/eot/eot_plusargs.h"
#include "cosim/utils/general/util.h"
#include "rv_tester_plusargs.h"
#include "rv_tester_structs.h"
#include "cvm/registry.hpp"


DEFINE_uint64(resetpc, 0x80000000, "Reset PC");
DEFINE_uint64(resetpcfw, 0xC0040000, "Reset firmware PC");
DEFINE_bool(nostop_standalone,false, "Do not stop if standalone whisper fails");
DEFINE_string(whisper_instr_lines, "", "Write instr cache line addresses used in test to a file");
DEFINE_string(whisper_data_lines, "", "Write data cache line addresses used in test to a file");
DEFINE_bool(whisper_csv_log, false, "Make whisper use a csv trace.");
DEFINE_int32(whisper_tlb_size, -1, "Specify whisper tlb size");
DEFINE_string(isa, "", "Override isa spec");
DEFINE_bool(whisper_log, true, "Enable whisper logging to iss_cosim.log and iss_cmd.log");
DEFINE_bool(whisper_cosim_log, false, "Enable whisper logging to iss_cosim.log");
DEFINE_bool(whisper_cmd_log, false, "Enable whisper logging to iss_cmd.log");
DEFINE_bool(whisper_stdin_null, false, "Redirect whisper stdin to null");
DEFINE_uint32(num_dm_randpc,    0, "Number of Random PCs to DM");
DEFINE_uint32(num_dm_randload,  0, "Number of Random loads to DM");
DEFINE_uint32(num_dm_randstore, 0, "Number of Random stores to DM");
DEFINE_bool(randpc_phy, false, "Random PCs selected are phys address ");
DEFINE_bool(randldst_phy, false, "Random Ld/St addresses selected are phys address");
DEFINE_uint64(dm_rand_addr, 0x9080500, "(Trickbox) Random address for DM: PC/Load/Store");;
DEFINE_bool(whisper_stdout_null, false, "Redirect whisoer stdout to null");
DEFINE_string(whisper_json_path, "", "Path to whisper json config");
DEFINE_uint64(nmi_vec, 0, "NMI handler PC");
DEFINE_uint64(nme_vec, 0, "NMI exception handler PC");
DEFINE_bool(ppo, true, "Enable ppo checks");
DEFINE_bool(traceptw, true, "Enable page table walk tracing");
DEFINE_bool(whisper_auto_increment_timer, false, "Enable whisper auto_increment_timer");
DEFINE_bool(whisper_aclint_deliver_interrupts, true, "Enable whisper aclint deliver_interrupts");
DEFINE_uint64(whisper_aclint_time_adjust, 0, "Set aclint adjust time compare offset");

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
    }
    catch (...) {
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
    }
    catch (...) {
      std::cout << "Warn: No nmevec symbol in elf\n";
    }
    return nmevec_addr_;
  }
}

template <typename URV>
whisperClient<URV>::whisperClient(cvm::topology::loc_t loc, unsigned) : loc_(loc) {
  cvm::log(cvm::MEDIUM, "[whisperClient] initializing whisperClient\n");

  ncores_ = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  std::string traceFile  = (FLAGS_whisper_log || FLAGS_whisper_cosim_log) ? "iss_cosim.log" : "";
  std::string commandLog = (FLAGS_whisper_log || FLAGS_whisper_cmd_log  ) ? "iss_cmd.log" : "";

  traceFile_ = traceFile.empty() ? nullptr : fopen(traceFile.c_str(), "w");
  commandLog_ = commandLog.empty() ? nullptr : fopen(commandLog.c_str(), "w");
  cvm::registry::messenger.procedure<get_dm_rand_addr_RPC>(loc, [this] () { return this->get_dm_rand_addr();});
  cvm::registry::messenger.procedure<get_dm_rand_val_RPC>(loc, [this] ()  { return this->get_dm_rand_val();});

  cvm::registry::messenger.procedure<whisperConnectRPC>(loc, [this] () {return this->whisperConnect();});
  cvm::registry::messenger.procedure<whisperConnectedRPC>(loc, [this] () {return this->whisperConnected();});
  cvm::registry::messenger.procedure<whisperStepRPC>(loc, [this] (int hart, uint64_t time, uint64_t instrTag, uint64_t& pc, uint32_t& instruction, unsigned& changeCount, std::string& disasm, uint32_t& privMode, uint32_t& fpFlags, bool& hasTrap, bool& hasStop, bool& isLoad, bool& valid) {return this->whisperStep(hart, time, instrTag, pc, instruction, changeCount, disasm, privMode, fpFlags, hasTrap, hasStop, isLoad, valid);});
  cvm::registry::messenger.procedure<whisperSimpleStepRPC>(loc, [this] (int hart, uint64_t& pc, uint32_t& instruction, unsigned& changeCount) {return this->whisperSimpleStep(hart, pc, instruction, changeCount);});
  cvm::registry::messenger.procedure<whisperChangeRPC>(loc, [this] (int hart, uint32_t& resource, uint64_t& addr, uint64_t& value, bool& valid) {return this->whisperChange(hart, resource, addr, value, valid);});
  cvm::registry::messenger.procedure<whisperMcmReadRPC>(loc, [this] (int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, unsigned elemIx, unsigned field, bool& valid) {return this->whisperMcmRead(hart, time, instrTag, addr, size, value, elemIx, field, valid);});
  cvm::registry::messenger.procedure<whisperMcmVecReadRPC>(loc, [this] (int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, std::vector<uint64_t> value, unsigned elemIx, unsigned field, bool& valid) {return this->whisperMcmVecRead(hart, time, instrTag, addr, size, value, elemIx, field, valid);});  
  cvm::registry::messenger.procedure<whisperMcmVecInsertRPC>(loc, [this] (int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, std::vector<uint64_t> value, bool& valid) {return this->whisperMcmVecInsert(hart, time, instrTag, addr, size, value, valid);});
  cvm::registry::messenger.procedure<whisperMcmInsertRPC>(loc, [this] (int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool& valid) {return this->whisperMcmInsert(hart, time, instrTag, addr, size, value, valid);});
  cvm::registry::messenger.procedure<whisperMcmVecBypassRPC>(loc, [this] (int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, std::vector<uint64_t> value, bool& valid) {return this->whisperMcmVecBypass(hart, time, instrTag, addr, size, value, valid);});
  cvm::registry::messenger.procedure<whisperMcmBypassRPC>(loc, [this] (int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool& valid) {return this->whisperMcmBypass(hart, time, instrTag, addr, size, value, valid);});
  cvm::registry::messenger.procedure<whisperMcmWriteRPC>(loc, [this] (int hart, uint64_t time, uint64_t addr, unsigned size, svOpenArrayHandle handle, uint64_t mask, bool& valid) {return this->whisperMcmWrite(hart, time, addr, size, handle, mask, valid);});
  cvm::registry::messenger.procedure<whisperMcmIFetchRPC>(loc, [this] (int hart, uint64_t time, uint64_t addr, bool& valid) {return this->whisperMcmIFetch(hart, time, addr, valid);});
  cvm::registry::messenger.procedure<whisperMcmIEvictRPC>(loc, [this] (int hart, uint64_t time, uint64_t addr, bool& valid) {return this->whisperMcmIEvict(hart, time, addr, valid);});
  cvm::registry::messenger.procedure<whisperMcmEndRPC>(loc, [this] (int hart, uint64_t time, bool& valid) {return this->whisperMcmEnd(hart, time, valid);});
  cvm::registry::messenger.procedure<whisperInjectExceptionRPC>(loc, [this] (int hart, bool isLoad, uint64_t code, unsigned elemIx, bool& valid) {return this->whisperInjectException(hart, isLoad, code, elemIx, valid);});
  cvm::registry::messenger.procedure<whisperPokeRPC>(loc, [this] (int hart, uint64_t time, char resource, uint64_t addr, uint64_t value, bool& valid) {return this->whisperPoke(hart, time, resource, addr, value, valid);});
  cvm::registry::messenger.procedure<whisperPokeMemRPC>(loc, [this] (int hart, uint64_t time, char resource, uint64_t addr, unsigned size, uint64_t value, bool& valid) {return this->whisperPokeMem(hart, time, resource, addr, size, value, valid);});
  cvm::registry::messenger.procedure<whisperPeekRPC>(loc, [this] (int hart, char resource, uint64_t addr, uint64_t& value, bool& valid) {return this->whisperPeek(hart, resource, addr, value, valid);});
  cvm::registry::messenger.procedure<whisperPeekPcRPC>(loc, [this] (int hart, uint64_t& value) {return this->whisperPeekPc(hart, value);});
  cvm::registry::messenger.procedure<whisperPeekCsrRPC>(loc, [this] (int hart, uint64_t addr, uint64_t& value, uint64_t& mask, uint64_t& reset_value, uint64_t& read_mask, bool& valid) {return this->whisperPeekCsr(hart, addr, value, mask, reset_value, read_mask, valid);});
  cvm::registry::messenger.procedure<whisperResetRPC>(loc, [this] (int hart, uint64_t addr, bool& valid) {return this->whisperReset(hart, addr, valid);});
  cvm::registry::messenger.procedure<whisperQuitRPC>(loc, [this] () {return this->whisperQuit();});
  cvm::registry::messenger.procedure<whisperPageTableWalkRPC>(loc, [this] (int hart, bool isInstr, bool isAddr, svOpenArrayHandle items, unsigned& itemCount, bool& valid) {return this->whisperPageTableWalk(hart, isInstr, isAddr, items, itemCount, valid);});
  cvm::registry::messenger.procedure<whisperTranslateRPC>(loc, [this] (int hart, uint64_t vaddr, bool r, bool w, bool x, bool twoStage, bool supervisor, uint64_t& paddr, bool& valid) {return this->whisperTranslate(hart, vaddr, r, w, x, twoStage, supervisor, paddr, valid);});
  cvm::registry::messenger.procedure<whisperEnterDebugRPC>(loc, [this] (int hart) {return this->whisperEnterDebug(hart);});
  cvm::registry::messenger.procedure<whisperExitDebugRPC>(loc, [this] (int hart) {return this->whisperExitDebug(hart);});
  cvm::registry::messenger.procedure<whisperCheckInterruptRPC>(loc, [this] (int hart, bool& interrupt, uint64_t& cause) {return this->whisperCheckInterrupt(hart, interrupt, cause);});
  cvm::registry::messenger.procedure<whisperGetSeiPinRPC>(loc, [this] (int hart, uint64_t& value) {return this->whisperGetSeiPin(hart, value);});
  cvm::registry::messenger.procedure<whisperCancelLrRPC>(loc, [this] (int hart, bool& valid) {return this->whisperCancelLr(hart, valid);});
  cvm::registry::messenger.procedure<whisperPeekGprRPC>(loc, [this] (int hart, uint64_t addr, uint64_t& value) {return this->whisperPeekGpr(hart, addr, value);});
  cvm::registry::messenger.procedure<whisperPeekFprRPC>(loc, [this] (int hart, uint64_t addr, uint64_t& value) {return this->whisperPeekFpr(hart, addr, value);});
  cvm::registry::messenger.procedure<whisperPeekVprRPC>(loc, [this] (int hart, uint64_t addr, std::array<std::uint8_t, 32>&  value) {return this->whisperPeekVpr(hart, addr, value);});
  cvm::registry::messenger.procedure<whisperGetLastLdStAddressRPC>(loc, [this] (int hart, uint64_t& pa) {return this->whisperGetLastLdStAddress(hart, pa);});
  cvm::registry::messenger.procedure<whisperNmiRPC>(loc, [this] (int hart, uint64_t time, uint64_t cause) {return this->whisperNmi(hart, time, cause);});
  cvm::registry::messenger.procedure<whisperClearNmiRPC>(loc, [this] (int hart, uint64_t time) {return this->whisperClearNmi(hart, time);});
  cvm::registry::messenger.procedure<whisperMcmSkipReadDataCheckRPC>(loc, [this] (uint64_t addr, unsigned size, bool enable) {return this->whisperMcmSkipReadDataCheck(addr,size,enable);});
  cvm::registry::messenger.procedure<secureRegionRPC> (loc, [this] (uint64_t start, uint64_t end) { this->secure_region_start_=start; this->secure_region_end_=end; });

}

template <typename URV>
static std::shared_ptr<WdRiscv::System<URV>>
constructSystem(uint16_t ncores, bool standalone, uint64_t secure_region_start=0, uint64_t secure_region_end=0) {

  WdRiscv::HartConfig config;
  if (not config.loadConfigFile(FLAGS_whisper_json_path.c_str()))
    return nullptr;

  unsigned hartsPerCore = 1;
  unsigned coreCount    = ncores;
  unsigned hartIdOffset = hartsPerCore;
  size_t pageSize       = 4*1024;
  size_t memorySize     = size_t(1) << 31;

  std::string isa;
  config.getPageSize(pageSize);
  config.getMemorySize(memorySize);
  config.getIsa(isa);

  std::shared_ptr<WdRiscv::System<URV>> system = std::make_shared<WdRiscv::System<URV>>(coreCount, hartsPerCore, hartIdOffset, memorySize, pageSize);

  auto parse = [&system](const std::string& flag, bool lz4_compressed) {
    std::stringstream ss;
    std::vector<std::string> targets;

    ss << flag;
    while (ss.good()) {
      std::string substr;

      getline(ss, substr, ',');
      targets.push_back(substr);
    }

    return lz4_compressed ? system->loadLz4Files(targets, 0, false) : system->loadBinaryFiles(targets, 0, false);

  };

  if (FLAGS_load_lz4 != "") {
    if (not parse(FLAGS_load_lz4, true)) {
      return nullptr;
    }
  }

  if (FLAGS_load_bin != "") {
    if (not parse(FLAGS_load_bin, false)) {
      return nullptr;
    }
  }

  if (FLAGS_hex != "") {
    std::vector<std::string> targets = {FLAGS_hex};
    if (not system->loadHexFiles(targets, false))
      return nullptr;
  }

  if (FLAGS_bootrom_path != "" || FLAGS_load != "" || FLAGS_cplfw_path != "") {
    std::vector<std::string> targets {};
    if (FLAGS_load != "")
      targets.push_back(FLAGS_load);
    if (FLAGS_bootrom && FLAGS_bootrom_path != "")
      targets.push_back(FLAGS_bootrom_path);
    if (FLAGS_cplfw && FLAGS_cplfw_path != "")
      targets.push_back(FLAGS_cplfw_path);
    if (not system->loadElfFiles(targets, false, false))
      return nullptr;
  }

  if (not config.configHarts(*system, false, false))
    return nullptr;

  if (not config.configMemory(*system, false))
    return nullptr;

  if (FLAGS_mcm && !standalone) {
    bool checkAll = false;
    config.getMcmCheckAll(checkAll);
    system->enableMcm(64, checkAll, FLAGS_ppo);
  }

  for (unsigned i = 0; i < system->hartCount(); ++i) {
    auto& hart = *(system->ithHart(i));
    // raw mode
    hart.enableNewlib(false);
    hart.enableLinux(false);
    hart.tracePtw(FLAGS_traceptw);
    if (FLAGS_cplfw)
      hart.defineResetPc(FLAGS_resetpcfw);
    else
      hart.defineResetPc(FLAGS_resetpc);
    hart.defineNmiPc(getNmiPc());
    hart.defineNmiExceptionPc(getNmiExceptionPc());
    hart.enableCsvLog(FLAGS_whisper_csv_log);
    if (FLAGS_whisper_tlb_size >= 0)
      hart.setTlbSize(FLAGS_whisper_tlb_size);
    if (FLAGS_whisper_stdout_null) hart.redirectOutputDescriptor(STDOUT_FILENO, "/dev/null");
    if (FLAGS_whisper_stdin_null)  hart.redirectOutputDescriptor(STDIN_FILENO,  "/dev/null");
    if (!standalone) {
      hart.setAclintDeliverInterrupts(FLAGS_whisper_aclint_deliver_interrupts);
      hart.autoIncrementTimer(FLAGS_whisper_auto_increment_timer);
      hart.setAclintAdjustTimeCompare(FLAGS_whisper_aclint_time_adjust);
    }
    if (! isa.empty()) {
      if (FLAGS_isa != "") {
        if (not hart.configIsa(FLAGS_isa, false))
          return nullptr;
      } else if (not hart.configIsa(isa, false)) {
        return nullptr;
      }
    }
    if (secure_region_start || secure_region_end) {
      hart.configSteeSecureRegion(secure_region_start, secure_region_end);
      hart.enableStee(true);
    }
    hart.reset();
  }
  if (not config.applyImsicConfig(*system))
    return nullptr;
  if ((standalone || FLAGS_aplic_is_memory) && (not config.applyAplicConfig(*system)))
    // We don't configure the APLIC in cosim because Whipser will take the
    // interrupt immediately when triggered and it will not be deferred because
    // the bridge considers it a Zicsr write interrupt. When an IMSIC interrupt
    // is triggered by the APLIC the bridge will poke it into whisper. We also
    // don't configure the APLIC when aplic_is_memory is true so that the
    // standalone run doesn't use the APLIC.
    return nullptr;

  if (FLAGS_whisper_data_lines != "")
    system->enableDataLineTrace(FLAGS_whisper_data_lines);
  if (FLAGS_whisper_instr_lines != "")
    system->enableInstructionLineTrace(FLAGS_whisper_instr_lines);
  return system;
}

template <typename URV>
void
constructHart (WdRiscv::Hart<URV>* hart, bool preload = false, FILE* preload_log = nullptr) {
  hart->setInstructionCountLimit(FLAGS_max_instr);
  hart->setWfiTimeout(0);

  if (FLAGS_tohost)
    hart->setToHostAddress(FLAGS_tohost);

  if (preload)
    hart->setInitialStateFile(preload_log);
};


template <typename URV>
int
whisperClient<URV>::whisperStandalone()
{
  std::atomic<bool> result       = true;
  std::atomic<bool> max_instr    = false;
  std::atomic<unsigned> finished = 0;
  FILE* whisper_log;
  auto threadFunc = [&result, &finished, &max_instr, &whisper_log] (WdRiscv::Hart<URV>* hart) {
                      bool r = hart->run(whisper_log);
                      result = result and r;
                      max_instr = max_instr or (hart->getInstructionCount() >= FLAGS_max_instr);
                      finished++;
                    };

  std::vector<std::thread> threadVec;
  FILE* preload_log[system_->hartCount()];
  whisper_log = fopen("iss_standalone.log", "w");
  for (unsigned i=0; i<system_->hartCount(); ++i) {
    if (FLAGS_preload)
      preload_log[i] = fopen(("preload_" + std::to_string(i) + ".csv").c_str(), "w");
    WdRiscv::Hart<URV>* hart = system_->ithHart(i).get();
    constructHart(hart, FLAGS_preload, preload_log[i]);
    threadVec.emplace_back(std::thread(threadFunc, hart));
  }

  for (auto& t : threadVec)
    t.join();

  fclose(whisper_log);
  for (unsigned i = 0; i < system_->hartCount(); ++i)
    if (FLAGS_preload)
      fclose(preload_log[i]);

  if (!FLAGS_nostop_standalone) {
    if (!result)
      cvm::log(cvm::ERROR, "Error: Test failed on Standalone Whisper, stopping simulation\n");
    else if (max_instr && (FLAGS_max_instr != 0))
      cvm::log(cvm::ERROR, "Error: Test reached max instr on standalone Whisper, stopping simulation\n");
  }

  if (result && (FLAGS_num_dm_randpc || FLAGS_num_dm_randload || FLAGS_num_dm_randstore)){
    WdRiscv::Hart<URV>* hart = system_->ithHart(0).get();

    std::shared_ptr<WdRiscv::System<URV>> system_new = constructSystem<URV>(1, true, secure_region_start_, secure_region_end_);
    WdRiscv::Hart<URV>* hart_new = system_new->ithHart(0).get();
    constructHart(hart_new, 0, nullptr);
    cvm::rand::uniform_dist<int> rng1;
    int percent = (rng1() % 20) + 60; // random pc betwen 60-80% of code
    uint64_t total_instr = hart->getInstructionCount();
    uint64_t num_instr = uint64_t((total_instr * percent) / 100);
    bool stop;
    hart_new->runSteps(num_instr, stop);
    dm_rand_addr_ = FLAGS_dm_rand_addr;
    int instructions = 0;
    std::vector<uint64_t> pcs, loads, stores;
    while ((num_instr <= total_instr) && (instructions<200)) {
      hart_new->singleStep();
      num_instr++; instructions++;
      uint64_t virt_addr, phys_addr, value, phys_pc;
      uint64_t pc = hart_new->lastPc();
      uint32_t inst;
      hart_new->readInst(pc, phys_pc, inst);
      if (FLAGS_randpc_phy) pc = phys_pc;
      if (   (inst & 0x10500073) // WFI
          || (inst & 0x30200073) // MRET
          || (((inst & 0x7fff) == 0x200f) && (inst>>20 <= 4))) // CBOs
      {
        pcs.push_back(pc); // giving more weightage
        pcs.push_back(pc);
        pcs.push_back(pc);

      } else if (hart_new->lastInstructionTrapped()) {
        pcs.push_back(pc);                 // Handler PC
        pcs.push_back(pc);
        uint64_t curr_pc = hart_new->pc(); // Exception PC
        pcs.push_back(curr_pc);
        pcs.push_back(curr_pc);

      } else if (hart_new->lastLdStAddress(virt_addr, phys_addr)) {
        if (FLAGS_randldst_phy) virt_addr = phys_addr;
        if (hart_new->lastStore(phys_addr, value)) {
          stores.push_back(virt_addr);
        } else {
          loads.push_back(virt_addr);
        }
      } else {
        pcs.push_back(pc);
      }
    }
    uint32_t num_dm = FLAGS_num_dm_randpc;
    while (num_dm && pcs.size()) {
      int rand_idx = rng1() % pcs.size();
      dm_rand_val_.push_back(pcs[rand_idx]);
      pcs.erase(std::remove(pcs.begin(), pcs.end(), pcs[rand_idx]), pcs.end()); // prevent duplicates
      num_dm--;
    }
    num_dm = FLAGS_num_dm_randload;
    while (num_dm && loads.size()) {
      int rand_idx = rng1() % loads.size();
      dm_rand_val_.push_back(loads[rand_idx]);
      loads.erase(std::remove(loads.begin(), loads.end(), loads[rand_idx]), loads.end());
      num_dm--;
    }
    num_dm = FLAGS_num_dm_randstore;
    while (num_dm && stores.size()) {
      int rand_idx = rng1() % stores.size();
      dm_rand_val_.push_back(stores[rand_idx]);
      stores.erase(std::remove(stores.begin(), stores.end(), stores[rand_idx]), stores.end());
      num_dm--;
    }
  }
  return 0;
}

template <typename URV>
int
whisperClient<URV>::whisperConnect()
{
  // Construct and run whisper standalone
  // This can be useful to compare with the cosim run
  if (FLAGS_standalone && (ncores_ == 1)) {
    system_ = constructSystem<URV>(ncores_, true, secure_region_start_, secure_region_end_);
    if (system_ == nullptr)
      cvm::log(cvm::ERROR, "Error: could not construct system\n");
    whisperStandalone();
  } else if (FLAGS_preload) {
    cvm::log(cvm::ERROR, "Error: Preloading works only on single core runs and +standalone plusarg enabled\n");
  }

  // Construct whisper for cosim
   system_ = constructSystem<URV>(ncores_, false, secure_region_start_, secure_region_end_);
  if (system_ == nullptr) {
    cvm::log(cvm::ERROR, "Error: could not construct system\n");
  }
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
bool
whisperClient<URV>::whisperConnected()
{
  return server_ != nullptr;
}

template <typename URV>
bool
whisperClient<URV>::whisperCommand(const WhisperMessage& req, WhisperMessage& reply)
{
  if(FLAGS_cosim && (server_ != nullptr)) {
    server_->interact(req, reply, traceFile_, commandLog_);
  }
  return true;
}


template <typename URV>
bool
whisperClient<URV>::whisperPeek(int hart, char resource, uint64_t addr, uint64_t& value,
	    bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::Peek;
  req.resource = resource;
  req.address = addr;
  req.tag[0] = 0;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  value = reply.value;
  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperPeekCsr(int hart, uint64_t addr, uint64_t& value, uint64_t& mask,
         uint64_t& poke_mask, uint64_t& read_mask, bool& valid)
{
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
bool
whisperClient<URV>::whisperPeekPc(int hart, uint64_t& value)
{
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
bool
whisperClient<URV>::whisperPeekGpr(int hart, uint64_t addr, uint64_t& value)
{
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
bool
whisperClient<URV>::whisperPeekFpr(int hart, uint64_t addr, uint64_t& value)
{
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
bool
whisperClient<URV>::whisperPeekVpr(int hart, uint64_t addr, std::array<std::uint8_t, 32>& value)
{
  req.hart = hart;
  req.type = WhisperMessageType::Peek;
  req.resource = 'v';
  req.address = addr;
  req.tag[0] = 0;

  if (not whisperCommand(req, reply))
    return false;

  for(int i=0; i<32; i++) {
     value[i] = reply.buffer[i];
  }
  return true;
}

// Send a whisper InjectException command. Return true on successful comunication
// and false on failure. Set valid to false if hart/resource/addr
// are invalid.
template <typename URV>
bool
whisperClient<URV>::whisperInjectException(int hart, bool isLoad, uint64_t code, unsigned elemIx,
	    bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::InjectException;
  WhisperFlags wflags;
  wflags.bits.load = isLoad;
  req.flags = wflags.value;
  req.address = code;
  req.resource = elemIx;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

// Send a whisper poke command. Retrun true on successful comunication
// and false on failure. Set valid to false if hart/resource/addr
// are invalid.
template <typename URV>
bool
whisperClient<URV>::whisperPoke(int hart, uint64_t time, char resource, uint64_t addr, uint64_t value,
	    bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::Poke;
  req.resource = resource;
  req.address = addr;
  req.value = value;
  req.time = time;
  req.tag[0] = 0;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

// Specialized poke for memory that accepts size argument.
template <typename URV>
bool
whisperClient<URV>::whisperPokeMem(int hart, uint64_t time, char resource, uint64_t addr, unsigned size,
    uint64_t value, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::Poke;
  req.resource = resource;
  req.address = addr;
  req.value = value;
  req.time = time;
  req.size = size;
  req.tag[0] = 0;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperStep(int hart, uint64_t time, uint64_t instrTag, uint64_t& pc,
	    uint32_t& instruction, unsigned& changeCount,
	    std::string& disasm, uint32_t& privMode,
	    uint32_t& fpFlags, bool& hasTrap, bool& hasStop, bool& isLoad, bool& valid)
{
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
  unsigned mode  = wflags.bits.privMode;
  unsigned flags = wflags.bits.fpFlags;
  unsigned trap  = wflags.bits.trap;
  unsigned stop  = wflags.bits.stop;
  unsigned virt  = wflags.bits.virt;
  unsigned debug = wflags.bits.debug;
  unsigned load  = wflags.bits.load;


  privMode = debug? 6 : mode | (virt << 3);
  fpFlags  = flags;
  hasTrap  = trap;
  hasStop  = stop;
  isLoad   = load;
  reply.buffer[reply.buffer.size() - 1] = '\0';
  disasm = reply.buffer.data();

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

// Copied from chuang's LSTB Whisper Client
template <typename URV>
bool
whisperClient<URV>::whisperSimpleStep(int hart, uint64_t& pc, uint32_t& instruction, unsigned& changeCount)
{
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
bool
whisperClient<URV>::whisperChange(int hart, uint32_t& resource, uint64_t& addr, uint64_t& value,
	      bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::Change;

  if (not whisperCommand(req, reply))
    return false;

  resource = reply.resource;
  addr     = reply.address;
  value    = reply.value;
  valid    = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperGetLastLdStAddress(int hart, uint64_t& value)
{
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
bool
whisperClient<URV>::whisperMcmRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
				   unsigned size, uint64_t value, unsigned elemIx,
				   unsigned field, bool& valid)
{
  req.type = WhisperMessageType::McmRead;
  req.hart = hart;
  req.time = time;
  req.value= value;
  req.size = size;
  req.address  = addr;
  req.instrTag = instrTag;
  req.resource = (elemIx << 16) | (field & 0xffff);  // Pack elemIx and field into resource.

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperMcmVecRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
				      unsigned size, std::vector<uint64_t> value,
				      unsigned elemIx, unsigned field, bool& valid)
{
  std::vector<uint8_t> byte_value = convert_to_byte_array(value);

  if (size <= 8){
    uint64_t u64 = 0;
    for (unsigned i = 0; i < size; ++i){
      uint8_t byte = byte_value[i];
      u64 = (u64 << 8) | byte;
    }
    return whisperMcmRead(hart, time, instrTag, addr, size, value[0], elemIx, field, valid);
  }

  req.hart = hart;
  req.type = WhisperMessageType::McmRead;
  req.time = time;
  req.instrTag = instrTag;
  req.address = addr;
  req.size = size; // Total size in bytes
  req.resource = (elemIx << 16) | (field & 0xffff);  // Pack elemIx and field into resource.

  for (unsigned i = 0; i < size; ++i){
    req.buffer.at(i) = byte_value[i];
  }

  WhisperMessage reply;
  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperMcmVecInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
		    unsigned size, std::vector<uint64_t> value, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::McmInsert;
  req.time = time;
  req.instrTag = instrTag;
  req.address = addr;
  req.size = size;   // Total size in bytes

  std::vector<uint8_t> byte_value = convert_to_byte_array(value);

  if (size <= 8)
  {
    uint64_t u64 = 0;
    for (unsigned i = 0; i < size; ++i)
    {
      uint8_t byte = byte_value[i];
      u64 = (u64 << 8) | byte;
    }
    return whisperMcmInsert(hart, time, instrTag, addr, size, value[0], valid);
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
bool
whisperClient<URV>::whisperMcmInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
		 unsigned size, uint64_t value, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::McmInsert;
  req.time = time;
  req.instrTag = instrTag;
  req.address  = addr;
  req.value    = value;
  req.size     = size;

  if (size > 8)
    {
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
bool
whisperClient<URV>::whisperMcmVecBypass(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
		    unsigned size, std::vector<uint64_t> value, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::McmBypass;
  req.time = time;
  req.instrTag = instrTag;
  req.address = addr;
  req.size = size;   // Total size in bytes

  std::vector<uint8_t> byte_value = convert_to_byte_array(value);

  if (size <= 8)
  {
    uint64_t u64 = 0;
    for (unsigned i = 0; i < size; ++i)
    {
      uint8_t byte = byte_value[i];
      u64 = (u64 << 8) | byte;
    }
    return whisperMcmBypass(hart, time, instrTag, addr, size, value[0], valid);
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
bool
whisperClient<URV>::whisperMcmBypass(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
		 unsigned size, uint64_t value, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::McmBypass;
  req.time = time;
  req.instrTag = instrTag;
  req.address = addr;
  req.value = value;
  req.size = size;

  if (size > 8)
    {
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
bool
whisperClient<URV>::whisperMcmWrite(int hart, uint64_t time, uint64_t addr,
		unsigned size, svOpenArrayHandle handle, uint64_t mask, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::McmWrite;
  req.time = time;
  req.address = addr;
  req.size = size;
  req.flags = 1;
  for (uint8_t i = 0; i < req.size/8; ++i)
    req.tag[i] = (uint8_t)((mask >> (i*8)) & 0xff);

  if (req.size > req.buffer.size()) {
    std::cerr << "whisperMcmWrite: write size too large: " << req.size << '\n'; // #FIXME: if it doesn't error, should it be cvm::medium?
    valid = false;
    return true;
  }

  uint8_t* data = reinterpret_cast<uint8_t*> (handle);
  for (unsigned i=0; i<size; ++i)
    req.buffer[i] = data[i];

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperMcmIFetch(int hart, uint64_t time, uint64_t addr, bool& valid)
{
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
bool
whisperClient<URV>::whisperMcmSkipReadDataCheck(uint64_t addr, unsigned size, bool enable)
{
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
bool
whisperClient<URV>::whisperMcmIEvict(int hart, uint64_t time, uint64_t addr, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::McmIEvict;
  req.time = time;
  req.address = addr;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperMcmEnd(int hart, uint64_t time, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::McmEnd;
  req.time = time;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperTranslate(int hart, uint64_t vaddr, bool r, bool w, bool x, bool twoStage,
         bool supervisor, uint64_t& paddr, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::Translate;
  req.address = vaddr;
  req.flags = 0;

  if      (r) req.flags |= 1;
  else if (w) req.flags |= 2;
  else if (x) req.flags |= 4;

  if (supervisor) req.flags |= 8;
  if (twoStage)   req.flags |= 16;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  if (valid)
    paddr = reply.address;
  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperCancelLr(int hart, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::CancelLr;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperReset(int hart, uint64_t addr, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::Reset;
  req.address = addr;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperQuit()
{
  WhisperMessage req(0 /*hart*/, WhisperMessageType::Quit);  // Any hart will do
  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperPageTableWalk(int, bool, bool,
		     svOpenArrayHandle, unsigned&, bool&)
{
  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperEnterDebug(int hart)
{
  std::cout<<"Whisper client Enter Debug\n";
  req.hart = hart;
  req.type = WhisperMessageType::EnterDebug;

  if (not whisperCommand(req, reply))
    return false;

  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperExitDebug(int hart)
{
  std::cout<<"Whisper client Exit Debug\n";
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
bool
whisperClient<URV>::whisperCheckInterrupt(int hart, bool& interrupt, uint64_t& cause)
{
  req.hart = hart;
  req.type = WhisperMessageType::CheckInterrupt;

  WhisperMessage reply;

  if (not whisperCommand(req, reply))
    return false;

  interrupt = reply.flags;
  cause = reply.value;

  return true;
}


// Set the supervisor mode external interrupt pin to the given
// value (0 or 1). Whisper will consider either the SEIP bit in MIP or the
// the external pin value set by this method for taking a supervisor
// external interrupt (assuming that interrupt is enabled).
template <typename URV>
bool
whisperClient<URV>::whisperGetSeiPin(int hart, uint64_t& value)
{
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
bool
whisperClient<URV>::whisperNmi(int hart, uint64_t time, uint64_t cause)
{
  req.hart = hart;
  req.type = WhisperMessageType::Nmi;
  req.time = time;
  req.value = cause;

  if (not whisperCommand(req, reply))
    return false;

  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperClearNmi(int hart, uint64_t time)
{
  req.hart = hart;
  req.type = WhisperMessageType::ClearNmi;
  req.time = time;

  if (not whisperCommand(req, reply))
    return false;

  return true;
}

template class whisperClient<uint32_t>;
template class whisperClient<uint64_t>;
