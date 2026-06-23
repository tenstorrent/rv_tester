#pragma once
#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "cvm/registry.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/topology.hpp"
#include "rv_tester_transactions.hpp"

#include "cosim/utils/util.h"
#include "htif/htif.h"

class eot {

  template <typename T, typename... Args>
  void connect(cvm::topology::loc_t loc) {
    cvm::registry::messenger.connect<T>(
        loc,
        [this](const T& v) {
          return this->process(v);
        });
    if constexpr (sizeof...(Args)) {
      connect<Args...>(loc);
    }
  }

public:
  // End-of-test (eot) options:
  // eot=tohost -- Look for mem store to 'tohost' address = success/fail

  void configure();
  eot(cvm::topology::loc_t loc, unsigned id = 1);
  ~eot();

  std::uint64_t get_tohost_addr();
  CVM_MESSENGER_procedure_call(get_tohost_addr_RPC, std::uint64_t());

  void init_tohost_addr();
  CVM_MESSENGER_procedure_call(init_tohost_addr_RPC, void ());

private:
  void process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi);
  void process(const rv_tester_transactions::cosim::m_steps<>& m_steps);
  void process(const rv_tester_transactions::cosim::m_eoti_normal<>& m_eoti_normal);
  void process(const rv_tester_transactions::cosim::offline_eoti<>& offline_eoti);
  void process_eoti(uint64_t hartid, uint64_t cycle, uint64_t instr_count, uint64_t data, int max_instr);
  void check_max_instr(uint64_t cycle, uint64_t count);
  void eot_terminate(bool passed);
  void mem_checks_snoop();
  bool mem_checks();

private:
  unsigned id_;
  cvm::topology::loc_t loc_;
  uint32_t previous_cycle_ = 0;
  uint32_t num_harts_ = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  std::vector<uint32_t> instr_count_;
  std::vector<uint64_t> terminated_harts_;
  std::uint64_t tohost_addr_ = -1;
  const std::uint8_t tohost_status_ = 1;
  const std::uint8_t tohost_device_syscall_ = 0;
  bool ended_ = false;
  std::chrono::time_point<std::chrono::system_clock> start, end;
  std::uint64_t recent_pc_instr_count_ = 0;
  int recent_pc_hart_ = -1;
  std::vector<std::pair<uint64_t, std::bitset<512>>> mem_lines_;
  bool eot_mem_checks_done_ = false;
};
