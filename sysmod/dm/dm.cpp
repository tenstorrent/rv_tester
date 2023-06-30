#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "dm.h"
#include "transactors/axi_sw/axi.h"


DECLARE_string(load);


dm::dm(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, size, loc), axi_mst_loc_l(axi_mst_loc)
{
  if (FLAGS_load != "") {
    std::cout << "loading " << FLAGS_load << "\n";
    init_elf(FLAGS_load);
  }
  
}

void dm::write(uint64_t addr, size_t length, const data_t& data, const strb_t&) {
  if (not has_addr(addr))
    return;
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data});

  return;
}

cvm::messenger::task<void> dm::read(uint64_t addr, size_t length, data_t& data) {
  transactor::read_response_t resp_data;
  auto channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l); //replace with local variable once type is known
  if (not has_addr(addr))
    co_return;
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});

  auto r = co_await cvm::registry::messenger.wait<axi::r_t>(channel);
  data = r.data;
 
  co_return;
}

void dm::write_axi_mst(uint64_t addr, size_t, const data_t&, const strb_t&) {
  if (not has_addr(addr))
    return;
  return;
}

void dm::read_axi_mst(uint64_t addr, size_t, data_t&) {
  if (not has_addr(addr))
    return;
  return;
}

bool dm::init_elf(const std::string& path) {
  std::cout<<"[dm]: Device init elf\n";
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}
