#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "tboxtrig_mem.h"
#include "sysmod/sysmod_plusargs.h"



tboxtrig_mem::tboxtrig_mem(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc)
  : subdevice(tag, addr, 0x1000, loc)
{
  tboxmem_base_ = addr;
  reset();
}



tboxtrig_mem::~tboxtrig_mem()
{
}

void
 tboxtrig_mem::write(uint64_t addr, size_t , const data_t& data,
 		 const strb_t&)
 {
   cvm::log(cvm::HIGH, "[tboxtrig_mem] write addr {:#x} \n",addr);
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  cvm::log(cvm::HIGH, "[tboxtrig_mem] write addr {:#x} write data {:#x}\n",addr,t_data);
  uint64_t offset;
  offset = (addr - tboxmem_base_)/8;
  cvm::log(cvm::HIGH, "[tboxtrig_mem] offset {:#x} \n",offset);
  if (offset < 1024) {
    tboxmem_[offset] = t_data;
    bool valid;
    if (!client_->whisperPokeMem(0, 0, 'm', addr, 8, t_data, valid)) {
      cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
      return;
    }
   cvm::log(cvm::HIGH, "[tboxtrig_mem] write whisper addr {:#x} data {:#x} \n",addr, t_data);
  }  
 }
void
tboxtrig_mem::read_dev(uint64_t addr, size_t length, data_t& data)
{
    cvm::log(cvm::HIGH, "[tboxtrig_mem] Came in tboxtrigmem \n",tag());
 if (not has_addr(addr)){
    cvm::log(cvm::HIGH, "[tboxtrig_mem] Discarding read request since tag {} is not matching \n",tag());
   return;
  }
  uint64_t offset;
  offset = (addr - tboxmem_base_)/8;
  cvm::log(cvm::HIGH, "[tboxtrig_mem] read: offset {:#x} \n",offset);
  uint64_t d;
  d = offset < 1024 ? tboxmem_[offset] : 0;
  cvm::log(cvm::HIGH, "[tboxtrig_mem] read: data {:#x} \n",d);
  serializeInt(d, length, data);
  return;
}
cvm::messenger::task<void>
tboxtrig_mem::read(uint64_t addr, size_t length, data_t& data)
{
  co_return;
}
