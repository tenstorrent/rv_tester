#pragma once
#include <iostream>
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"
#include "overlay_driver.hpp"
#include "transactor.h"
#include "svdpi.h"
#include "cvm/topology.hpp"
#include <string>
#include <queue>
#include <random>
#include "pcg_random.hpp"
#include "cvm/registry.hpp"
#include "transactor.h"
//#include "transactors/axi_sw/axi.h"
#include "sysmod/sysmod_plusargs.h"
#include "whisper_client.h"
#include "axi_sw_mst.h"

class scratchpad_random_sequence {

  public:

    scratchpad_random_sequence(cvm::topology::loc_t loc, unsigned id);
    ~scratchpad_random_sequence();

    void set_scope(svScope s) { scope_ = s; }

  private:

    void random_mode_thread();


    cvm::messenger::task<void> random_mode();


    cvm::messenger::task<void> tick();
    cvm::messenger::task<void> trigger();
    cvm::messenger::task<void> reset();
    cvm::messenger::task<void> axi_write_mmr_data_granular();
    cvm::messenger::task<void> axi_write_mmr_granular();
    cvm::messenger::task<void> axi_write_granular(uint64_t addr);
    cvm::messenger::task<void> axi_write_data_granular();
    cvm::messenger::task<void> sp_rand_traffic();
    cvm::messenger::task<void> sp_mmr_prog();
    cvm::messenger::task<void> axi_read_granular(const transactor::read_t& r );
    cvm::messenger::task<void> axi_read(uint64_t addr, size_t length, uint32_t id);


    using overlay_mst_t = axi_sw_mst<
        rv_tester_transactions::axi_sw_mst::b<>,
        rv_tester_transactions::axi_sw_mst::r<>,
        rv_tester_transactions::axi_sw_mst::ar_q_ptr<>,
        rv_tester_transactions::axi_sw_mst::aw_q_ptr<>,
        rv_tester_transactions::axi_sw_mst::w_q_ptr<>
    >;

    cvm::rand::uniform_dist<int64_t> rng;
    cvm::messenger::pool<axi::r_t>::channel_info channel;

    uint32_t cnt_tick=0;
    uint32_t start_scratchpad_cnt,read_ram;
    uint32_t rnd_traffic_cnt_tick=20;
    uint64_t sp_base = 0x60000000;
    uint64_t sp_addr = 0x60000000;
    uint64_t scratchpad_addr_in_flight;
    uint64_t sp_xtor_num_accesses=0;
    cvm::topology::loc_t axi_mst_loc_l;
            struct scratchpad_wr_t {
          uint32_t addr;
          uint32_t data;
        };
        struct scratchpad_xtor_read_t {
          uint64_t addr;
          size_t length;
          uint32_t id;
        };
        struct scratchpad_xtor_read_req_t {
          uint64_t addr;
          size_t length;
          uint32_t id;
          std::vector<uint8_t> data;
          std::vector<bool> strb;
        };
        std::queue<scratchpad_xtor_read_req_t> scratchpad_read_resp_q;
        std::queue<scratchpad_wr_t>           scratchpad_wr_txn_q;
  private:

    cvm::topology::loc_t loc_;
    cvm::topology::loc_t tick_loc_;
    unsigned id_;
    svScope scope_;

    uint32_t nmi_count_ = 0;
};
