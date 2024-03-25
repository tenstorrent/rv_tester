#pragma once

#include "sysmod/device.h"
#include "sysmod/pm_common.h"
#include "cvm/topology.hpp"
#include <string>
#include <queue>
#include <random>
#include <mem_manager.h>
#include "pcg_random.hpp"
#include "cvm/registry.hpp"
#include "transactor.h"
#include "transactors/axi_sw/axi.h"
#include "pll_xtor_defines.h"



class pll_xtor : public device {

    private:

        mem_manager m_;
        cvm::topology::loc_t axi_mst_loc_l;
        cvm::messenger::pool<axi::r_t>::channel_info channel;
        pcg_extras::seed_seq_from<std::random_device> seed_source;
        pcg32 rng;
        void complete_pll_test()
        {
           cvm::registry::messenger.signal(loc(), pll_info_t{1,0});
        }

    public:
        uint32_t start_pll_cnt = 0;
        uint32_t read_ram = 0;
        uint32_t cnt_tick = 0;
        struct pll_wr_t {
          uint32_t addr;
          uint32_t data;
        };
        struct pll_xtor_read_t {
          uint64_t addr;
          size_t length;
          uint32_t id;
        };
        struct pll_info_t{
            uint32_t pll_quiesced;
            uint64_t pll_status;
        };
        struct pll_xtor_read_req_t {
          uint64_t addr;
          size_t length;
          uint32_t id;
          std::vector<uint8_t> data;
          std::vector<bool> strb;
        };
        std::queue<pll_xtor_read_req_t> pll_read_resp_q;
        std::queue<pll_wr_t>           pll_wr_txn_q;
        std::queue<pll_wr_t>           pm_nw_txn_q;
        std::queue<pm_common::pm_common_tx_t> txn_q ;
        //std::queue<pm_common::pm_common_tx_t> pll_in_q;
        bool pll_en = true;
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
            if(start_pll_cnt == 0) {
              start_pll_cnt = (rng()% 5) + 30;
            }
            if(pll_en) {
              cvm::log(cvm::HIGH, "[pll_xtor] pll_xtor timer tick advance interval {} start_pll_cnt {} \n",cnt_tick,start_pll_cnt);
               //add your dv logic here
            }
            pm_common::pm_common_tx_t i;
            i.addr = 12;
            i.data = 123;
            write_pm_nw_xtor_q(i);
            cnt_tick ++;
        }
      void write_pll_xtor_q(pm_common::pm_common_tx_t i){
        cvm::log(cvm::HIGH, "[pll_xtor] pll_xtor pushing txn in pll_txn q {} \n",i.addr);
      }
      
      void write_pm_nw_xtor_q(pm_common::pm_common_tx_t i){
        cvm::log(cvm::HIGH, "[pll_xtor] pll_xtor sending txn in pm_nw_txn q {} \n",i.addr);
        auto pm_nw_loc = cvm::topology::get_from_type("PM_NW_XTOR", 0);
        cvm::registry::messenger.signal(pm_nw_loc, pm_common::pm_common_tx_t{i.addr,i.data});
        
      }

        // add max mem size
        pll_xtor(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);


           /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

};
