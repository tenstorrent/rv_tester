#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "rv_tester_transactions.hpp"
#include "pwrmgmt.hpp"
#include "transactor.h"
#include "svdpi.h"

class reset_sequence {

  public:

    reset_sequence(cvm::topology::loc_t loc, unsigned id);
    ~reset_sequence() {}

    void check();

    void set_scope(svScope s) { scope_ = s; }

  private:

    void cold_reset_sequence_thread();
    void warm_reset_random_mode_sequence_thread();
    void warm_reset_trigger_mode_sequence_thread();

    cvm::messenger::task<void> cold_reset_sequence();
    cvm::messenger::task<void> warm_reset_random_mode_sequence();
    cvm::messenger::task<void> warm_reset_trigger_mode_sequence();
    cvm::messenger::task<void> warm_reset_sequence();
    void warm_reset_init_rand();

    cvm::messenger::task<void> tick();
    cvm::messenger::task<void> force_ref_clk();
    cvm::messenger::task<void> trigger();
    cvm::messenger::task<void> cpl_reset_sequence(rst_t );
    cvm::messenger::task<void> check_pll_status();
    cvm::messenger::task<void> clear_pll_status();
    cvm::messenger::task<void> release_cpl_reset();
    cvm::messenger::task<void> program_fuses();
    cvm::messenger::task<void> release_cpl_nofetch();

    cvm::messenger::task<uint64_t> read(uint64_t addr, size_t sz);
    cvm::messenger::task<void> write(uint64_t addr, size_t sz, uint64_t data);

    std::vector<uint64_t> convert_to_dword_array(const std::vector<uint8_t>& byte_array);
    std::vector<uint8_t> convert_to_byte_array(const std::vector<uint64_t>& dword_array);

    uint64_t fuse_val();
    uint64_t core_fuse_val();
    uint64_t trace_fuse_val();
    uint64_t dm_fuse_val();
    uint64_t sc_fuse_val();
    uint64_t core_en(uint32_t c);
    std::vector<uint64_t> mhartid();

    void init();
    void cold_reset(uint8_t assert);
    void warm_reset(uint8_t assert);
    void reset_hold(uint8_t sram, uint8_t debug, uint8_t critical);
    void force_ref_clk(uint8_t assert);

  private:

    cvm::topology::loc_t loc_, smc_axi_loc_;
    svScope scope_;

    uint32_t warm_reset_count_ = 0;
};
