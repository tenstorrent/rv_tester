#include "snoop_gen_sequence.hpp"
#include "sysmod/sysmod_plusargs.h"
#include "rv_tester/rv_tester_plusargs.h"
#include "sysmod/sysmod_rpc.h"

REGISTRY_register(snoop_gen_sequence, SNOOP_GEN, cvm::registry::all);

DEFINE_bool(rand_snoop_en, false, "Enable random snoops on overlay path in the sim");
DEFINE_bool(rand_snoop_size_en, true, "Enable random snoops of different size on overlay path in the sim");
DEFINE_bool(rand_snoop_unaligned_addr_en, true, "Enable random snoops of unaligned on overlay path in the sim");
DEFINE_string(max_snoop_count, "7:10", "Number of snoops to be sent if  enabled");
DEFINE_string(rand_snoop_mode, "single", "snoop req modes: burst:single:single_shuffled");
DEFINE_int32(snoop_start_delay, 500, "TB cycle after which snoop driving random mode enabled");
DEFINE_int32(snoop_trigger_threshold, 5, "Number of snoops to populate in queue before triggering snoop request");
DEFINE_int32(snoop_max_burst_size, 2, "Number of b2b snoop request");

extern "C" {
  uint8_t get_rand_snoop_en() {
    return FLAGS_rand_snoop_en;
  }
}

snoop_gen_sequence::snoop_gen_sequence(cvm::topology::loc_t loc, unsigned id) : loc_(loc), id_(id), scope_(nullptr) {

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });

  axi_mst_loc_ = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST", 0);
  channel = cvm::registry::messenger.channel<axi::r_t>(axi_mst_loc_);
  
  cvm::registry::messenger.connect<uint64_t>(loc_, [this](uint64_t i) { return this->push_snoop_info(i); });

  max_snoop_count =  cvm::rand::get<uint32_t>(FLAGS_max_snoop_count);
  // trigger sequence threads`
  cvm::log(cvm::HIGH, "[SNOOP_GEN_SEQUENCE] constructing.... rand_snoop_en = {} \n",FLAGS_rand_snoop_en);
  if (FLAGS_rand_snoop_en) {
    rand_mode_thread();
  }
  cvm::registry::messenger.connect<rv_tester::snoop_addrs_eot>(cvm::topology::get_from_hierarchy("TOP.PLATFORM", 0), [this](rv_tester::snoop_addrs_eot addrs) { return this->eot_addr_queue(addrs);});
}

snoop_gen_sequence::~snoop_gen_sequence() {
  if (FLAGS_metrics) {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"snoopgen_num_rand_snoops\": \"{}\"}}\n", snoops_driven);
  }
}


void snoop_gen_sequence::push_snoop_info(uint64_t push_addr){
  if(FLAGS_rand_snoop_en){
    if(push_addr != 0xffffffffffff0){
      snoop_addrs.push_back(push_addr);
    }
    //snoop_addrs.erase(std::remove(snoop_addrs.begin(), snoop_addrs.end(), 0xffffffffffff0), snoop_addrs.end());
    cvm::log(cvm::HIGH, "[SNOOP_GEN_SEQUENCE] push addr {:#x}  \n",push_addr);
  }
}


void snoop_gen_sequence::rand_mode_thread() {
  cvm::log(cvm::HIGH, "[SNOOP_GEN_SEQUENCE] rand_mode_thread \n");
  auto *task = +[] (snoop_gen_sequence* m) -> cvm::messenger::task<void> {
    co_await m->rand_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void snoop_gen_sequence::eot_addr_queue(rv_tester::snoop_addrs_eot addrs) {
  cvm::log(cvm::MEDIUM, "Calling co-routine for Snoop to addresses\n");
  auto *task = +[] (rv_tester::snoop_addrs_eot addrs , snoop_gen_sequence* m) -> cvm::messenger::task<void> {
    co_await m->eot_snoop_addr(addrs.address);
    co_return;
  };
  cvm::registry::messenger.fork(task, addrs, this);
};

cvm::messenger::task<void>
snoop_gen_sequence::eot_snoop_addr(std::queue<uint64_t>& addr_q) {
  uint32_t id1 = rng1();
  while(!addr_q.empty()) {
    transactor::read_t r;
    r.id = id1++;
    r.addr = addr_q.front();
    r.length = 0x40;
    cvm::log(cvm::MEDIUM, "[EOT_SNOOP] Address to Snoop {:#x} via Overlay Read\n", r.addr);
    co_await blocking_read(r);
    transactor::write_t w;
    w.data = resp_.data;
    w.length = 64;
    w.addr = (r.addr >> 6) << 6;
    for(size_t i=0; i<w.strb.size(); i++)
      w.strb[i] = true;
    cvm::registry::messenger.signal<transactor::write_t>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.SNOOP_GEN", 0), w);
    addr_q.pop();
  }
  
  cvm::registry::messenger.signal<rv_tester::snoop_mem>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.SNOOP_GEN", 0), rv_tester::snoop_mem {.done = true});
  co_return;
}

cvm::messenger::task<void> snoop_gen_sequence::rand_mode() {
   while(1){
      cvm::log(cvm::HIGH, "[SNOOP_GEN_SEQUENCE] rand_mode thread wait for tick \n");
      for(int delay_cnt=0; delay_cnt < int(FLAGS_snoop_start_delay); delay_cnt++){
        co_await tick();
      }
      
      cvm::log(cvm::HIGH, "[SNOOP_GEN_SEQUENCE] snoops driven {} max snoop count {} \n",snoops_driven,max_snoop_count);
     if(snoops_driven < max_snoop_count){//max_snoop_count){
        if(FLAGS_rand_snoop_mode == "burst"){
          cvm::log(cvm::HIGH, "[SNOOP_GEN_SEQUENCE] burst mode : snoops_driven {} max_snoop_count {}  \n",snoops_driven,max_snoop_count);
          if(int(snoop_addrs.size()) > FLAGS_snoop_trigger_threshold ){
            unsigned snoop_loop  =  (rng1() % (FLAGS_snoop_max_burst_size ));
          cvm::log(cvm::HIGH, "[SNOOP_GEN_SEQUENCE] burst mode : snoop_loop size {}  \n",snoop_loop);
            for(int i = 0; i<int(snoop_loop); i++){
               unsigned idx = rng1() % snoop_addrs.size(); 
               cvm::log(cvm::HIGH, "[SNOOP_GEN_SEQUENCE] burst mode : snoop_loop selected idx  {}  and addr: {:#x} \n",idx,snoop_addrs[idx]);
               overlay_read(snoop_addrs[idx]);
               snoop_addrs.erase(snoop_addrs.begin() + idx); 
               snoops_driven++;
            }
            
          } 
        }
        if(FLAGS_rand_snoop_mode == "single"){
          cvm::log(cvm::HIGH, "[SNOOP_GEN_SEQUENCE] single mode : snoops_driven {} max_snoop_count {}  \n",snoops_driven,max_snoop_count);
          if(int(snoop_addrs.size()) > FLAGS_snoop_trigger_threshold){
               cvm::log(cvm::HIGH, "[SNOOP_GEN_SEQUENCE] single mode : snoop_loop selected addr: {:#x} \n",snoop_addrs[0]);
               overlay_read(snoop_addrs[0]);
               snoop_addrs.erase(snoop_addrs.begin()); 
               snoops_driven++;
          }
        }
        if(FLAGS_rand_snoop_mode == "single_shuffled"){
          cvm::log(cvm::HIGH, "[SNOOP_GEN_SEQUENCE] single shuffled  mode : snoops_driven {} max_snoop_count {}  \n",snoops_driven,max_snoop_count);
          if(int(snoop_addrs.size()) > FLAGS_snoop_trigger_threshold ){
               unsigned idx = rng1() % snoop_addrs.size(); 
               cvm::log(cvm::HIGH, "[SNOOP_GEN_SEQUENCE] single shuffled mode : snoop_loop selected idx  {}  and addr: {:#x} \n",idx,snoop_addrs[idx]);
               overlay_read(snoop_addrs[idx]);
               snoop_addrs.erase(snoop_addrs.begin() + idx); 
               snoops_driven++;
            
          } 
        }
     }else{
      break;
     }

   }
  co_return;
}


cvm::messenger::task<void> snoop_gen_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::snoop_gen::m_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> snoop_gen_sequence::trigger() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::snoop_gen::m_tick<>>(loc_);
  co_return;
}


void snoop_gen_sequence::overlay_read(uint64_t addr) {
  cvm::log(cvm::FULL, "[io_coh_helper] axi read addr= {:#X}   \n",addr);
   transactor::read_t r ;
   r.addr = addr;
   r.length = 0x40;
   auto* l = +[](transactor::read_t r, snoop_gen_sequence* dev) -> cvm::messenger::task<void>{
     co_await dev->blocking_read(r);
   };
   cvm::registry::messenger.fork(l, r, this);
}

cvm::messenger::task<void> snoop_gen_sequence::blocking_read(const transactor::read_t& r ) {

  axi::a_no_id_t ar_txn;
  unsigned id;
  ar_txn.w     = false;
  ar_txn.addr  = r.addr;
  ar_txn.size  = 6;
  ar_txn.len   = 0;
  ar_txn.burst = axi::burst_t(0);
  ar_txn.lock  = 0;
  ar_txn.cache = axi::cache_mem_attr_t(0);
  ar_txn.prot  = 2;
  ar_txn.qos   = 0;
  ar_txn.region= 0;
  ar_txn.atop  = 0;
  ar_txn.user  = 0;
  ar_txn.seqid = SNOOP_GEN_SEQ_ID;
  if (FLAGS_rand_snoop_unaligned_addr_en)
    ar_txn.addr = r.addr + (rng1() % 64);
  if (FLAGS_rand_snoop_size_en)
    ar_txn.size = rng1() % 7;
  ar_txn.rsp_err_chk = ((ar_txn.addr & 0x3) == 0) && (ar_txn.size != 0 && ar_txn.size != 1) && !((ar_txn.addr & 0x7) == 4 && ar_txn.size >= 3);

  cvm::log(cvm::HIGH, "[snoop_gen_sequence] blocking read data begin: \n");

  read_in_flight = true;
  using overlay_mst_t = axi_sw_mst<
      rv_tester_transactions::axi_sw_mst::b<>,
      rv_tester_transactions::axi_sw_mst::r<>,
      rv_tester_transactions::axi_sw_mst::ar_q_ptr<>,
      rv_tester_transactions::axi_sw_mst::aw_q_ptr<>,
      rv_tester_transactions::axi_sw_mst::w_q_ptr<>
  >;
  if (!cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_mst_loc_, ar_txn, id)) {
    auto axi_idalloc_done = co_await check_axi_rresp_timeout(ar_txn, id);
    if (!axi_idalloc_done) {
      co_return;
    }
  }

  auto resp = co_await cvm::registry::messenger.wait<axi::r_t>(channel, [&id](const auto& r) { return r.id == id; });
  resp_ = resp;
  read_in_flight = false;
  co_return;
}

cvm::messenger::task<bool> snoop_gen_sequence::check_axi_rresp_timeout(axi::a_no_id_t ar_txn, unsigned& id) {

  uint32_t axi_rresp_cycle_cnt = 0;

  while (true) {
    co_await tick();

    if (axi_rresp_cycle_cnt >= FLAGS_axi_resp_timeout) {
      cvm::log(cvm::ERROR, "[snoop_gen_sequence] [axi_mst] Error: No free id's remaining for axi master\n");
      co_return false;
    }
    axi_rresp_cycle_cnt++;

    if (cvm::registry::messenger.call<overlay_mst_t::push_ar_no_id_rpc>(axi_mst_loc_, ar_txn, id)) {
      co_return true;
    }
  }

  co_return true;
}
