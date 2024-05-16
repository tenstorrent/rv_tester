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
DECLARE_bool(smc_sweep_test);
DECLARE_int32(smc_reset_seq_start_ticks);
class smc_xtor : public device {

    private:

        bool end_test=0;
        mem_manager m_;
        cvm::topology::loc_t axi_mst_loc_l;
        cvm::messenger::pool<axi::r_t>::channel_info channel;
        pcg_extras::seed_seq_from<std::random_device> seed_source;
        bool in_boot_seq = true; 
        bool reset_completion = false;
        bool read_in_flight = false;
        std::pair<uint64_t,size_t> read_req;
        pcg32 rng;
        void complete_smc_test()
        {
           cvm::registry::messenger.signal(loc(), smc_info_t{1,0});
        }

    public:
        uint32_t start_smc_cnt=0,read_ram;
        uint64_t write_ram; 
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
        std::queue<smc_xtor_read_req_t>   smc_read_resp_q;
        std::queue<smc_wr_t>              smc_wr_txn_q;
        std::queue<std::pair<uint64_t,size_t>> smc_rd_txn_q;
        std::queue<smc_wr_t>              smc_boot_wr_txn_q;
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
            for (uint8_t i = 0; i < 8; ++i) {
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
            if(!FLAGS_smc_en)
              return;
                        
            cvm::log(cvm::FULL, "[SMC] tick {:#X} \n",cnt_tick);
            if(in_boot_seq && ( cnt_tick > uint32_t(FLAGS_smc_reset_seq_start_ticks))){
            cvm::log(cvm::FULL, "[SMC] IN BOOT SEQ {} reset complition {} \n",in_boot_seq,reset_completion);
              if(!reset_completion){
                  cvm::log(cvm::FULL, "[SMC] Check axi read response for 0xC000300C \n");
                  cvm::log(cvm::FULL, "[SMC] axi read response  Queue size for  0xC000300C = {} \n",smc_read_resp_q.size());
                  if(smc_read_resp_q.size() >0){
                     smc_xtor_read_req_t smc_xtor_rd;
                     smc_xtor_rd = smc_read_resp_q.front();
                     smc_read_resp_q.pop();
                    //  std::string d;
                     for (int i=0; i<8; i++)
                    //    d += fmt::format("{:02x}", smc_xtor_rd.data[i]);
                    //  cvm::log(cvm::FULL, "[SMC] read resp data= {:#X} \n", d);
       
                     if(smc_xtor_rd.data[4] == 0x10){
                      reset_completion = true;
                     }
                  }else if (!read_in_flight) {

                    cvm::log(cvm::FULL, "[SMC] axi read 0xC000300C \n");

                    read_in_flight = true;
                    axi_read(0xC000300C,4,204);
                  }
              }else{
                  cvm::log(cvm::FULL, "[SMC] Drive axi write requests for boot sequence  \n");
                 if(smc_boot_wr_txn_q.size()>0){
                   smc_wr_t smc_boot_txn;
                   smc_boot_txn = smc_boot_wr_txn_q.front();
                   smc_boot_wr_txn_q.pop();
                   smc_wr_txn_q.push(smc_boot_txn);
                 }else{
                  //boot done
                  cvm::log(cvm::FULL, "[SMC]  boot sequence completed \n");
                  in_boot_seq = false;
                 }
              }
            }


            if(start_smc_cnt == 0){
              start_smc_cnt = (rng()% 5) + 25;
              // reset_completion = true;
              if(FLAGS_smc_en){
                cvm::log(cvm::LOW, "[SMC] Enable switch is set");
              }
            }
            
            cvm::log(cvm::FULL, "[SMC] tick {:#X} start_smc_cnt {} \n",cnt_tick,start_smc_cnt);
           
            if(smc_wr_txn_q.size() > 0) axi_write();

            cvm::log(cvm::HIGH, "[SMC] tick {:#X} \n",cnt_tick);

            if(reset_completion && !in_boot_seq && FLAGS_smc_sweep_test){
                if(end_test==1) complete_smc_test();

                if(cnt_tick==start_smc_cnt) push_smc_sram_write_seq();

                // if(smc_wr_txn_q.size() > 0) axi_write();
                if(smc_rd_txn_q.size() > 0) {
                  read_req = smc_rd_txn_q.front();
                  smc_rd_txn_q.pop();
                  axi_read(read_req.first,read_req.second,(start_smc_cnt+200)%1024);
                }

                while((smc_read_resp_q.size() >0) ){
                  print_read_request(smc_read_resp_q.front());
                  smc_read_resp_q.pop();
                  cvm::log(cvm::HIGH, "[security test] queue size {} \n",smc_read_resp_q.size());
                  if(smc_read_resp_q.size() == 0){
                    end_test = 1;
                    cvm::log(cvm::HIGH, "[security test] write and read check to small core sram ended\n");
                }
              }
            }

            cnt_tick ++;
        }
        
        void print_read_request(const smc_xtor_read_req_t &request) {
           
            
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

        void push_smc_sram_write_seq() {
          cvm::log(cvm::HIGH, "[smc_xtor] smc_xtor sram write seq\n");
          write_ram = (rng()%0xFFFFFFFFFFFFFFFF);
          for(int i = 0; i < 8;i++){
            smc_wr_txn_q.push({CPL_SRAM_BASE+i*8 ,0xFFFFFFFFFFFFFFFF});
          }

          for(int i = 200; i < 212;i++){
            write_ram = (rng()%0xFFFFFFFFFFFFFFFF);
            smc_wr_txn_q.push({CPL_SRAM_BASE+i*8 ,write_ram});
          }

          for(int i = 504; i < 511;i++){
            write_ram = (rng()%0xFFFFFFFFFFFFFFFF);
            smc_wr_txn_q.push({CPL_SRAM_BASE+i*8 ,write_ram});
          }
          cvm::log(cvm::HIGH, "[smc_xtor] smc_xtor sram write seq\n");
        }

         void push_smc_sram_read_seq() {
          cvm::log(cvm::HIGH, "[smc_xtor] smc_xtor sram read seq\n");
          smc_rd_txn_q.push({CPL_SRAM_BASE,8});
          smc_rd_txn_q.push({CPL_SRAM_BASE+0x8,8});
          cvm::log(cvm::HIGH, "[smc_xtor] smc_xtor sram read seq\n");
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
