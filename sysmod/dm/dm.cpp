#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "dm.h"
#include "transactors/axi_sw/axi.h"


DECLARE_string(load);


dm::dm(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, size, loc, &dm::write, &dm::read, this), axi_mst_loc_l(axi_mst_loc)
{
  if (FLAGS_load != "") {
    std::cout << "loading " << FLAGS_load << "\n";
    init_elf(FLAGS_load);
  }
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
}

void dm::write(const transactor::write_t& w) {
  auto& addr = w.addr;
  auto& length = w.length;
  auto& data = w.data;
  auto& strb = w.strb;

  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});

  return;
}

cvm::messenger::task<void> dm::read(const transactor::read_t& r, data_t& data) {
  auto& addr = r.addr;
  auto& length = r.length;

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
  // cvm::log(cvm::HIGH, " [Rv_Tester] rv_tester reads addr:{:#x} from transactor, length :{:#x}\n",r.addr,r.length);
  // printf("element : ");
  // for (auto element = data.rbegin(); element!= data.rend(); ++element) {
  //     printf("%02x",*element);
  //   }
  // std::cout << std::endl;
  

  
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
