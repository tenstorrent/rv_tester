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
#include "pll_xtor_defines.h"


DECLARE_bool(pll_en);

class pll_xtor : public device {

    private:

        bool end_test=0;
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
        uint32_t start_pll_cnt,read_ram;
        uint32_t cnt_tick=0;
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
            if(FLAGS_pll_en) {
              cvm::log(cvm::HIGH, "[pll_xtor] pll_xtor timer tick advance interval {} start_pll_cnt {} \n",cnt_tick,start_pll_cnt);
              if(end_test==1) complete_pll_test();
              if(cnt_tick==start_pll_cnt) push_pll_enable_seq();
              if(cnt_tick==(start_pll_cnt+120)) push_pll_disable_seq();
              if(pll_wr_txn_q.size() > 0) axi_write();
              if(cnt_tick==(start_pll_cnt+130)) read_pointers();
              if((pll_read_resp_q.size() == 2) && (cnt_tick == start_pll_cnt+132)) read_sram();
              if(read_ram > 0) {
                cvm::log(cvm::HIGH, "[pll_xtor] read RAM {} \n",read_ram);
                axi_read(0xa082040,2,5);
                read_ram = read_ram - 1;
                if(read_ram == 0) end_test = 1;
              }
            }
            cnt_tick ++;
        }
        
        void push_pll_enable_seq() {
          cvm::log(cvm::HIGH, "[pll_xtor] pll_xtor inside enable pll seq\n");
          cvm::log(cvm::HIGH, "[pll_xtor] pll_xtor completed enable pll seq\n");
        }

        void push_pll_disable_seq() {
          cvm::log(cvm::HIGH, "[pll_xtor] pll_xtor inside disable pll seq\n");
          cvm::log(cvm::HIGH, "[pll_xtor] pll_xtor completed disable pll seq\n");
        }

        void read_pointers(){
          cvm::log(cvm::HIGH, "[pll_xtor] pll_xtor reading WRITE/READ pointers\n");
        }

        void read_sram() {
          
           cvm::log(cvm::HIGH, "[pll_xtor] pll_xtor reading SRAM done \n");
        }        

        // add max mem size
        pll_xtor(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);


           /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

};
