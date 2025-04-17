#include "whisper_client.h"
#include "cosim/utils/general/util.h"
#include "whisper_decoder.h"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include <vector>
#include "cvm/plusargs.hpp"
//include this file only in whisper_client.cpp
DEFINE_uint32(iss_select_num_rand,      0, "Number of Random addresses"); // "num_rand" and other "num_rand_XX" should be mutually exclusive
DEFINE_uint32(iss_select_num_randpc,    0, "Number of Random PCs");
DEFINE_uint32(iss_select_num_randload,  0, "Number of Random Load addresses");
DEFINE_uint32(iss_select_num_randstore, 0, "Number of Random store addresses");
DEFINE_uint32(iss_select_test_stage_pct, 50, "Percentage of test run, after which we select randoms");
DEFINE_uint32(iss_select_test_range, 200, "Number of steps to get data from");
DEFINE_string(iss_select_bias_instr, "", "these instruction/opcodes will be picked with higher probability");
DEFINE_string(iss_select_priv_mode, "", "(comma separated)select only from these privilege modes: M/S/U/VS/VU (no HS)");
DEFINE_bool(randpc_phy,   false, "Random PCs selected are phys address");
DEFINE_bool(randldst_phy, false, "Random Ld/St addresses selected are phys address");
DEFINE_uint32(num_dm_randpc,    0, "Number of Random PCs to DM");
DEFINE_uint32(num_dm_randload,  0, "Number of Random loads to DM");
DEFINE_uint32(num_dm_randstore, 0, "Number of Random stores to DM");
DEFINE_uint64(dm_rand_addr, 0x9080500, "(Trickbox) Random address for DM: PC/Load/Store");;


template <typename URV>
int
whisperClient<URV>::processStandaloneInfo() {
  if (FLAGS_iss_select_num_randpc    == 0 &&
      FLAGS_iss_select_num_randload  == 0 &&
      FLAGS_iss_select_num_randstore == 0 &&
      FLAGS_iss_select_num_rand      == 0) {
    return 0;
  }
  cvm::log(cvm::HIGH, "Processing Standalone Information\n");
// TODO: MP support
    std::vector<std::string> bias_instrs = cosim_util::split_string(FLAGS_iss_select_bias_instr, ",");
    std::unordered_map<int, std::string> priv_map = { {0, "U"}, {1, "S"}, {3, "M"}, {4, "VU"}, {5, "VS"}}; // VS/VU is mode Ored with V<<2
    std::vector<std::string> priv_mode = cosim_util::split_string(FLAGS_iss_select_priv_mode,  ",");
    std::shared_ptr<WdRiscv::Session<URV>> session_iss;
    std::shared_ptr<WdRiscv::System<URV>> system_iss;
    WdRiscv::Args args_iss = WdRiscv::Args();
    constructSystem(session_iss, system_iss, args_iss, ncores_, true);
    auto total_instr = system_->ithHart(0).get()->getInstructionCount();
    auto num_instr = uint64_t((total_instr * FLAGS_iss_select_test_stage_pct) / 100);
    bool stop = false;
    auto hart_iss = system_iss->ithHart(0);
    hart_iss->runSteps(num_instr, stop, nullptr);

    unsigned icount = 0;
    std::vector<iss_select_s> rands;
    while (!hart_iss->hasTargetProgramFinished() && icount++<=FLAGS_iss_select_test_range) {

      hart_iss->singleStep();
      if (FLAGS_iss_select_priv_mode != "") {
        auto priv = hart_iss->lastPrivMode();
        auto virt = hart_iss->lastVirtMode();
        auto foo = int(priv) | (int(virt)<<2);
        if (std::find(priv_mode.begin(), priv_mode.end(), priv_map[foo]) == priv_mode.end())
          continue;
      }
      uint64_t virt_pc, phys_pc, curr_phys_pc, virt_addr, phys_addr, value;
      uint32_t inst, curr_inst;
      bool matches_bias_instrs=false, trapped=false; 
      std::string disasm, curr_disasm, instr, curr_instr;
      iss_select_s pc, ld_st, excp;

      virt_pc = hart_iss->lastPc();
      hart_iss->readInst(virt_pc, phys_pc, inst);
      disasm = whisper::disassemble(inst);
      instr = cosim_util::get_nth_word(disasm, 1);
      cvm::log(cvm::FULL, "PC: 0x{:x} Inst: 0x{:x} disasm:{} Step:{}\n", virt_pc, inst, instr, icount+num_instr);
      pc.init(PC, instr, virt_pc, phys_pc);
      AddrCategory categ = NONE;

      if (cosim_util::has_substring(bias_instrs, instr))
        matches_bias_instrs = true;
      if (hart_iss->lastInstructionTrapped()) {
        trapped = true;
        auto curr_pc = hart_iss->pc(); // Exception PC
        hart_iss->readInst(curr_pc, curr_phys_pc, curr_inst);
        curr_disasm = whisper::disassemble(inst);
        curr_instr  = cosim_util::get_nth_word(curr_disasm, 1);
        excp.init(PC, curr_instr, curr_pc, curr_phys_pc);
      }
      if (hart_iss->lastLdStAddress(virt_addr, phys_addr)) {
        categ = LOAD;
        if (hart_iss->lastStore(phys_addr, value))
          categ = STORE;
      }
      // Page Table entries
      for (unsigned i = 0; i<2; i++) { // assume always page crosses
        std::vector<uint64_t> ptes;
        hart_iss->getPageTableWalkEntries(true/*isinstr*/,i, ptes);
        for (auto &pte_pa: ptes) {
          iss_select_s pte(PC, instr, virt_pc, pte_pa);
          rands.push_back(pte);
        }
        hart_iss->getPageTableWalkEntries(false,i, ptes);
        for (auto &pte_pa: ptes) {
          iss_select_s pte(categ, instr, virt_pc, pte_pa);
          rands.push_back(pte);
        }
      }

      rands.push_back(pc);
      if (matches_bias_instrs) {
          rands.push_back(pc);
          rands.push_back(pc);
      }
      if (trapped) {
          rands.push_back(pc);
          rands.push_back(pc);
          rands.push_back(excp);
          rands.push_back(excp);
      }
      if (categ != NONE) { //ld/st
        rands.emplace_back(categ, instr, virt_addr, phys_pc);
      }
    }
    // store the number thats requested by randomly selecting from the list
    auto fill = [&](AddrCategory filter, uint32_t num) {
      if (!num) return;
      std::vector<iss_select_s> result;
      if (filter != NONE) {
        std::copy_if(rands.begin(), rands.end(), std::back_inserter(result),
                 [filter](const iss_select_s& s) { return s.categ == filter; });
      } else {
        result = rands;
      }
      cvm::rand::uniform_dist<int> rng1;
      while (num && result.size()) {
        int rand_idx = rng1() % result.size();
        rand_addrs_.push_back(result[rand_idx]);
        result.erase(std::remove(result.begin(), result.end(), result[rand_idx]), result.end());
        num--;
      }
    };
    fill(PC,    FLAGS_iss_select_num_randpc);
    fill(LOAD,  FLAGS_iss_select_num_randload);
    fill(STORE, FLAGS_iss_select_num_randstore);
    fill(NONE,  FLAGS_iss_select_num_rand);
  return 0;
}
