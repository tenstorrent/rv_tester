#pragma once
#include "algorithm"
#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "cvm/registry.hpp"
#include "cvm/callbacks.hpp"
#include "cvm/topology.hpp"
#include "rv_tester_transactions.hpp"

class sot {
  template<typename T, typename... Args> void connect(cvm::topology::loc_t loc) {
    cvm::registry::messenger.connect<T>(
      loc, [this] (const T& v) { return this->process(v); }
    );
    if constexpr (sizeof...(Args)) { connect<Args...>(loc); }
  }
  public:
  sot(cvm::topology::loc_t loc, unsigned) {
    loc_ = loc;
    init_label();
    if (execution_labels_.size() == 0) return;
    for (uint32_t i=0; i <num_harts_; i++) {
      actual_test_started_.push_back(0);
    }
  }
  sot(cvm::topology::loc_t loc): sot(loc, 1) {}
  ~sot() {}
  void configure() {
    if (execution_labels_.size() == 0) return;
    for (uint32_t i=0; i <num_harts_; i++) {
      connect<
        rv_tester_transactions::cosim::m_rvfi<>
      >(cvm::topology::get_from_type("COSIM", i));
    }
  }
  void init_label();
  private:
  cvm::topology::loc_t loc_;
  uint32_t already_started_=0;
  uint32_t num_harts_ = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;
  std::vector<std::uint64_t> execution_labels_;
  std::vector<uint32_t> actual_test_started_;

  void process(const rv_tester_transactions::cosim::m_rvfi<>& m_rvfi);
};
