#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"
#include "pwrmgmt.hpp"
#include "transactor.h"
#include "svdpi.h"

class smc_axi_sequence {

  public:

    smc_axi_sequence(cvm::topology::loc_t loc, unsigned id);
    ~smc_axi_sequence();

  private:

    void set_scope(svScope s) { scope_ = s; }
    void main_thread();

    cvm::messenger::task<void> main();
    cvm::messenger::task<void> core_no_fetch();

    cvm::messenger::task<void> tick();

    cvm::messenger::task<uint32_t> scratchpad_write();

    cvm::messenger::task<uint32_t> csr_write();
    cvm::messenger::task<uint64_t> csr_read(uint32_t addr, size_t sz);

    cvm::messenger::task<void> smc_trns_read_check(smc_dest_path_t smc_dest_path, uint32_t addr, uint64_t exp_data, size_t sz, block_t block = BLOCK);

    cvm::messenger::task<std::pair<uint32_t, uint64_t>> cpl_sram_write();
    
    cvm::messenger::task<uint64_t> read(uint64_t addr, size_t sz, block_t block = BLOCK);
    cvm::messenger::task<void> write(uint64_t addr, size_t sz, uint64_t data, block_t block = BLOCK);

    std::vector<uint64_t> convert_to_dword_array(const std::vector<uint8_t>& byte_array);
    std::vector<uint8_t> convert_to_byte_array(const std::vector<uint64_t>& dword_array);

    void blocking_seq_tick(uint8_t val);

  private:

    cvm::topology::loc_t loc_, smc_axi_loc_;
    svScope scope_;

    int smc_axi_read_count_;
    int smc_axi_write_count_;
};
