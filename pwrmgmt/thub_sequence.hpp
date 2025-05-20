#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"
#include "pwrmgmt.hpp"
#include "transactor.h"
#include "svdpi.h"

DECLARE_uint32(num_harts);

namespace {
  constexpr uint32_t core_mcountinhibit         = 0x0000'0320;
  constexpr uint32_t core_mhpmevent10           = 0x0000'032A;
  constexpr uint32_t core_mhpmcounter10         = 0x0000'0B0A;
}

class thub_sequence {

  public:

    thub_sequence(cvm::topology::loc_t loc, unsigned id);
    ~thub_sequence();

  private:

    void set_scope(svScope s) { scope_ = s; }
    void main_thread();

    cvm::messenger::task<void> main();

    cvm::messenger::task<void> temp_throttle_configuration();
    cvm::messenger::task<void> temp_throttle_enable();
    cvm::messenger::task<void> temp_throttle_disable();

    cvm::messenger::task<void> tick();
    cvm::messenger::task<void> wait_for_ticks();
    cvm::messenger::task<uint64_t>  read(uint64_t addr, size_t sz, block_t block = BLOCK);
    cvm::messenger::task<void>      write(uint64_t addr, size_t sz, uint64_t data, block_t block = BLOCK);
    cvm::messenger::task<void>      csr_write(uint32_t core_id, uint32_t unit,uint64_t addr, uint64_t data);
    cvm::messenger::task<uint64_t>  csr_read(uint32_t core_id, uint32_t unit,uint64_t addr);

    std::vector<uint64_t> convert_to_dword_array(const std::vector<uint8_t>& byte_array);
    std::vector<uint8_t> convert_to_byte_array(const std::vector<uint64_t>& dword_array);
    void blocking_seq_tick(uint8_t val);

  private:

    svScope scope_;
    cvm::topology::loc_t loc_, smc_axi_loc_;
    uint32_t core_throttle;

};
