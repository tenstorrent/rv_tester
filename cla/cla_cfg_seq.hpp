#pragma once

#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"
#include "cla.hpp"
#include "transactor.h"
#include "axi_sw_mst.h"
#include "svdpi.h"

DECLARE_bool(cla_clk_halt);
DECLARE_bool(cla_nmi);
DECLARE_bool(cla_rand_nmi_trig_en);
DECLARE_uint32(axi_resp_timeout);

class cla_cfg_seq {

  public:

    cla_cfg_seq(cvm::topology::loc_t loc, unsigned id);
    ~cla_cfg_seq();

    using overlay_mst_t = axi_sw_mst<
        rv_tester_transactions::axi_sw_mst::b<>,
        rv_tester_transactions::axi_sw_mst::r<>,
        rv_tester_transactions::axi_sw_mst::ar_q_ptr<>,
        rv_tester_transactions::axi_sw_mst::aw_q_ptr<>,
        rv_tester_transactions::axi_sw_mst::w_q_ptr<>
    >;

    void set_scope(svScope s) { scope_ = s; }

  private:

    bool nmi_event, reenable_nmi=0, reenable_rand_trig=0, end_cla_cfg_seq=0;
    uint32_t core_offset;
    uint32_t eap_ctrl, active_core, mask, cntr_data, cnt_loop_max;
    uint32_t nmi_total_cnt, trig_total_cnt, custom_action_cnt;
    void cla_main_thread();
    void cla_custom_action_thread();

    cvm::messenger::task<void> tick();
    cvm::messenger::task<void> core_no_fetch();

    cvm::messenger::task<void> cla_main();
    cvm::messenger::task<void> cla_custom_action_main();
    cvm::messenger::task<void> configure_cla_custom_action();
    cvm::messenger::task<void> disable_cla_custom_action();
    
    cvm::messenger::task<void> configure_cla();
    cvm::messenger::task<void> disable_cla();
    cvm::messenger::task<void> configure_cla_clk_halt();
    cvm::messenger::task<void> configure_cla_nmi();
    cvm::messenger::task<void> configure_cla_rand_nmi_trig_en();
    cvm::messenger::task<void> disable_cla_nmi();
    cvm::messenger::task<void> disable_cla_rand_nmi_trig_en();
    cvm::messenger::task<void> clear_pend_nmi_on_terminate();
    cvm::messenger::task<void> wait_for_clocks(uint32_t max);
    
    cvm::messenger::task<uint64_t> read(uint64_t addr, size_t sz, block_t block = BLOCK);
    cvm::messenger::task<void> write(uint64_t addr, size_t sz, uint64_t data, block_t block = BLOCK);
    cvm::messenger::task<void> csr_write(uint32_t core_id, uint32_t unit,uint64_t addr, uint64_t data);
    cvm::messenger::task<void> axi_write_mmr_granular(uint64_t addr);
    cvm::messenger::task<void> axi_write_mmr_data_granular(uint64_t addr, uint64_t data);
    cvm::messenger::task<uint64_t> axi_read_mmr_granular(const transactor::read_t& r );
    cvm::messenger::pool<axi::r_t>::channel_info channel;
    void terminate_test(uint8_t terminate_test);


    uint64_t convert_to_dword_array(const std::vector<uint8_t>& byte_array, uint8_t shift, size_t sz);
    std::vector<uint8_t> convert_to_byte_array(uint64_t data, uint8_t shift);

    cvm::messenger::task<bool> check_axi_bresp_timeout(axi::a_no_id_t aw_txn, unsigned& id);
    cvm::messenger::task<bool> check_axi_rresp_timeout(axi::a_no_id_t ar_txn, unsigned& id);

  private:

    cvm::topology::loc_t loc_, axi_mst_loc_;
    svScope scope_;
};
