#pragma once

#include <string>

#include "svdpi.h"
#include "whisper_client.h"

class whisperClientShm : public whisperClient {

  public:

    whisperClientShm() {}
    ~whisperClientShm() {}

    virtual int whisperConnect(const char* filePath) override;
    virtual void whisperDisconnect() override;
    virtual bool whisperStep(int hart, uint64_t time, uint64_t instrTag, uint64_t& pc, uint32_t& instruction, unsigned& changeCount, std::string& buffer, uint32_t& privMode, uint32_t& fpFlags, bool& hasTrap, bool& hasStop) override;
    virtual bool whisperSimpleStep(int hart, uint64_t& pc, uint32_t& instruction, unsigned& changeCount) override;
    virtual bool whisperChange(int hart, uint32_t& resource, uint64_t& addr, uint64_t& value, bool& valid) override;
    virtual bool whisperMcmRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool internal, bool& valid) override;
    virtual bool whisperMcmInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool& valid) override;
    virtual bool whisperMcmWrite(int hart, uint64_t time, uint64_t addr, unsigned size, svOpenArrayHandle handle, uint64_t mask, bool& valid) override;
    virtual bool whisperPoke(int hart, char resource, uint64_t addr, uint64_t value, bool& valid) override;
    virtual bool whisperPeek(int hart, char resource, uint64_t addr, uint64_t& value, bool& valid) override;
    virtual bool whisperReset(int hart, bool& valid) override;
    virtual bool whisperQuit() override;
    virtual bool whisperPageTableWalk(int hart, bool isInstr, bool isAddr, svOpenArrayHandle items, unsigned& itemCount, bool& valid) override;
    virtual bool whisperTranslate(int hart, uint64_t vaddr, bool r, bool w, bool x, bool supervisor, uint64_t& paddr, bool& valid) override;
    virtual bool whisperEnterDebug() override;
    virtual bool whisperExitDebug() override;

  private:

    int whisperConnectHostPort(const char* host, unsigned port);
    void deserializeMessage(const char buffer[], size_t bufferLen, WhisperMessage& msg);
    size_t serializeMessage(const WhisperMessage& msg, char buffer[], size_t bufferLen);
    bool receiveMessage(char* shm, WhisperMessage& msg);
    bool sendMessage(char* shm, const WhisperMessage& msg);
    bool whisperCommand(const WhisperMessage& req, WhisperMessage& reply);

  private:

    //static int fd = -1;
    //static char* shm = nullptr;
    //static std::string path;
    //
    //static WhisperMessage req;
    //static WhisperMessage reply;

};
