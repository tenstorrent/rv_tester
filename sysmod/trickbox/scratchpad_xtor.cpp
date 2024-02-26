#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "scratchpad_xtor.h"


DECLARE_string(load);
DECLARE_int32(seed);


scratchpad_xtor::scratchpad_xtor(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc,cvm::topology::loc_t axi_mst_loc)
  : subdevice(tag, addr, 0x4000 /* size */, loc), axi_mst_loc_l(axi_mst_loc) 
{
  scratchpad_xtor_base = addr;
  hartCount_l = hartCount;
  rng.seed(FLAGS_seed);
  if (FLAGS_load != "") {
    init_elf(FLAGS_load);
  }
 
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
}

void scratchpad_xtor::axi_write() {
  uint64_t addr;
  size_t length = 0x40;
  std::vector<uint8_t> data;
  std::vector<bool> strb;
  trace_wr_t wr;

 // wr = trace_wr_txn_q.front();
 // trace_wr_txn_q.pop();
  addr = (uint64_t)wr.addr;
  gen_data_strb(wr.addr,wr.data,data,strb);
  
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
}

void scratchpad_xtor::axi_read(uint64_t addr, size_t length,
                          uint32_t id) {
   cvm::registry::messenger.signal(loc(), scratchpad_xtor_read_t{addr, length, id});
}


void
scratchpad_xtor::write(uint64_t addr, size_t, const data_t& data,
		 const strb_t&)
{
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  cvm::log(cvm::LOW, "[Trickbox] SCRATCHPAD_XTOR write - addr={:#x} data={:#x}\n", addr, t_data);
  if (addr == scratchpad_xtor_base) {
    cvm::log(cvm::LOW, "[Trickbox] SCRATCHPAD_XTOR write - addr={:#x} data={:#x}\n", addr, t_data);
  }else if(addr == scratchpad_xtor_base + 0x100){

    cvm::log(cvm::LOW, "[Trickbox] SCRATCHPAD_XTOR write - addr={:#x} data={:#x}\n", addr, t_data);
  }else if(addr == scratchpad_xtor_base + 0x200){
    cvm::log(cvm::LOW, "[Trickbox] SCRATCHPAD_XTOR write - addr={:#x} data={:#x}\n", addr, t_data);

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

  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);

  scratchpad_xtor_read_req_t scratchpad_xtor_rd;
  scratchpad_xtor_rd.addr = addr;
  scratchpad_xtor_rd.length = length;
  scratchpad_xtor_rd.id = r.id;
  scratchpad_xtor_rd.data = resp.data;  
  //trace_read_resp_q.push(scratchpad_xtor_rd); 
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

void
scratchpad_xtor::read_dev(uint64_t addr, size_t , data_t& )
{
  cvm::log(cvm::HIGH, "[Trickbox] IMSIC read addr={:#x} \n", addr);
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
