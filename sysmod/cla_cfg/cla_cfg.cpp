#include <iostream>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/random.hpp"
#include "cla_cfg.h"
#include "transactors/axi_sw/axi.h"
#include "sysmod/sysmod_plusargs.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <random>

DEFINE_bool(cla_clk_halt, false, "Enable CLA clk halt events");
DEFINE_bool(cla_nmi, false, "Enable CLA NMI events");
DEFINE_bool(cla_rand_nmi_trig_en, false, "Enable CLA NMI/XTrigger events");

cla_cfg::cla_cfg(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, size, loc, &cla_cfg::write, &cla_cfg::read, this), axi_mst_loc_l(axi_mst_loc)
{
  rng.seed(FLAGS_seed);
  if (FLAGS_load != "") {
    init_elf(FLAGS_load);
  }
 
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
}

void cla_cfg::axi_write() {
  uint64_t addr;
  size_t length = 0x40;
  std::vector<uint8_t> data;
  std::vector<bool> strb;
  cla_wr_t wr;

  wr = cla_wr_txn_q.front();
  cla_wr_txn_q.pop();
  addr = (uint64_t)wr.addr;
  gen_data_strb(wr.addr,wr.data,data,strb);
  addr = addr & 0xFFFFFFC0;
  
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
}

void cla_cfg::axi_read(uint64_t addr, size_t length,
                          uint32_t id) {
  cvm::log(cvm::FULL, "[CLA CFG] axi read addr= {:#X} id = {} length = {}  \n",addr,id,length);
  transactor::read_t r ;
  r.addr = addr;
  r.length = length;
  auto* l = +[](transactor::read_t r, cla_cfg* dev) -> cvm::messenger::task<void>{
    data_t d;
    co_await dev->read(r,d);
  };
  cvm::registry::messenger.fork(l, r, this);
}

void cla_cfg::write(const transactor::write_t& ) {
}

cvm::messenger::task<void> cla_cfg::read(const transactor::read_t& r, data_t& ) {
  auto& addr = r.addr;
  auto& length = r.length;

  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);

  cla_cfg_read_req_t cla_cfg_rd;
  cla_cfg_rd.addr = addr;
  cla_cfg_rd.length = length;
  cla_cfg_rd.id = r.id;
  cla_cfg_rd.data = resp.data;  
  cla_read_resp_q.push(cla_cfg_rd); 
  co_return;
}

void cla_cfg::write_axi_mst(uint64_t addr, size_t, const data_t&, const strb_t&) {
  if (not has_addr(addr))
    return;
  return;
}

void cla_cfg::read_axi_mst(uint64_t addr, size_t, data_t&) {
  if (not has_addr(addr))
    return;
  return;
}

void cla_cfg::gen_data_strb(uint64_t addr, uint32_t value, data_t& wdata, std::vector<bool>& strb) {
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

void cla_cfg::push_clk_halt_cfg() {
  cvm::log(cvm::NONE, "[CLA_CFG] Push CLK HALT Configs\n");

  for(uint32_t i=0; i< 8 ; i++){
    if((mask & (1 << i))){
      cvm::log(cvm::NONE, "[CLA_CFG] Push CLA HALT Configs for Core-{} \n",i);
      addr_offset = 0x10000 * i;
      cntr_data = (rng() % 0x2000) + 0x4000;
      cntr_data = cntr_data << 16;
      cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_CTRL_STS_CFG + addr_offset),0x40});
      cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_COUNTER0_CFG + addr_offset),cntr_data});
      cla_wr_txn_q.push({(cla_mmr::CDBG_NODE0_EAP0_CFG + addr_offset),0x10049});
      cla_wr_txn_q.push({(cla_mmr::CDBG_NODE1_EAP0_CFG + addr_offset),0x101306});
      cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_CTRL_STS_CFG + addr_offset),0x60});
    }
  }
}

void cla_cfg::push_cla_nmi_cfg() {
  cvm::log(cvm::NONE, "[CLA_CFG] Push CLA NMI Configs start_cla_nmi_cnt {} \n", start_cla_nmi_cnt);

  rand_disable_dly = (rng() % 200)+ 300 + cnt_tick;  // 300 - 500 delay before disabling
  for(uint32_t i=0; i< 8 ; i++){
    if((mask & (1 << i))){
      cvm::log(cvm::LOW, "[CLA_CFG] Push CLA NMI Configs for Core-{} \n",i);
      addr_offset = 0x10000 * i;
      cntr_data = rng()%0x2000 + 0x4000;
      cntr_data = cntr_data << 16;
      if(reenable_nmi){
        cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_COUNTER0_CFG + addr_offset),cntr_data});
        cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_CTRL_STS_CFG + addr_offset),0x40});
        cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_CTRL_STS_CFG + addr_offset),0x60});
      }
      else{
        cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_CTRL_STS_CFG + addr_offset),0x40});
        cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_COUNTER0_CFG + addr_offset),cntr_data});
        cla_wr_txn_q.push({(cla_mmr::CDBG_NODE0_EAP0_CFG + addr_offset),0x10049});
        cla_wr_txn_q.push({(cla_mmr::CDBG_NODE1_EAP0_CFG + addr_offset),0x10110A});
        cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_CTRL_STS_CFG + addr_offset),0x60});
      }
    }
  }

}

void cla_cfg::push_cla_nmi_cfg_disable() {
  
  cvm::log(cvm::NONE, "[CLA_CFG] Push CLA NMI Disable EAP...\n");
  reenable_nmi = 1;
  start_cla_nmi_cnt = (rng()%100) + 400 + cnt_tick; // 400-500 off

  for(uint32_t i=0; i< 8 ; i++){
    if((mask & (1 << i))){
      cvm::log(cvm::LOW, "[CLA_CFG] Push CLA NMI Configs for Core-{} \n",i);
      addr_offset = 0x10000 * i;
      cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_CTRL_STS_CFG + addr_offset),0x0});
    }
  }
  nmi_total_cnt = nmi_total_cnt - 1;

}

void cla_cfg::push_rand_nmi_trigg_cfg() {
  uint32_t wait_on_count,wait_off_count,event_count;
  uint32_t wdata;

  wait_on_count = (rng()% 201) + 1000;    // On Delay 1000-1200 CLK cycle
  wait_off_count = (rng()% 101) + 700;    // Off Delay 700-800 CLK cycle
  event_count = (rng()% 101) + 200;       // Event on Delay 200-300 CLK cycle
  eap_ctrl = 12;                          // Considering 12 value as per waves
  active_core = (FLAGS_num_harts == 1) ? 0 : (rng() % FLAGS_num_harts);
  rand_disable_trig_dly = (rng() % 50)+ 500 + cnt_tick;  // 500 - 550 delay before disabling

  cvm::log(cvm::NONE, "[CLA_CFG] Push NMI/Trigger Configs for Core - {} nmi_event {} start_rand_nmi_trig_cnt {} \n", active_core, nmi_event, start_rand_nmi_trig_cnt);

  if(reenable_rand_trig) {
    if(nmi_event){
      cla_wr_txn_q.push({(cla_mmr::CDBG_NODE1_EAP1_CFG + (0x10000 * active_core)),0x10009});// ALWAYS ON, NMI
    }
    else {
      cla_wr_txn_q.push({(cla_mmr::CDBG_NODE1_EAP1_CFG + (0x10000 * active_core)),0x1081D});// ALWAYS ON, TRIGGER-0,1
    }
    cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_CTRL_STS_CFG + (0x10000 * active_core)),(eap_ctrl | 0x40)});
    cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_CTRL_STS_CFG + (0x10000 * active_core)),(eap_ctrl | 0x60)});
  }
  else {
    cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_CTRL_STS_CFG + (0x10000 * active_core)),(eap_ctrl | 0x40)});
    wdata = 0; wdata = (wait_on_count << 16);
    cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_COUNTER0_CFG + (0x10000 * active_core)),wdata}); // CNT0 - On count
    wdata = 0; wdata = (event_count << 16);
    cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_COUNTER1_CFG + (0x10000 * active_core)),wdata}); // CNT1 - event count
    wdata = 0; wdata = (wait_off_count << 16);
    cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_COUNTER2_CFG + (0x10000 * active_core)),wdata}); // CNT2 - Off count

    cla_wr_txn_q.push({(cla_mmr::CDBG_NODE0_EAP1_CFG + (0x10000 * active_core)),0x10040}); // ALWAYS ON, AUTOINCR0
    cla_wr_txn_q.push({(cla_mmr::CDBG_NODE0_EAP0_CFG + (0x10000 * active_core)),0x101645});// TARGET MATCH-0, CLRCNT0, AUTOINCR1, DEST-1
    if(nmi_event){
      cla_wr_txn_q.push({(cla_mmr::CDBG_NODE1_EAP1_CFG + (0x10000 * active_core)),0x10009});// ALWAYS ON, NMI
    }
    else {
      cla_wr_txn_q.push({(cla_mmr::CDBG_NODE1_EAP1_CFG + (0x10000 * active_core)),0x1081D});// ALWAYS ON, TRIGGER-0,1
    }
    cla_wr_txn_q.push({(cla_mmr::CDBG_NODE1_EAP0_CFG + (0x10000 * active_core)),0x131A56});// TRAGET MATCH-1. CLRCNT1, AUTOINCR2, DEST-2
    cla_wr_txn_q.push({(cla_mmr::CDBG_NODE2_EAP0_CFG + (0x10000 * active_core)),0x161900});// TRAGET MATCH-2. CLRCNT2, DEST-0
    cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_CTRL_STS_CFG + (0x10000 * active_core)),(eap_ctrl | 0x60)});
  }

}

void cla_cfg::push_rand_nmi_trigg_cfg_off() {

  cvm::log(cvm::NONE, "[CLA_CFG] Push NMI/Trigger Disable EAP... \n");
  start_rand_nmi_trig_cnt = (rng()%100) + 200 + cnt_tick; // 200-300 off
  cla_wr_txn_q.push({(cla_mmr::CDBG_CLA_CTRL_STS_CFG + (0x10000 * active_core)),(eap_ctrl & 0x3F80)});
  reenable_rand_trig = 1;
  nmi_event = !nmi_event;
  trig_total_cnt = trig_total_cnt - 1;
}

void cla_cfg::overlay_tick(uint64_t) {

    if(start_clk_halt_cnt == 0) {
      reenable_nmi = 0;
      reenable_rand_trig = 0;
      nmi_event = rng();                                      // NMI = 1, Trigger = 0
      mask = FLAGS_hart_enable_mask;
      nmi_total_cnt = (rng() % 5) + 5;                        // NMI total enable count
      trig_total_cnt = (rng() % 3) + 2;                       // Xtrigger/rand NMI total count
      start_clk_halt_cnt = (rng()% 40) + 50 ;
      start_cla_nmi_cnt = (rng()% 40) + 50 ;
      start_rand_nmi_trig_cnt = (rng()% 40) + 50 ;
      rand_disable_dly = (rng() % 200)+ 300 + start_cla_nmi_cnt;  // 300 - 500 delay before disabling
      rand_disable_trig_dly = (rng() % 50)+ 500 + start_rand_nmi_trig_cnt;  // 500 - 550 delay before disabling
      cvm::log(cvm::NONE, "[CLA_CFG] cla_cfg start_clk_halt_cnt {} start_cla_nmi_cnt {} start_rand_nmi_trig_cnt {} nmi_total_cnt {} trig_total_cnt {} \n",start_clk_halt_cnt,start_cla_nmi_cnt,start_rand_nmi_trig_cnt,nmi_total_cnt, trig_total_cnt);
    }

    //--------------------------------- CLK HALT--------------------------------------
    if(FLAGS_cla_clk_halt) {
      cvm::log(cvm::LOW, "[CLA_CFG::CLK_HALT] cla_cfg timer tick advance interval {} start_clk_halt_cnt {} \n",cnt_tick,start_clk_halt_cnt);
      if(cnt_tick==start_clk_halt_cnt) push_clk_halt_cfg();
      if(cla_wr_txn_q.size() > 0) axi_write();
     }

    //--------------------------------- CLA NMI --------------------------------------
    if(FLAGS_cla_nmi) {
      cvm::log(cvm::LOW, "[CLA_CFG::NMI] cla_cfg timer tick advance interval {} start_cla_nmi_cnt {} \n",cnt_tick,start_cla_nmi_cnt);
      if((cnt_tick==start_cla_nmi_cnt) && (nmi_total_cnt != 0)) push_cla_nmi_cfg();
      if(cnt_tick==rand_disable_dly) push_cla_nmi_cfg_disable();
      if(cla_wr_txn_q.size() > 0) axi_write();
     }
    //--------------------------------- CLA XTrigger/NMI ------------------------------
    if(FLAGS_cla_rand_nmi_trig_en) {
      cvm::log(cvm::LOW, "[CLA_CFG::NMI/XTRIGGER] cla_cfg timer tick advance interval {} start_rand_nmi_trig_cnt {} \n",cnt_tick,start_rand_nmi_trig_cnt);
      if((cnt_tick==start_rand_nmi_trig_cnt) && (trig_total_cnt > 0)) push_rand_nmi_trigg_cfg();
      if(cnt_tick==(rand_disable_trig_dly)) push_rand_nmi_trigg_cfg_off();
      if(cla_wr_txn_q.size() > 0) axi_write();
     }     
    cnt_tick ++;
}

bool cla_cfg::init_elf(const std::string& path) {
    try {
        m_.load_ELF(path);
    } catch(const std::exception& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    return true;
}
