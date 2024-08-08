#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"
#include "trace.hpp"
#include "transactor.h"
#include "svdpi.h"

class ntrace_stop_on_wrap_sequence {

  public:

    ntrace_stop_on_wrap_sequence(cvm::topology::loc_t loc, unsigned id);
    ~ntrace_stop_on_wrap_sequence();

    void set_scope(svScope s) { scope_ = s; }

  private:

    void main_thread();

    cvm::messenger::task<void> main();
    cvm::messenger::task<void> core_no_fetch();

    cvm::messenger::task<void> enable_trace_funnel();
    cvm::messenger::task<void> disable_trace_funnel();
    cvm::messenger::task<void> deactivate_trace_funnel();
    cvm::messenger::task<void> poll_ntrace_ram_en();
    cvm::messenger::task<void> disable_ntrace_ram();
    cvm::messenger::task<void> read_ntrace_ram();
    cvm::messenger::task<void> enable_ntrace_ram();
    cvm::messenger::task<void> enable_ntrace();
    cvm::messenger::task<void> check_dst_trace_ram_en();
    cvm::messenger::task<void> poll_dst_trace_ram_en();
    cvm::messenger::task<void> disable_dst_trace_ram();
    cvm::messenger::task<void> read_dst_trace_ram();
    cvm::messenger::task<void> enable_dst_trace_ram();
    cvm::messenger::task<void> enable_dst_trace();
    cvm::messenger::task<void> disable_dst_trace();

    cvm::messenger::task<void> tick();
    cvm::messenger::task<uint64_t> read(uint64_t addr, size_t sz);
    cvm::messenger::task<void> write(uint64_t addr, size_t sz, uint64_t data, bool block = true);

    std::vector<uint64_t> convert_to_dword_array(const std::vector<uint8_t>& byte_array);
    std::vector<uint8_t> convert_to_byte_array(const std::vector<uint64_t>& dword_array);

    //void init();

  private:

    cvm::topology::loc_t loc_, axi_mst_loc_;
    svScope scope_;
};
