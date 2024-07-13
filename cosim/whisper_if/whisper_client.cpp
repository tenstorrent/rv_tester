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

#include "whisper_client.h"
#include "HartConfig.hpp"
#include "Hart.hpp"
#include "cosim/dut_if/rvfi/rvfi_plusargs.h"
#include "sysmod/sysmod_plusargs.h"
#include "cosim/bridge/bridge_plusargs.h"
#include "cosim/utils/eot/eot_plusargs.h"
#include "rv_tester_plusargs.h"


DEFINE_bool(nostop_standalone,false, "Do not stop if standalone whisper fails");

DEFINE_string(whisper_instr_lines, "", "Write instr cache line addresses used in test to a file");
DEFINE_string(whisper_data_lines, "", "Write data cache line addresses used in test to a file");
DEFINE_bool(whisper_csv_log, false, "Make whisper use a csv trace.");
DEFINE_uint32(whisper_tlb_size, 0, "Specify whisper tlb size");
DEFINE_uint64(resetpc, 0x80000000, "Reset PC");
DEFINE_uint64(resetpcfw, 0xC0040000, "Reset firmware PC");
DEFINE_string(isa, "", "Override isa spec");

extern void (*__tracerExtension)(void*);

template <typename URV>
static std::shared_ptr<WdRiscv::System<URV>>
constructSystem(uint16_t ncores, bool standalone, bool firmware) {

  WdRiscv::HartConfig config;
  if (not config.loadConfigFile(FLAGS_whisper_json_path.c_str()))
    return nullptr;

  unsigned hartsPerCore = 1;
  unsigned coreCount = ncores;
  unsigned hartIdOffset = hartsPerCore;
  size_t pageSize = 4*1024;
  size_t memorySize = size_t(1) << 31;
  std::string isa;
  config.getPageSize(pageSize);
  config.getMemorySize(memorySize);
  config.getIsa(isa);

  std::shared_ptr<WdRiscv::System<URV>> system = std::make_shared<WdRiscv::System<URV>>(coreCount, hartsPerCore, hartIdOffset, memorySize, pageSize);

  if (FLAGS_mcm && !standalone) {
    bool checkAll = false;
    config.getMcmCheckAll(checkAll);
    system->enableMcm(64, checkAll);
  }


  if (FLAGS_load_lz4 != "") {
    std::vector<std::string> targets = {FLAGS_load_lz4};
    if (not system->loadLz4Files(targets, 0, false))
      return nullptr;
  }

  if (FLAGS_hex != "") {
    std::vector<std::string> targets = {FLAGS_hex};
    if (not system->loadHexFiles(targets, false))
      return nullptr;
  }

  if (FLAGS_bootrom_path != "" || FLAGS_load != "" || FLAGS_cplfw_path != "") {
    std::vector<std::string> targets {};
    if(!firmware){
      if (FLAGS_load != "")
        targets.push_back(FLAGS_load);
      if (FLAGS_bootrom && FLAGS_bootrom_path != "")
        targets.push_back(FLAGS_bootrom_path);
    } else {
      if (FLAGS_cplfw_path != "")
        targets.push_back(FLAGS_cplfw_path);
    }
    if (not system->loadElfFiles(targets, false, false))
      return nullptr;
  }

  if (not config.configHarts(*system, false, false))
    return nullptr;

  if (not config.configMemory(*system, false))
    return nullptr;

  for (unsigned i = 0; i < system->hartCount(); ++i) {
    auto& hart = *(system->ithHart(i));
    // raw mode
    hart.enableNewlib(false);
    hart.enableLinux(false);
    hart.tracePtw(true);
    if (firmware) hart.defineResetPc(FLAGS_resetpcfw);
    else hart.defineResetPc(FLAGS_resetpc);
    hart.enableCsvLog(FLAGS_whisper_csv_log);
    hart.setTlbSize(FLAGS_whisper_tlb_size);
    if (FLAGS_whisper_stdout_null) hart.redirectOutputDescriptor(STDOUT_FILENO, "/dev/null");
    if (FLAGS_whisper_stdin_null) hart.redirectOutputDescriptor(STDIN_FILENO, "/dev/null");
    if (not isa.empty()){
      if (FLAGS_isa != ""){
        if (not hart.configIsa(FLAGS_isa, false))
          return nullptr;
      }
      else if (not hart.configIsa(isa, false)){
        return nullptr;
      }
    }
    hart.reset();
  }

  if (not config.applyImsicConfig(*system))
    return nullptr;

  return system;
}

template <typename URV>
int
whisperClient<URV>::whisperConnect(uint16_t ncores)
{
  if(FLAGS_preload) {
    system_ = constructSystem<URV>(ncores, false, true);
    if (system_ == nullptr) {
      std::cerr << "Error: could not construct system\n";
      return -1;
    }

    std::vector<std::thread> threadVec;

    std::atomic<bool> result = true;
    std::atomic<bool> max_instr = false;
    std::atomic<unsigned> finished = 0;  // Count of finished threads.

    FILE* whisper_log = fopen("iss_firmware.log", "w");
    FILE* preload_log[system_->hartCount()];

    auto threadFunc = [&result, &finished, &max_instr, whisper_log] (WdRiscv::Hart<URV>* hart) {
                        bool r = hart->run(whisper_log);
                        result = result and r;
                        max_instr = max_instr or (hart->getInstructionCount() >= FLAGS_max_instr);
                        finished++;
                      };

    for (unsigned i = 0; i < system_->hartCount(); ++i) {
      WdRiscv::Hart<URV>* hart = system_->ithHart(i).get();
      if (FLAGS_preload) {
        preload_log[i] = fopen(("firmware_preload_" + std::to_string(i) + ".csv").c_str(), "w");
        hart->setInitialStateFile(preload_log[i]);
      }
      hart->setInstructionCountLimit(FLAGS_max_instr);
      hart->setWfiTimeout(0);
      threadVec.emplace_back(std::thread(threadFunc, hart));
    }

    if (FLAGS_whisper_data_lines != "")
      system_->enableDataLineTrace(FLAGS_whisper_data_lines);
    if (FLAGS_whisper_instr_lines != "")
      system_->enableInstructionLineTrace(FLAGS_whisper_instr_lines);

    for (auto& t : threadVec)
      t.join();

    fclose(whisper_log);

    if (FLAGS_preload) {
      for (unsigned i = 0; i < system_->hartCount(); ++i) {
        fclose(preload_log[i]);
      }
    }
  }

  // run once before starting cosim
  if (FLAGS_standalone && FLAGS_hart_enable_mask == 0x1) {
    system_ = constructSystem<URV>(ncores, true, false);
    if (system_ == nullptr) {
      std::cerr << "Error: could not construct system\n";
      return -1;
    }

    std::vector<std::thread> threadVec;

    std::atomic<bool> result = true;
    std::atomic<bool> max_instr = false;
    std::atomic<unsigned> finished = 0;  // Count of finished threads.

    FILE* whisper_log = fopen("iss_standalone.log", "w");
    FILE* preload_log[system_->hartCount()];

    auto threadFunc = [&result, &finished, &max_instr, whisper_log] (WdRiscv::Hart<URV>* hart) {
                        bool r = hart->run(whisper_log);
                        result = result and r;
                        max_instr = max_instr or (hart->getInstructionCount() >= FLAGS_max_instr);
                        finished++;
                      };

    for (unsigned i = 0; i < system_->hartCount(); ++i) {
      WdRiscv::Hart<URV>* hart = system_->ithHart(i).get();
      if (FLAGS_preload) {
        preload_log[i] = fopen(("preload_" + std::to_string(i) + ".csv").c_str(), "w");
        hart->setInitialStateFile(preload_log[i]);
      }
      if (FLAGS_tohost)
        hart->setToHostAddress(FLAGS_tohost);
      hart->setInstructionCountLimit(FLAGS_max_instr);
      hart->setWfiTimeout(0);
      threadVec.emplace_back(std::thread(threadFunc, hart));
    }

    if (FLAGS_whisper_data_lines != "")
      system_->enableDataLineTrace(FLAGS_whisper_data_lines);
    if (FLAGS_whisper_instr_lines != "")
      system_->enableInstructionLineTrace(FLAGS_whisper_instr_lines);

    for (auto& t : threadVec)
      t.join();

    fclose(whisper_log);

    if (FLAGS_preload) {
      for (unsigned i = 0; i < system_->hartCount(); ++i) {
        fclose(preload_log[i]);
      }
    }

    if (!FLAGS_nostop_standalone) {
        if (!result) {
          std::cerr << "Error: Test failed on Standalone Whisper, stopping simulation\n";
          return -1;
        } else if (max_instr && (FLAGS_max_instr != 0)) {
          std::cerr << "Error: Test reached max instr on standalone Whisper, stopping simulation\n";
          return -1;
        }
    }
  }

  if (FLAGS_cov) {
    auto soPtr = dlopen(FLAGS_archsample_lib_path.c_str(), RTLD_NOW);
    if (not soPtr) {
      std::cerr << "Error: Failed to load shared libarary " << dlerror() << '\n';
      return -1;
    }

    std::string entry("tracerExtension");
    entry += sizeof(URV) == 4 ? "32" : "64";

    __tracerExtension = reinterpret_cast<void (*)(void*)>(dlsym(soPtr, entry.c_str()));
    if (not __tracerExtension) {
      std::cerr << "Error: Could not find symbol tracerExtension in " << std::string(FLAGS_archsample_lib_path) << '\n';
      return -1;
    }
  }

  system_ = constructSystem<URV>(ncores, false, false);
  if (system_ == nullptr) {
    std::cerr << "Error: could not construct system\n";
    return -1;
  }

  server_ = std::make_unique<WdRiscv::Server<URV>>(*system_);

  return 0;
}


template <typename URV>
bool
whisperClient<URV>::whisperConnected()
{
  return server_ != nullptr;
}

template <typename URV>
void
whisperClient<URV>::whisperDisableMcm()
{
  system_->enableMcm(64, false);
}


template <typename URV>
bool
whisperClient<URV>::whisperCommand(const WhisperMessage& req, WhisperMessage& reply)
{
  server_->interact(req, reply, traceFile_, commandLog_);
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

  if (not whisperCommand(req, reply))
    return false;

  value = reply.value;
  return true;
}
template <typename URV>
bool
whisperClient<URV>::whisperPeekVpr(int hart, uint64_t addr, std::array<std::uint8_t, 32>& value)
{
  int i;
  req.hart = hart;
  req.type = WhisperMessageType::Peek;
  req.resource = 'v';
  req.address = addr;

  if (not whisperCommand(req, reply))
    return false;

  for(i=0;i<32;i++) {
     value[i] = reply.buffer[i]; 
  }
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
	    uint32_t& fpFlags, bool& hasTrap, bool& hasStop, bool& isLoad)
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

  unsigned mode = wflags.bits.privMode;
  unsigned flags = wflags.bits.fpFlags;
  unsigned trap = wflags.bits.trap;
  unsigned stop = wflags.bits.stop;
  unsigned virt = wflags.bits.virt;
  unsigned load = wflags.bits.load;

  privMode = mode | (virt << 3);
  fpFlags = flags;
  hasTrap = trap;
  hasStop = stop;
  isLoad = load;
  reply.buffer[reply.buffer.size() - 1] = '\0';
  disasm = reply.buffer.data();

  return true;
}

// Copied from chuang's LSTB Whisper Client
template <typename URV>
bool
whisperClient<URV>::whisperSimpleStep(int hart, uint64_t& pc, uint32_t& instruction, unsigned& changeCount)
{
  req.hart = hart;
  req.type = WhisperMessageType::Step;
  req.instrTag = 0;
  req.time = 0;

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
  addr = reply.address;
  value = reply.value;
  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

//void
//whisperClient::whisperChanges(int hart, std::unordered_map<uint32_t,uint64_t>& addrs, std::unordered_map<uint32_t,uint64_t>& datas,
//               int changeCount)
//{
//  for (int i = 0; i < changeCount; i++)
//    {
//      uint32_t res;
//      uint64_t addr, value;
//      bool valid;
//      assert(whisperChange(hart, res, addr, value, valid) and valid);
//      addrs[res] = addr;
//      datas[res] = value;
//    }
//}

template <typename URV>
bool
whisperClient<URV>::whisperMcmRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
	       unsigned size, uint64_t value, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::McmRead;
  req.time = time;
  req.instrTag = instrTag;
  req.address = addr;
  req.value = value;
  req.size = size;

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
  req.address = addr;
  req.value = value;
  req.size = size;

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
  for (uint8_t i = 0; i < req.size/8; ++i) {
    req.tag[i] = (uint8_t)((mask >> (i*8)) & 0xff);
  }
  if (req.size > req.buffer.size())
    {
      std::cerr << "whisperMcmWrite: write size too large: " << req.size << '\n';
      valid = false;
      return true;
    }

  uint8_t* data = reinterpret_cast<uint8_t*> (handle);
  for (unsigned i = 0; i < size; ++i)
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

  // There is no reply for quit command.
  return true;
}

//bool
//whisperClient::whisperQuit()
//{
//  req.hart = 0;
//  req.type = WhisperMessageType::Quit;
//
//  if (not whisperCommand(req, reply))
//    {
//      std::cerr << "Failed to send request to whisper\n";
//      return false;
//    }
//
//  whisperDisconnect();
//
//  // There is no reply for quit command.
//  return true;
//}

template <typename URV>
bool
whisperClient<URV>::whisperPageTableWalk(int, bool, bool,
		     svOpenArrayHandle, unsigned&,
		     bool&)
{
  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperEnterDebug()
{
  req.hart = 0;
  req.type = WhisperMessageType::EnterDebug;

  if (not whisperCommand(req, reply))
    return false;

  return true;
}

template <typename URV>
bool
whisperClient<URV>::whisperExitDebug()
{
  req.hart = 0;
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
whisperClient<URV>::whisperCheckInterrupt(int hart, uint64_t mip, bool& interrupt, uint64_t& cause)
{
  req.hart = hart;
  req.type = WhisperMessageType::CheckInterrupt;
  req.address = mip;

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

  WhisperMessage reply;

  if (not whisperCommand(req, reply))
    return false;

  value = reply.value;

  return true;
}

template class whisperClient<uint32_t>;
template class whisperClient<uint64_t>;
