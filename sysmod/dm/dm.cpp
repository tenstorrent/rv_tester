#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "dm.h"


DECLARE_string(load);


dm::dm(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, size, loc), axi_mst_loc_l(axi_mst_loc)
{
  if (FLAGS_load != "") {
    std::cout << "loading " << FLAGS_load << "\n";
    init_elf(FLAGS_load);
  }
}

void dm::write(uint64_t addr, size_t length, const data_t& data, const strb_t& strb) {
  std::cout<<"PRT DM WRITE "<<std::hex<<addr<<" \n";
  if (not has_addr(addr))
    return;
  //cvm::registry::messenger.signal(loc(), transactor::write_request_t{addr, length, data});
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data});
  
  return;
}

void dm::read(uint64_t addr, size_t length, data_t& data) {
  std::cout<<"PRT DM READ "<<std::hex<<addr<<" \n";
  if (not has_addr(addr))
    return;
  //cvm::registry::messenger.signal(loc(), transactor::read_request_t{addr, length});
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});

  return;
}

void dm::write_axi_mst(uint64_t addr, size_t length, const data_t& data, const strb_t& strb) {
  if (not has_addr(addr))
    return;
  //connect axi_mst dpi here
  // struct write_request_t {
  //       uint64_t addr;
  //       size_t length;
  //       std::vector<uint8_t> data;
  //   };
  //cvm::registry::messenger.signal(loc(), transactor::write_request_t{addr, length, data});
  return;
}

void dm::read_axi_mst(uint64_t addr, size_t length, data_t& data) {
  if (not has_addr(addr))
    return;
  //connect axi mst dpi hereo
  //cvm::registry::messenger.signal(loc(), transactor::read_request_t{addr, length});
  //cvm::registry::messenger.connect(loc(), transactor::read_response_t{data});

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
