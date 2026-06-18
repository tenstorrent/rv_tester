#pragma once
#include <iostream>
#include <optional>
#include <unistd.h>
#include <sstream>
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"
#include "interrupts.hpp"
#include "transactor.h"
#include "axi_sw_mst.h"
#include "trickbox/interrupter.h"
#include "svdpi.h"

DECLARE_bool(enable_external_interrupt_sequence_debug);

using axi_mst_t = axi_sw_mst<
    rv_tester_transactions::axi_sw_mst::b<>,
    rv_tester_transactions::axi_sw_mst::r<>,
    rv_tester_transactions::axi_sw_mst::ar_q_ptr<>,
    rv_tester_transactions::axi_sw_mst::aw_q_ptr<>,
    rv_tester_transactions::axi_sw_mst::w_q_ptr<>>;

class external_interrupt_sequence {

public:
  external_interrupt_sequence(cvm::topology::loc_t loc, unsigned id);
  ~external_interrupt_sequence();

  void configure();

private:
  void interrupt_injection_thread();

  cvm::messenger::task<void> interrupt_trigger();

  cvm::messenger::task<void> delayed_trigger();

  void init();
  void capture_trigger_info(int32_t a, int32_t b);

  void on_sysmod_tick(uint64_t advance);

  void on_directed_msi(const directed_msi_request_t& req);

  void send_msi(uint64_t intr_num, unsigned intr_file, unsigned intr_hart,
                unsigned vs_id, bool disable_vs_id_rand, bool exp_err_rsp);

  bool check_axi_backpressure();

  void gen_interrupt_timings();
  uint32_t get_logical_core_id(uint32_t physical_hart_id);

  cvm::file_logger sequence_log_file_;
  template <typename... Args>
  inline void log(cvm::verbosity_level level, const std::string& fmt_string, Args&&... args) {
    if (FLAGS_enable_external_interrupt_sequence_debug)
      sequence_log_file_(cvm::NONE, fmt_string, std::forward<Args>(args)...);
    cvm::log(level, fmt_string, std::forward<Args>(args)...);
  }

private:
  cvm::topology::loc_t loc_;
  cvm::topology::loc_t sysmod_loc_;
  cvm::topology::loc_t axi_mst_loc_l;
  cvm::topology::loc_t triggers_loc;
  unsigned id_;

  cvm::rand::uniform_dist<uint32_t> rng1;
  uint32_t trigger_interrupt_count_;

  uint32_t ext_interrupt_count_ = 0;
  uint32_t ext_trig_interrupt_count_ = 0;
  uint32_t msi_m_file_addr;
  uint32_t msi_s_file_addr;
  uint32_t msi_vs_file_addr;
  int32_t last_trigger = 0;
  int32_t current_trigger = 0;
  bool drive_msi_in_curr_hart;

  uint32_t intr_vs_id_vgein_ = 0;
  uint32_t intr_vs_id_random_ = 0;
  uint32_t intr_vs_id_two_ = 0;
  uint32_t ncores_;

  uint64_t timer_ = 0;
  uint64_t timer_advance_ = 200;
  uint64_t timer_rand_intr_ = 500;
  int intr_count_ = 0;
  int limit_interrupts_ = 0;
  bool intr_enable_flag_cached_ = false;

  uint32_t msi_wait_timeout_ = 0;
};
