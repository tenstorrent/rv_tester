#pragma once

#include <string>

#include "svdpi.h"
#include "WhisperMessage.h"

class whisperClient {

  public:

    virtual ~whisperClient() {}

    virtual int whisperConnect(const char* filePath) = 0;
    virtual void whisperDisconnect() = 0;
    virtual bool whisperStep(int hart, uint64_t time, uint64_t instrTag, uint64_t& pc, uint32_t& instruction, unsigned& changeCount, std::string& disasm, uint32_t& privMode, uint32_t& fpFlags, bool& hasTrap, bool& hasStop) = 0;
    virtual bool whisperSimpleStep(int hart, uint64_t& pc, uint32_t& instruction, unsigned& changeCount) = 0;
    virtual bool whisperChange(int hart, uint32_t& resource, uint64_t& addr, uint64_t& value, bool& valid) = 0;
    virtual bool whisperMcmRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool internal, bool& valid) = 0;
    virtual bool whisperMcmInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool& valid) = 0;
    virtual bool whisperMcmWrite(int hart, uint64_t time, uint64_t addr, unsigned size, svOpenArrayHandle handle, uint64_t mask, bool& valid) = 0;
    virtual bool whisperPoke(int hart, uint64_t time, char resource, uint64_t addr, uint64_t value, bool& valid) = 0;
    virtual bool whisperPeek(int hart, char resource, uint64_t addr, uint64_t& value, bool& valid) = 0;
    virtual bool whisperReset(int hart, bool& valid) = 0;
    virtual bool whisperQuit() = 0;
    virtual bool whisperPageTableWalk(int hart, bool isInstr, bool isAddr, svOpenArrayHandle items, unsigned& itemCount, bool& valid) = 0;
    virtual bool whisperTranslate(int hart, uint64_t vaddr, bool r, bool w, bool x, bool supervisor, uint64_t& paddr, bool& valid) = 0;
    virtual bool whisperEnterDebug() = 0;
    virtual bool whisperExitDebug() = 0;
    virtual bool whisperCheckInterrupt(int hart, uint64_t mip, bool& interrupt) = 0;
    virtual bool whisperSetSeiPin(int hart, uint64_t value) = 0;

};
