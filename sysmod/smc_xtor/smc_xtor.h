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
#include "smc_defines.h"


DECLARE_bool(smc_en);

class smc_xtor : public device {

    private:

        //bool end_test=0;
        mem_manager m_;
        cvm::topology::loc_t axi_mst_loc_l;
        cvm::messenger::pool<axi::r_t>::channel_info channel;
        pcg_extras::seed_seq_from<std::random_device> seed_source;
        bool in_boot_seq = true;
        bool reset_completion = false;
        pcg32 rng;
        void complete_smc_test()
        {
           cvm::registry::messenger.signal(loc(), smc_info_t{1,0});
        }

    public:
        uint32_t start_smc_cnt,read_ram;
        uint32_t cnt_tick=0;
        struct smc_wr_t {
          uint32_t addr;
          uint64_t data;
        };
        struct smc_xtor_read_t {
          uint64_t addr;
          size_t length;
          uint32_t id;
        };
        struct smc_info_t{
            uint32_t smc_quiesced;
            uint64_t smc_status;
        };
        struct smc_xtor_read_req_t {
          uint64_t addr;
          size_t length;
          uint32_t id;
          std::vector<uint8_t> data;
          std::vector<bool> strb;
        };
        std::queue<smc_xtor_read_req_t> smc_read_resp_q;
        std::queue<smc_wr_t>            smc_wr_txn_q;
        std::queue<smc_xtor_read_req_t> smc_rd_txn_q;
        std::queue<smc_wr_t>            smc_boot_wr_txn_q;
        uint32_t smc_id = 0;
        virtual void axi_write();
        virtual void axi_read(uint64_t addr, size_t length, uint32_t id);
        void write(const transactor::write_t& );
       
        cvm::messenger::task<void> read(const transactor::read_t& , data_t& );

        void gen_data_strb(uint64_t addr, uint32_t value, data_t& wdata, std::vector<bool>& strb) {
            uint8_t b_index =  static_cast<uint8_t>(addr & 0x7);

            for (uint8_t i = 0; i < 8; ++i) {
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

            cvm::log(cvm::FULL, "[SMC] tick {:#X} \n",cnt_tick);
            if(in_boot_seq && ( cnt_tick > 14)){
            cvm::log(cvm::FULL, "[SMC] IN BOOT SEQ {} reset complition {} \n",in_boot_seq,reset_completion);
              if(!reset_completion){
                  cvm::log(cvm::FULL, "[SMC] CHK AXI READ RESP FOR 0xC000300C \n");
                  cvm::log(cvm::FULL, "[SMC] CHK AXI READ RESP FOR 0xC000300C Q size {} \n",smc_read_resp_q.size());
                  if(smc_read_resp_q.size() >0){
                     smc_xtor_read_req_t smc_xtor_rd;
                     smc_xtor_rd = smc_read_resp_q.front();
                     smc_read_resp_q.pop();
                     for (int i = 0; i < 8; i++) {
                     cvm::log(cvm::FULL, "[SMC] READ RESP data[{}]= {:#X} \n",i,smc_xtor_rd.data[i]);
                     }
                     cvm::log(cvm::FULL, "[SMC] READ RESP {:#X} \n",smc_xtor_rd.data[0]);
                     if(smc_xtor_rd.data[4] == 0x10){
                      reset_completion = true;
                     }
                  }else{

                    cvm::log(cvm::FULL, "[SMC] AXI READ 0xC000300C \n");

                    axi_read(0xC000300C,4,4);
                  }
              }else{
                  cvm::log(cvm::FULL, "[SMC] DRIVE AXI BOOT SEQ WR  \n");
                 if(smc_boot_wr_txn_q.size()>0){
                   smc_wr_t smc_boot_txn;
                   smc_boot_txn = smc_boot_wr_txn_q.front();
                   smc_boot_wr_txn_q.pop();
                   smc_wr_txn_q.push(smc_boot_txn);
                 }else{
                  //boot done
                  cvm::log(cvm::FULL, "[SMC]  BOOT SEQ DONE \n");
                  in_boot_seq = false;
                 }
              }
            }
            
            if(smc_wr_txn_q.size() > 0) axi_write();

            cvm::log(cvm::HIGH, "[SMC] tick {:#X} \n",cnt_tick);

            cnt_tick ++;
        }
        
        void print_read_request(const smc_xtor_read_req_t &request) {
            cvm::log(cvm::FULL, "Address: {} \n",request.addr);
            cvm::log(cvm::FULL, "Length: {} \n",request.length);
            cvm::log(cvm::FULL, "ID: {} \n ",request.id );
            
            
            std::stringstream ss;

            for (const auto &byte : request.data) {
                ss << static_cast<int>(byte) << " ";
            }

   
            std::string output = ss.str();
            ss.clear();
            ss.str("");

            for (const auto &bit : request.strb) {
                ss <<  bit << " ";
            }
            std::string output2 = ss.str();
            cvm::log(cvm::FULL,"Data: {} \n",output);
            cvm::log(cvm::FULL,"STRB: {} \n",output2);
        }

        void push_smc_enable_seq() {
          cvm::log(cvm::FULL, "[smc_xtor] smc_xtor inside enable smc seq\n");
          cvm::log(cvm::FULL, "[smc_xtor] smc_xtor completed enable smc seq\n");
        }
        
        void push_smc_boot_seq() {
          //Write 0x10 in 0xc000_300C // Clears cold power up done interrupt
          smc_boot_wr_txn_q.push({ 0xC000300C,0x10});
          //Write 0x1 in 0xc000_2004  // Release cluster cold reset
          smc_boot_wr_txn_q.push({ 0xC0002004,0x1});
          //Write 0x1 in 0xc000_2008  // Release cluster warm reset
          smc_boot_wr_txn_q.push({ 0xC0002008,0x1});
          //Write 0x00 in 0xc000_200C // Release core no fetch control
          smc_boot_wr_txn_q.push({ 0xC000200C,0x000000000});
        }
        void push_smc_disable_seq() {
          cvm::log(cvm::FULL, "[smc_xtor] smc_xtor inside disable smc seq\n");
          cvm::log(cvm::FULL, "[smc_xtor] smc_xtor completed disable smc seq\n");
        }


        // add max mem size
        smc_xtor(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);


           /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

};
