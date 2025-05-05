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
#include "rv_tester_structs.h"

DECLARE_uint32(axi_resp_timeout); // Cycles to wait after Transactor-id pool overflow condition before raising no free ids error

class snoop_gen_sequence {

  public:
    struct snoop_async {
      uint64_t address;
      std::atomic<bool>* flag;
    };


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
    cvm::messenger::task<void> eot_snoop_addr(std::queue<uint64_t>& addr_q);
    void eot_addr_queue(rv_tester::snoop_addrs_eot addrs);
  
  private:

    cvm::topology::loc_t loc_, axi_mst_loc_;
    cvm::messenger::pool<axi::r_t>::channel_info channel;
    unsigned id_;
    svScope scope_;
    axi::r_t resp_;
    transactor::read_t r_;
   
    cvm::rand::uniform_dist<uint32_t> rng1;
    uint32_t max_snoop_count;
    std::queue<uint64_t> eot_address_;

    std::vector<uint64_t> snoop_addrs; 
    uint32_t snoops_driven = 0;
    uint8_t axi_id = 30;
    bool read_in_flight = false; 
    uint64_t backdoor_read_data_ = 0;

};
