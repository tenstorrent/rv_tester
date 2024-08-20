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

DEFINE_bool(cla_clk_halt, false, "Enable CLA clk halt events");
DEFINE_bool(cla_nmi, false, "Enable CLA NMI events");
DEFINE_bool(cla_rand_nmi_trig_en, false, "Enable CLA NMI/XTrigger events");

trace_cfg::trace_cfg(const std::string& tag, uint64_t addr, size_t size, cvm::topology::loc_t loc, cvm::topology::loc_t axi_mst_loc)
  : device(tag, addr, size, loc, &trace_cfg::write, &trace_cfg::read, this), axi_mst_loc_l(axi_mst_loc)
{
  rng.seed(FLAGS_seed);
  if (FLAGS_load != "") {
    init_elf(FLAGS_load);
  }
 
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
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
  
  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::write_request_t{addr, length, data, strb});
}

void trace_cfg::axi_read(uint64_t addr, size_t length,
                          uint32_t id) {
   //cvm::registry::messenger.signal(loc(), trace_cfg_read_t{addr, length, id});
   //cvm::log(cvm::HIGH, "[axi_read prints] cnt_tick {} trace_start_cnt {} \n",cnt_tick,trace_start_cnt);
    cvm::log(cvm::FULL, "[TRACE CFG] axi read addr= {:#X} id = {} length = {}  \n",addr,id,length);
  transactor::read_t r ;
  r.addr = addr;
  r.length = length;
  auto* l = +[](transactor::read_t r, trace_cfg* dev) -> cvm::messenger::task<void>{
    data_t d;
    co_await dev->read(r,d);
  };
  cvm::registry::messenger.fork(l, r, this);
}

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

  cvm::registry::messenger.signal(axi_mst_loc_l, transactor::read_request_t{addr, length});

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel);

  trace_cfg_read_req_t trace_cfg_rd;
  trace_cfg_rd.addr = addr;
  trace_cfg_rd.length = length;
  trace_cfg_rd.id = r.id;
  trace_cfg_rd.data = resp.data;  
  trace_read_resp_q.push(trace_cfg_rd); 
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

void trace_cfg::push_clk_halt_cfg() {
  cvm::log(cvm::LOW, "[overlay axi] Push CLK HALT Configs\n");
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_CTRL_STS_CFG,0x40});
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_COUNTER0_CFG,0x40000000});
  trace_wr_txn_q.push({trace_mmr::CDBG_NODE0_EAP0_CFG,0x10049});
  trace_wr_txn_q.push({trace_mmr::CDBG_NODE1_EAP0_CFG,0x101306});
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_CTRL_STS_CFG,0x60});
}

void trace_cfg::push_cla_nmi_cfg() {
  cvm::log(cvm::NONE, "[overlay axi] Push CLA NMI Configs\n");
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_CTRL_STS_CFG,0x40});
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_COUNTER0_CFG,0x25000000});
  trace_wr_txn_q.push({trace_mmr::CDBG_NODE0_EAP0_CFG,0x10049});
  trace_wr_txn_q.push({trace_mmr::CDBG_NODE1_EAP0_CFG,0x10130A});
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_CTRL_STS_CFG,0x60});

}

void trace_cfg::push_rand_nmi_trigg_cfg() {
  uint32_t wait_on_count,wait_off_count,event_count;
  uint32_t wdata;
  bool nmi_event;

  wait_on_count = (rng()% 201) + 1000;    // On Delay 1000-1200 CLK cycle
  wait_off_count = (rng()% 101) + 700;    // Off Delay 700-800 CLK cycle
  event_count = (rng()% 101) + 400;       // Event on Delay 400-500 CLK cycle
  nmi_event = rng();                      // NMI = 1, Trigger = 0

  cvm::log(cvm::NONE, "[overlay axi] Push NMI/Trigger Configs nmi_event {} \n",nmi_event);
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_CTRL_STS_CFG,0x40});
  wdata = 0; wdata = (wait_on_count << 16);
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_COUNTER0_CFG,wdata}); // CNT0 - On count
  wdata = 0; wdata = (event_count << 16);
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_COUNTER1_CFG,wdata}); // CNT1 - event count
  wdata = 0; wdata = (wait_off_count << 16);
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_COUNTER2_CFG,wdata}); // CNT2 - Off count

  trace_wr_txn_q.push({trace_mmr::CDBG_NODE0_EAP0_CFG,0x10048}); // ALWAYS ON, AUTOINCR0
  trace_wr_txn_q.push({trace_mmr::CDBG_NODE0_EAP1_CFG,0x101645});// TARGET MATCH-0, CLRCNT0, AUTOINCR1, DEST-1
  if(nmi_event){
    trace_wr_txn_q.push({trace_mmr::CDBG_NODE1_EAP0_CFG,0x10009});// ALWAYS ON, NMI
  }
  else {
    trace_wr_txn_q.push({trace_mmr::CDBG_NODE1_EAP0_CFG,0x1081D});// ALWAYS ON, TRIGGER-0,1
  }
  trace_wr_txn_q.push({trace_mmr::CDBG_NODE1_EAP1_CFG,0x131A56});// TRAGET MATCH-1. CLRCNT1, AUTOINCR2, DEST-2
  trace_wr_txn_q.push({trace_mmr::CDBG_NODE2_EAP0_CFG,0x161900});// TRAGET MATCH-2. CLRCNT2, DEST-0
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_CTRL_STS_CFG,0x60});
}

void trace_cfg::push_axi_mmr_seq() {
  cvm::log(cvm::HIGH, "[overlay axi] overlay axi write seq\n");
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_COUNTER3_CFG,0xFF});
  trace_wr_txn_q.push({mmr::CDBG_NODE3_EAP1_CFG,0xFF});
  cvm::log(cvm::HIGH, "[overlay axi] overlay axi write seq completed\n");
}

void trace_cfg::push_read_axi_mmr_seq() {
  cvm::log(cvm::HIGH, "[overlay axi] overlay axi read seq\n");
  trace_misc_rd_txn_q.push({trace_mmr::CDBG_CLA_COUNTER3_CFG,8});
  trace_misc_rd_txn_q.push({mmr::CDBG_NODE3_EAP1_CFG,8});
  cvm::log(cvm::HIGH, "[overlay axi] overlay axi read seq completed\n");
}

void trace_cfg::read_axi_pointers(){
  cvm::log(cvm::HIGH, "[overlay axi]reading WRITE/READ pointers\n");
  trace_misc_rd_txn_q.push({trace_mmr::TR_DST_CONTROL,4});
  trace_misc_rd_txn_q.push({trace_mmr::CDBG_NTRACE_CFG,8});
}

void trace_cfg::push_random_axi_read(const random_list&  elements){
  cvm::log(cvm::HIGH, "[overlay axi regress] success reading with {} elements \n",elements.size());

  // Loop through elements and write to file
  for(int i = 0; i < 15;i++){
    for (const auto& e : elements) {
        cvm::log(cvm::FULL, "[overlay axi reads] register {} address {:#x} size {} \n", e.name, e.value, 64);
        trace_misc_rd_txn_q.push({e.value,8});
    }
  }
}

void trace_cfg::print_read_request(const trace_cfg_read_req_t  &request,int read=0) {
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
    if(axi_read_resp == 0x0){
      cvm::log(cvm::HIGH, "[overlay axi] expected data matched {:#x} \n",axi_read_resp);
    }
    else{
      cvm::log(cvm::ERROR, "ERROR: [overlay axi] expected data :{:#x} and received data:{}\n",0x0,axi_read_resp);
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

void trace_cfg::push_trace_enable_seq() {
  uint32_t ram_control;
  cvm::log(cvm::LOW, "[Trace_cfg] trace_cfg inside enable trace seq\n");
  // Funnel-RAM config
  trace_wr_txn_q.push({trace_mmr::TR_DST_RAM_START_LOW,0x40});
  trace_wr_txn_q.push({trace_mmr::TR_DST_RAM_LIMIT_LOW,0x8000});
  trace_wr_txn_q.push({trace_mmr::TR_DST_RAM_WP_LOW,0x40});
  trace_wr_txn_q.push({trace_mmr::TR_DST_RAM_RP_LOW,0x40});
  ram_control = (0x3 | (is_smem_mode << 4));
  trace_wr_txn_q.push({trace_mmr::TR_DST_RAM_CONTROL,ram_control});
  trace_wr_txn_q.push({trace_mmr::TR_FUNNEL_CONTROL,0x3});

  // CLA/Paketizer config
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_CTRL_STS_CFG,0x40});
  trace_wr_txn_q.push({trace_mmr::TR_DST_CONTROL,0x3000003});
  trace_wr_txn_q.push({trace_mmr::CDBG_MUX_SEL_CFG,0x1});
  trace_wr_txn_q.push({trace_mmr::CDBG_MUX_SEL_CFG,0x5});
  trace_wr_txn_q.push({trace_mmr::CDBG_MUX_SEL_CFG,0x9});
  ram_control = 0x10000000 + (is_smem_mode * 0x40000000);
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_COUNTER0_CFG,ram_control});
  trace_wr_txn_q.push({trace_mmr::CDBG_NODE0_EAP0_CFG,0x11211});
  trace_wr_txn_q.push({trace_mmr::CDBG_NODE1_EAP0_CFG,0x101316});
  trace_wr_txn_q.push({trace_mmr::CDBG_NODE0_EAP1_CFG,0x1001D});
  trace_wr_txn_q.push({trace_mmr::CDBG_NODE1_EAP1_CFG,0x100022});
  trace_wr_txn_q.push({trace_mmr::TR_DST_INSTFEATURES,0x40000000});
  // RTL FIX Needed for this MMR access
  // trace_wr_txn_q.push({TR_DST_IMPL,0x1000000});
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_CTRL_STS_CFG,0x60});
  cvm::log(cvm::LOW, "[Trace_cfg] trace_cfg completed enable trace seq\n");
}

void trace_cfg::push_trace_disable_seq() {
  cvm::log(cvm::LOW, "[Trace_cfg] trace_cfg inside disable trace seq\n");
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_CTRL_STS_CFG,0x40});
  trace_wr_txn_q.push({trace_mmr::TR_DST_CONTROL,0x0});
  trace_wr_txn_q.push({trace_mmr::CDBG_CLA_CTRL_STS_CFG,0x0});
  if(!is_smem_mode){
    trace_wr_txn_q.push({trace_mmr::TR_FUNNEL_CONTROL,0x1});
    trace_wr_txn_q.push({trace_mmr::TR_DST_RAM_CONTROL,0x1});
  }
  cvm::log(cvm::LOW, "[Trace_cfg] trace_cfg completed disable trace seq\n");
}

void trace_cfg::check_dst_ram_empty() {
  trace_cfg_read_req_t rdata;
  uint32_t empty_val;

  cvm::log(cvm::LOW, "[Trace_cfg] trace_cfg inside check DST ram empty\n");
    rdata = trace_read_resp_q.front();
    trace_read_resp_q.pop();
    deserializeInt(rdata.data,empty_val,trace_mmr::TR_DST_RAM_CONTROL);
    if(empty_val & (1 << 3)) {
      cvm::log(cvm::LOW, "[Trace_cfg] checking empty val %x \n",empty_val);
      trace_wr_txn_q.push({trace_mmr::TR_FUNNEL_CONTROL,0x0});
      trace_wr_txn_q.push({trace_mmr::TR_DST_RAM_CONTROL,0x0});
      end_test = 1;
    }
    else{
      trace_misc_rd_txn_q.push({trace_mmr::TR_DST_RAM_CONTROL,4});
    }
  cvm::log(cvm::LOW, "[Trace_cfg] trace_cfg completed disable trace seq\n");
}

void trace_cfg::read_pointers(){
  cvm::log(cvm::LOW, "[Trace_cfg] trace_cfg reading WRITE/READ pointers\n");
  trace_misc_rd_txn_q.push({trace_mmr::TR_DST_RAM_WP_LOW,4});
  trace_misc_rd_txn_q.push({trace_mmr::TR_DST_RAM_RP_LOW,4});
}

void trace_cfg::read_sram() {
  uint32_t wp=0,rp=0;
  trace_cfg_read_req_t rdata;

   rdata = trace_read_resp_q.front();
   trace_read_resp_q.pop();
   if(rdata.addr == trace_mmr::TR_DST_RAM_WP_LOW) deserializeInt_wp(rdata.data,wp);
   wp = wp & 0xFFFFFFFC;
   cvm::log(cvm::LOW, "[Trace_cfg] trace_cfg reading write pointer {:#X}\n",wp);

   rdata = trace_read_resp_q.front();
   trace_read_resp_q.pop();
   if(rdata.addr == trace_mmr::TR_DST_RAM_RP_LOW) deserializeInt_rp(rdata.data,rp);
   cvm::log(cvm::LOW, "[Trace_cfg] trace_cfg reading read pointer {:#X}\n",rp);

   read_ram = (wp - rp)/4;
}        

void trace_cfg::overlay_tick(uint64_t) {

    if(trace_start_cnt == 0) {
      trace_start_cnt = (rng()% 5) + 20;
      trace_stop_cnt = trace_start_cnt + (rng()% 20) + 100;
      start_clk_halt_cnt = (rng()% 40) + 50 ;
      start_cla_nmi_cnt = (rng()% 40) + 50 ;
      start_rand_nmi_trig_cnt = (rng()% 40) + 50 ;
      n = (rng()% 5) + 3;
      id_val = 0;
      is_smem_mode = rng();
      cvm::log(cvm::NONE, "[Trace_cfg] trace_cfg IS_SMEM {} trace_start_cnt  {} trace_stop_cnt {} start_clk_halt_cnt {} start_cla_nmi_cnt {} start_rand_nmi_trig_cnt {} \n",is_smem_mode,trace_start_cnt,trace_stop_cnt,start_clk_halt_cnt,start_cla_nmi_cnt,start_rand_nmi_trig_cnt);
    }
    if(FLAGS_trace_en && !FLAGS_overlay_mmr_en) {
      cvm::log(cvm::HIGH, "[Trace_cfg] trace_cfg timer tick advance interval {} trace_start_cnt {} \n",cnt_tick,trace_start_cnt);
      if(end_test && (trace_wr_txn_q.size() == 0)) complete_trace_test();
      if(cnt_tick==trace_start_cnt) push_trace_enable_seq();
      if(cnt_tick==trace_stop_cnt) push_trace_disable_seq();
      if(trace_wr_txn_q.size() > 0) axi_write();
      if((trace_read_resp_q.size() > 0) && is_smem_mode) check_dst_ram_empty();
      if((cnt_tick==(trace_stop_cnt+10)) && (!is_smem_mode)) read_pointers();
      if((cnt_tick==(trace_stop_cnt+10)) && is_smem_mode) {
        trace_misc_rd_txn_q.push({trace_mmr::TR_DST_RAM_CONTROL,4});
      }
      if(trace_misc_rd_txn_q.size() > 0){
        std::pair<uint64_t,size_t> read_req;
        read_req = trace_misc_rd_txn_q.front();
        trace_misc_rd_txn_q.pop();
        axi_ids = rng()%200+400;
        axi_read(read_req.first,read_req.second,axi_ids);
      }
      if((trace_read_resp_q.size() == 2) && (read_ram == 0) && (!end_test) && (!is_smem_mode)) {
        read_sram();
      }
      if(read_ram > 0) {
        cvm::log(cvm::HIGH, "[Trace_cfg] read RAM {} \n",read_ram);
        axi_read(trace_mmr::TR_DST_RAM_DATA,4,400+id_val);
        id_val++;
        read_ram = read_ram - 1;
        if(read_ram == 0) end_test = 1;
      }
    }else if(FLAGS_trace_en && FLAGS_overlay_mmr_en){
      // cvm::log(cvm::HIGH, "[overlay axi regress] overlay timer tick advance interval {} trace_start_cnt{} n {} \n",cnt_tick,trace_start_cnt,n);
        
        if(end_test==1) complete_trace_test();
        if(cnt_tick==trace_start_cnt){
          randomElements = pickRandomElements(n);
          cvm::log(cvm::HIGH, "[overlay axi regress] overlay timer tick advance interval {} trace_start_cnt{} n {} size {} \n",cnt_tick,trace_start_cnt,n,randomElements.size());
        }

        // if(cnt_tick==(trace_start_cnt+20)) push_random_axi_write(randomElements);
        // if(trace_wr_txn_q.size() > 0) axi_write();
        if(cnt_tick==(trace_start_cnt+30)) push_random_axi_read(randomElements);

        if(trace_misc_rd_txn_q.size() > 0){
          std::pair<uint64_t,size_t> read_req;
          read_req = trace_misc_rd_txn_q.front();
          trace_misc_rd_txn_q.pop();
          axi_ids = rng()%200+400;
          axi_read(read_req.first,read_req.second,axi_ids);
          cvm::log(cvm::HIGH, "[overlay axi] recieved {:#X} with id {} tick {}\n",read_req.first,axi_ids,cnt_tick);
        }

        while((trace_read_resp_q.size() >0) ){
          print_read_request(trace_read_resp_q.front(),1);
          trace_read_resp_q.pop();
          cvm::log(cvm::HIGH, "[overlay axi] queue size {} \n",trace_read_resp_q.size());
          if(trace_read_resp_q.size() == 0){
            end_test = 1;
            cvm::log(cvm::HIGH, "[overlay axi] write and read check ended\n");
          }
        }
      }

    //--------------------------------- CLK HALT--------------------------------------
    if(FLAGS_cla_clk_halt) {
      cvm::log(cvm::HIGH, "[Trace_cfg::CLK_HALT] trace_cfg timer tick advance interval {} start_clk_halt_cnt {} \n",cnt_tick,start_clk_halt_cnt);
      if(cnt_tick==start_clk_halt_cnt) push_clk_halt_cfg();
      if(trace_wr_txn_q.size() > 0) axi_write();
     }

    //--------------------------------- CLA NMI --------------------------------------
    if(FLAGS_cla_nmi) {
      cvm::log(cvm::HIGH, "[Trace_cfg::NMI] trace_cfg timer tick advance interval {} start_cla_nmi_cnt {} \n",cnt_tick,start_cla_nmi_cnt);
      if(cnt_tick==start_cla_nmi_cnt) push_cla_nmi_cfg();
      if(trace_wr_txn_q.size() > 0) axi_write();
     }
    //--------------------------------- CLA NMI --------------------------------------
    if(FLAGS_cla_rand_nmi_trig_en) {
      cvm::log(cvm::HIGH, "[Trace_cfg::NMI/XTRIGGER] trace_cfg timer tick advance interval {} start_rand_nmi_trig_cnt {} \n",cnt_tick,start_rand_nmi_trig_cnt);
      if(cnt_tick==start_rand_nmi_trig_cnt) push_rand_nmi_trigg_cfg();
      if(trace_wr_txn_q.size() > 0) axi_write();
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
