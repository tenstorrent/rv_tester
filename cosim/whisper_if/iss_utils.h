#include "whisper_client.h"
#include "cosim/utils/general/util.h"
#include "whisper_decoder.h"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include <vector>
#include "cvm/plusargs.hpp"
#include "sysmod/sysmod_plusargs.h"
//include this file only in whisper_client.cpp
DEFINE_uint32(iss_select_num_rand,      0, "Number of Random addresses"); // "num_rand" and other "num_rand_XX" should be mutually exclusive
DEFINE_uint32(iss_select_num_randpc,    0, "Number of Random PCs");
DEFINE_uint32(iss_select_num_randload,  0, "Number of Random Load addresses");
DEFINE_uint32(iss_select_num_randstore, 0, "Number of Random store addresses");
DEFINE_uint32(iss_select_test_stage_pct, 50, "Percentage of test run, after which we select randoms");
DEFINE_uint32(iss_select_test_range, 200, "Number of steps to get data from");
DEFINE_string(iss_select_bias_instr, "", "these instruction/opcodes will be picked with higher probability");
DEFINE_string(iss_select_ignore, "", "types such as trap/pte will not be picked");
DEFINE_string(iss_select_priv_mode, "", "(comma separated)select only from these privilege modes: M/S/U/VS/VU (no HS)");

template <typename URV>
void
whisperClient<URV>::collectRands(HartInfo& h) {

  auto hart = h.hart;
  // do free run for num_free_steps
  if (h.steps < h.num_free_steps) {
    unsigned steps;
    if (FLAGS_num_harts == 1)
      steps = h.num_free_steps - h.steps;
    else
      steps = FLAGS_whisper_deterministic; 
    bool stop;
    hart->runSteps(steps, stop, nullptr);
  }

  // start collecting data
  uint32_t deterministic = 0;
  uint32_t max_steps = (ncores_>1) ? FLAGS_whisper_deterministic : 10000;
  while ((deterministic++ < max_steps) && !h.stopped())
  {
    uint64_t virt_pc, curr_pc, phys_pc, curr_phys_pc, virt_addr, phys_addr, value;
    uint32_t inst, curr_inst;
    bool is_special=false, is_trap=false, is_branch_taken=false; 
    AddrType type = NONE;
    std::string disasm, curr_disasm, instr, curr_instr;
    cvm::rand::uniform_dist<int> rng1;
    std::vector<iss_select_s> iss_ptes_pc, iss_ptes_ld, iss_ptes_st;

    std::vector<std::string> bias_instrs = cosim_util::split_string(FLAGS_iss_select_bias_instr, ",");
    std::unordered_map<int, std::string> priv_map = { {0, "U"}, {1, "S"}, {3, "M"}, {4, "VU"}, {5, "VS"}}; // VS/VU is mode Ored with V<<2
    std::vector<std::string> priv_mode = cosim_util::split_string(FLAGS_iss_select_priv_mode,  ",");

    hart->singleStep();
    h.steps++;
    h.remaining_steps--;

    if (FLAGS_iss_select_priv_mode != "") {
      auto priv = hart->lastPrivMode();
      auto virt = hart->lastVirtMode();
      auto foo = int(priv) | (int(virt)<<2);
      if (std::find(priv_mode.begin(), priv_mode.end(), priv_map[foo]) == priv_mode.end())
        continue;
    }

    virt_pc = hart->lastPc();
    hart->readInst(virt_pc, phys_pc, inst);
    disasm = whisper::disassemble(inst);
    instr = cosim_util::get_nth_word(disasm, 1);
    cvm::log(cvm::FULL, "Hart:{} PC: 0x{:x} Inst: 0x{:x} disasm:{} Step:{} \n", h.id, virt_pc, inst, instr, h.steps);

    if (cosim_util::has_substring(bias_instrs, instr))
      is_special = true;
    if (hart->lastBranchTaken())
      is_branch_taken = true;
    if (hart->lastLdStAddress(virt_addr, phys_addr)) {
      type = LOAD;
      if (hart->lastStore(phys_addr, value))
        type = STORE;
    }

    // handle traps
    if (hart->lastInstructionTrapped()) {
      is_trap = true;
      auto curr_pc = hart->pc();
      hart->readInst(curr_pc, curr_phys_pc, curr_inst);
      curr_disasm = whisper::disassemble(inst);
      curr_instr  = cosim_util::get_nth_word(curr_disasm, 1);
    }
    if (h.in_trap && !is_trap) {
      if (cosim_util::has_substring(cosim_util::split_string(curr_instr, ","), "ret")) // FIXME
        h.in_trap = false;
    } else if (is_trap) {
      h.in_trap = true;
    }
    if (h.in_trap && cosim_util::has_substring(cosim_util::split_string(FLAGS_iss_select_ignore, ","), "trap"))
      continue;

    // Page Table entries
    if ((cosim_util::has_substring(bias_instrs, "pte")) ||
        (!cosim_util::has_substring(cosim_util::split_string(FLAGS_iss_select_ignore, ","), "pte"))
       ) {
      for (int i = 0; i<2; i++) { //  page crossing
        std::vector<uint64_t> ptes;
        hart->getPageTableWalkEntries(true/*isinstr*/, i, ptes);
        for (auto &pte_pa: ptes)
          iss_ptes_pc.emplace_back(PC, instr, virt_pc, pte_pa);
        hart->getPageTableWalkEntries(false, i, ptes);
        for (auto &pte_pa: ptes) {
          if (type == LOAD)
            iss_ptes_ld.emplace_back(type, instr, virt_pc, pte_pa);
          else 
            iss_ptes_st.emplace_back(type, instr, virt_pc, pte_pa);
        }
      }
    }

    // add PCs to randoms
    if (is_special || is_trap || is_branch_taken || (rng1() % 100 < 30)) {
      h.pc.emplace_back(PC, instr, virt_pc, phys_pc);
      if (is_trap)
        h.pc.emplace_back(PC, curr_instr, curr_pc, curr_phys_pc);
      for (auto &iss_pte: iss_ptes_pc)
        if (rng1() % 100 < 25)
          h.pc.emplace_back(PC, iss_pte.disasm, iss_pte.virt_addr, iss_pte.phys_addr);
    }

    if (type == LOAD) {
      h.ld.emplace_back(type, instr, virt_addr, phys_addr);
      for (auto &iss_pte: iss_ptes_ld)
        if (rng1() % 100 < 25)
          h.ld.emplace_back(LOAD, iss_pte.disasm, iss_pte.virt_addr, iss_pte.phys_addr);
    } else if (type == STORE) {
      h.st.emplace_back(type, instr, virt_addr, phys_addr);
      for (auto &iss_pte: iss_ptes_st)
        if (rng1() % 100 < 25)
          h.st.emplace_back(STORE, iss_pte.disasm, iss_pte.virt_addr, iss_pte.phys_addr);
    }
    h.uniqify();
   if ( (h.ld.size() >= FLAGS_iss_select_num_randload &&
          h.st.size() >= FLAGS_iss_select_num_randstore &&
          h.pc.size() >= FLAGS_iss_select_num_randpc) ||
        (FLAGS_iss_select_num_rand > 0 &&
         h.ld.size() + h.st.size() + h.pc.size() >= FLAGS_iss_select_num_rand)
      )
     h.done = true;
  }
}

template <typename URV>
int
whisperClient<URV>::processStandaloneInfo() {

  if (FLAGS_iss_select_num_randpc    == 0 &&
      FLAGS_iss_select_num_randload  == 0 &&
      FLAGS_iss_select_num_randstore == 0 &&
      FLAGS_iss_select_num_rand      == 0)
    return 0;

  cvm::log(cvm::HIGH, "Processing Standalone Information\n");
  std::vector<std::string> bias_instrs = cosim_util::split_string(FLAGS_iss_select_bias_instr, ",");
  std::unordered_map<int, std::string> priv_map = { {0, "U"}, {1, "S"}, {3, "M"}, {4, "VU"}, {5, "VS"}}; // VS/VU is Mode ORed with V<<2
  std::vector<std::string> priv_mode = cosim_util::split_string(FLAGS_iss_select_priv_mode,  ",");
  std::shared_ptr<WdRiscv::Session<URV>> session_iss;
  std::shared_ptr<WdRiscv::System<URV>>  system_iss;
  WdRiscv::Args args_iss = WdRiscv::Args();
  bool ok = constructSystem(session_iss, system_iss, args_iss, FLAGS_num_harts, true);
  if (!ok) {
    cvm::log(cvm::ERROR, "Error: Couldn't construct system for Standalone processing\n");
    return 1;
  }

  for (unsigned i=0; i<FLAGS_num_harts; i++) {
    HartInfo h;
    h.id = i; h.hart = system_iss->ithHart(i).get();
    h.total_instrs = system_->ithHart(i).get()->getInstructionCount();
    h.num_free_steps = uint64_t((h.total_instrs * FLAGS_iss_select_test_stage_pct) / 100);
    rand_addrs_map_[i] = h;
  }

  unsigned harts_done = 0;
  while (harts_done < (FLAGS_eot == "tohost_all"? FLAGS_num_harts: 1)) {
    harts_done = 0;
    for (auto& [i, h]: rand_addrs_map_) {
      cvm::log(cvm::FULL, "Collecting ISS data for Hart:{}\n", i);
      collectRands(h);
      harts_done += h.done;
    }
  }

  auto fill = [&](AddrType filter, uint32_t num, HartInfo& h) {
    if (!num) return;
    std::vector<iss_select_s> rands;
    if (filter == PC) {
      rands = h.pc;
    } else if (filter == LOAD) {
      rands = h.ld;
    } else if (filter == STORE) {
      rands = h.st;
    } else if (filter == NONE) {
      rands.insert(rands.end(), h.pc.begin(), h.pc.end());
      rands.insert(rands.end(), h.ld.begin(), h.ld.end());
      rands.insert(rands.end(), h.st.begin(), h.st.end());
    }
    cvm::rand::uniform_dist<int> rng1;
    while (num && rands.size()) {
      int rand_idx = rng1() % rands.size();
      h.rand_addrs.push_back(rands[rand_idx]);
      cvm::log(cvm::MEDIUM, "[iss_select] Type: {}, IsFetch: {} VA: {:#x} PA: {:#x} Disasm:{}\n", rands[rand_idx].type, rands[rand_idx].is_fetch, rands[rand_idx].virt_addr, rands[rand_idx].phys_addr, rands[rand_idx].disasm);
      rands.erase(std::remove(rands.begin(), rands.end(), rands[rand_idx]), rands.end());
      num--;
    }
  };

  for (auto& [i, h]: rand_addrs_map_) {
    fill(PC,    FLAGS_iss_select_num_randpc,    h);
    fill(LOAD,  FLAGS_iss_select_num_randload,  h);
    fill(STORE, FLAGS_iss_select_num_randstore, h);
    fill(NONE,  FLAGS_iss_select_num_rand,      h);
    cvm::log(cvm::MEDIUM, "Total ISS SELECTS: {} hart:{}\n", h.rand_addrs.size(), i);
    h.clear();
  }
  return 0;
}
