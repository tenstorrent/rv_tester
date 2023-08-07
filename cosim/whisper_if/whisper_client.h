#pragma once

#include <string>
#include <memory>

#include "svdpi.h"
#include "System.hpp"
#include "Server.hpp"
#include "WhisperMessage.h"

template <typename URV>
class whisperClient {

  public:
    whisperClient(std::string traceFile, std::string commandLog) :
      traceFile_(traceFile.empty() ? nullptr : fopen(traceFile.c_str(), "w")),
      commandLog_(commandLog.empty() ? nullptr : fopen(commandLog.c_str(), "w"))
  {}

    ~whisperClient() {
      if (traceFile_ != nullptr) {
        fclose(traceFile_);
      }
      if (commandLog_ != nullptr) {
        fclose(commandLog_);
      }
    }

    int whisperConnect();
    void whisperDisconnect();
    bool whisperStep(int hart, uint64_t time, uint64_t instrTag, uint64_t& pc, uint32_t& instruction, unsigned& changeCount, std::string& disasm, uint32_t& privMode, uint32_t& fpFlags, bool& hasTrap, bool& hasStop);
    bool whisperSimpleStep(int hart, uint64_t& pc, uint32_t& instruction, unsigned& changeCount);
    bool whisperChange(int hart, uint32_t& resource, uint64_t& addr, uint64_t& value, bool& valid);
    bool whisperMcmRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool& valid);
    bool whisperMcmInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool& valid);
    bool whisperMcmWrite(int hart, uint64_t time, uint64_t addr, unsigned size, svOpenArrayHandle handle, uint64_t mask, bool& valid);
    bool whisperPoke(int hart, uint64_t time, char resource, uint64_t addr, uint64_t value, bool& valid);
    bool whisperPeek(int hart, char resource, uint64_t addr, uint64_t& value, bool& valid);
    bool whisperReset(int hart, bool& valid);
    bool whisperQuit();
    bool whisperPageTableWalk(int hart, bool isInstr, bool isAddr, svOpenArrayHandle items, unsigned& itemCount, bool& valid);
    bool whisperTranslate(int hart, uint64_t vaddr, bool r, bool w, bool x, bool supervisor, uint64_t& paddr, bool& valid);
    bool whisperEnterDebug();
    bool whisperExitDebug();
    bool whisperCheckInterrupt(int hart, uint64_t mip, bool& interrupt, uint64_t& cause);
    bool whisperSetSeiPin(int hart, uint64_t value);

  private:

    bool whisperCommand(const WhisperMessage& req, WhisperMessage& reply);

    std::shared_ptr<WdRiscv::System<URV>> system_;
    std::unique_ptr<WdRiscv::Server<URV>> server_;
    FILE* traceFile_ = nullptr;
    FILE* commandLog_ = nullptr;
    WhisperMessage req {};
    WhisperMessage reply {};
};
