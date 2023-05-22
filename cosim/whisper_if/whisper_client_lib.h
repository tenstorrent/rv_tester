#pragma once

#include <string>
#include <memory>

#include "svdpi.h"
#include "whisper_client.h"
#include "System.hpp"
#include "Server.hpp"

template <typename URV>
class whisperClientLib : public whisperClient {

  public:
    whisperClientLib(std::string traceFile, std::string commandLog, std::string prog) :
      traceFile_(traceFile.empty() ? nullptr : fopen(traceFile.c_str(), "w")),
      commandLog_(commandLog.empty() ? nullptr : fopen(commandLog.c_str(), "w")),
      prog_(prog)
  {}
    
    ~whisperClientLib() {
      if (traceFile_ != nullptr) {
        fclose(traceFile_);
      }
      if (commandLog_ != nullptr) {
        fclose(commandLog_);
      }
    }

    virtual int whisperConnect(const char* filePath) override;
    virtual void whisperDisconnect() override;
    virtual bool whisperStep(int hart, uint64_t time, uint64_t instrTag, uint64_t& pc, uint32_t& instruction, unsigned& changeCount, std::string& disasm, uint32_t& privMode, uint32_t& fpFlags, bool& hasTrap, bool& hasStop) override;
    virtual bool whisperSimpleStep(int hart, uint64_t& pc, uint32_t& instruction, unsigned& changeCount) override;
    virtual bool whisperChange(int hart, uint32_t& resource, uint64_t& addr, uint64_t& value, bool& valid) override;
    virtual bool whisperMcmRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool internal, bool& valid) override;
    virtual bool whisperMcmInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool& valid) override;
    virtual bool whisperMcmWrite(int hart, uint64_t time, uint64_t addr, unsigned size, svOpenArrayHandle handle, uint64_t mask, bool& valid) override;
    virtual bool whisperPoke(int hart, uint64_t time, char resource, uint64_t addr, uint64_t value, bool& valid) override;
    virtual bool whisperPeek(int hart, char resource, uint64_t addr, uint64_t& value, bool& valid) override;
    virtual bool whisperReset(int hart, bool& valid) override;
    virtual bool whisperQuit() override;
    virtual bool whisperPageTableWalk(int hart, bool isInstr, bool isAddr, svOpenArrayHandle items, unsigned& itemCount, bool& valid) override;
    virtual bool whisperTranslate(int hart, uint64_t vaddr, bool r, bool w, bool x, bool supervisor, uint64_t& paddr, bool& valid) override;
    virtual bool whisperEnterDebug() override;
    virtual bool whisperExitDebug() override;
    virtual bool whisperCheckInterrupt(int hart, uint64_t mip, bool& interrupt) override;
    virtual bool whisperSetSeiPin(int hart, uint64_t value) override;

  private:

    bool whisperCommand(const WhisperMessage& req, WhisperMessage& reply);

    std::shared_ptr<WdRiscv::System<URV>> system_;
    std::unique_ptr<WdRiscv::Server<URV>> server_;
    FILE* traceFile_ = nullptr;
    FILE* commandLog_ = nullptr;
    std::string prog_;
    WhisperMessage req {};
    WhisperMessage reply {};
};
