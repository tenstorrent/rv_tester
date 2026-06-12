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
    enum AddrType {
      PC    = 0,
      LOAD  = 1,
      STORE = 2,
      NONE  = 3
    };
    struct iss_select_s {
      AddrType type;
      std::string disasm;
      uint64_t virt_addr;
      uint64_t phys_addr;
      bool is_fetch;
      iss_select_s() {}
      iss_select_s(AddrType c, const std::string d, uint64_t v, uint64_t p) { init(c, d, v, p);}
      void init(AddrType c, const std::string d, uint64_t v, uint64_t p) { type=c; disasm=d; virt_addr=v; phys_addr=p;   }
      bool operator==(const iss_select_s& other) const { return virt_addr == other.virt_addr && phys_addr == other.phys_addr; }
      bool operator< (const iss_select_s& other) const { return phys_addr < other.phys_addr; }
    };
    struct HartInfo {
       WdRiscv::Hart<URV>* hart;
       unsigned id;
       uint64_t total_instrs;
       int steps = 0;
       int num_free_steps = 0;
       int remaining_steps = 5000;
       bool done = false;
       bool in_trap = false;
       std::vector<iss_select_s> ld;
       std::vector<iss_select_s> st;
       std::vector<iss_select_s> pc;
       std::vector<iss_select_s> rand_addrs; // final rand addrs
       bool stopped() {
         if (!done && (hart->hasTargetProgramFinished() || remaining_steps <= 0))
           done = true;
         return done;
       }
       void clear() { ld.clear(); st.clear(); pc.clear(); }
       void uniqify() {
         std::sort(ld.begin(), ld.end());
         std::sort(st.begin(), st.end());
         std::sort(pc.begin(), pc.end());
         auto last = std::unique(ld.begin(), ld.end());
         ld.erase(last, ld.end());
         last = std::unique(st.begin(), st.end());
         st.erase(last, st.end());
         last = std::unique(pc.begin(), pc.end());
         pc.erase(last, pc.end());
       }
    };

    whisperClient(cvm::topology::loc_t loc, unsigned);

    void configure();

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
    std::vector<iss_select_s> get_iss_select(uint32_t hart=0)  { return rand_addrs_map_[hart].rand_addrs; }

    static bool constructSystem(std::shared_ptr<WdRiscv::Session<URV>>&, std::shared_ptr<WdRiscv::System<URV>>&, WdRiscv::Args&, uint16_t ncores, bool standalone, std::string logfile="");
    static std::string overrideWhisperJson(bool standalone);
    int whisperConnect();
    bool whisperConnected();
    int whisperStandalone();
    int processStandaloneInfo();
    void collectRands(HartInfo&);
    bool whisperStep(int hart, uint64_t time, uint64_t instrTag, uint64_t& pc, uint32_t& instruction, unsigned& changeCount, std::string& disasm, uint32_t& privMode, uint32_t& fpFlags, bool& hasTrap, bool& hasStop, bool& isLoad, bool& isCancelled, bool& valid);
    bool whisperSimpleStep(int hart, uint64_t& pc, uint32_t& instruction, unsigned& changeCount);
    bool whisperChange(int hart, uint32_t& resource, uint64_t& addr, uint64_t& value, bool& valid);
    bool whisperMcmRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, unsigned elemIx, unsigned field, bool cache, bool& valid);
    bool whisperMcmVecRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, std::vector<uint64_t> value, unsigned elemIx, unsigned field, bool cache, bool& valid);
    bool whisperMcmVecInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, std::vector<uint64_t> value, unsigned elemIx, unsigned field, bool& valid);
    bool whisperMcmInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, unsigned elemIx, unsigned field, bool& valid);
    bool whisperMcmVecBypass(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, std::vector<uint64_t> value, unsigned elemIx, unsigned field, bool cache, bool& valid);
    bool whisperMcmBypass(int hart, uint64_t time, uint64_t instrTag, uint64_t addr, unsigned size, uint64_t value, unsigned elemIx, unsigned field, bool cache, bool& valid);
    bool whisperMcmWrite(int hart, uint64_t time, uint64_t addr, unsigned size, svOpenArrayHandle handle, uint64_t mask, bool error, bool& valid);
    bool whisperMcmIFetch(int hart, uint64_t time, uint64_t addr, bool& valid);
    bool whisperMcmIEvict(int hart, uint64_t time, uint64_t addr, bool& valid);
    bool whisperMcmDEvict(int hart, uint64_t time, uint64_t addr, bool& valid);
    bool whisperMcmDWriteback(int hart, uint64_t time, uint64_t addr, bool& valid);
    bool whisperMcmDFetch(int hart, uint64_t time, uint64_t addr, bool& valid);
    bool whisperMcmEnd(int hart, uint64_t time, bool& valid);
    bool whisperInjectException(int hart, bool isLoad, uint64_t code, unsigned elemIx, uint64_t addr, bool& valid);
    bool whisperPoke(int hart, uint64_t time, char resource, uint64_t addr, uint64_t value, bool cache, bool skipmem, bool& valid);
    bool whisperPokeMem(int hart, uint64_t time, char resource, uint64_t addr, unsigned size, uint64_t value, bool cache, bool skipmem, bool& valid);
    bool whisperPokeMemBatch(int hart, uint64_t time, char resource, uint64_t addr, const std::vector<uint8_t> &data, bool &valid);
    bool whisperPeek(int hart, char resource, uint64_t addr, uint64_t& value, bool& valid);
    bool whisperPeek(int hart, char resource, uint64_t addr, uint64_t& value, uint64_t& reply_addr, bool& valid);
    bool whisperPeekPc(int hart, uint64_t& value);
    bool whisperPeekCsr(int hart, uint64_t addr, uint64_t& value, uint64_t& mask, uint64_t& reset_value, uint64_t& read_mask, bool& valid);
    bool whisperReset(int hart, uint64_t addr, bool& valid);
    bool whisperQuit();
    bool whisperPageTableWalk(int hart, bool isInstr, bool isAddr, svOpenArrayHandle items, unsigned& itemCount, bool& valid);
    bool whisperTranslate(int hart, uint64_t vaddr, bool r, bool w, bool x, bool twoStage, bool supervisor, uint64_t& paddr, bool& valid);
    bool whisperEnterDebug(int hart);
    bool whisperExitDebug(int hart);
    bool whisperCheckInterrupt(int hart, bool& interrupt, uint64_t& cause, bool& virt_mode);
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
    bool whisperClearNmiCause(int hart, uint64_t time, uint64_t cause);

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
    std::unordered_map<uint32_t, HartInfo> rand_addrs_map_;
    uint64_t secure_region_start_=0, secure_region_end_=0;
    std::vector<uint64_t> dummy;

  public:
    CVM_MESSENGER_procedure_call(iss_select_rand_RPC, std::vector<iss_select_s> (uint32_t));
    CVM_MESSENGER_procedure_call(whisperConnectRPC, int (void));
    CVM_MESSENGER_procedure_call(whisperConnectedRPC, bool (void));
    CVM_MESSENGER_procedure_call(whisperStepRPC, bool(int, uint64_t, uint64_t, uint64_t&, uint32_t&, unsigned&, std::string&, uint32_t&, uint32_t&, bool&, bool&, bool&, bool&, bool&));
    CVM_MESSENGER_procedure_call(whisperSimpleStepRPC, bool (int, uint64_t&, uint32_t&, unsigned&));
    CVM_MESSENGER_procedure_call(whisperChangeRPC, bool (int, uint32_t&, uint64_t&, uint64_t&, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmReadRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, uint64_t, unsigned, unsigned, bool, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmVecReadRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, std::vector<uint64_t>, unsigned, unsigned, bool, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmVecInsertRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, std::vector<uint64_t>, unsigned, unsigned, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmInsertRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, uint64_t, unsigned, unsigned, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmVecBypassRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, std::vector<uint64_t>, unsigned, unsigned, bool, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmBypassRPC, bool (int, uint64_t, uint64_t, uint64_t, unsigned, uint64_t, unsigned, unsigned, bool, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmWriteRPC, bool (int, uint64_t, uint64_t, unsigned, svOpenArrayHandle, uint64_t, bool, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmIFetchRPC, bool (int, uint64_t, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmIEvictRPC, bool (int, uint64_t, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmDEvictRPC, bool (int, uint64_t, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmDWritebackRPC, bool (int, uint64_t, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmDFetchRPC, bool (int, uint64_t, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperMcmEndRPC, bool (int, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperInjectExceptionRPC, bool (int, bool, uint64_t, unsigned, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperPokeRPC, bool (int, uint64_t, char, uint64_t, uint64_t, bool, bool, bool&));
    CVM_MESSENGER_procedure_call(whisperPokeMemRPC, bool (int, uint64_t, char, uint64_t, unsigned, uint64_t, bool, bool, bool&));
    CVM_MESSENGER_procedure_call(whisperPokeMemBatchRPC, bool(int, uint64_t, char, uint64_t, const std::vector<uint8_t> &, bool &));
    CVM_MESSENGER_procedure_call(whisperPeekRPC, bool (int, char, uint64_t, uint64_t&, bool&));
    CVM_MESSENGER_procedure_call(whisperPeekExtendedRPC, bool (int, char, uint64_t, uint64_t&, uint64_t&, bool&));
    CVM_MESSENGER_procedure_call(whisperPeekPcRPC, bool (int, uint64_t& value));
    CVM_MESSENGER_procedure_call(whisperPeekCsrRPC, bool (int, uint64_t, uint64_t&, uint64_t&, uint64_t&, uint64_t&, bool&));
    CVM_MESSENGER_procedure_call(whisperResetRPC, bool (int, uint64_t, bool&));
    CVM_MESSENGER_procedure_call(whisperQuitRPC, bool (void));
    CVM_MESSENGER_procedure_call(whisperPageTableWalkRPC, bool (int, bool, bool, svOpenArrayHandle, unsigned&, bool&));
    CVM_MESSENGER_procedure_call(whisperTranslateRPC, bool (int, uint64_t, bool, bool, bool, bool, bool, uint64_t&, bool&));
    CVM_MESSENGER_procedure_call(whisperEnterDebugRPC, bool (int));
    CVM_MESSENGER_procedure_call(whisperExitDebugRPC, bool (int));
    CVM_MESSENGER_procedure_call(whisperCheckInterruptRPC, bool (int, bool&, uint64_t&, bool&));
    CVM_MESSENGER_procedure_call(whisperGetSeiPinRPC, bool (int, uint64_t&));
    CVM_MESSENGER_procedure_call(whisperCancelLrRPC, bool (int, bool&));
    CVM_MESSENGER_procedure_call(whisperPeekGprRPC, bool (int, uint64_t, uint64_t&));
    CVM_MESSENGER_procedure_call(whisperPeekFprRPC, bool (int, uint64_t, uint64_t&));
    CVM_MESSENGER_procedure_call(whisperPeekVprRPC, bool (int, uint64_t, std::array<std::uint8_t, 32>&));
    CVM_MESSENGER_procedure_call(whisperGetLastLdStAddressRPC, bool (int, uint64_t&));
    CVM_MESSENGER_procedure_call(whisperNmiRPC, bool (int, uint64_t, uint64_t));
    CVM_MESSENGER_procedure_call(whisperClearNmiRPC, bool (int, uint64_t));
    CVM_MESSENGER_procedure_call(whisperClearNmiCauseRPC, bool (int, uint64_t, uint64_t));
    CVM_MESSENGER_procedure_call(whisperMcmSkipReadDataCheckRPC, bool (uint64_t, unsigned, bool));
    CVM_MESSENGER_procedure_call(secureRegionRPC, void (uint64_t, uint64_t));
};


