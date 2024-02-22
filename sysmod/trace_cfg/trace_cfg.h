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


DECLARE_bool(trace_en);

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

    public:
        uint32_t start_trace_cnt,read_ram;
        uint32_t cnt_tick=0;
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
        std::queue<trace_cfg_read_req_t> trace_read_resp_q;
        std::queue<trace_wr_t>           trace_wr_txn_q;
        virtual void axi_write();
        virtual void axi_read(uint64_t addr, size_t length, uint32_t id);
        void write(const transactor::write_t& );

        cvm::messenger::task<void> read(const transactor::read_t& , data_t& );

        void gen_data_strb(uint64_t addr, uint32_t value, data_t& wdata, std::vector<bool>& strb) {
            uint8_t b_index =  static_cast<uint8_t>(addr & 0x3F);

            for (uint8_t i = 0; i < 64; ++i) {
                  wdata.push_back(0x0);
                  strb.push_back(0x0);
            }  
            for (uint8_t i = 0; i < 4; ++i) {
                  uint8_t currentByte = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
                  wdata[i+b_index] = currentByte;
                  strb[i+b_index] = 0x1;
            }  
        }

        virtual void write_axi_mst(uint64_t addr, size_t length,
                            const data_t& data, const strb_t& strb);

        virtual void read_axi_mst(uint64_t addr, size_t length,
                          data_t& data);

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
        
        virtual void tick(uint64_t) override
        {
            if(start_trace_cnt == 0) {
              start_trace_cnt = (rng()% 5) + 30;
            }
            if(FLAGS_trace_en) {
              cvm::log(cvm::HIGH, "[Trace_cfg] trace_cfg timer tick advance interval {} start_trace_cnt {} \n",cnt_tick,start_trace_cnt);
              if(end_test==1) complete_trace_test();
              if(cnt_tick==start_trace_cnt) push_trace_enable_seq();
              if(cnt_tick==(start_trace_cnt+120)) push_trace_disable_seq();
              if(trace_wr_txn_q.size() > 0) axi_write();
              if(cnt_tick==(start_trace_cnt+130)) read_pointers();
              if((trace_read_resp_q.size() == 2) && (cnt_tick == start_trace_cnt+132)) read_sram();
              if(read_ram > 0) {
                cvm::log(cvm::HIGH, "[Trace_cfg] read RAM {} \n",read_ram);
                axi_read(0xa082040,2,5);
                read_ram = read_ram - 1;
                if(read_ram == 0) end_test == 1;
              }
            }
            cnt_tick ++;
        }
        
        void push_trace_enable_seq() {
          cvm::log(cvm::HIGH, "[Trace_cfg] trace_cfg inside enable trace seq\n");
          // Funnel-RAM config
          trace_wr_txn_q.push({TR_DST_RAM_START_LOW,0x40});
          trace_wr_txn_q.push({TR_DST_RAM_LIMIT_LOW,0x8000});
          trace_wr_txn_q.push({TR_DST_RAM_WP_LOW,0x40});
          trace_wr_txn_q.push({TR_DST_RAM_RP_LOW,0x40});
          trace_wr_txn_q.push({TR_DST_RAM_CONTROL,0x3});
          trace_wr_txn_q.push({TR_FUNNEL_CONTROL,0x3});
        
          // CLA/Paketizer config
          trace_wr_txn_q.push({CDBG_CLA_CTRL_STS_CFG,0x40});
          trace_wr_txn_q.push({TR_DST_CONTROL,0x3});
          trace_wr_txn_q.push({CDBG_MUX_SEL_CFG,0x0});
          trace_wr_txn_q.push({CDBG_CLA_COUNTER0_CFG,0x20000000});
          trace_wr_txn_q.push({CDBG_NODE0_EAP0_CFG,0x11211});
          trace_wr_txn_q.push({CDBG_NODE1_EAP0_CFG,0x101316});
          trace_wr_txn_q.push({CDBG_TRACE_CFG,0x102810});
          // RTL FIX Needed for this MMR access
          //trace_wr_txn_q.push({TR_DST_IMPL,0x1000000});
          trace_wr_txn_q.push({CDBG_CLA_CTRL_STS_CFG,0x60});
          cvm::log(cvm::HIGH, "[Trace_cfg] trace_cfg completed enable trace seq\n");
        }

        void push_trace_disable_seq() {
          cvm::log(cvm::HIGH, "[Trace_cfg] trace_cfg inside disable trace seq\n");
          trace_wr_txn_q.push({CDBG_CLA_CTRL_STS_CFG,0x40});
          trace_wr_txn_q.push({TR_DST_CONTROL,0x0});
          trace_wr_txn_q.push({CDBG_CLA_CTRL_STS_CFG,0x0});
          trace_wr_txn_q.push({TR_FUNNEL_CONTROL,0x0});
          trace_wr_txn_q.push({TR_DST_RAM_CONTROL,0x0});
          cvm::log(cvm::HIGH, "[Trace_cfg] trace_cfg completed disable trace seq\n");
        }

        void read_pointers(){
          cvm::log(cvm::HIGH, "[Trace_cfg] trace_cfg reading WRITE/READ pointers\n");
           axi_read(TR_DST_RAM_WP_LOW,2,5);
           axi_read(TR_DST_RAM_RP_LOW,2,5);
        }

        void read_sram() {
          uint32_t wp=0,rp=0;
          trace_cfg_read_req_t rdata;

           rdata = trace_read_resp_q.front();
           trace_read_resp_q.pop();
           if(rdata.addr == 0xa082020) deserializeInt_wp(rdata.data,wp);
           wp = wp & 0xFFFFFFFC;
           cvm::log(cvm::HIGH, "[Trace_cfg] trace_cfg reading write pointer {:#X}\n",wp);

           rdata = trace_read_resp_q.front();
           trace_read_resp_q.pop();
           if(rdata.addr == 0xa082028) deserializeInt_rp(rdata.data,rp);
           cvm::log(cvm::HIGH, "[Trace_cfg] trace_cfg reading read pointer {:#X}\n",rp);

           cvm::log(cvm::HIGH, "[Trace_cfg] trace_cfg reading SRAM started\n");
           read_ram = (wp - rp)/4;
          
           cvm::log(cvm::HIGH, "[Trace_cfg] trace_cfg reading SRAM done \n");
        }        

        // add max mem size
        trace_cfg(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);


           /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

};
