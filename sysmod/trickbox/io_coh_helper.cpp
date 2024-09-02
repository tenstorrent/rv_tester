#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"
#include "io_coh_helper.h"
#include "sysmod/sysmod_plusargs.h"


DEFINE_bool(debug_io_coh_helper, false, "Enable internal uc helper debug logging");

io_coh_helper::io_coh_helper(const std::string& tag, uint64_t addr, unsigned, cvm::topology::loc_t loc, mem_manager &m_)
  : subdevice(tag, addr, 0x1000, loc),m_(m_)
{
  rng.seed(FLAGS_seed);
  io_coh_helper_base = addr;
  axi_mst_loc_l  = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST", 0);
  
  reset();
  checkUsage();
}


cvm::messenger::task<void>
io_coh_helper::read(uint64_t addr, size_t length, data_t& data)
{
  mem::datum_t m_data = 0;
  uint32_t word = (uint32_t)m_data;
  cvm::log(cvm::HIGH, "[io_coh_helper] COROUTINE read read addr {:#x} data {:#x} \n",addr,word);
  serializeInt(word, length, data);

  co_return;
}



void
io_coh_helper::read_dev(uint64_t addr, size_t length, data_t& data)
{

 if (not has_addr(addr)){
    cvm::log(cvm::HIGH, "[io_coh_helper] Descarding read request at io_coh_helper since tag {} is not matching \n",tag());
   return;
  }
  cvm::log(cvm::HIGH, "[io_coh_helper] read address: {:#x} \n", addr);
  if(addr ==(io_coh_helper_base + 0x500)) {
    uint64_t read_in_flight_data =  (uint64_t)read_in_flight;
    serializeInt(read_in_flight_data, length, data);
  }
  if(addr ==(io_coh_helper_base + 0x580)) {
    uint64_t backdoor_read_data =  0;
    serializeInt(backdoor_read_data, length, data);
  }
  if(addr ==(io_coh_helper_base + 0x5b0)) {
    uint64_t backdoor_write_status =  (uint64_t)write_in_flight;
    serializeInt(backdoor_write_status, length, data);
  }


  return;
}


io_coh_helper::~io_coh_helper()
{
}

void
io_coh_helper::checkUsage(){

}

void
io_coh_helper::overlay_write(uint64_t addr,uint64_t data)
{
  cvm::log(cvm::HIGH, "[io_coh_helper] overlay_write: {:#x} \n", addr);
     uint32_t length = 0x40;
    std::vector<uint8_t> data1;
    std::vector<bool> strb1;
    for (uint8_t i = 0; i < 64; ++i) {
      data1.push_back(0x0);
      strb1.push_back(0x0);
    }  
    for (uint8_t i = 0; i < 4; ++i) {
      uint8_t currentByte = static_cast<uint8_t>((data >> (8 * i)) & 0xFF);
      data1[i] = currentByte;
      strb1[i] = 0x1;
    }
    cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data1, strb1});
    cvm::topology::loc_t axi_mst_loc_lambda = axi_mst_loc_l;
    write_in_flight = true;
    auto t = std::make_tuple(axi_mst_loc_lambda, std::ref(write_in_flight));
    auto* l = +[](decltype(t) t) -> cvm::messenger::task<void>{
    co_await cvm::registry::messenger.wait<transactor::write_response_t>(std::get<0>(t));
    std::get<1>(t) = false;
  };
  cvm::registry::messenger.fork(l, t);
}




void io_coh_helper::overlay_read(uint64_t addr) {
  cvm::log(cvm::FULL, "[TRACE CFG] axi read addr= {:#X} id = {} length = {}  \n",addr);
   transactor::read_t r ;
   r.addr = addr;
   r.length = 0x40;
   auto* l = +[](transactor::read_t r, io_coh_helper* dev) -> cvm::messenger::task<void>{
     data_t d;
     co_await dev->blocking_read(r,d);
   };
   cvm::registry::messenger.fork(l, r, this);
}

cvm::messenger::task<void> io_coh_helper::blocking_read(const transactor::read_t& r, data_t& ) {
  auto& addr = r.addr;
  auto& length = r.length;
  read_in_flight = true;
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(axi_mst_loc_l);
  read_in_flight = false;

  cvm::log(cvm::HIGH, "[io_coh_helper] blocking read data begin: \n");
  backdoor_read_data = 0;
    for (size_t i = 0; i < 8; ++i) {
        backdoor_read_data |= static_cast<uint64_t>(resp.data[i]) << (8 *  i);
    }
  std::stringstream ss;
    for (const auto &byte : resp.data) {
    ss << static_cast<int>(byte) << " ";
  }
  std::string output = ss.str();
  cvm::log(cvm::HIGH, "[io_coh_helper] blocking read data end:  {}\n",output);
  co_return;
}


void
 io_coh_helper::write(uint64_t addr, size_t , const data_t& data,
 		 const strb_t&)
{
  cvm::log(cvm::HIGH, "[io_coh_helper] write addr {:#x}  \n",addr);
  if (not has_addr(addr))
    return;
  uint64_t t_data = 0;
  deserializeInt(data, t_data);
  cvm::log(cvm::HIGH, "[io_coh_helper] write data {:#x} \n",t_data);

  if (addr == io_coh_helper_base) {
    if (t_data > 0)
      cvm::log(cvm::ERROR, "[io_coh_helper] Only Clearing of io_coh_helper Status allowed, Illegal to set status bit manually \n");
    tx_status = t_data & 0x1;

  } else if(addr == (io_coh_helper_base + 0x100)) {
    tx_addr = t_data;
    cvm::log(cvm::HIGH, "[io_coh_helper] Transfer Start Addr {:#x} \n",t_data);

  } else if(addr ==(io_coh_helper_base + 0x200)) {
    tx_data = t_data;
    cvm::log(cvm::HIGH, "[io_coh_helper] Transfer wdata {:#x}  \n",t_data);

  } else if (addr ==(io_coh_helper_base + 0x300)) {
    cvm::log(cvm::HIGH, "[io_coh_helper] Transfer type {:#x}  \n",t_data);
    tx_type = t_data;
    

  } else if(addr ==(io_coh_helper_base + 0x400)) {
    cvm::log(cvm::HIGH, "[io_coh_helper] Transfer trigger {:#x}  \n",t_data);
    
    if(tx_type == 0){
      overlay_write(tx_addr,tx_data);
    }else if(tx_type == 1){
      backdoor_read_data = 0;
      overlay_read(tx_addr);
    }

  } else if(addr ==(io_coh_helper_base + 0x500)) {
    cvm::log(cvm::HIGH, "[io_coh_helper] Backdoor randpc address: {:#x} Data:{:#x}\n", addr, t_data);
 
  }
}
