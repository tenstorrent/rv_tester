#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "pm_nw_xtor.h"
#include "transactors/axi_sw/axi.h"
#include "sysmod/sysmod_plusargs.h"


pm_nw_xtor::pm_nw_xtor(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, size, loc, &pm_nw_xtor::write, &pm_nw_xtor::read, this), axi_mst_loc_l(axi_mst_loc)
{
  rng.seed(FLAGS_seed);
  if (FLAGS_load != "") {
    init_elf(FLAGS_load);
  }
 
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
  auto pm_nw_loc = cvm::topology::get_from_type("PM_NW_XTOR", 0); 
  cvm::registry::messenger.connect<pm_common::pm_common_tx_t>(
            pm_nw_loc,
            [&](pm_common::pm_common_tx_t i) { return this->write_pm_nw_xtor_q(i); });
}

void pm_nw_xtor::axi_write() {
  uint64_t addr;
  size_t length = 0x40;
  std::vector<uint8_t> data;
  std::vector<bool> strb;
  pm_nw_wr_t wr;

  wr = pm_nw_wr_txn_q.front();
  pm_nw_wr_txn_q.pop();
  addr = (uint64_t)wr.addr;
  gen_data_strb(wr.addr,wr.data,data,strb);
  addr = addr & 0xFFFFFFC0;
  
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
}

void pm_nw_xtor::axi_read(uint64_t addr, size_t length,
                          uint32_t id) {
   cvm::registry::messenger.signal(loc(), pm_nw_xtor_read_t{addr, length, id});
}


void pm_nw_xtor::write(const transactor::write_t& ) {
  // auto& addr = w.addr;
  // auto& length = w.length;
  // auto& data = w.data;
  // auto& strb = w.strb;

  // cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});

  // return;
}

cvm::messenger::task<void> pm_nw_xtor::read(const transactor::read_t& r, data_t& ) {
   auto& addr = r.addr;
   //auto& addr = r.addr;
   auto& length = r.length;

  //cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{0x123456, 2});

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);

  pm_nw_xtor_read_req_t pm_nw_xtor_rd;
  pm_nw_xtor_rd.addr = addr;
  pm_nw_xtor_rd.length = length;
  pm_nw_xtor_rd.id = r.id;
  pm_nw_xtor_rd.data = resp.data;  
  pm_nw_read_resp_q.push(pm_nw_xtor_rd); 
  co_return;
}

void pm_nw_xtor::write_axi_mst(uint64_t addr, size_t, const data_t&, const strb_t&) {
  if (not has_addr(addr))
    return;
  return;
}

void pm_nw_xtor::read_axi_mst(uint64_t addr, size_t, data_t&) {
  if (not has_addr(addr))
    return;
  return;
}

bool pm_nw_xtor::init_elf(const std::string& path) {
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}
