#pragma once

#include "sysmod/device.h"
#include "cvm/topology.hpp"
#include <string>
#include <queue>
#include <random>
#include <mem_manager.h>
#include "pcg_random.hpp"
#include "cvm/registry.hpp"
#include "transactor.h"
#include "transactors/axi_sw/axi.h"
#include "trace_defines.h"
#include <unistd.h>

DECLARE_bool(trace_en);
DECLARE_bool(overlay_mmr_en);
DECLARE_bool(cla_clk_halt);
DECLARE_bool(cla_nmi);

class trace_cfg : public device {

    private:

        bool end_test=0;
        mem_manager m_;
        cvm::topology::loc_t axi_mst_loc_l;
        cvm::messenger::pool<axi::r_t>::channel_info channel;
        pcg_extras::seed_seq_from<std::random_device> seed_source;
        pcg32 rng;
        void complete_trace_test()
        {
           cvm::registry::messenger.signal(loc(), trace_info_t{1,0});
        }
        typedef std::vector<decltype(mmr::list)::value_type> random_list;

    public:
        bool is_smem_mode;
        uint32_t start_clk_halt_cnt=0,trace_stop_cnt;
        uint32_t trace_start_cnt=0,n,id_val,read_ram=0,axi_ids = 0;
        uint32_t cnt_tick=0;
        uint64_t axi_read_resp=0;
        std::unordered_map<std::string, uint32_t> macros;
        struct trace_wr_t {
          uint32_t addr;
          uint32_t data;
        };
        struct trace_cfg_read_t {
          uint64_t addr;
          size_t length;
          uint32_t id;
        };
        struct trace_info_t{
            uint32_t trace_quiesced;
            uint64_t trace_status;
        };
        struct trace_cfg_read_req_t {
          uint64_t addr;
          size_t length;
          uint32_t id;
          std::vector<uint8_t> data;
          std::vector<bool> strb;
        };
        std::queue<trace_cfg_read_req_t>  trace_read_resp_q;
        std::queue<trace_wr_t>            trace_wr_txn_q;
        std::queue<std::pair<uint64_t,size_t>> trace_misc_rd_txn_q;
        random_list randomElements;
        virtual void axi_write();
        virtual void axi_read(uint64_t addr, size_t length, uint32_t id);
        void write(const transactor::write_t& );
        void overlay_tick(uint64_t) override;

        void gen_data_strb(uint64_t addr, uint32_t value, data_t& wdata, std::vector<bool>& strb);
        void push_clk_halt_cfg();
        void push_cla_nmi_cfg();
        void push_axi_mmr_seq();
        void push_read_axi_mmr_seq();
        void read_axi_pointers();
        void push_random_axi_read(const random_list&  elements);
        void push_random_axi_write(const random_list& elements);
        void print_read_request(const trace_cfg_read_req_t  &request,int read);
        void push_trace_enable_seq();
        void push_trace_disable_seq();
        void check_dst_ram_empty();
        void read_pointers();
        void read_sram();

        virtual void write_axi_mst(uint64_t addr, size_t length, const data_t& data, const strb_t& strb);
        virtual void read_axi_mst(uint64_t addr, size_t length, data_t& data);

        std::unordered_map<std::string, uint32_t> extractMacros(const std::string& filename);
        random_list pickRandomElements(uint32_t n) ;

        cvm::messenger::task<void> read(const transactor::read_t& , data_t& );

        template <typename INT>
        void deserializeInt_wp(const data_t &data, INT &x)
        {
          x = 0;
          for (unsigned i = 0; i < sizeof(x); ++i)
            x |= INT(data[i+32]) << (i * 8);
        }     
        template <typename INT>
        void deserializeInt_rp(const data_t &data, INT &x)
        {
          x = 0;
          for (unsigned i = 0; i < sizeof(x); ++i)
            x |= INT(data[i+40]) << (i * 8);
        }               

        template <typename INT>
        void deserializeInt(const data_t &data, INT &x,const uint64_t &addr)
        {
          x = 0;
          for (unsigned i = 0; i < sizeof(x); ++i)
            x |= INT(data[i+(addr%64)]) << (i * 8);
        }     
        

        // add max mem size
        trace_cfg(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);


           /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

};
