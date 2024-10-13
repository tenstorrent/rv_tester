#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"
#include "pwrmgmt.hpp"
#include "transactor.h"
#include "svdpi.h"

class patch_control_sequence {

  public:

    patch_control_sequence(cvm::topology::loc_t loc, unsigned id);
    ~patch_control_sequence();

  private:

    void main_thread();

    cvm::messenger::task<void> main();
    cvm::messenger::task<void> core_no_fetch();

    cvm::messenger::task<void> pcontrol_write();

    cvm::messenger::task<void> tick();
    cvm::messenger::task<uint64_t> read(uint64_t addr, size_t sz, block_t block = BLOCK);
    cvm::messenger::task<void> write(uint64_t addr, size_t sz, uint64_t data, block_t block = BLOCK);

    std::vector<uint64_t> convert_to_dword_array(const std::vector<uint8_t>& byte_array);
    std::vector<uint8_t> convert_to_byte_array(const std::vector<uint64_t>& dword_array);

  private:

    cvm::topology::loc_t loc_, smc_axi_loc_;

    int pcontrol_read_count_;
    int pcontrol_write_count_;
};
