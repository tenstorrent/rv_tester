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
        
      //   template <typename U, typename V>
      //   requires requires(U read, V* dev, transactor::read_t r, device::data_t d)  {{ std::invoke(read, dev, r, d) } -> std::same_as<cvm::messenger::task<>>; }
      //   inline void spawn_read_thread1(U read, V* dev) {
      //   auto* l = +[](cvm::topology::loc_t axi_mst_loc_l, U read, V* dev) -> cvm::messenger::task<void> {
      //        auto channel = cvm::registry::messenger.channel<read_t>(axi_mst_loc_l);

      //        while (1) {
      //           std::cout<<"\nspwn thrd1:coawiat1 \n";
      //            auto r = co_await cvm::registry::messenger.wait<read_t>(channel, [this](const auto& r) { return this->has_addr(r.r.addr); });
      //            data_t data(r.r.length, 0);
      //           std::cout<<"\nspwn thrd1:coawiat 2\n";
      //            co_await std::invoke(read, dev, r.r, data);
      //            cvm::registry::messenger.signal(r.source, transactor::read_response_t{r.r.id, std::move(data)});
      //           std::cout<<"\nspwn thrd1:coawiat 3\n";
      //        }
      //        co_return;
      //    };
      //    cvm::registry::messenger.fork(l, axi_mst_loc_l, std::forward<U>(read), std::forward<V*>(dev));
      //  }
       
        cvm::messenger::task<void> read(const transactor::read_t& , data_t& );
        //cvm::messenger::task<void> read1(const transactor::read_t& , data_t& );
        bool has_addr(uint64_t val)   const { 
          std::cout<<"val"<<val<<"\n";
          return true;
        }
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
            cvm::log(cvm::HIGH, "[SMC] tick {:#X} \n",cnt_tick);
            if(in_boot_seq && ( cnt_tick > 14)){
            cvm::log(cvm::HIGH, "[SMC] IN BOOT SEQ {} reset complition {} \n",in_boot_seq,reset_completion);
              if(!reset_completion){
                  cvm::log(cvm::HIGH, "[SMC] CHK AXI READ RESP FOR 0xC000300C \n");
                  cvm::log(cvm::HIGH, "[SMC] CHK AXI READ RESP FOR 0xC000300C Q size {} \n",smc_read_resp_q.size());
                  if(smc_read_resp_q.size() >0){
                     smc_xtor_read_req_t smc_xtor_rd;
                     smc_xtor_rd = smc_read_resp_q.front();
                     smc_read_resp_q.pop();
                     for (int i = 0; i < 8; i++) {
                     cvm::log(cvm::HIGH, "[SMC] READ RESP data[{}]= {:#X} \n",i,smc_xtor_rd.data[i]);
                     }
                     cvm::log(cvm::HIGH, "[SMC] READ RESP {:#X} \n",smc_xtor_rd.data[0]);
                     if(smc_xtor_rd.data[4] == 0x10){
                      reset_completion = true;
                     }
                  }else{

                    cvm::log(cvm::HIGH, "[SMC] AXI READ 0xC000300C \n");
                    // smc_xtor_read_req_t smc_rd_req;
                    // smc_rd_req.addr = 0xC000300C;
                    // smc_rd_req.length = 4;
                    // smc_rd_req.id = smc_id;
                    // smc_id++;
                    // smc_rd_txn_q.push(smc_rd_req);
                    axi_read(0xC000300C,4,4);
                  }
              }else{
                  cvm::log(cvm::HIGH, "[SMC] DRIVE AXI BOOT SEQ WR  \n");
                 if(smc_boot_wr_txn_q.size()>0){
                   smc_wr_t smc_boot_txn;
                   smc_boot_txn = smc_boot_wr_txn_q.front();
                   smc_boot_wr_txn_q.pop();
                   smc_wr_txn_q.push(smc_boot_txn);
                 }else{
                  //boot done
                  cvm::log(cvm::HIGH, "[SMC]  BOOT SEQ DONE \n");
                  in_boot_seq = false;
                 }
              }
            }
            
            //if(cnt_tick == 40) smc_wr_txn_q.push({CPL_SRAM_BASE + 0x1000,0xFFFF});
            //if(cnt_tick == 41) smc_wr_txn_q.push({CPL_SRAM_BASE + 0x1008,0xFFFF});
            if(smc_wr_txn_q.size() > 0) axi_write();
            

            //if(cnt_tick==22) axi_read(CPL_SRAM_BASE + 0x1000,4,4);
            //if(cnt_tick==29) axi_read(CPL_SRAM_BASE + 0x1008,4,5);

            //while((smc_read_resp_q.size() >0) ){
            //    print_read_request(smc_read_resp_q.front());
            //    smc_read_resp_q.pop();
            //    cvm::log(cvm::HIGH, "[smc] queue size {} \n",smc_read_resp_q.size());
            //  }
            cnt_tick ++;
        }
        
        void print_read_request(const smc_xtor_read_req_t &request) {
            cvm::log(cvm::HIGH, "Address: {} \n",request.addr);
            cvm::log(cvm::HIGH, "Length: {} \n",request.length);
            cvm::log(cvm::HIGH, "ID: {} \n ",request.id );
            
            
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
            cvm::log(cvm::HIGH,"Data: {} \n",output);
            cvm::log(cvm::HIGH,"STRB: {} \n",output2);
        }

        void push_smc_enable_seq() {
          cvm::log(cvm::HIGH, "[smc_xtor] smc_xtor inside enable smc seq\n");
          cvm::log(cvm::HIGH, "[smc_xtor] smc_xtor completed enable smc seq\n");
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
          cvm::log(cvm::HIGH, "[smc_xtor] smc_xtor inside disable smc seq\n");
          cvm::log(cvm::HIGH, "[smc_xtor] smc_xtor completed disable smc seq\n");
        }


        // add max mem size
        smc_xtor(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc);


           /// Initialize memory with elf file.
        bool init_elf(const std::string& path);

};
