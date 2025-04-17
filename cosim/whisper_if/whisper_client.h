#pragma once

#include <string>
#include <memory>

#include "svdpi.h"
#include "System.hpp"
#include "Server.hpp"
#include "WhisperMessage.h"
#include "cvm/messenger.hpp"
#include "cvm/registry.hpp"
#include "HartConfig.hpp"
#include "Hart.hpp"
#include "Args.hpp"
#include "Session.hpp"


template <typename URV>
class whisperClient {

  public:
    enum AddrCategory {
      PC = 0,
      LOAD = 1,
      STORE = 2, // AMOs will be returned as Store
      NONE = 3
    };
    struct iss_select_s {
      AddrCategory categ;
      std::string disasm;
      uint64_t virt_addr;
      uint64_t phys_addr;
      bool is_fetch;
      iss_select_s() {}
      iss_select_s(AddrCategory c, const std::string d, uint64_t v, uint64_t p) { init(c, d, v, p);}
      void init(AddrCategory c, const std::string d, uint64_t v, uint64_t p) { categ=c; disasm=d; virt_addr=v; phys_addr=p;   }
      bool operator==(const iss_select_s& other) const { return virt_addr == other.virt_addr && phys_addr == other.phys_addr; }
    };


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

    // Whisper Client Procedure Calls
    // 1. Add the function declaration here
    // 2. Use CVM_MESSENGER_procedure_call() to add a procedure call type
    // 3. Register the function with messenger in the whisperClient constructor
    // 4. Call the function with cvm::registry::messenger.call<whisper...RPC>() 

    // getter and setter to access class variable through procedure calls
    // 
    std::vector<iss_select_s> get_iss_select()  { return rand_addrs_; }
    std::vector<uint64_t> get_dm_rand_val(void)  { return dm_rand_val_; }
    uint64_t              get_dm_rand_addr(void) { return dm_rand_addr_;}

    static bool constructSystem(std::shared_ptr<WdRiscv::Session<URV>>&, std::shared_ptr<WdRiscv::System<URV>>&, WdRiscv::Args&, uint16_t ncores, bool standalone, std::string logfile="");
    int whisperConnect();
    bool whisperConnected();
    int whisperStandalone();
    int processStandaloneInfo();
    bool whisperStep(int hart, uint64_t time, uint64_t instrTag, uint64_t& pc, uint32_t& instruction, unsigned& changeCount, std::string& disasm, uint32_t& privMode, uint32_t& fpFlags, bool& hasTrap, bool& hasStop, bool& isLoad, bool& valid);
    bool whisperSimpleStep(int hart, uint64_t& pc, uint32_t& instruction, unsigned& changeCount);
    bool whisperChange(int hart, uint32_t& resource, uint64_t& addr, uint64_t& value, bool& valid);
    bool whisperMcmRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, unsigned elemIx, unsigned field, bool& valid);
    bool whisperMcmVecRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, std::vector<uint64_t> value, unsigned elemIx, unsigned field, bool& valid);
    bool whisperMcmVecInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, std::vector<uint64_t> value, bool& valid);
    bool whisperMcmInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool& valid);
    bool whisperMcmVecBypass(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, std::vector<uint64_t> value, bool& valid);
    bool whisperMcmBypass(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, bool& valid);
    bool whisperMcmWrite(int hart, uint64_t time, uint64_t addr, unsigned size, svOpenArrayHandle handle, uint64_t mask, bool& valid);
    bool whisperMcmIFetch(int hart, uint64_t time, uint64_t addr, bool& valid);
    bool whisperMcmIEvict(int hart, uint64_t time, uint64_t addr, bool& valid);
    bool whisperMcmEnd(int hart, uint64_t time, bool& valid);
    bool whisperInjectException(int hart, bool isLoad, uint64_t code, unsigned elemIx, bool& valid);
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
    bool whisperCheckInterrupt(int hart, bool& interrupt, uint64_t& cause);
    bool whisperGetSeiPin(int hart, uint64_t& value);
    bool whisperCancelLr(int hart, bool& valid);
    bool whisperPeekGpr(int hart, uint64_t addr, uint64_t& value);
    bool whisperPeekFpr(int hart, uint64_t addr, uint64_t& value);
    bool whisperPeekVpr(int hart, uint64_t addr, std::array<std::uint8_t, 32>&  value);
    bool whisperGetLastLdStAddress(int hart, uint64_t& value);
    bool whisperMcmSkipReadDataCheck(uint64_t addr, unsigned size, bool enable);

    // Deliver a non-maskable interrupt to whisper.
    bool whisperNmi(int hart, uint64_t time, uint64_t cause);
    bool whisperClearNmi(int hart, uint64_t time);

  private:

    bool whisperCommand(const WhisperMessage& req, WhisperMessage& reply);

    std::shared_ptr<WdRiscv::Session<URV>> session_;
    std::shared_ptr<WdRiscv::System<URV>> system_;
    std::unique_ptr<WdRiscv::Server<URV>> server_;
    WdRiscv::Args args_ ;
    FILE* traceFile_ = nullptr;
    FILE* commandLog_ = nullptr;
    WhisperMessage req {};
    WhisperMessage reply {};

    cvm::topology::loc_t loc_;
    uint32_t ncores_ = 0;
    uint64_t dm_rand_addr_ = 0;
    std::vector<uint64_t> dm_rand_val_;
    std::vector<iss_select_s> rand_addrs_;
    uint64_t secure_region_start_=0, secure_region_end_=0;

  public:
    CVM_MESSENGER_procedure_call(iss_select_rand_RPC, std::vector<iss_select_s> (void));
    CVM_MESSENGER_procedure_call(get_dm_rand_addr_RPC, uint64_t (void));
    CVM_MESSENGER_procedure_call(get_dm_rand_val_RPC, std::vector<uint64_t> (void));

    CVM_MESSENGER_procedure_call(whisperConnectRPC, int (void));
    CVM_MESSENGER_procedure_call(whisperConnectedRPC, bool (void));
    CVM_MESSENGER_procedure_call(whisperStepRPC, bool(int, uint64_t, uint64_t, uint64_t&, uint32_t&, unsigned&, std::string&, uint32_t&, uint32_t&, bool&, bool&, bool&, bool&));
    CVM_MESSENGER_procedure_call(whisperSimpleStepRPC, bool (int, uint64_t&, uint32_t&, unsigned&));
    CVM_MESSENGER_procedure_call(whisperChangeRPC, bool (int, uint32_t&, uint64_t&, uint64_t&, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmReadRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, uint64_t, unsigned, unsigned, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmVecReadRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, std::vector<uint64_t>, unsigned, unsigned, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmVecInsertRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, std::vector<uint64_t>, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmInsertRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmVecBypassRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, std::vector<uint64_t>, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmBypassRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmWriteRPC, bool (int, uint64_t, uint64_t, unsigned, svOpenArrayHandle, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmIFetchRPC, bool (int, uint64_t, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmIEvictRPC, bool (int, uint64_t, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmEndRPC, bool (int, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperInjectExceptionRPC, bool (int, bool, uint64_t, unsigned, bool&));
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
    CVM_MESSENGER_procedure_call(whisperCheckInterruptRPC, bool (int, bool&, uint64_t&));
    CVM_MESSENGER_procedure_call(whisperGetSeiPinRPC, bool (int, uint64_t&));
    CVM_MESSENGER_procedure_call(whisperCancelLrRPC, bool (int, bool&));
    CVM_MESSENGER_procedure_call(whisperPeekGprRPC, bool (int, uint64_t, uint64_t&));
    CVM_MESSENGER_procedure_call(whisperPeekFprRPC, bool (int, uint64_t, uint64_t&));
    CVM_MESSENGER_procedure_call(whisperPeekVprRPC, bool (int, uint64_t, std::array<std::uint8_t, 32>&)); 
    CVM_MESSENGER_procedure_call(whisperGetLastLdStAddressRPC, bool (int, uint64_t&));
    CVM_MESSENGER_procedure_call(whisperNmiRPC, bool (int, uint64_t, uint64_t));
    CVM_MESSENGER_procedure_call(whisperClearNmiRPC, bool (int, uint64_t));
    CVM_MESSENGER_procedure_call(whisperMcmSkipReadDataCheckRPC, bool (uint64_t, unsigned, bool));
    CVM_MESSENGER_procedure_call(secureRegionRPC, void (uint64_t, uint64_t));
};


