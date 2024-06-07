#pragma once

#include "cvm/topology.hpp"
#include <string>
#include <queue>
#include <random>
#include <mem_manager.h>
#include "sysmod/device.h"
#include "pcg_random.hpp"
#include "cvm/registry.hpp"
#include "transactor.h"
#include "transactors/axi_sw/axi.h"

DECLARE_bool(sp_xtor_en);
class scratchpad_xtor : public device {

    private:

        //bool end_test=0;
        mem_manager m_;
        bool trigger_flag =0;
        uint64_t trigger_addr =0;
        uint64_t trigger_mode =0;
        uint64_t sp_mmr_base =0;
        cvm::topology::loc_t axi_mst_loc_l;
        cvm::messenger::pool<axi::r_t>::channel_info channel;
        pcg_extras::seed_seq_from<std::random_device> seed_source;
        pcg32 rng;
        uint64_t scratchpad_xtor_base  = 0x9070000;

    public:
        uint32_t start_scratchpad_cnt,read_ram;
        uint32_t cnt_tick=0;
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
        virtual void axi_write();
        virtual void axi_read(uint64_t addr, size_t length, uint32_t id);
        virtual void axi_write_granular();
        virtual void axi_write_data_granular();
        //void write(const transactor::write_t& );
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
        
          // Copy bytes from data iterator into the given integer following
  // lilttle endian convention.
  template <typename INT>
  void deserializeInt(const data_t& data, INT& x)
  {
    x = 0;
    for (unsigned i = 0; i < sizeof(x); ++i)
      x |= INT(data[i]) << i*8;
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
            cnt_tick ++;
            if(!FLAGS_sp_xtor_en)
            return;
            cvm::log(cvm::HIGH, " SCRATCHPAD_XTOR tick {}\n",cnt_tick);
            if(cnt_tick == 60){
            cvm::log(cvm::HIGH, " SCRATCHPAD_XTOR trigger flag set \n");
            axi_write_granular();
            trigger_flag = 0;
            }
            if(cnt_tick == 60){
            axi_write_data_granular();
            }
        }
        
        // add max mem size
        scratchpad_xtor(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);


           /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

};
