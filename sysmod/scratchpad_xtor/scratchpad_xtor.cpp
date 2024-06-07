#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "scratchpad_xtor.h"


DEFINE_bool(sp_xtor_en, false, "Enable ");
DECLARE_string(load);
DECLARE_int32(seed);

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
void scratchpad_xtor::axi_write_data_granular() {
 axi::w_t w_txn;
 
  w_txn.data = {0,0,0,0,1};
  w_txn.strb = {0,0,0,0,1};

  w_txn.last = 1;
  cvm::registry::messenger.signal(axi_mst_loc_l, w_txn);
}
void scratchpad_xtor::axi_write_granular() {

  axi::a_t aw_txn;
  aw_txn.w    = true;
  aw_txn.id   = 1;
  aw_txn.addr = 0x60000000;
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
    cvm::log(cvm::LOW, "[Trickbox] SCRATCHPAD_XTOR AXI READ - addr={:#x} SEND SYSMOD SIGNAL\n", addr);
   cvm::registry::messenger.signal(loc(), scratchpad_xtor_read_t{addr, length, id});
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
  // auto& addr = w.addr;
  // auto& length = w.length;
  // auto& data = w.data;
  // auto& strb = w.strb;

  // cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});

  // return;
}

cvm::messenger::task<void> scratchpad_xtor::read(const transactor::read_t& r, data_t& ) {
   auto& addr = r.addr;
   auto& length = r.length;
    cvm::log(cvm::LOW, "[Trickbox] SCRATCHPAD_XTOR READ - addr={:#x} \n", addr);

  //cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{0x123456, 2});

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);

  scratchpad_xtor_read_req_t scratchpad_xtor_rd;
  scratchpad_xtor_rd.addr = addr;
  scratchpad_xtor_rd.length = length;
  scratchpad_xtor_rd.id = r.id;
  scratchpad_xtor_rd.data = resp.data;  
  
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
