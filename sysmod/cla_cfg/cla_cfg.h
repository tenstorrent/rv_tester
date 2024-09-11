#pragma once

#include "sysmod/device.h"
#include "cvm/topology.hpp"
#include <string>
#include <queue>
#include <random>
#include <mem_manager.h>
#include "pcg_random.hpp"
#include "cvm/registry.hpp"
#include "cla_defines.h"
#include "transactor.h"
#include "transactors/axi_sw/axi.h"
#include <unistd.h>

DECLARE_bool(cla_clk_halt);
DECLARE_bool(cla_nmi);
DECLARE_bool(cla_rand_nmi_trig_en);

class cla_cfg : public device {

    private:

        mem_manager m_;
        cvm::topology::loc_t axi_mst_loc_l;
        cvm::messenger::pool<axi::r_t>::channel_info channel;
        pcg_extras::seed_seq_from<std::random_device> seed_source;
        pcg32 rng;

    public:
        bool reenable_nmi, reenable_rand_trig;
        uint32_t start_clk_halt_cnt=0, start_cla_nmi_cnt, start_rand_nmi_trig_cnt;
        uint32_t eap_ctrl, active_core, mask, addr_offset, cntr_data;
        uint32_t rand_disable_dly, nmi_total_cnt, trig_total_cnt, rand_disable_trig_dly;
        uint32_t cnt_tick=0;
        uint64_t axi_read_resp=0;
        std::unordered_map<std::string, uint32_t> macros;
        struct cla_wr_t {
          uint32_t addr;
          uint32_t data;
        };
        struct cla_cfg_read_t {
          uint64_t addr;
          size_t length;
          uint32_t id;
        };
        struct cla_info_t{
            uint32_t cla_quiesced;
            uint64_t cla_status;
        };
        struct cla_cfg_read_req_t {
          uint64_t addr;
          size_t length;
          uint32_t id;
          std::vector<uint8_t> data;
          std::vector<bool> strb;
        };
        std::queue<cla_cfg_read_req_t>  cla_read_resp_q;
        std::queue<cla_wr_t>            cla_wr_txn_q;
        virtual void axi_write();
        virtual void axi_read(uint64_t addr, size_t length, uint32_t id);
        void write(const transactor::write_t& );
        void overlay_tick(uint64_t) override;

        void gen_data_strb(uint64_t addr, uint32_t value, data_t& wdata, std::vector<bool>& strb);
        void push_clk_halt_cfg();
        void push_cla_nmi_cfg();
        void push_cla_nmi_cfg_disable();
        void push_rand_nmi_trigg_cfg();
        void push_rand_nmi_trigg_cfg_off();

        virtual void write_axi_mst(uint64_t addr, size_t length, const data_t& data, const strb_t& strb);
        virtual void read_axi_mst(uint64_t addr, size_t length, data_t& data);

//        std::unordered_map<std::string, uint32_t> extractMacros(const std::string& filename);

        cvm::messenger::task<void> read(const transactor::read_t& , data_t& );

        template <typename INT>
        void deserializeInt(const data_t &data, INT &x,const uint64_t &addr)
        {
          x = 0;
          for (unsigned i = 0; i < sizeof(x); ++i)
            x |= INT(data[i+(addr%64)]) << (i * 8);
        }     
        

        // add max mem size
        cla_cfg(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);


           /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

};
