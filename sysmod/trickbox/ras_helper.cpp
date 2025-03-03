#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "ras_helper.h"
#include "sysmod/sysmod_plusargs.h"
#include "cosim/bridge/bridge_plusargs.h"


DEFINE_bool(debug_ras_helper, false, "Enable internal uc helper debug logging");

ras_helper::ras_helper(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc, mem_manager &m_)
  : subdevice(tag, addr, 0x200, loc),m_(m_)
{
  rng.seed(FLAGS_seed);
  ras_helper_base = addr;
  reset();
  checkUsage();

  cvm::registry::messenger.procedure<ras_helper_backdoor_read_RPC>(loc, [this] (std::uint64_t addr) {return this->ras_helper_backdoor_read(addr);});
  cvm::registry::messenger.procedure<ras_helper_backdoor_write_RPC>(loc, [this] (std::uint64_t addr, std::uint64_t data) {return this->ras_helper_backdoor_write(addr,data);});


}


cvm::messenger::task<void>
ras_helper::read(uint64_t addr, size_t length, data_t& data)
{
  mem::datum_t m_data;
  m_.read(addr, length, &m_data);
  uint32_t word = (uint32_t)m_data;
  cvm::log(cvm::HIGH, "[ras_helper] couroutine read addr {:#x} data {:#x} \n",addr,word);
  serializeInt(word, length, data);

  co_return;
}




void
ras_helper::read_dev(uint64_t addr, size_t length, data_t& data)
{

 if (not has_addr(addr)){
    cvm::log(cvm::HIGH, "[ras_helper] Descarding read request at ras_helper since tag {} is not matching \n",tag());
   return;
  }

  m_.read(addr, length, data.data());

  for (unsigned i = 0; i < length; i++)
      cvm::log(cvm::HIGH, "[ras_helper] read_dev for loop Read data  {:#x} \n",(uint32_t)data[i]);
  return;
}


ras_helper::~ras_helper()
{
}

void
ras_helper::checkUsage()
{
 //For Future FLAG usage
}

std::uint64_t ras_helper::ras_helper_backdoor_read(uint64_t addr){
 
  cvm::log(cvm::HIGH, "[ras_helper]  Backdoor Read addr {:#x}  \n",addr);
   if (not has_addr(addr)){
    cvm::log(cvm::HIGH, "[ras_helper] Descarding read request at ras_helper since tag {} is not matching \n",tag());
   return -1;
  }
  return localStorage[addr - ras_helper_base];
}

bool ras_helper::ras_helper_backdoor_write(uint64_t addr,uint64_t data){
  cvm::log(cvm::HIGH, "[ras_helper] backdoor write addr {:#x} and Data {:#x}  \n",addr,data);
   if (not has_addr(addr)){
    cvm::log(cvm::HIGH, "[ras_helper] Descarding read request at ras_helper since tag {} is not matching \n",tag());
   return false;
  }
  localStorage[addr - ras_helper_base] = data;
  return true;
}
void
 ras_helper::write(uint64_t addr, size_t , const data_t& data,
 		 const strb_t&)
{
  cvm::log(cvm::HIGH, "[ras_helper] write addr {:#x}  \n",addr);
  if (not has_addr(addr))
    return;
  uint64_t t_data = 0;
  deserializeInt(data, t_data);
  cvm::log(cvm::HIGH, "[ras_helper] write data {:#x} \n",t_data);

  if (addr == ras_helper_base) {
     localStorage[0] = t_data; 
  } 
}
