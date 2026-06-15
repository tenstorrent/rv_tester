#include "sot.h"
#include "svdpi.h"
#include <chrono>
#include <iostream>
#include "common/memmap.h"
#include "cosim/utils/util.h"
#include "rv_tester_structs.h"
#include "rv_tester_plusargs.h"
#include "sysmod_plusargs.h"

REGISTRY_register(sot, TOP.PLATFORM, cvm::registry::all);

void sot::init_label() {
  if (FLAGS_test_start_label == "")
    return;
  std::vector<std::string> labels = cosim_util::split_string(FLAGS_test_start_label, ",");
  bool found = false;
  for (auto& label : labels) {
    std::string cmd = "nm " + FLAGS_load + " | grep " + label;
    std::string result = cosim_util::exec(cmd.c_str());
    auto results = cosim_util::split_string(result, "\n");
    for (auto& res : results) {
      try {
        std::string addr_str = res.substr(0, 16);
        auto addr = std::stoul(addr_str, nullptr, 16);
        execution_labels_.push_back(addr);
        found = true;
        cvm::log(cvm::MEDIUM, "[SOT] Test start Label={} PC={:#x}\n", label, addr);
      } catch (...) {
          continue;
      }
    }
  }
  if (!found) {
    cvm::log(cvm::ERROR, "Error: test_start_label={} not found in elf\n", FLAGS_test_start_label);
  }
}

void sot::process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi) {
  if (already_started_ || (execution_labels_.size() == 0))
    return;

  if (std::find(execution_labels_.begin(), execution_labels_.end(), m_rvfi.pc_rdata) != execution_labels_.end()) {
    actual_test_started_[m_rvfi.hart] = 1;
    cvm::log(cvm::MEDIUM, "[SOT] Found test start PC: {:#x}\n", m_rvfi.pc_rdata);
  }

  std::vector<int> result;
  std::copy_if(actual_test_started_.begin(), actual_test_started_.end(), std::back_inserter(result), [](int x) { return x == 1; });
  if (result.size() == num_harts_) {
    already_started_ = 1;
    cvm::log(cvm::HIGH, "[SOT] Actual test started for all harts\n");
    cvm::registry::messenger.signal<rv_tester::test_started>(loc_, rv_tester::test_started{});
  }
}

