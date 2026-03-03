#include "mmr_txn_router.h"

mmr_txn_router::mmr_txn_router(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, size, loc, &mmr_txn_router::write, &mmr_txn_router::read, this), axi_mst_loc_l(axi_mst_loc)
{
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
  cvm::log(cvm::HIGH, " [mmr_txn_router] Constructor \n");
}


cvm::messenger::task<void> mmr_txn_router::read(const transactor::read_t& r, data_t& data) {
  auto& addr = r.addr;
  auto& length = r.length;
  uint32_t value = 0;

  //aplic_->read(addr, size, value);
  //reroute mmr tead
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);
  data = resp.data;
  serializeInt(value, length, data);
  //cvm::log(cvm::HIGH, " [mmr_txn_router] routing mmr read back to overlay: Addr = {:#x}, Data = {:#x}\n", addr, data);
  co_return;
}


void
mmr_txn_router::write(const transactor::write_t& w)
{
  uint64_t addr = w.addr;
  size_t length = w.length;
  uint32_t value;
  auto& data = w.data;
  auto& strb = w.strb;
  deserializeInt(w.data, value);
  cvm::log(cvm::HIGH, " [mmr_txn_router] routing mmr write back to overlay: Addr = {:#x}\n", addr);
  //re route mmr write
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
  
}
