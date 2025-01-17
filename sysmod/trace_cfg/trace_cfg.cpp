#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/random.hpp"
#include "trace_cfg.h"
#include "transactors/axi_sw/axi.h"
#include "sysmod/sysmod_plusargs.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <random>


trace_cfg::trace_cfg(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, size, loc, &trace_cfg::write, &trace_cfg::read, this), axi_mst_loc_l(axi_mst_loc)
{
  rng.seed(FLAGS_seed);
  if (FLAGS_load != "") {
    init_elf(FLAGS_load);
  }
 
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
}


void trace_cfg::axi_write_mmr_granular() {
  uint64_t addr;
  std::vector<uint8_t> data(64);
  std::vector<bool> strb(64);
  axi::a_t aw_txn;
  axi::w_t w_txn;
  trace_wr_t wr;

  wr = trace_wr_txn_q.front();
  trace_wr_txn_q.pop();
  addr = (uint64_t)wr.addr;
  for (uint8_t i = 0; i < 64; ++i) {
          data.push_back(0x0);
          strb.push_back(0x0);
    }  
  if(FLAGS_strobe_type == 4) {
     gen_data_strb(wr.addr,wr.data,data,strb);
  }
  write_in_flight = true; 
  aw_txn.w    = true;
  aw_txn.id   =  id_val1++;
  aw_txn.addr = addr;
  aw_txn.len  = 0x40;
  if(FLAGS_strobe_type == 0) {
  cvm::log(cvm::LOW, "[overlay axi] ENTERED STROBE\n");
  aw_txn.size = 1;
  // strb_temp = 0x3;
  std::bitset<64> bits;
  bits.set(0);
  bits.set(1);
  data[0] = wr.data & 0x00FF ; 
  data[1] = wr.data & 0xFF00 ; 
    for (int i = 0; i < 64; i++) {
        strb[i] = bits[i];  // Copy each bit
    }
  } else if ( FLAGS_strobe_type ==1) {
  aw_txn.size = 1;
  std::bitset<64> bits;  
  bits.set(2);
  bits.set(3);
  data[2] = wr.data & 0x00FF0000 ; 
  data[3] = wr.data & 0xFF000000 ; 
    for (int i = 0; i < 64; i++) {
        strb[i] = bits[i];  // Copy each bit
    }
  } else if ( FLAGS_strobe_type ==2) {
  aw_txn.size = 1;
  std::bitset<64> bits;  
  bits.set(4);
  bits.set(5);
  data[4] = wr.data & 0x00FF00000000 ; 
  data[5] = wr.data & 0xFF0000000000 ; 
    for (int i = 0; i < 64; i++) {
        strb[i] = bits[i];  // Copy each bit
    } 
  } else if ( FLAGS_strobe_type ==3) {
  aw_txn.size = 1;
  std::bitset<64> bits;  
  bits.set(6);
  bits.set(7);
  data[6] = wr.data & 0x00FF000000000000 ; 
  data[7] = wr.data & 0xFF00000000000000 ; 
    for (int i = 0; i < 64; i++) {
        strb[i] = bits[i];  // Copy each bit
    } 
  } else {
  aw_txn.size = 3;
  }

  aw_txn.burst = axi::burst_t(0);
  aw_txn.lock  =0;
  aw_txn.cache  =axi::cache_mem_attr_t(0);
  aw_txn.prot  =0;
  aw_txn.qos  =0;
  aw_txn.region  =0;
  aw_txn.atop  =0;
  aw_txn.user  =3;
  
  w_txn.data = data ;
  w_txn.strb = strb; 
  w_txn.last = 1;
 
  cvm::log(cvm::LOW, "Address Initiated from AXI write\n", aw_txn.addr);

  cvm::registry::messenger.signal(axi_mst_loc_l, aw_txn);
  cvm::registry::messenger.signal(axi_mst_loc_l, w_txn);
  cvm::topology::loc_t axi_mst_loc_lambda = axi_mst_loc_l;
  auto t = std::make_tuple(axi_mst_loc_lambda, std::ref(write_in_flight));
  auto* l = +[](decltype(t) t) -> cvm::messenger::task<void>{
    co_await cvm::registry::messenger.wait<transactor::write_response_t>(std::get<0>(t));
    //auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(axi_mst_loc_lambda);
    std::get<1>(t) = false;
  };
  cvm::registry::messenger.fork(l, t);

 
}




void trace_cfg::axi_write() {
  uint64_t addr;
  size_t length = 0x40;
  std::vector<uint8_t> data;
  std::vector<bool> strb;
  trace_wr_t wr;

  wr = trace_wr_txn_q.front();
  trace_wr_txn_q.pop();
  addr = (uint64_t)wr.addr;
  gen_data_strb(wr.addr,wr.data,data,strb);
  addr = addr & 0xFFFFFFC0;
  write_in_flight = true; 
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
  cvm::topology::loc_t axi_mst_loc_lambda = axi_mst_loc_l;
  auto t = std::make_tuple(axi_mst_loc_lambda, std::ref(write_in_flight));
  auto* l = +[](decltype(t) t) -> cvm::messenger::task<void>{
    co_await cvm::registry::messenger.wait<transactor::write_response_t>(std::get<0>(t));
    //auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(axi_mst_loc_lambda);
    std::get<1>(t) = false;
  };
  cvm::registry::messenger.fork(l, t);
  
}

cvm::messenger::task<void>trace_cfg::axi_write_blocking() {
  uint64_t addr;
  size_t length = 0x40;
  std::vector<uint8_t> data;
  std::vector<bool> strb;
  trace_wr_t wr;

  wr = trace_wr_txn_q.front();
  trace_wr_txn_q.pop();
  addr = (uint64_t)wr.addr;
  gen_data_strb(wr.addr,wr.data,data,strb);
  addr = addr & 0xFFFFFFC0;
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
  co_await cvm::registry::messenger.wait<transactor::write_response_t>(axi_mst_loc_l);
  co_return;
}

void trace_cfg::axi_read(uint64_t addr, size_t length,
                          uint32_t id) {
   //cvm::registry::messenger.signal(loc(), trace_cfg_read_t{addr, length, id});
   //cvm::log(cvm::HIGH, "[axi_read prints] cnt_tick {} trace_start_cnt {} \n",cnt_tick,trace_start_cnt);
    cvm::log(cvm::FULL, "[TRACE CFG] axi read addr= {:#X} id = {} length = {}  \n",addr,id,length);
  transactor::read_t r ;
  r.addr = addr;
  r.length = length;
  r.id     = id;
  auto* l = +[](transactor::read_t r, trace_cfg* dev) -> cvm::messenger::task<void>{
    data_t d;
    co_await dev->read(r,d);
  };
  cvm::registry::messenger.fork(l, r, this);
}

std::vector<uint8_t> trace_cfg::convert_to_byte_array(const std::vector<uint64_t>& dword_array) {
  std::vector<uint8_t> result(dword_array.size() * sizeof(uint64_t));
  std::copy(reinterpret_cast<const uint8_t*>(dword_array.data()),
            reinterpret_cast<const uint8_t*>(dword_array.data()) + dword_array.size() * sizeof(uint64_t),
            result.begin());
  return result;
}

//cvm::messenger::task<void> trace_cfg::write_blocking(uint64_t addr, size_t lenght ,sz, uint64_t data) {
//  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, SZ_8B, byte_array, strb});
//  auto resp = co_await cvm::registry::messenger.wait<transactor::write_response_t>(axi_mst_loc_l);
//  co_return;
//}

void trace_cfg::write(const transactor::write_t& ) {
}

auto trace_cfg::pickRandomElements(uint32_t n) -> trace_cfg::random_list {
    random_list picks;
    cvm::log(cvm::FULL, "[overlay axi] no of mmrs selected {} \n",n);
    for (uint32_t i = 0; i < n; i++) picks.push_back(mmr::list[rng() % mmr::list.size()]);

    return picks;
}

cvm::messenger::task<void> trace_cfg::read(const transactor::read_t& r, data_t& ) {
  auto& addr = r.addr;
  auto& length = r.length;
  read_in_flight = true;
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);

  trace_cfg_read_req_t trace_cfg_rd;
  trace_cfg_rd.addr = addr;
  trace_cfg_rd.length = length;
  trace_cfg_rd.id = r.id;
  trace_cfg_rd.data = resp.data;  
  trace_read_resp_q.push(trace_cfg_rd); 
  read_in_flight = false;
  co_return;
}

void trace_cfg::write_axi_mst(uint64_t addr, size_t, const data_t&, const strb_t&) {
  if (not has_addr(addr))
    return;
  return;
}

void trace_cfg::read_axi_mst(uint64_t addr, size_t, data_t&) {
  if (not has_addr(addr))
    return;
  return;
}

void trace_cfg::gen_data_strb(uint64_t addr, uint32_t value, data_t& wdata, std::vector<bool>& strb) {
    uint8_t b_index =  static_cast<uint8_t>(addr & 0x3F);

    for (uint8_t i = 0; i < 64; ++i) {
          wdata.push_back(0x0);
          strb.push_back(0x0);
    }  
    for (uint8_t i = 0; i < 4; ++i) {
          uint8_t currentByte = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
          wdata[i+b_index] = currentByte;
          strb[i+b_index] = 0x1;
    }  
}


void trace_cfg::push_axi_mmr_seq() {
  cvm::log(cvm::HIGH, "[overlay axi] overlay axi write seq\n");
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_COUNTER3_CFG,0xFF});
  trace_wr_txn_q.push({mmr::CDBG_NODE3_EAP1_CFG,0xFF});
  cvm::log(cvm::HIGH, "[overlay axi] overlay axi write seq completed\n");
}

void trace_cfg::push_default_rd_seq() {
  cvm::log(cvm::HIGH, "[overlay axi] overlay RD SCRATCH Seq\n");
  trace_misc_rd_txn_q.push({trace_mmr::DM_SCRATCHPAD,8,0xafafafafafafafaf});
  trace_misc_rd_txn_q.push({trace_mmr::CR_SCRATCHPAD,8,0xbfbfbfbfbfbfbfbf});
  trace_misc_rd_txn_q.push({trace_mmr::SW_SCRATCHPAD,8,0xcfcfcfcfcfcfcfcf});
  trace_misc_rd_txn_q.push({trace_mmr::TR_SCRATCHPAD_LOW,4,0xefefefef});
  trace_misc_rd_txn_q.push({trace_mmr::AC_SCRATCHPAD,8,0xdfdfdfdfdfdfdfdf});
  cvm::log(cvm::HIGH, "[overlay axi] overlay RD SCRATCH Seq completed\n");

}
void trace_cfg::push_rd_scratchpad_seq() {
  trace_misc_rd_txn_q.push({trace_mmr::AC_SCRATCHPAD,8,0xdfdfdfdfffffffff});
  trace_misc_rd_txn_q.push({trace_mmr::CR_SCRATCHPAD,8,0xbfbfbfbfffffffff});
  trace_misc_rd_txn_q.push({trace_mmr::TR_SCRATCHPAD_LOW,4,0xffffffff});
  overlay_in_progress =1;
  cvm::log(cvm::HIGH, "[overlay axi] overlay RD SCRATCH Seq completed\n");
}

void trace_cfg::push_wr_scratchpad_seq() {
  cvm::log(cvm::LOW, "[overlay axi] overlay WR/RD Scratch seq\n");
  trace_wr_txn_q.push({trace_mmr::AC_SCRATCHPAD,0xFFFFFFFF});
  trace_wr_txn_q.push({trace_mmr::CR_SCRATCHPAD,0xFFFFFFFF});
  trace_wr_txn_q.push({trace_mmr::TR_SCRATCHPAD_LOW,0xFFFFFFFF});
}

void trace_cfg::push_read_axi_mmr_seq() {
  cvm::log(cvm::HIGH, "[overlay axi] overlay axi read seq\n");
  trace_misc_rd_txn_q.push({trace_mmr::CDBG_CLA_COUNTER3_CFG,8,0});
  trace_misc_rd_txn_q.push({mmr::CDBG_NODE3_EAP1_CFG,8,0});
  cvm::log(cvm::HIGH, "[overlay axi] overlay axi read seq completed\n");
}

void trace_cfg::read_axi_pointers(){
  cvm::log(cvm::HIGH, "[overlay axi]reading WRITE/READ pointers\n");
  trace_misc_rd_txn_q.push({trace_mmr::TR_DST_CONTROL,4,0});
  trace_misc_rd_txn_q.push({trace_mmr::CDBG_NTRACE_CFG,8,0});
}

void trace_cfg::push_random_axi_read(const random_list&  elements){
  cvm::log(cvm::HIGH, "[overlay axi regress] success reading with {} elements \n",elements.size());

  // Loop through elements and write to file
  for(int i = 0; i < 15;i++){
    for (const auto& e : elements) {
        cvm::log(cvm::FULL, "[overlay axi reads] register {} address {:#x} size {} \n", e.name, e.value, 64);
        trace_misc_rd_txn_q.push({e.value,8,0});
    }
  }
}

void trace_cfg::print_read_request(const trace_cfg_read_req_t  &request,int read=0) {
uint64_t exp_data;
  cvm::log(cvm::HIGH, "Address: {:#X} \n",request.addr);
  cvm::log(cvm::HIGH, "Length: {} \n",request.length);
  cvm::log(cvm::HIGH, "ID: {} \n ",request.id );
    
    
  std::stringstream ss;

  for (const auto &byte : request.data) {
    ss << static_cast<int>(byte) << " ";
  }


  std::string output = ss.str();
  ss.clear();
  ss.str("");

  for (const auto &bit : request.strb) {
    ss <<  bit << " ";
  }
  std::string output2 = ss.str();
  cvm::log(cvm::HIGH,"Data: {} \n",output);
  cvm::log(cvm::HIGH,"STRB: {} \n",output2);

  if(read == 1){
    deserializeInt(request.data,axi_read_resp, uint64_t(request.addr));
    exp_data = exp_read_data_q.front();
    exp_read_data_q.pop();
   
    if (start_default ==1) {
      exp_data = exp_data & 0xFFFFFFFFFFFFFFFF;
    } else if(FLAGS_strobe_type == 0) {
      exp_data = exp_data & 0xFFFFFFFF000000FF; 
    } else if(FLAGS_strobe_type == 1) {
      exp_data = exp_data & 0xFFFFFFFF0000FF00; 
    } else if(FLAGS_strobe_type == 2) {
      exp_data = exp_data & 0xFFFFFFFF00FF0000; 
    } else if(FLAGS_strobe_type == 3) {
      exp_data = exp_data & 0xFFFFFFFFFF000000; 
    } else {
    }
       if(axi_read_resp == exp_data){
         cvm::log(cvm::HIGH, "[overlay axi] expected data matched {:#x} \n",axi_read_resp);
       }
       else{
         cvm::log(cvm::ERROR, "ERROR: [overlay axi] expected data :{:#x} and received data:{}\n",exp_data,axi_read_resp);
       }

  }else{
    deserializeInt(request.data,axi_read_resp, uint64_t(request.addr));
    if(axi_read_resp == 0xffff){
      cvm::log(cvm::HIGH, "[overlay axi] expected data matched {:#x} \n",axi_read_resp);
    }
    else{
      cvm::log(cvm::ERROR, "ERROR: [overlay axi] expected data :{:#x} and received data:{}\n",0xffff,axi_read_resp);
    }
  }
}



void trace_cfg::push_random_axi_write(const random_list& elements){
  cvm::log(cvm::HIGH, "[overlay axi] overlay write axi seq size :{}\n",elements.size());
  for(int i = 0; i < 15;i++){
    for (const auto& e : elements) {
        trace_wr_txn_q.push({e.value,0xFFFF});
        cvm::log(cvm::HIGH, "[overlay axi] overlay write for address {:#x} done\n",e.value);
    }
  }
  cvm::log(cvm::HIGH, "[overlay axi] overlay write axi seq completed with queue size {}\n",trace_wr_txn_q.size());
}

void trace_cfg::overlay_tick(uint64_t) {

    if(trace_start_cnt == 0) {
      trace_start_cnt = (rng()% 5) + 20;
      n = (rng()% 5) + 3;
      id_val = 0;
      id_val1 = 0;
      write_id = 0;
      is_smem_mode = rng();
      start_wr_access = FLAGS_start_overlay_access + 30;
      start_rd_access = FLAGS_start_overlay_access + 100;
   
      cvm::log(cvm::NONE, "[Trace_cfg] trace_cfg IS_SMEM {} trace_start_cnt  {} \n",is_smem_mode,trace_start_cnt);
    }
      
    if(cnt_tick== 30) {
        start_default = 0;
    }

    if(FLAGS_overlay_mmr_en && FLAGS_overlay_num_times >= overlay_exec_cntr){
        
        if(end_test==1) complete_trace_test();
        if(cnt_tick==trace_start_cnt){
          randomElements = pickRandomElements(n);
          cvm::log(cvm::HIGH, "[overlay axi regress] overlay timer tick advance interval {} trace_start_cnt{} n {} size {} \n",cnt_tick,trace_start_cnt,n,randomElements.size());
        }
        cvm::log(cvm::LOW, "STROBE TYPE= {:#X} \n",FLAGS_strobe_type);

        if((cnt_tick==20) && start_default ) push_default_rd_seq();
        cvm::log(cvm::LOW, "[overlay axi] overlay2 WR/RD Scratch seq\n");
        if(cnt_tick== (start_wr_access) && !start_default && (overlay_in_progress ==0)) push_wr_scratchpad_seq();
        if(cnt_tick== (start_rd_access) && !start_default && (overlay_in_progress ==0)) push_rd_scratchpad_seq();

        if((trace_wr_txn_q.size() > 0) && !write_in_flight) axi_write_mmr_granular();

        if((trace_misc_rd_txn_q.size() > 0)&& ! read_in_flight){
          trace_cfg_read_t read_req;
          read_req = trace_misc_rd_txn_q.front();
          trace_misc_rd_txn_q.pop();
	  exp_read_data_q.push(read_req.exp_data);
          axi_ids = id_val1++;
          axi_read(read_req.addr,read_req.length,axi_ids);
          cvm::log(cvm::LOW, "[overlay axi] recieved {:#X} with id {} tick {}\n",read_req.addr,axi_ids,cnt_tick);
        }
        if((trace_misc_rd_txn_q.size() == 0)&& overlay_in_progress ==1){
            overlay_in_progress =0 ;
            overlay_exec_cntr++;
            start_wr_access = cnt_tick + FLAGS_overlay_idle;
            start_rd_access = cnt_tick + FLAGS_overlay_idle;
        }

        while((trace_read_resp_q.size() >0) ){
          print_read_request(trace_read_resp_q.front(),1);
          trace_read_resp_q.pop();
          if(trace_read_resp_q.size() == 0){
            end_test = 1;
            cvm::log(cvm::HIGH, "[overlay axi] write and read check ended\n");
          }
        }
      }

    cnt_tick ++;
}


bool trace_cfg::init_elf(const std::string& path) {
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}
