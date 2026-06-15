#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "src/sysmod/trickbox/ras_helper.h"
#include "sysmod_plusargs.h"
#include "bridge_plusargs.h"



ras_helper::ras_helper(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc, mem_manager &m_)
  : subdevice(tag, addr, 0x800, loc),m_(m_)
{
  rng.seed(FLAGS_seed);
  ras_helper_base = addr;
  reset();
  checkUsage();
}

void ras_helper::configure()
{
  subdevice::configure();
  cvm::registry::messenger.procedure<ras_helper_backdoor_read_RPC>(loc(), [this] (std::uint64_t addr, std::uint64_t& data) {return this->ras_helper_backdoor_read(addr,data);});
  cvm::registry::messenger.procedure<ras_helper_backdoor_write_RPC>(loc(), [this] (std::uint64_t addr, std::uint64_t data) {return this->ras_helper_backdoor_write(addr,data);});
}

ras_helper::~ras_helper()
{
}

void
ras_helper::checkUsage()
{
 //For Future FLAG usage
}

cvm::messenger::task<void>
ras_helper::read(uint64_t addr, size_t length, data_t& data)
{
  mem::datum_t m_data;
  m_.read(addr, length, &m_data);
  uint32_t word = (uint32_t)m_data;
  cvm::log(cvm::FULL, "[ras_helper] couroutine read addr {:#x} data {:#x} \n",addr,word);
  serializeInt(word, length, data);

  co_return;
}

void
ras_helper::read_dev(uint64_t addr, size_t length, data_t& data)
{
 if (not has_addr(addr)){
   return;
  }

  m_.read(addr, length, data.data());

  for (unsigned i = 0; i < length; i++)
      cvm::log(cvm::FULL, "[ras_helper] read_dev addr {:#x} data {:#x} \n", addr, (uint32_t)data[i]);
  return;
}

bool ras_helper::ras_helper_backdoor_read(uint64_t addr,uint64_t& data){
   if (not has_addr(addr)){
   return false;
  }
   data = local64BStorage[(addr - ras_helper_base)/8];
   cvm::log(cvm::FULL, "[ras_helper]  Backdoor Read addr {:#x} resp data {:#x} \n",addr,data);
   return true;
}

bool ras_helper::ras_helper_backdoor_write(uint64_t addr,uint64_t data){
   if (not has_addr(addr)){
   return false;
  }
  data_t local_data = {0, 0, 0, 0, 0, 0, 0, 0};
  serializeInt(data,8, local_data);
  for (int j=0; j<8; j++) {
      mem::datum_t m_data_p = (mem::datum_t) local_data[j];
      m_.write(addr+j, 1, &m_data_p);
  }
  local64BStorage[(addr - ras_helper_base)/8] = data;
  return true;
}

void
 ras_helper::write(uint64_t addr, size_t , const data_t& data,
 		 const strb_t&)
{
  if (not has_addr(addr))
    return;
  uint64_t t_data = 0;
  deserializeInt(data, t_data);
  
  cvm::log(cvm::FULL, "[ras_helper] addr={:#x} data={:#x} \n", addr, t_data);
  
  for (int j=0; j<8; j++) {
      mem::datum_t m_data_p = (mem::datum_t) data[j];
      m_.write(addr+j, 1, &m_data_p);
  }
  
  local64BStorage[(addr - ras_helper_base)/8] = t_data;
}
