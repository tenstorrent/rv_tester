#include "scratchpad_random_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "cosim/dut_if/rvfi/rvfi_plusargs.h"
REGISTRY_register(scratchpad_random_sequence, OVERLAY_DRIVER, cvm::registry::all);

DEFINE_bool(sp_xtor_rand_en, true, "Enable scratchpad_random_sequence tick");
DEFINE_bool(sp_xtor_en, false, "Enable scratchpad_random_sequence tick");
DEFINE_bool(sp_xtor_mmr_prog_en, false, "Enable scratchpad transactor acceses ");
DEFINE_bool(sp_xtor_rnd_traffic_en, false, "Enable programming of SP mmr from Scraptchpad transactor ");
DEFINE_bool(sp_xtor_ot_traffic_en, false, "Enable Outstanding transactions (rd,wr) to SC Scraptchpad memory region from Scraptchpad transactor ");
DEFINE_bool(sp_xtor_test_cwfr, false, "Read SP data written by core ");
DEFINE_bool(sp_xtor_test_fwcr, false, "Write SP data for core to read ");
DEFINE_bool(sp_xtor_test_false_sharing, false, "false sharing test ");

scratchpad_random_sequence::scratchpad_random_sequence(cvm::topology::loc_t loc, unsigned id) : loc_(loc), id_(id), scope_(nullptr) {
  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });
  tick_loc_ = cvm::topology::get_from_hierarchy("TOP.PLATFORM.OVERLAY_DRIVER", 0);

  axi_mst_loc_l = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST",0);
  r_channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_l);
  
  // nmi sequence threads
  if (FLAGS_sp_xtor_rand_en) {
    random_mode_thread();
  }
}

scratchpad_random_sequence::~scratchpad_random_sequence() {
}

void scratchpad_random_sequence::random_mode_thread() {
  auto *task = +[] (scratchpad_random_sequence* m) -> cvm::messenger::task<void> {
    co_await m->random_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> scratchpad_random_sequence::random_mode() {
  while (FLAGS_sp_xtor_en) {
    // Wait for next tick generated after a random interval "nmi_interval"
    co_await tick();

    if (FLAGS_sp_xtor_mmr_prog_en) {
      if (cnt_tick == 24) {
        cvm::registry::messenger.clear_channel<axi::r_t>(r_channel);
        if(!sc_slice_array_initial_done){
          co_await check_sc_slice_status(ONLY_ARAY_INIT_DONE_CHK);
        }
        cvm::log(cvm::LOW, "[scratchpad_random_sequence] Check Slice status for arry_init_done : {} \n", sc_slice_array_initial_done);

        if (!FLAGS_cluster_axi_sp_perf) {
          cvm::log(cvm::HIGH, " [scratchpad_random_sequence] Programming SP MMR \n");
          co_await axi_write_mmr_granular();
          co_await axi_write_mmr_data_granular();

          // Delay
          for (int i=0; i<6; i=i+1){
            co_await tick();
          }
        }
       
        cvm::registry::messenger.clear_channel<axi::r_t>(r_channel);
        if(!slice_chk_done){
          co_await check_sc_slice_status(ARAY_INIT_DONE_AND_SPRECONFIG_DONE_CHK);
        }
        cvm::log(cvm::LOW, "[scratchpad_random_sequence] Check Slice status for arry_init_done and spreconfiguration_done: {:#x} \n", slice_chk_done);

        sc_polling_done = true;
        rnd_traffic_cnt_tick_1 = (cnt_tick+1);
      }
    }

    if (FLAGS_sp_xtor_rnd_traffic_en) {
      if ((!FLAGS_sp_xtor_mmr_prog_en || sc_polling_done) && cnt_tick == rnd_traffic_cnt_tick_1) {
        uint64_t offset = rng() & 0x1ff;
        sp_addr = sp_base + (offset<<6);

        cvm::log(cvm::HIGH, " [scratchpad_random_sequence] Random Traffic Write req:- sp_addr={:#x}, sp_base={:#x}, offset={:#x} \n", sp_addr, sp_base, offset);
        co_await axi_write_granular(sp_addr);
        co_await axi_write_data_granular();

        for(int i=0 ; i<8; i=i+1) {
          co_await tick();
        }

        cvm::log(cvm::HIGH, " [scratchpad_random_sequence] Random Traffic Read req:- sp_addr={:#x}, sp_base={:#x}, offset={:#x} \n", sp_addr, sp_base, offset);
        co_await axi_read(sp_addr, 4, 4);
        rnd_traffic_cnt_tick_1 = cnt_tick + 5 + std::abs(rng()% 60);
      }
    } else if (FLAGS_sp_xtor_ot_traffic_en) {
      if ((!FLAGS_sp_xtor_mmr_prog_en || sc_polling_done) && cnt_tick == rnd_traffic_cnt_tick_1) {
        uint64_t even_offset = rng() & 0x1FE; // EvenNW - Addr[6] = 0
        uint64_t sp_addr_even_nw = sp_base + (even_offset<<6);
        uint64_t odd_offset = (rng() & 0x1FF) | 0x1; // OddNW - Addr[6] = 1
        uint64_t sp_addr_odd_nw = sp_base + (odd_offset<<6);

        cvm::log(cvm::HIGH, " [scratchpad_random_sequence] Random OT Traffic Write req :- sp_addr={:#x}, sp_base={:#x}, offset={:#x} \n", send_wr_to_odd_network?sp_addr_odd_nw:sp_addr_even_nw, sp_base, send_wr_to_odd_network?even_offset:odd_offset);
        co_await axi_write_granular(send_wr_to_odd_network?sp_addr_odd_nw:sp_addr_even_nw);
        co_await axi_write_data_granular();

        cvm::log(cvm::HIGH, " [scratchpad_random_sequence] Random OT Traffic Read req :- sp_addr={:#x}, sp_base={:#x}, offset={:#x} \n", send_wr_to_odd_network?sp_addr_even_nw:sp_addr_odd_nw, sp_base, send_wr_to_odd_network?odd_offset:even_offset);
        co_await axi_read(send_wr_to_odd_network?sp_addr_even_nw:sp_addr_odd_nw, 4, 4, NO_BLOCK);
        send_wr_to_odd_network = !send_wr_to_odd_network;
        rnd_traffic_cnt_tick_1 = cnt_tick + 1;
      }
    } else if (FLAGS_sp_xtor_test_cwfr) {
      if(cnt_tick == 60){
        cvm::log(cvm::HIGH, " **** [scratchpad_random_sequence] CORE Write Fabric Read Test **** \n");
        co_await axi_read(sp_base, 4, 4); 
      }
    } else if (FLAGS_sp_xtor_test_fwcr) {
      if (cnt_tick == 34) {
        cvm::log(cvm::HIGH, " **** [scratchpad_random_sequence] Fabric Write Core Read Test **** \n");
        uint64_t addr = sp_base;
        co_await axi_write_granular(addr);
        co_await axi_write_data_granular();
      }
    } else if (FLAGS_sp_xtor_test_false_sharing) {
      if (cnt_tick == 34) {
        cvm::log(cvm::HIGH, " **** [scratchpad_random_sequence] Fabric Write Core Read Test **** \n");
        uint64_t addr = sp_base;
        co_await axi_write_granular(addr);
        co_await axi_write_data_granular();
      }
       if(cnt_tick == 60){
        cvm::log(cvm::HIGH, " **** [scratchpad_random_sequence] CORE Write Fabric Read Test **** \n");
        co_await axi_read(sp_base+16, 4, 4); 
      }
    } else {
      cvm::log(cvm::HIGH, " [scratchpad_random_sequence] tick {}\n",cnt_tick);
      if(cnt_tick == 60) {
        cvm::log(cvm::HIGH, " [scratchpad_random_sequence] trigger flag set \n");
        uint64_t addr = sp_base;
        co_await axi_write_granular(addr);
        co_await axi_write_data_granular();
      }
      if (cnt_tick == 62) {
        cvm::log(cvm::HIGH, " [scratchpad_random_sequence] READ SP DATA \n");
        co_await axi_read(sp_base, 4, 4);
      }
    }
  }

  co_return;
}

cvm::messenger::task<void> scratchpad_random_sequence::check_sc_slice_status(sc_slice_status_t sc_slice_status_type) {
  transactor::read_t r;
  r.addr = sp_prog_addr;
  uint64_t status; 

  // Poll for array_init_done in sc_status
  if (sc_slice_status_type == ONLY_ARAY_INIT_DONE_CHK) {
    cvm::log(cvm::LOW, "[scratchpad_random_sequence] Check Slice status for arry_init_done_checking of slice addr {:#x}: \n",r.addr);
    do {
      co_await tick();
      cvm::registry::messenger.clear_channel<axi::r_t>(r_channel);
      status = co_await axi_read_mmr_granular(r);
      cvm::log(cvm::LOW, "[scratchpad_random_sequence] Check Slice status for arry_init_done status {:#x} \n",status ); 
    } while ((status & 0x1) != 0x1);
    sc_slice_array_initial_done = true;
  }
  else {
    // Poll for array_init_done in sc_status & sp_reconfiguration_done
    for(int i=0; i<16; i=i+1) {
      r.addr = (sp_prog_addr | (i<<12));
      cvm::log(cvm::FULL, "[scratchpad_random_sequence] Check Slice status for arry_init_done_and_spreconfiguration_checking for slice addr {:#x} and slice_no {} \n", r.addr, i);
      do {
        co_await tick();
        cvm::registry::messenger.clear_channel<axi::r_t>(r_channel);
        status = co_await axi_read_mmr_granular(r);
        cvm::log(cvm::LOW, "[scratchpad_random_sequence] Check Slice status for arry_init_done_and_spreconfiguration_checking status {:#x} \n", status);
      } while ((status & 0xF) != 0x9);
    }

    slice_chk_done = true;
  }
}

cvm::messenger::task<uint64_t> scratchpad_random_sequence::axi_read_mmr_granular(const transactor::read_t& r) {
  axi::a_no_id_t ar_txn;
  unsigned id;
  uint64_t rdata = 0;
  ar_txn.w      = false;
  ar_txn.addr   = r.addr;
  ar_txn.len    = 0;
  ar_txn.size   = 3;
  ar_txn.burst  = axi::burst_t(0);
  ar_txn.lock   = 0;
  ar_txn.cache  = axi::cache_mem_attr_t(0);
  ar_txn.prot   = 0;
  ar_txn.qos    = 0;
  ar_txn.region = 0;
  ar_txn.atop   = 0;
  ar_txn.user   = 0;
  ar_txn.seqid  =SCRATCHPAD_MEM_SEQ_ID;
  if (FLAGS_cluster_axi_sp_perf) {
    ar_txn.is_manual_id = true;
    ar_txn.manual_id =  0x300;
  }

  cvm::log(cvm::LOW, "[scratchpad_random_sequence] AXI READ MMR GRANULAR (read req):- addr={:#x} SEND SYSMOD SIGNAL\n", ar_txn.addr);

  if (!cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_mst_loc_l, ar_txn, id)) {
    auto axi_idalloc_done = co_await check_axi_rresp_timeout(ar_txn, id);
    if (!axi_idalloc_done) {
      co_return 0;
    }
  }

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(r_channel, [&id](const auto& r) { return r.id == id;});
  rdata = convert_to_dword_array(resp.data, 8, 8);

  cvm::log(cvm::FULL, "[scratchpad_random_sequence] AXI READ MMR GRANULAR (read resp):- addr={:#x}, rdata={:#x}\n", ar_txn.addr, rdata);

  co_return rdata;
}

cvm::messenger::task<void> scratchpad_random_sequence::axi_read_granular(const transactor::read_t& r, bool block /* = BLOCK*/) {

  axi::a_no_id_t ar_txn;
  unsigned id;
  ar_txn.w    = false;
  //ar_txn.id   = 2;
  //ar_txn.addr = 0x60000000;
  ar_txn.addr = r.addr;
  ar_txn.len  = 0;
  ar_txn.size = 6;
  ar_txn.burst = axi::burst_t(0);
  ar_txn.lock  =0;
  ar_txn.cache  =axi::cache_mem_attr_t(0);
  ar_txn.prot  =2;
  ar_txn.qos  =0;
  ar_txn.region  =0;
  ar_txn.atop  =0;
  ar_txn.user  =0;
  ar_txn.seqid  = SCRATCHPAD_MEM_SEQ_ID;
  if (FLAGS_cluster_axi_sp_perf) {
    ar_txn.is_manual_id = true;
    ar_txn.manual_id = 0x301;
  }

  sp_xtor_num_accesses++;
  cvm::log(cvm::LOW, "[scratchpad_random_sequence] SP_XTOR AXI READ GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", ar_txn.addr);

  if (!cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_mst_loc_l, ar_txn , id)) {
    auto axi_idalloc_done = co_await check_axi_rresp_timeout(ar_txn, id);
    if (!axi_idalloc_done) {
      co_return;
    }
  }

  if (!block){
    co_return;
  }

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(r_channel, [&id](const auto& r) { return r.id == id;});
  scratchpad_xtor_read_req_t scratchpad_xtor_rd;
  scratchpad_xtor_rd.addr = ar_txn.addr ;
  scratchpad_xtor_rd.length = 0 ;
  scratchpad_xtor_rd.id = id;
  scratchpad_xtor_rd.data = resp.data;  
  for (int i = 0; i < int(resp.data.size()); ++i) {
    cvm::log(cvm::FULL, "[scratchpad_random_sequence] read resp byte {} =  {:#X} \n",i,uint32_t(resp.data[i]));
  }

  scratchpad_read_resp_q.push(scratchpad_xtor_rd); 
  cvm::log(cvm::FULL, "[scratchpad_random_sequence] read addr {:#X} completed\n",scratchpad_xtor_rd.addr);

  co_return;
}

cvm::messenger::task<void> scratchpad_random_sequence::axi_read(uint64_t addr, size_t length, uint32_t id, bool block /* = BLOCK*/) {
  cvm::log(cvm::FULL, "[scratchpad_random_sequence] axi read addr= {:#X} id = {} length = {}  \n",addr,id,length);
  transactor::read_t r ;
  r.addr = addr;
  r.length = length;
  sp_xtor_num_accesses++;
  co_await axi_read_granular(r, block);
}

cvm::messenger::task<void> scratchpad_random_sequence::axi_write_mmr_granular() {
  axi::a_no_id_t aw_txn;
  unsigned id;
  aw_txn.w    = true;
  aw_txn.addr = 0x421a0010;
  aw_txn.len  = 0;
  aw_txn.size = 3;
  aw_txn.burst = axi::burst_t(0);
  aw_txn.lock  =0;
  aw_txn.cache  =axi::cache_mem_attr_t(0);
  aw_txn.prot  =0;
  aw_txn.qos  =0;
  aw_txn.region  =0;
  aw_txn.atop  =0;
  aw_txn.user  =8;
  aw_txn.seqid  =SCRATCHPAD_MEM_SEQ_ID;
  if (FLAGS_cluster_axi_sp_perf) {
    aw_txn.is_manual_id = true;
    aw_txn.manual_id =  0x302;
  }

  cvm::log(cvm::LOW, "[scratchpad_random_sequence] SP_XTOR AXI MMR WRITE GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", aw_txn.addr);

  if (!cvm::registry::messenger.call<overlay_mst_t::push_aw_no_id_rpc>(axi_mst_loc_l, aw_txn, id)) {
    auto axi_idalloc_done = co_await check_axi_bresp_timeout(aw_txn, id);
    if (!axi_idalloc_done) {
      co_return;
    }
  }
  
  co_return;
}

cvm::messenger::task<void> scratchpad_random_sequence::axi_write_mmr_data_granular() {
  axi::w_t w_txn;

  w_txn.data = {0,0,0,0, 0,0,0,0, 0,0,0,0 ,0,0,0,0,  1,0,0,0, 0,0,0,0 ,0,0,0,0,  0,0,0,0, 0,0,0,0 ,0,0,0,0, 0,0,0,0 ,0,0,0,0 ,0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
  w_txn.strb = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,  1,1,1,1, 1,1,1,1 ,0,0,0,0,  0,0,0,0, 0,0,0,0 ,0,0,0,0 ,0,0,0,0 ,0,0,0,0 ,0,0,0,0 ,0,0,0,0, 0,0,0,0, 0,0,0,0};

  w_txn.last = 1;
  cvm::registry::messenger.signal(axi_mst_loc_l, w_txn);

  co_return;
}

cvm::messenger::task<void> scratchpad_random_sequence::axi_write_granular(uint64_t addr) {
  axi::a_no_id_t aw_txn;
  unsigned id;
  aw_txn.w    = true;
  aw_txn.addr = addr;
  scratchpad_addr_in_flight = addr;
  aw_txn.len  = 0;
  aw_txn.size = 6;
  aw_txn.burst = axi::burst_t(0);
  aw_txn.lock  =0;
  aw_txn.cache  =axi::cache_mem_attr_t(0);
  aw_txn.prot  =2;
  aw_txn.qos  =0;
  aw_txn.region  =0;
  aw_txn.atop  =0;
  aw_txn.user  =0;
  aw_txn.seqid  = SCRATCHPAD_MEM_SEQ_ID;
  if (FLAGS_cluster_axi_sp_perf) {
    aw_txn.is_manual_id = true;
    aw_txn.manual_id =  0x303;
  }

  sp_xtor_num_accesses++;
  cvm::log(cvm::LOW, "[scratchpad_random_sequence] SP_XTOR AXI WRITE GRANULAR - addr={:#x} SEND SYSMOD SIGNAL\n", aw_txn.addr);

  while(1) {
    auto lock = cvm::registry::messenger.call<overlay_mst_t::try_lock_rpc>(axi_mst_loc_l);
    if(lock) { break; }
    co_await tick();
  }

  if (!cvm::registry::messenger.call<overlay_mst_t::push_aw_no_id_rpc>(axi_mst_loc_l, aw_txn, id)) {
    auto axi_idalloc_done = co_await check_axi_bresp_timeout(aw_txn, id);
    if (!axi_idalloc_done) {
      co_return;
    }
  }
  co_return;
}

cvm::messenger::task<void> scratchpad_random_sequence::axi_write_data_granular() {
  axi::w_t w_txn;

  w_txn.data = {1,2,3,4, 5,6,7,8, 9,0xa,0xb,0xc, 0xd,0xe,0xf,0,  0xd,0xe,0xa,0xd, 0xb,0xe,0xe,0xf,  0xc,0,0,1, 0xb,0xa,0xd,0xa, 1,1,1,1, 2,2,2,2,  0,0,0,0 ,0,0,0,0 ,0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
  w_txn.strb = {1,1,1,1, 1,1,1,1, 1,1,1,1,       1,1,1,1,             1,1,1,1,       1,1,1,1,         1,1,1,1,      1,1,1,1,    1,1,1,1, 1,1,1,1,  0,0,0,0, 0,0,0,0 ,0,0,0,0 ,0,0,0,0 ,0,0,0,0 ,0,0,0,0};
  w_txn.last = 1;
  cvm::registry::messenger.signal(axi_mst_loc_l, w_txn);

  // Poke data to SP memory
  if(FLAGS_cosim){
  int hart = 0;
  bool valid;
  cvm::log(cvm::HIGH, "[scratchpad_random_sequence] Backdoor whisper poke addr{:#x} poke_data {:#x} \n", scratchpad_addr_in_flight, w_txn.data[0]);
  for (uint8_t i = 0; i < 64; ++i) {
    if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, 0, 'm', scratchpad_addr_in_flight+i, 1, w_txn.data[i], false, false, valid) || !valid) {
      cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
      co_return;
    }
    else {
      cvm::log(cvm::HIGH, "[scratchpad_random_sequence] backdoor whisper poke  Successful for addr{:#x} poke_data {:#x} \n",scratchpad_addr_in_flight+i,w_txn.data[i]);
    }
  }
  }
  co_return;
}

cvm::messenger::task<void> scratchpad_random_sequence::sp_rand_traffic() {
  if (cnt_tick == rnd_traffic_cnt_tick){
    uint64_t offset = std::abs(rng()% 500);
    sp_addr = sp_base + (offset <<6);
    co_await axi_write_granular(sp_addr);
    co_await axi_write_data_granular();

  }
  if (cnt_tick == rnd_traffic_cnt_tick+8) {
    axi_read(sp_addr,4,4);
    rnd_traffic_cnt_tick = cnt_tick + 5 +std::abs(rng()% 60); //5 cycle min buffer
  }
  co_return;
}

cvm::messenger::task<void> scratchpad_random_sequence::sp_mmr_prog(){
	if(FLAGS_sp_xtor_mmr_prog_en){
     cvm::log(cvm::HIGH, " [scratchpad_random_sequence] trigger flag set \n");
     co_await axi_write_mmr_granular();
     co_await axi_write_mmr_data_granular();
  }
  co_return;
}

cvm::messenger::task<void> scratchpad_random_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::overlay_driver::m_overlay_driver_tick<>>(tick_loc_);
  cnt_tick++;
  co_return;
}

cvm::messenger::task<void> scratchpad_random_sequence::trigger() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::overlay_driver::m_overlay_driver_tick<>>(tick_loc_);
  co_return;
}

cvm::messenger::task<bool> scratchpad_random_sequence::check_axi_bresp_timeout(axi::a_no_id_t aw_txn, unsigned& id) {

  uint32_t axi_bresp_cycle_cnt = 0;

  while (true) {
    co_await tick();

    if (axi_bresp_cycle_cnt >= FLAGS_axi_resp_timeout) {
      cvm::log(cvm::ERROR, "[scratchpad_random_sequence] [axi_mst] Error: No free id's remaining for axi master\n");
      co_return false;
    }
    axi_bresp_cycle_cnt++;

    if (cvm::registry::messenger.call<overlay_mst_t::push_aw_no_id_rpc>(axi_mst_loc_l, aw_txn, id)) {
      co_return true;
    }
  }

  co_return true;
}

cvm::messenger::task<bool> scratchpad_random_sequence::check_axi_rresp_timeout(axi::a_no_id_t ar_txn, unsigned& id) {

  uint32_t axi_rresp_cycle_cnt = 0;

  while (true) {
    co_await tick();

    if (axi_rresp_cycle_cnt >= FLAGS_axi_resp_timeout) {
      cvm::log(cvm::ERROR, "[scratchpad_random_sequence] [axi_mst] Error: No free id's remaining for axi master\n");
      co_return false;
    }
    axi_rresp_cycle_cnt++;

    if (cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_mst_loc_l, ar_txn, id)) {
      co_return true;
    }
  }

  co_return true;
}
