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

DECLARE_string(load);
DECLARE_string(hex);
DECLARE_string(bootrom_path);
DECLARE_string(whisper_json_path);
DECLARE_bool(whisper_stdin_null);
DECLARE_bool(whisper_stdout_null);
DECLARE_bool(mcm);
DECLARE_bool(cov);
DECLARE_uint32(max_instr);
DECLARE_string(archsample_lib_path);
DECLARE_bool(standalone);
DEFINE_string(whisper_instr_lines, "", "Write instr cache line addresses used in test to a file");
DEFINE_string(whisper_data_lines, "", "Write data cache line addresses used in test to a file");
DEFINE_uint64(resetpc, 0x80000000, "Reset PC");

extern void (*__tracerExtension)(void*);

template <typename URV>
static std::shared_ptr<WdRiscv::System<URV>>
constructSystem(uint16_t ncores) {

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

  if (FLAGS_mcm) {
    bool checkAll = false;
    config.getMcmCheckAll(checkAll);
    system->enableMcm(64, checkAll);
  }


  if (FLAGS_bootrom_path != "" || FLAGS_load != "") {
    std::vector<std::string> targets {};
    if (FLAGS_load != "")
      targets.push_back(FLAGS_load);
    if (FLAGS_bootrom_path != "")
      targets.push_back(FLAGS_bootrom_path);
    if (not system->loadElfFiles(targets, false, false))
      return nullptr;
  }

  if (FLAGS_hex != "") {
    std::vector<std::string> targets = {FLAGS_hex};
    if (not system->loadHexFiles(targets, false))
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
    hart.defineResetPc(FLAGS_resetpc);
    if (FLAGS_whisper_stdout_null) hart.redirectOutputDescriptor(STDOUT_FILENO, "/dev/null");
    if (FLAGS_whisper_stdin_null) hart.redirectOutputDescriptor(STDIN_FILENO, "/dev/null");
    if (not isa.empty())
      if (not hart.configIsa(isa, false))
        return nullptr;
    hart.reset();
  }

  return system;
}

template <typename URV>
int
whisperClient<URV>::whisperConnect(uint16_t ncores)
{
  system_ = constructSystem<URV>(ncores);

  // run once before starting cosim
  if (FLAGS_standalone) {
    std::cout << "starting standalone"  << std::endl;
    std::vector<std::thread> threadVec;

    std::atomic<bool> result = true;
    std::atomic<unsigned> finished = 0;  // Count of finished threads.

    FILE* whisper_log = fopen("iss_standalone.log", "w");

    auto threadFunc = [&result, &finished, whisper_log] (WdRiscv::Hart<URV>* hart) {
                        bool r = hart->run(whisper_log);
                        result = result and r;
                        finished++;
                      };

    for (unsigned i = 0; i < system_->hartCount(); ++i) {
      WdRiscv::Hart<URV>* hart = system_->ithHart(i).get();
      hart->setInstructionCountLimit(FLAGS_max_instr);
      threadVec.emplace_back(std::thread(threadFunc, hart));
    }

    if (FLAGS_whisper_data_lines != "")
      system_->enableDataLineTrace(FLAGS_whisper_data_lines);
    if (FLAGS_whisper_instr_lines != "")
      system_->enableInstructionLineTrace(FLAGS_whisper_instr_lines);

    for (auto& t : threadVec)
      t.join();

    fclose(whisper_log);

    system_ = constructSystem<URV>(ncores);
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

  server_ = std::make_unique<WdRiscv::Server<URV>>(*system_);

  std::cout << "[WHISPER CLIENT] Connect successful..\n";

  return 0;
}


template <typename URV>
void
whisperClient<URV>::whisperDisconnect()
{
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


template <typename URV>
bool
whisperClient<URV>::whisperStep(int hart, uint64_t time, uint64_t instrTag, uint64_t& pc,
	    uint32_t& instruction, unsigned& changeCount,
	    std::string& disasm, uint32_t& privMode,
	    uint32_t& fpFlags, bool& hasTrap, bool& hasStop)
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

  // Recover privilege mode (2 bits), fpFlags (4 bits), and trap (1
  // bit) from flags field.
  unsigned mode = reply.flags & 3;
  unsigned flags = (reply.flags >> 2) & 0xf;
  unsigned trap = (reply.flags >> 6) & 1;
  unsigned stop = (reply.flags >> 7) & 1;

  privMode = mode;
  fpFlags = flags;
  hasTrap = trap;
  hasStop = stop;
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
whisperClient<URV>::whisperTranslate(int hart, uint64_t vaddr, bool r, bool w, bool x,
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

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  if (valid)
    paddr = reply.address;
  return true;
}

//bool
//whisperClient::whisperCancelLr(int hart, bool& valid)
//{
//  req.hart = hart;
//  req.type = WhisperMessageType::CancelLr;
//
//  if (not whisperCommand(req, reply))
//    return false;
//
//  valid = reply.type != WhisperMessageType::Invalid;
//  return true;
//}

template <typename URV>
bool
whisperClient<URV>::whisperReset(int hart, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::Reset;

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

  whisperDisconnect();

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
whisperClient<URV>::whisperSetSeiPin(int hart, uint64_t value)
{
  req.hart = hart;
  req.type = WhisperMessageType::SeiPin;
  req.value = value;

  WhisperMessage reply;

  if (not whisperCommand(req, reply))
    return false;

  return true;
}

template class whisperClient<uint32_t>;
template class whisperClient<uint64_t>;
