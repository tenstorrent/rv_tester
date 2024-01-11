#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "aplic_mmr.h"
#include "transactors/axi_sw/axi.h"


DECLARE_string(load);


aplic_mmr::aplic_mmr(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, size, loc, &aplic_mmr::write, &aplic_mmr::read, this), axi_mst_loc_l(axi_mst_loc)
{
  if (FLAGS_load != "") {
    init_elf(FLAGS_load);
  }
  //std::cout<<"\nCREATING MMR APLIC\n";
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
}

void aplic_mmr::write(const transactor::write_t& w) {
  auto& addr = w.addr;
  auto& length = w.length;
  auto& data = w.data;
  auto& strb = w.strb;
  cvm::log(cvm::HIGH, "[APLIC_MMR] Write Request for Addr: {:#x} \n", addr);
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});

  return;
}

cvm::messenger::task<void> aplic_mmr::read(const transactor::read_t& r, data_t& data) {
  auto& addr = r.addr;
  auto& length = r.length;
  cvm::log(cvm::HIGH, "[APLIC_MMR] Read Request for Addr: {:#x} \n", addr);

  // @Pravin, this needs to be fixed, with a real id store
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);
  data = resp.data;
  

  //if not 64B aligned
  //left shift byte offset
  auto lower_bytes = addr&0x3F;
  if(lower_bytes!=0){
    std::rotate(
                  std::begin(data),
                  std::next(std::begin(data),lower_bytes),
                  std::end(data)
                  );
  }
  
  co_return;
}

void aplic_mmr::write_axi_mst(uint64_t addr, size_t, const data_t&, const strb_t&) {
  if (not has_addr(addr))
    return;
  return;
}

void aplic_mmr::read_axi_mst(uint64_t addr, size_t, data_t&) {
  if (not has_addr(addr))
    return;
  return;
}

bool aplic_mmr::init_elf(const std::string& path) {
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}
