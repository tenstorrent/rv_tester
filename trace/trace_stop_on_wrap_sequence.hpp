#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"
#include "trace.hpp"
#include "transactor.h"
#include "svdpi.h"

class trace_stop_on_wrap_sequence {

  public:

    trace_stop_on_wrap_sequence(cvm::topology::loc_t loc, unsigned id);
    ~trace_stop_on_wrap_sequence();

    void set_scope(svScope s) { scope_ = s; }

  private:

    void ntrace_main_thread();
    void dst_main_thread();

    cvm::messenger::task<void> tick();
    cvm::messenger::task<void> core_no_fetch();
    cvm::messenger::task<void> detect_core_hang();

    cvm::messenger::task<void> ntrace_main();
    cvm::messenger::task<void> dst_main();
    
    cvm::messenger::task<void> poll_ntrace_ram_en(trace_ram_status_t trace_ram_status = ENABLE);
    cvm::messenger::task<void> disable_ntrace_encoder();
    cvm::messenger::task<void> disable_ntrace_ram();
    cvm::messenger::task<void> reset_ntrace_ram();
    cvm::messenger::task<void> read_ntrace_ram();
    cvm::messenger::task<void> enable_ntrace_ram();
    cvm::messenger::task<void> enable_ntrace_encoder();
    
    cvm::messenger::task<void> check_dst_trace_ram_en();
    cvm::messenger::task<void> poll_dst_trace_ram_en();
    cvm::messenger::task<void> disable_dst_trace();
    cvm::messenger::task<void> disable_dst_trace_ram();
    cvm::messenger::task<void> read_dst_trace_ram();
    cvm::messenger::task<void> enable_dst_trace_ram();
    cvm::messenger::task<void> enable_dst_trace();
    
    cvm::messenger::task<void> enable_trace_funnel();
    cvm::messenger::task<void> disable_trace_funnel();
    cvm::messenger::task<void> reset_trace_funnel();
        
    cvm::messenger::task<uint64_t> read(uint64_t addr, size_t sz, block_t block = BLOCK);
    cvm::messenger::task<void> write(uint64_t addr, size_t sz, uint64_t data, block_t block = BLOCK);

    uint64_t convert_to_dword_array(const std::vector<uint8_t>& byte_array, uint8_t shift, size_t sz);
    std::vector<uint8_t> convert_to_byte_array(uint64_t data, uint8_t shift);

    void terminate_test(uint8_t terminate_test);
    void warm_reset_request(uint8_t warm_reset_req);

  private:

    cvm::topology::loc_t loc_, axi_mst_loc_;
    svScope scope_;
};
