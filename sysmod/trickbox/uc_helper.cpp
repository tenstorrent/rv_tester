#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "uc_helper.h"
#include "sysmod/sysmod_plusargs.h"
#include "cosim/bridge/bridge_plusargs.h"


DEFINE_bool(debug_uc_helper, false, "Enable internal uc helper debug logging");

uc_helper::uc_helper(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc, mem_manager &m_)
  : subdevice(tag, addr, 0x1000, loc),m_(m_)
{
  rng.seed(FLAGS_seed);
  uc_helper_base = addr;
  reset();
  checkUsage();
  auto tbox_loc = cvm::topology::get_from_type("TRICKBOX", 0);
  cvm::registry::messenger.connect<uc_helper::trickbox_mem_req_t>(
            tbox_loc,
            [&](uc_helper::trickbox_mem_req_t i) { return this->update_mem_model(i); });
}


cvm::messenger::task<void>
uc_helper::read(uint64_t addr, size_t length, data_t& data)
{
  mem::datum_t m_data;
  m_.read(addr, length, &m_data);
  uint32_t word = (uint32_t)m_data;
  cvm::log(cvm::HIGH, "[UC_HELPER] COROUTINE read read addr {:#x} data {:#x} \n",addr,word);
  serializeInt(word, length, data);

  co_return;
}

void
uc_helper::update_mem_model(uc_helper::trickbox_mem_req_t& i) {

  uint64_t word = 0;
  int hart = 0;
  bool valid;

  for(int j=0;j<(int)i.length;j++){
    mem::datum_t m_data_p = (mem::datum_t)i.data[j];
    cvm::log(cvm::HIGH,"\nUC_HELPER updating mem model : addr {:#x} Data {:#x} \n",i.addr+j,(uint32_t)i.data[j]);
    m_.write(uc_helper_base + j,1,&m_data_p); //update trickbox mem model
  }
  word = convertToUInt64(i.data);
  cvm::log(cvm::HIGH, "[UC_HELPER] BACKDOOR read_dev read addr {:#x} data {:#x} \n",tx_addr,word);
  uint64_t poke_data = word;

  //Poke same data to whisper memory
  cvm::log(cvm::HIGH, "[UC_HELPER] BACKDOOR whisper poke addr{:#x} poke_data {:#x} \n",uc_helper_base,poke_data);
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, 0, 'm', uc_helper_base,8, poke_data, false, false, valid)|| !valid) && FLAGS_whisper_client_check) {
    cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
    return;
  }{

  cvm::log(cvm::HIGH, "[UC_HELPER] BACKDOOR whisper poke  Successful for addr{:#x} poke_data {:#x} \n",uc_helper_base,poke_data);
  }
  read_flag = 1;
}


void
uc_helper::read_dev(uint64_t addr, size_t length, data_t& data)
{

 if (not has_addr(addr)){
    cvm::log(cvm::HIGH, "[UC_HELPER] Descarding read request at uc_helper since tag {} is not matching \n",tag());
   return;
  }

  m_.read(addr, length, data.data());

  for (unsigned i = 0; i < length; i++)
      cvm::log(cvm::HIGH, "[UC_HELPER] read_dev for loop Read data  {:#x} \n",(uint32_t)data[i]);
  return;
}


uc_helper::~uc_helper()
{
}

void
uc_helper::checkUsage()
{
 //For Future FLAG usage
}


void
 uc_helper::write(uint64_t addr, size_t , const data_t& data,
 		 const strb_t&)
{
  cvm::log(cvm::HIGH, "[UC_HELPER] write addr {:#x}  \n",addr);
  if (not has_addr(addr))
    return;
  uint64_t t_data = 0;
  deserializeInt(data, t_data);
  cvm::log(cvm::HIGH, "[UC_HELPER] write data {:#x} \n",t_data);

  if (addr == uc_helper_base) {
    if (t_data > 0)
      cvm::log(cvm::ERROR, "Error: [UC_Helper] Only Clearing of UC_helper Status allowed, Illegal to set status bit manually \n");
    tx_status = t_data & 0x1;

  } else if(addr == (uc_helper_base + 0x100)) {
    if((t_data & (1ULL << 55))){
      tx_addr = t_data & ~(1ULL <<55);
      cvm::log(cvm::NONE, "[UC_HELPER] Secure Address Detected {:#x} \n",t_data);
      cvm::log(cvm::NONE, "[UC_HELPER] Memory Operations will happen withpout looking at secure (55th) bit in address {:#x} \n",t_data);
    } else {
       tx_addr = t_data;
    }
    cvm::log(cvm::HIGH, "[UC_HELPER] Transfer Start Addr {:#x} \n",t_data);

  } else if(addr ==(uc_helper_base + 0x200)) {
    tx_size = t_data;
    cvm::log(cvm::HIGH, "[UC_HELPER] Transfer Size {:#x} bytes \n",t_data);

  } else if (addr ==(uc_helper_base + 0x300)) {
    cvm::log(cvm::HIGH, "[UC_HELPER] Transfer triggered {:#x}  \n",t_data);
    tx_trigger = 0;
    for (size_t i = 0; i < tx_size; i++) {
      uint32_t pcg_op = rng();
      pcg_op = pcg_op & 0xff;
      uint8_t m_data = (uint8_t)pcg_op;
      uint64_t poke_data = m_data;
      cvm::log(cvm::HIGH, "[UC_HELPER] writing random data to uc area : addr {:#x} data {:#x} \n",tx_addr+i, pcg_op);

      mem::datum_t m_data_p = (mem::datum_t)m_data;

      bool valid;
      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0/*hart*/, 0, 'm', tx_addr + i, 1, poke_data, false, false, valid)) && FLAGS_whisper_client_check) { //Poke same data to whisper memory
       cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
       return;
      }

      //send signal to sysmod memory
      uint64_t push_addr = tx_addr + i;
      std::vector<uint8_t> data_vec={};
      std::vector<bool> strb_vec={};
      data_vec.push_back(m_data_p);
      strb_vec.push_back(1);
      cvm::registry::messenger.signal(loc(), uc_helper_write_t{push_addr, 1, data_vec, strb_vec});
    }
    //Transfer(DMA) writes completed, Indicate completion by setting status bit to non zero
    tx_trigger = 1;
    mem::datum_t m_data_p1 = 0xff;

    m_.write(uc_helper_base, 1, &m_data_p1);

    uint64_t poke_data = m_data_p1;

    cvm::log(cvm::HIGH, "[UC_HELPER] Init of Address Range Completed  \n");
    bool valid;
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0/*hart*/, 0, 'm', uc_helper_base,1, poke_data, false, false, valid)) && FLAGS_whisper_client_check) {
      cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
      return;
    }
    return;

  } else if(addr ==(uc_helper_base + 0x400)) {
    cvm::log(cvm::HIGH, "[UC_HELPER] Backdoor read address {:#x}  \n",tx_addr);
    data_t data_rd = {};
    std::vector<uint8_t> data_vec={};
    std::vector<bool> strb_vec={};

    read_flag = 0;
    cvm::registry::messenger.signal(loc(), uc_helper_read_req_t{tx_addr, 8, data_vec, strb_vec});
    cvm::log(cvm::HIGH, "[UC_HELPER] START Poll for sysmod to send read response  \n");
    while (read_flag == 0)
      cvm::log(cvm::HIGH, "[UC_HELPER] Poll for sysmod to send read response  \n");
    cvm::log(cvm::HIGH, "[UC_HELPER] Recieved  read response From sysmod  \n");

  } else if(addr >= (uc_helper_base + 0x500) && (addr <= (uc_helper_base + 0x600))) {
    cvm::log(cvm::HIGH, "[UC_HELPER] Backdoor rand pc/ld/st address: {:#x} Data:{:#x}\n", addr, t_data);
    for (int j=0; j<8; j++) {
      mem::datum_t m_data_p = (mem::datum_t) data[j];
      m_.write(addr+j, 1, &m_data_p);
    }
  }
}
