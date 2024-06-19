#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "smc_xtor.h"
#include "transactors/axi_sw/axi.h"


DECLARE_string(load);
DECLARE_int32(seed);
DEFINE_int32(smc_reset_seq_start_ticks, 14, "Number of sysmod ticks after which smc should start reset boot sequence");
DEFINE_bool(smc_en, false, "Enable smc transactor");

smc_xtor::smc_xtor(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, size, loc, &smc_xtor::write, &smc_xtor::read, this), axi_mst_loc_l(axi_mst_loc)
{
  rng.seed(FLAGS_seed);
  if (FLAGS_load != "") {
    init_elf(FLAGS_load);
  }
  auto smc_xtor_loc = cvm::topology::get_from_type("SMC_XTOR", 0); 
  cvm::registry::messenger.connect<smc_xtor::smc_ip_data_t>(
            smc_xtor_loc,
            [&](smc_xtor::smc_ip_data_t i) { return this->update_reset_driver_status(i); });
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
  push_smc_boot_seq();
}
void smc_xtor::update_reset_driver_status(smc_ip_data_t i){
  std::cout<<"GOT update from reset driver "<<i.data<<"\n";
}
void smc_xtor::axi_write_granular() {

  axi::a_t aw_txn;
  aw_txn.w    = true;
  aw_txn.id   = 1;
  aw_txn.addr = 0x1234;
  aw_txn.len  = 1;
  aw_txn.size = 1;
  aw_txn.burst = axi::burst_t(0);
  aw_txn.lock  =0;
  aw_txn.cache  =axi::cache_mem_attr_t(0);
  aw_txn.prot  =0;
  aw_txn.qos  =0;
  aw_txn.region  =0;
  aw_txn.atop  =0;
  aw_txn.user  =0;

  cvm::log(cvm::LOW, "[Trickbox] SCMC_XTOR AXI WRITE GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", aw_txn.addr);
  //cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
  cvm::registry::messenger.signal(axi_mst_loc_l, aw_txn);
}
void smc_xtor::send_info_to_reset_driver(){
//send sig to reset driver
auto reset_driver_loc = cvm::topology::get_from_type("RESET_DRIVER", 0);
cvm::registry::messenger.signal(reset_driver_loc, smc_xtor::smc_reset_driver_data_t{443});
cvm::log(cvm::FULL, "[SMC] sending info to reset driver *** 443  \n");

}

void smc_xtor::axi_write() {
  uint64_t addr;
  size_t length = 0x8;
  std::vector<uint8_t> data;
  std::vector<bool> strb;
  smc_wr_t wr;

  wr = smc_wr_txn_q.front();
  smc_wr_txn_q.pop();
  addr = (uint64_t)wr.addr;
  gen_data_strb(wr.addr,wr.data,data,strb);
  cvm::log(cvm::FULL, "[SMC] write {:#X} loc :{:#X} data:{:#X} \n",addr,axi_mst_loc_l,wr.data);
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
}

void smc_xtor::axi_read(uint64_t addr, size_t length,
                          uint32_t id) {
  cvm::log(cvm::FULL, "[SMC] axi read addr= {:#X} id = {} length = {}  \n",addr,id,length);
  transactor::read_t r ;
  r.addr = addr;
  r.length = length;
  auto* l = +[](transactor::read_t r, smc_xtor* dev) -> cvm::messenger::task<void>{
    data_t d;
    co_await dev->read(r,d);
  };
  cvm::registry::messenger.fork(l, r, this);

}


void smc_xtor::write(const transactor::write_t& ) {
  // auto& addr = w.addr;
  // auto& length = w.length;
  // auto& data = w.data;
  // auto& strb = w.strb;

  // cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});

  // return;
}

cvm::messenger::task<void> smc_xtor::read(const transactor::read_t& r, data_t& ) {
   auto& addr = r.addr;
   auto& length = r.length;

  smc_xtor_read_req_t rd;
  cvm::log(cvm::FULL, "[SMC] read addr {:#X} len {} axi transactor loc :{:#X} \n",addr,length,axi_mst_loc_l);

  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);

  smc_xtor_read_req_t smc_xtor_rd;
  smc_xtor_rd.addr = addr;
  smc_xtor_rd.length = length;
  smc_xtor_rd.id = r.id;
  smc_xtor_rd.data = resp.data;  
  smc_read_resp_q.push(smc_xtor_rd); 
  cvm::log(cvm::FULL, "[SMC] read addr {:#X} completed\n",addr);
  read_in_flight = false;
  co_return;
}

void smc_xtor::write_axi_mst(uint64_t addr, size_t, const data_t&, const strb_t&) {
  if (not has_addr(addr))
    return;
  return;
}

void smc_xtor::read_axi_mst(uint64_t addr, size_t, data_t&) {
  if (not has_addr(addr))
    return;
  return;
}

bool smc_xtor::init_elf(const std::string& path) {
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}
