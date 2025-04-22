#pragma once
#include <iostream>
#include <unistd.h>
#include <sstream>
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"
#include "snoop_gen.hpp"
#include "transactor.h"
#include "svdpi.h"
#include "sysmod/device.h"
#include "transactor.h"
//#include "transactors/axi_sw/axi.h"
#include "axi_sw_mst.h"

DECLARE_uint32(axi_resp_timeout); // Cycles to wait after Transactor-id pool overflow condition before raising no free ids error

class snoop_gen_sequence {

  public:

    snoop_gen_sequence(cvm::topology::loc_t loc, unsigned id);
    ~snoop_gen_sequence();

    void set_scope(svScope s) { scope_ = s; }
    

    using overlay_mst_t = axi_sw_mst<
        rv_tester_transactions::axi_sw_mst::b<>,
        rv_tester_transactions::axi_sw_mst::r<>,
        rv_tester_transactions::axi_sw_mst::ar_q_ptr<>,
        rv_tester_transactions::axi_sw_mst::aw_q_ptr<>,
        rv_tester_transactions::axi_sw_mst::w_q_ptr<>
    >;
    
  private:

    void rand_mode_thread();

    cvm::messenger::task<void> rand_mode();

    cvm::messenger::task<void> tick();
    cvm::messenger::task<void> trigger();

    cvm::messenger::task<void> check_axi_rresp_timeout(axi::a_no_id_t ar_txn, unsigned& id);

    void init();
    void push_snoop_info(uint64_t push_addr);
    void overlay_read(uint64_t addr);
    cvm::messenger::task<void> blocking_read(const transactor::read_t& r);
  
  private:

    cvm::topology::loc_t loc_;
    cvm::topology::loc_t axi_mst_loc_l;
    cvm::messenger::pool<axi::r_t>::channel_info channel;
    cvm::topology::loc_t snoop_gen_loc;
    unsigned id_;
    svScope scope_;

   
   
    cvm::rand::uniform_dist<uint32_t> rng1;
    uint32_t max_snoop_count;

    std::vector<uint64_t> snoop_addrs; 
    bool read_in_flight = false; 
    uint32_t snoops_driven = 0;
    uint8_t axi_id = 30;
};
