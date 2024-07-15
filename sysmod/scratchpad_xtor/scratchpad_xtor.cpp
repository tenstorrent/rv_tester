#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "scratchpad_xtor.h"
#include "sysmod/sysmod_plusargs.h"
#include "rv_tester/rv_tester_plusargs.h"


DEFINE_bool(sp_xtor_en, false, "Enable scratchpad transactor acceses ");
DEFINE_bool(sp_xtor_mmr_prog_en, false, "Enable programming of SP mmr from Scraptchpad transactor ");
DEFINE_bool(sp_xtor_rnd_traffic_en, false, "Enable programming of SP mmr from Scraptchpad transactor ");


scratchpad_xtor::scratchpad_xtor(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, size, loc, &scratchpad_xtor::write, &scratchpad_xtor::read, this), axi_mst_loc_l(axi_mst_loc)
{
  scratchpad_xtor_base = addr;
  rng.seed(FLAGS_seed);
  if (FLAGS_load != "") {
    init_elf(FLAGS_load);
  }
 
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
}
void scratchpad_xtor::axi_write_mmr_data_granular() {
 axi::w_t w_txn;
 
 w_txn.data = {0,0,0,0, 0,0,0,0, 0,0,0,0 ,0,0,0,0,  1,0,0,0, 0,0,0,0 ,0,0,0,0,  0,0,0,0, 0,0,0,0 ,0,0,0,0, 0,0,0,0 ,0,0,0,0 ,0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
 w_txn.strb = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  1,1,1,1, 1,1,1,1 ,0,0,0,0,  0,0,0,0, 0,0,0,0 ,0,0,0,0 ,0,0,0,0 ,0,0,0,0 ,0,0,0,0 ,0,0,0,0, 0,0,0,0, 0,0,0,0};

  w_txn.last = 1;
  cvm::registry::messenger.signal(axi_mst_loc_l, w_txn);
}
void scratchpad_xtor::axi_write_mmr_granular() {

  axi::a_t aw_txn;
  aw_txn.w    = true;
  aw_txn.id   = 12;
  aw_txn.addr = 0x421a0010;
  aw_txn.len  = 0;
  aw_txn.size = 3;
  aw_txn.burst = axi::burst_t(0);
  aw_txn.lock  =0;
  aw_txn.cache  =axi::cache_mem_attr_t(0);
  aw_txn.prot  =0;
  aw_txn.qos  =0;
  aw_txn.region  =0;
  aw_txn.atop  =0;
  aw_txn.user  =8;
  
 
  cvm::log(cvm::LOW, "[Trickbox] SCMC_XTOR AXI MMR WRITE GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", aw_txn.addr);

  cvm::registry::messenger.signal(axi_mst_loc_l, aw_txn);
 
}


void scratchpad_xtor::axi_write_data_granular() {
 axi::w_t w_txn;
 
  w_txn.data = {1,2,3,4, 5,6,7,8, 9,0xa,0xb,0xc, 0xd,0xe,0xf,0,  0xd,0xe,0xa,0xd, 0xb,0xe,0xe,0xf,  0xc,0,0,1, 0xb,0xa,0xd,0xa, 1,1,1,1, 2,2,2,2,  0,0,0,0 ,0,0,0,0 ,0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
  w_txn.strb = {1,1,1,1, 1,1,1,1, 1,1,1,1,       1,1,1,1,             1,1,1,1,       1,1,1,1,         1,1,1,1,      1,1,1,1,    1,1,1,1, 1,1,1,1,  0,0,0,0, 0,0,0,0 ,0,0,0,0 ,0,0,0,0 ,0,0,0,0 ,0,0,0,0};
  ref_data = w_txn.data;
  ref_data_strb = w_txn.strb;
  w_txn.last = 1;
  cvm::registry::messenger.signal(axi_mst_loc_l, w_txn);
}
//void scratchpad_xtor::axi_write_granular() {
void scratchpad_xtor::axi_write_granular(uint64_t addr) {

  axi::a_t aw_txn;
  aw_txn.w    = true;
  aw_txn.id   = 1;
  //aw_txn.addr = 0x60000000;
  aw_txn.addr = addr;
  aw_txn.len  = 0;
  aw_txn.size = 6;
  aw_txn.burst = axi::burst_t(0);
  aw_txn.lock  =0;
  aw_txn.cache  =axi::cache_mem_attr_t(0);
  aw_txn.prot  =2;
  aw_txn.qos  =0;
  aw_txn.region  =0;
  aw_txn.atop  =0;
  aw_txn.user  =0;
  
 
  cvm::log(cvm::LOW, "[Trickbox] SCMC_XTOR AXI WRITE GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", aw_txn.addr);

  cvm::registry::messenger.signal(axi_mst_loc_l, aw_txn);
 
}
cvm::messenger::task<void> scratchpad_xtor::axi_read_granular(const transactor::read_t& r , data_t&) {

  axi::a_t ar_txn;
  ar_txn.w    = false;
  ar_txn.id   = 2;
  //ar_txn.addr = 0x60000000;
  ar_txn.addr = r.addr;
  ar_txn.len  = 0;
  ar_txn.size = 6;
  ar_txn.burst = axi::burst_t(0);
  ar_txn.lock  =0;
  ar_txn.cache  =axi::cache_mem_attr_t(0);
  ar_txn.prot  =2;
  ar_txn.qos  =0;
  ar_txn.region  =0;
  ar_txn.atop  =0;
  ar_txn.user  =0;
  
 
  cvm::log(cvm::LOW, "[Trickbox] SCMC_XTOR AXI READ GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", ar_txn.addr);

  cvm::registry::messenger.signal(axi_mst_loc_l, ar_txn);

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);

  scratchpad_xtor_read_req_t scratchpad_xtor_rd;
  scratchpad_xtor_rd.addr =ar_txn.addr ;
  scratchpad_xtor_rd.length = 0 ;
  scratchpad_xtor_rd.id = 2;
  scratchpad_xtor_rd.data = resp.data;  
  for (int i = 0; i < int(resp.data.size()); ++i) {

         cvm::log(cvm::FULL, "[scratchpad] read resp byte {} =  {:#X} \n",i,uint32_t(resp.data[i]));
         if(ref_data_strb[i]){
          if(ref_data[i] != resp.data[i]){
            cvm::log(cvm::ERROR, "Error: [Scratchpad] Read data {:#X}  not matching with previously written data {:#X} ",uint32_t(ref_data[i]),uint32_t(resp.data[i]));
          }
         }
    }

  scratchpad_read_resp_q.push(scratchpad_xtor_rd); 
  cvm::log(cvm::FULL, "[scratchpad] read addr {:#X} completed\n",scratchpad_xtor_rd.addr);
  read_in_flight = false;
  co_return;
 
}
void scratchpad_xtor::axi_write() {
  uint64_t addr;
  size_t length = 0x40;
  std::vector<uint8_t> data = {1,2,3,4};
  std::vector<bool> strb= {1,1,1,1};

  addr = 0x60000000;

  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
}


void scratchpad_xtor::axi_read(uint64_t addr, size_t length,
                          uint32_t id) {
  cvm::log(cvm::FULL, "[scratchpad] axi read addr= {:#X} id = {} length = {}  \n",addr,id,length);
  transactor::read_t r ;
  r.addr = addr;
  r.length = length;
  auto* l = +[](transactor::read_t r, scratchpad_xtor* dev) -> cvm::messenger::task<void>{
    data_t d;
    //co_await dev->read(r,d);
    co_await dev->axi_read_granular(r,d);
  };
  cvm::registry::messenger.fork(l, r, this);

}


void scratchpad_xtor::write(const transactor::write_t& w ) {
  auto& addr = w.addr;
  auto& length = w.length;
  auto& data = w.data;
  //auto& strb = w.strb;
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  cvm::log(cvm::LOW, "[Trickbox] SCRATCHPAD_XTOR write - addr={:#x} len= {} data={:#x}  \n", addr, length,t_data);
  if (addr == scratchpad_xtor_base) {
    cvm::log(cvm::LOW, "[Trickbox] SCRATCHPAD_XTOR  base write - addr={:#x} data={:#x}\n", addr, t_data);
    trigger_addr = t_data;
  }else if(addr == scratchpad_xtor_base + 0x40){
    trigger_mode = t_data;
    cvm::log(cvm::LOW, "[Trickbox] SCRATCHPAD_XTOR base + 0x40 write - addr={:#x} data={:#x}\n", addr, t_data);
  }else if(addr == scratchpad_xtor_base + 0x80){
    trigger_flag = t_data != 0;
    cvm::log(cvm::LOW, "[Trickbox] SCRATCHPAD_XTOR base + 0x80 write - addr={:#x} data={:#x}\n", addr, t_data);
  }else if(addr == scratchpad_xtor_base + 0x120){
    sp_mmr_base = t_data;
    cvm::log(cvm::LOW, "[Trickbox] SCRATCHPAD_XTOR base + 0x120 write - addr={:#x} data={:#x}\n", addr, t_data);

  }

}

cvm::messenger::task<void> scratchpad_xtor::read(const transactor::read_t& r, data_t& ) {
   auto& addr = r.addr;
   auto& length = r.length;

  scratchpad_xtor_read_req_t rd;
  cvm::log(cvm::FULL, "[scratchpad] read addr {:#X} len {} axi transactor loc :{:#X} \n",addr,length,axi_mst_loc_l);

  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);

  scratchpad_xtor_read_req_t scratchpad_xtor_rd;
  scratchpad_xtor_rd.addr = addr;
  scratchpad_xtor_rd.length = length;
  scratchpad_xtor_rd.id = r.id;
  scratchpad_xtor_rd.data = resp.data;  

  scratchpad_read_resp_q.push(scratchpad_xtor_rd); 
  cvm::log(cvm::FULL, "[scratchpad] read addr {:#X} completed\n",addr);
  read_in_flight = false;
  co_return;
}


void scratchpad_xtor::write_axi_mst(uint64_t addr, size_t, const data_t&, const strb_t&) {
  if (not has_addr(addr))
    return;
  return;
}

void scratchpad_xtor::read_axi_mst(uint64_t addr, size_t, data_t&) {
  if (not has_addr(addr))
    return;
  return;
}


bool scratchpad_xtor::init_elf(const std::string& path) {
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}
