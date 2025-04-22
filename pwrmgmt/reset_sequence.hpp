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

DECLARE_uint32(axi_resp_timeout); // Cycles to wait after Transactor-id pool overflow condition before raising no free ids error

class reset_sequence {

  public:

    reset_sequence(cvm::topology::loc_t loc, unsigned id);
    ~reset_sequence();

     using overlay_mst_t = axi_sw_mst<
        rv_tester_transactions::axi_sw_mst::b<>,
        rv_tester_transactions::axi_sw_mst::r<>,
        rv_tester_transactions::axi_sw_mst::ar_q_ptr<>,
        rv_tester_transactions::axi_sw_mst::aw_q_ptr<>,
        rv_tester_transactions::axi_sw_mst::w_q_ptr<>
    >;
    using smc_mst_t = axi_sw_mst<
        rv_tester_transactions::axi_sw_mst::b<1>,
        rv_tester_transactions::axi_sw_mst::r<1>,
        rv_tester_transactions::axi_sw_mst::ar_q_ptr<1>,
        rv_tester_transactions::axi_sw_mst::aw_q_ptr<1>,
        rv_tester_transactions::axi_sw_mst::w_q_ptr<1>
    >;

  private:

    std::array<cvm::messenger::pool<axi::b_t>::channel_info, INTF_COUNT> b_channel_;
    std::array<cvm::messenger::pool<axi::r_t>::channel_info, INTF_COUNT> r_channel_;

    void set_scope(svScope s) { scope_ = s; }
    void start(int reset_count);

    void cold_reset_sequence_thread();
    void warm_reset_sequence_thread();
    void smc_random_sequence_thread();

    cvm::messenger::task<void> cold_reset_sequence();
    cvm::messenger::task<void> warm_reset_sequence();

    cvm::messenger::task<void> cold_reset_ack();
    cvm::messenger::task<void> force_ref_clk_ack();
    cvm::messenger::task<void> tick();
    cvm::messenger::task<void> trigger();
    cvm::messenger::task<void> cpl_reset_sequence(rst_t );
    cvm::messenger::task<void> cpl_fw_reset_sequence(rst_t );
    cvm::messenger::task<void> check_system_ready();
    cvm::messenger::task<void> send_start_of_execution_to_cpl();
    cvm::messenger::task<void> pll_startup_sequence();
    cvm::messenger::task<void> check_pll_status();
    cvm::messenger::task<void> clear_pll_status();
    cvm::messenger::task<void> pll_dfs_sequence();
    cvm::messenger::task<void> release_cpl_reset();
    cvm::messenger::task<void> wait_reset_release();
    cvm::messenger::task<void> wait_nofetch_release();
    cvm::messenger::task<void> program_fuses();
    cvm::messenger::task<void> program_patch();
    cvm::messenger::task<void> release_cpl_nofetch();
    cvm::messenger::task<void> patch_ram_check();
    cvm::messenger::task<void> fuse_mmr_check( rst_t rst_type = WARM);
    cvm::messenger::task<void> disabled_mmr_csr_check();
    cvm::messenger::task<void> program_fe_resetvector();
    cvm::messenger::task<void> mmr_read_write_check(uint64_t addr, interface_t interface, bool rsp_err_chk = true);

    cvm::messenger::task<void> write_thub_reg(uint8_t addr, uint32_t data, uint8_t satellite_num, uint8_t mbox_num);
    cvm::messenger::task<void> program_thub_threshold();

    cvm::messenger::task<void> init_smc_filters();

    cvm::messenger::task<uint64_t> read(uint64_t addr, size_t sz, interface_t interface = SMC, bool rsp_err_chk = true);
    cvm::messenger::task<void> write(uint64_t addr, size_t sz, uint64_t data, interface_t interface = SMC, bool rsp_err_chk = true);
    cvm::messenger::task<void> write(uint64_t addr, size_t sz, const std::vector<uint64_t>& data, bool rsp_err_chk = true);
    cvm::messenger::task<void>csr_write(uint32_t core_id, uint32_t unit,uint64_t addr, uint64_t data);
    cvm::messenger::task<uint64_t>csr_read(uint32_t core_id, uint32_t unit,uint64_t addr);
    cvm::messenger::task<void>init_csr();
    cvm::messenger::task<void>init_mmr();
    cvm::messenger::task<void>rmw_csr();
    cvm::messenger::task<void>rmw_mmr();

    std::vector<uint64_t> convert_to_dword_array(const std::vector<uint8_t>& byte_array);
    std::vector<uint8_t> convert_to_byte_array(const std::vector<uint64_t>& dword_array);
    std::vector<uint64_t> concatenate_uint32_to_uint64(const std::vector<uint32_t>& input); 

    cvm::messenger::task<void> check_axi_bresp_timeout(interface_t interface, unsigned& id, uint64_t addr, size_t sz, bool rsp_err_chk = true);
    cvm::messenger::task<void> check_axi_rresp_timeout(interface_t interface, unsigned& id, uint64_t addr, size_t sz, bool rsp_err_chk = true);

    uint64_t fuse_val();
    uint64_t core_fuse_val();
    uint64_t trace_fuse_val();
    uint64_t dm_fuse_val();
    uint64_t sc_fuse_val();
    uint64_t export_control_fuse_val();
    uint64_t cla_fuse_val();
    uint64_t io_coherency_fuse_val();
    uint64_t dst_fuse_val();
    uint64_t jtag_to_axi_fuse_val();
    uint64_t core_en(uint32_t c);
    std::vector<uint64_t> mhartid();

    void init();
    void cold_reset(uint8_t assert);
    void warm_reset(uint8_t assert);
    void reset_hold(uint8_t sram, uint8_t debug, uint8_t critical);
    void force_ref_clk(uint8_t assert);
    void populate_patch_ram(uint64_t addr, const std::vector<uint64_t>& data);
    void read_patch_csv();
    std::string get_intf_name(interface_t value);

  private:

    cvm::topology::loc_t loc_;
    std::array<cvm::topology::loc_t, INTF_COUNT> axi_loc_;

    svScope scope_;

    int reset_count_ = 0;
    uint32_t num_cores_ = 0;
    interface_t boot_interface = SMC;
};
