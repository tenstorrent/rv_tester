#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "evt_trigger.h"
#include "sysmod/sysmod_plusargs.h"
#include "cosim/bridge/bridge_plusargs.h"


evt_trigger::evt_trigger(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc)
  : subdevice(tag, addr, 0x1000, loc)
{
  tboxmem_base_ = addr;
  reset();
}



evt_trigger::~evt_trigger()
{
}

void
 evt_trigger::write(uint64_t addr, size_t , const data_t& data,
 		 const strb_t&)
 {
   cvm::log(cvm::HIGH, "[evt_trigger] write addr {:#x} \n",addr);
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  cvm::log(cvm::HIGH, "[evt_trigger] write addr {:#x} write data {:#x}\n",addr,t_data);
  uint64_t offset;
  offset = (addr - tboxmem_base_)/8;
  cvm::log(cvm::HIGH, "[evt_trigger] offset {:#x} \n",offset);
  if (offset < 1024) {
    tboxmem_[offset] = t_data;
    bool valid;
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', addr, 8, t_data, valid)|| !valid) && FLAGS_whisper_client_check) {
      cvm::log(cvm::MEDIUM, "Whisper client check : {}\n",FLAGS_whisper_client_check);
      cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
      return;
    }
   cvm::log(cvm::HIGH, "[evt_trigger] write whisper addr {:#x} data {:#x} \n",addr, t_data);
  }  
 }
void
evt_trigger::read_dev(uint64_t addr, size_t length, data_t& data)
{
    cvm::log(cvm::HIGH, "[evt_trigger] Came in tboxtrigmem tag {}\n",tag());
 if (not has_addr(addr)){
    cvm::log(cvm::HIGH, "[evt_trigger] Discarding read request since tag {} is not matching \n",tag());
   return;
  }
  uint64_t offset;
  offset = (addr - tboxmem_base_)/8;
  cvm::log(cvm::HIGH, "[evt_trigger] read: offset {:#x} \n",offset);
  uint64_t d;
  d = offset < 1024 ? tboxmem_[offset] : 0;
  cvm::log(cvm::HIGH, "[evt_trigger] read: data {:#x} \n",d);
  serializeInt(d, length, data);
  return;
}
cvm::messenger::task<void>
evt_trigger::read(uint64_t addr, size_t length, data_t& data)
{
  co_return;
}
