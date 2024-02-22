#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "trace_cfg.h"
#include "transactors/axi_sw/axi.h"


DECLARE_string(load);
DECLARE_int32(seed);


trace_cfg::trace_cfg(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, size, loc, &trace_cfg::write, &trace_cfg::read, this), axi_mst_loc_l(axi_mst_loc)
{
  rng.seed(FLAGS_seed);
  if (FLAGS_load != "") {
    init_elf(FLAGS_load);
  }
 
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
}

void trace_cfg::axi_write() {
  uint64_t addr;
  size_t length = 0x40;
  std::vector<uint8_t> data;
  std::vector<bool> strb;
  trace_wr_t wr;

  wr = trace_wr_txn_q.front();
  trace_wr_txn_q.pop();
  addr = (uint64_t)wr.addr;
  gen_data_strb(wr.addr,wr.data,data,strb);
  
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
}

void trace_cfg::axi_read(uint64_t addr, size_t length,
                          uint32_t id) {
   cvm::registry::messenger.signal(loc(), trace_cfg_read_t{addr, length, id});
}


void trace_cfg::write(const transactor::write_t& ) {
  // auto& addr = w.addr;
  // auto& length = w.length;
  // auto& data = w.data;
  // auto& strb = w.strb;

  // cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});

  // return;
}

cvm::messenger::task<void> trace_cfg::read(const transactor::read_t& r, data_t& ) {
   auto& addr = r.addr;
   auto& length = r.length;

  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);

  trace_cfg_read_req_t trace_cfg_rd;
  trace_cfg_rd.addr = addr;
  trace_cfg_rd.length = length;
  trace_cfg_rd.id = r.id;
  trace_cfg_rd.data = resp.data;  
  trace_read_resp_q.push(trace_cfg_rd); 
  co_return;
}

void trace_cfg::write_axi_mst(uint64_t addr, size_t, const data_t&, const strb_t&) {
  if (not has_addr(addr))
    return;
  return;
}

void trace_cfg::read_axi_mst(uint64_t addr, size_t, data_t&) {
  if (not has_addr(addr))
    return;
  return;
}

bool trace_cfg::init_elf(const std::string& path) {
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}
