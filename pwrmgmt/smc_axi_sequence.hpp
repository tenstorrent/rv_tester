#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"
#include "pwrmgmt.hpp"
#include "transactor.h"
#include "svdpi.h"
#include "axi_sw_mst.h"

class smc_axi_sequence {

  public:

    smc_axi_sequence(cvm::topology::loc_t loc, unsigned id);
    ~smc_axi_sequence();

    using smc_axi_mst_t = axi_sw_mst<
        rv_tester_transactions::axi_sw_mst::b<1>,
        rv_tester_transactions::axi_sw_mst::r<1>,
        rv_tester_transactions::axi_sw_mst::ar_q_ptr<1>,
        rv_tester_transactions::axi_sw_mst::aw_q_ptr<1>,
        rv_tester_transactions::axi_sw_mst::w_q_ptr<1>
    >;

  private:

    void set_scope(svScope s) { scope_ = s; }

    void scratchpad_write_thread();
    void scratchpad_read_thread();
    void csr_access_thread();

    cvm::messenger::task<void> scratchpad_tick();
    cvm::messenger::task<void> csr_tick();

    cvm::messenger::task<void> scratchpad_write();
    cvm::messenger::task<void> scratchpad_read();
    cvm::messenger::task<void> csr_access();
    cvm::messenger::task<void> csr_write(uint32_t addr, size_t sz);
    cvm::messenger::task<uint64_t> csr_read(uint32_t addr, size_t sz);

    cvm::messenger::task<std::pair<uint32_t, uint64_t>> cpl_sram_write();
    cvm::messenger::task<void> smc_trns_read_check(smc_dest_path_t smc_dest_path, uint32_t addr, uint64_t exp_data, size_t sz, block_t block = BLOCK);
    
    cvm::messenger::task<uint64_t> read(unsigned& id, uint64_t addr, size_t sz, block_t block = BLOCK);
    cvm::messenger::task<void> write(unsigned& id, uint64_t addr, size_t sz, uint64_t data, block_t block = BLOCK);

    std::vector<uint64_t> convert_to_dword_array(const std::vector<uint8_t>& byte_array);
    std::vector<uint8_t> convert_to_byte_array(const std::vector<uint64_t>& dword_array);

    void blocking_seq_tick(uint8_t val);

  private:

    cvm::topology::loc_t loc_, smc_axi_loc_;
    svScope scope_;

    cvm::messenger::pool<axi::b_t>::channel_info b_channel_;
    cvm::messenger::pool<axi::r_t>::channel_info r_channel_;

    std::unordered_map<unsigned, uint32_t> sp_inflight_writes_;

    int smc_axi_read_count_;
    int smc_axi_write_count_;
};
