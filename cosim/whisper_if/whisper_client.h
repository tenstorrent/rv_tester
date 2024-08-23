#pragma once

#include <string>
#include <memory>

#include "svdpi.h"
#include "System.hpp"
#include "Server.hpp"
#include "WhisperMessage.h"
#include "cvm/messenger.hpp"
#include "cvm/registry.hpp"

// #define FUNCTION_TYPE(func) std::remove_pointer<decltype(&func)>::type    // TODO: move this to messenger.hpp

// #define CVM_MESSENGER_procedure_call(name, func_type) \
// struct name : cvm::messenger::procedure_call_function<func_type> {};

// Procedure Call Declarations

// CVM_MESSENGER_procedure_call(whisperPokeRPC, bool (int, uint64_t, char, uint64_t, uint64_t, bool&));

#define WHISPER_LOC cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0)

template <typename URV>
class whisperClient {

  public:
  //   whisperClient(std::string traceFile, std::string commandLog) :
  //     traceFile_(traceFile.empty() ? nullptr : fopen(traceFile.c_str(), "w")),
  //     commandLog_(commandLog.empty() ? nullptr : fopen(commandLog.c_str(), "w"))
  // {}

    whisperClient(cvm::topology::loc_t loc, unsigned);

    ~whisperClient() {
      if (traceFile_ != nullptr) {
        fclose(traceFile_);
      }
      if (commandLog_ != nullptr) {
        fclose(commandLog_);
      }

      whisperQuit();
    }

    int whisperConnect(uint16_t ncores);
    bool whisperConnected();
    void whisperDisableMcm();
    bool whisperStep(int hart, uint64_t time, uint64_t instrTag, uint64_t& pc, uint32_t& instruction, unsigned& changeCount, std::string& disasm, uint32_t& privMode, uint32_t& fpFlags, bool& hasTrap, bool& hasStop, bool& isLoad);
    bool whisperSimpleStep(int hart, uint64_t& pc, uint32_t& instruction, unsigned& changeCount);
    bool whisperChange(int hart, uint32_t& resource, uint64_t& addr, uint64_t& value, bool& valid);
    bool whisperMcmRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool& valid);
    bool whisperMcmVecRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, std::vector<uint64_t> value, bool& valid);
    bool whisperMcmVecInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, std::vector<uint64_t> value, bool& valid);
    bool whisperMcmInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool& valid);
    bool whisperMcmBypass(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool& valid);
    bool whisperMcmWrite(int hart, uint64_t time, uint64_t addr, unsigned size, svOpenArrayHandle handle, uint64_t mask, bool& valid);
    bool whisperMcmIFetch(int hart, uint64_t time, uint64_t addr, bool& valid);
    bool whisperMcmIEvict(int hart, uint64_t time, uint64_t addr, bool& valid);
    bool whisperPoke(int hart, uint64_t time, char resource, uint64_t addr, uint64_t value, bool& valid);
    bool whisperPokeMem(int hart, uint64_t time, char resource, uint64_t addr, unsigned size, uint64_t value, bool& valid);
    bool whisperPeek(int hart, char resource, uint64_t addr, uint64_t& value, bool& valid);
    bool whisperPeekPc(int hart, uint64_t& value);
    bool whisperPeekCsr(int hart, uint64_t addr, uint64_t& value, uint64_t& mask, uint64_t& reset_value, uint64_t& read_mask, bool& valid);
    bool whisperReset(int hart, uint64_t addr, bool& valid);
    bool whisperQuit();
    bool whisperPageTableWalk(int hart, bool isInstr, bool isAddr, svOpenArrayHandle items, unsigned& itemCount, bool& valid);
    bool whisperTranslate(int hart, uint64_t vaddr, bool r, bool w, bool x, bool twoStage, bool supervisor, uint64_t& paddr, bool& valid);
    bool whisperEnterDebug(int hart);
    bool whisperExitDebug(int hart);
    bool whisperCheckInterrupt(int hart, uint64_t mip, bool& interrupt, uint64_t& cause);
    bool whisperGetSeiPin(int hart, uint64_t& value);
    bool whisperCancelLr(int hart, bool& valid);
    bool whisperPeekGpr(int hart, uint64_t addr, uint64_t& value);
    bool whisperPeekFpr(int hart, uint64_t addr, uint64_t& value);
    bool whisperPeekVpr(int hart, uint64_t addr, std::array<std::uint8_t, 32>&  value);


  private:

    bool whisperCommand(const WhisperMessage& req, WhisperMessage& reply);

    std::shared_ptr<WdRiscv::System<URV>> system_;
    std::unique_ptr<WdRiscv::Server<URV>> server_;
    FILE* traceFile_ = nullptr;
    FILE* commandLog_ = nullptr;
    WhisperMessage req {};
    WhisperMessage reply {};

  public:
    CVM_MESSENGER_procedure_call(whisperConnectRPC, int (uint16_t));
    CVM_MESSENGER_procedure_call(whisperConnectedRPC, bool (void));
    CVM_MESSENGER_procedure_call(whisperDisableMcmRPC, void(void));
    CVM_MESSENGER_procedure_call(whisperStepRPC, bool(int, uint64_t, uint64_t, uint64_t&, uint32_t&, unsigned&, std::string&, uint32_t&, uint32_t&, bool&, bool&, bool&));
    CVM_MESSENGER_procedure_call(whisperSimpleStepRPC, bool (int, uint64_t&, uint32_t&, unsigned&));
    CVM_MESSENGER_procedure_call(whisperChangeRPC, bool (int, uint32_t&, uint64_t&, uint64_t&, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmReadRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmVecReadRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, std::vector<uint64_t>, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmVecInsertRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, std::vector<uint64_t>, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmInsertRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmBypassRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmWriteRPC, bool (int, uint64_t, uint64_t, unsigned, svOpenArrayHandle, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmIFetchRPC, bool (int, uint64_t, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmIEvictRPC, bool (int, uint64_t, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperPokeRPC, bool (int, uint64_t, char, uint64_t, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperPokeMemRPC, bool (int, uint64_t, char, uint64_t, unsigned, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperPeekRPC, bool (int, char, uint64_t, uint64_t&, bool&));
    CVM_MESSENGER_procedure_call(whisperPeekPcRPC, bool (int, uint64_t& value));
    CVM_MESSENGER_procedure_call(whisperPeekCsrRPC, bool (int, uint64_t, uint64_t&, uint64_t&, uint64_t&, uint64_t&, bool&));
    CVM_MESSENGER_procedure_call(whisperResetRPC, bool (int, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperQuitRPC, bool (void));
    CVM_MESSENGER_procedure_call(whisperPageTableWalkRPC, bool (int, bool, bool, svOpenArrayHandle, unsigned&, bool&));
    CVM_MESSENGER_procedure_call(whisperTranslateRPC, bool (int, uint64_t, bool, bool, bool, bool, bool, uint64_t&, bool&));
    CVM_MESSENGER_procedure_call(whisperEnterDebugRPC, bool (int));
    CVM_MESSENGER_procedure_call(whisperExitDebugRPC, bool (int));
    CVM_MESSENGER_procedure_call(whisperCheckInterruptRPC, bool (int, uint64_t, bool&, uint64_t&));
    CVM_MESSENGER_procedure_call(whisperGetSeiPinRPC, bool (int, uint64_t&));
    CVM_MESSENGER_procedure_call(whisperCancelLrRPC, bool (int, bool&));
    CVM_MESSENGER_procedure_call(whisperPeekGprRPC, bool (int, uint64_t, uint64_t&));
    CVM_MESSENGER_procedure_call(whisperPeekFprRPC, bool (int, uint64_t, uint64_t&));
    CVM_MESSENGER_procedure_call(whisperPeekVprRPC, bool (int, uint64_t, std::array<std::uint8_t, 32>&)); 
};


