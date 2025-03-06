// Licensed under the Apache License, Version 2.0, see LICENSE.TT for details
// vim: ft=c et ts=2 sw=0 sts

#include "bridge.h"
#include "util.h"
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/topology.hpp"
#include "cvm/random.hpp"
#include "src/cac_lib.h"
#include "sysmod/htif/htif.h"
#include "whisper_decoder.h"
#include "rv_tester/rv_tester_plusargs.h"
#include "sysmod/trickbox/interrupter.h"
#include "cosim/dut_if/rvfi/rvfi_plusargs.h"
#include "sysmod/sysmod_plusargs.h"
#include "sysmod/sysmod_params.hpp"
#include "cosim/utils/eot/eot_plusargs.h"
#include "whisper_client.h"

#include <cstring>          // strlen
#include <sstream>          // stringstream
#include <thread>           // std::this_thread::sleep_for
#include <chrono>           // std::chrono::seconds
#include <cstdlib>          // system
#include <vector>
#include <fmt/format.h>
#include <random>

DEFINE_uint64(resetpc, 0x80000000, "Reset PC");
DEFINE_uint64(resetpcfw, 0xC0040000, "Reset firmware PC");
DEFINE_bool(whisper_exec, true, "Enable rvfi instr processing...disable useful for measuring rvfi DPI performance");
DEFINE_bool(bridge_log, true, "Enable bridge logging");
DEFINE_bool(cosim_resynch, false, "Resynch whisper with dut state on every instruction");
DEFINE_string(cosim_resynch_instr, "", "List of instruction mnemonics to resynch whisper with dut state");
DEFINE_string(cosim_error_instr, "", "List of instruction mnemonics on which we should terminate with an error");
DEFINE_string(cosim_resynch_prev_instr, "", "List of instruction mnemonics to resynch whisper with dut state");
DEFINE_string(cosim_resynch_csr, "", "List of csr mnemonics to resynch whisper with dut state");
DEFINE_string(cosim_resynch_excp, "", "List of exception codes on which we should resynch whisper with dut state");
DEFINE_string(cosim_error_excp, "", "List of exception codes on which we should terminate with an error");
DEFINE_bool(poke_mip_timer, false, "Poke mip timer bits to handle timer interrupts instead of poking time csr");
DEFINE_bool(mip_resynch, true, "Resynch whisper with dut state on mip mismatch condition");
DEFINE_uint64(mip_resynch_threshold, 256, "Resynch whisper with dut state on mip mismatch if within threshold number of instructions");
DEFINE_bool(topi_resynch, true, "Resynch whisper with dut state on topi mismatch condition");
DEFINE_bool(topei_resynch, true, "Resynch whisper with dut state on topei mismatch condition");
DEFINE_uint64(topei_claim_threshold, 1, "Replay claim process N times on topei mismatch condition to match DUT");
DEFINE_bool(intr_defer_spcl, true, "Defer all interrupts in special cases");
DEFINE_bool(intr_timeout_resynch, true, "Ignore whisper timeout error condition");
DEFINE_bool(fcvt_cracked, false, "Break fcvt instruction into uops");
DEFINE_bool(scalar_fp64_er, false, "Break scalar FP64 instructions into two uops");
DEFINE_bool(retire_ucode_trap, true, "DUT indicates retire on a trap after executing the ucode trap handler");
DEFINE_bool(pc_check, true, "Enable cosim checks on pc");
DEFINE_bool(priv_check, true, "Enable cosim checks on priv mode");
DEFINE_bool(insn_check, true, "Enable cosim checks on insn bytes");
DEFINE_bool(gpr_check, true, "Enable cosim checks on gprs");
DEFINE_bool(fpr_check, true, "Enable cosim checks on fprs");
DEFINE_bool(vec_check, true, "Enable cosim checks on vector regs");
DEFINE_bool(csr_rd_check, true, "Enable cosim checks on sw csr writes");
DEFINE_bool(csr_wr_check, true, "Enable cosim checks on hw csr writes");
DEFINE_bool(memattr_check, true, "Enable cosim checks on mem attributes");
DEFINE_bool(flags_check, true, "Enable cosim checks on fflags");
DEFINE_uint64(max_cycle, 1000000, "Max cycle limit to terminate the sim");
DEFINE_int32(debug_excp_mcause, 24, "MCAUSE value for debug exception");
DEFINE_bool(whisper_client_check, true, "Removing Whisper API client checks");
DEFINE_bool(translation_check, false, "Do VA-PA translation check");
DEFINE_bool(emulate_debug_mode, true, "Emulate debug mode by forcing whisper to be in sync with DUT");
DEFINE_bool(delay_satp_update, false, "Delay satp update till next sfence.vma");
DEFINE_bool(cov, false, "Enable Arch coverage");
DEFINE_string(archsample_lib_path, "", "Path to libarchsample.so");
DEFINE_bool(standalone, true, "Enable whisper standalone run at beginning of sim");
DEFINE_bool(metrics, true, "Enable printing metrics in log file");
DEFINE_uint32(max_pend_nmi_age, 16, "Number of instructions allowed to retire before a pending nmi should be taken");
DEFINE_uint32(max_pend_intr_age, 128, "Number of instructions allowed to retire before a pending interrupt should be taken");
DEFINE_bool(preload, false, "Whisper preload");

DEFINE_int32(mcmi_poke_enables, 0, "MCM interface poke enables");
DEFINE_bool(psc_compare_only, true, "Peridoic COSIM will only compare current register states preload");
DEFINE_uint64(bridge_debug_cycle, 0, "enabled C debug messages at clock=<n>");
DEFINE_uint64(cosim_period, 0, "COSIM periodic mode enable");
DEFINE_uint64(bridge_cvm_debug_cycle, 0, "Turn on CVM debug at clock=<n>");

#define IF_DEBUG(str) if (debug_on_)  print(cvm::NONE, "DEBUG::line={: <5}::{: <30} ::{}\n",__LINE__,__FUNCTION__,str);
#define IF_DEBUG1(str,arg1) if (debug_on_)  print(cvm::NONE, "DEBUG::line={: <5}::{: <30} ::{}  arg={}\n",__LINE__,__FUNCTION__,str,arg1);
//#define IF_DEBUG(str) if (0)  print(cvm::NONE, "DEBUG::line={: <5}::{: <30} ::{}\n",__LINE__,__FUNCTION__,str);


#define log \
#   error "Don't use cvm::log, use print() instead. This will cause errors to be reported to rvfi and stop further cosim checking."

static std::vector<uint64_t> create_dword_vec(const std::bitset<256>& input) {
    // Calculate the number of 8-byte chunks needed for the 256-bit input
    size_t num_chunks = (256 + 63) / 64; // Round up division

    // Create a vector to store the chunks
    std::vector<uint64_t> dword_vec(num_chunks);

    // Convert and store each chunk of 64 bits (8 bytes)
    for (size_t i = 0; i < num_chunks; ++i) {
        uint64_t chunk = 0;
        for (size_t j = 0; j < 64; ++j) {
            size_t bit_index = i * 64 + j;
            if (bit_index < 256 && input[bit_index]) {
                chunk |= (uint64_t(1) << j);
            }
        }
        dword_vec[i] = chunk;
    }

    return dword_vec;
}

// Constructor
bridge::bridge(int num_harts, int xlen, int vlen, cvm::topology::loc_t loc, unsigned id)
  : bridge_log_("h" + std::to_string(id) + "_bridge.log"),
    loc_(loc),
    id_(id),
    num_harts_(num_harts),
    xlen_(xlen),
    vlen_(vlen),
    cac_(CacCore(num_harts)),
    csr_cac_(CacCore(num_harts))
{
    if (FLAGS_cosim_period > 0) {
      print(cvm::MEDIUM, "[RVFI loc {} id{}] COSIM periodic-state-check mode enabled\n", loc_, id_);
      if (FLAGS_cosim_resynch) {
        error("COSIM periodic-state-check mode enabled with cosim_resynch=1\n");
      }
      if (FLAGS_cosim_resynch_instr != "") {
        error("COSIM periodic-state-check mode enabled with cosim_resynch_instr being used\n");
      }
      //if (FLAGS_mcm == 1) {
        //error("COSIM periodic-state-check mode enabled with mcm=1 .. not yet validated\n");
      //}
    }

    // Reset value
    hw_mip_age_ = FLAGS_mip_resynch_threshold;

    cosim_resynch_csr_defaults = {

      //"htval","mtval2", // RVDE-10043
      "mtinst","htinst", // RVDE-10005
      "sstatus","mstatus","hstatus","mie","hie","vsie","sie", // RVDE-11840
      "vxsat", // Vectors RVDE-17338
      "tselect","tdata1","tdata2","tdata3","mcontext","tinfo", // Unimplemented: RVDE-7518, RVTOOLS-3124
      "fflags","fcsr", // Unimplemented
      "menvcfg","senvcfg","henvcfg", // FIXME: pointer masking change
      "pma","pmp", // FIXME: Performant NC change
      "vtype", // Permanent: Vector vtype will not be implemented
      "mip", "mvip", "hip","hgeip","vsip","hvip","sip","mireg","sireg","vsireg","mtopei","stopei","vstopei", // Permanent: Interrupts
      "mtopi", "stopi", "vstopi", // RVTOOLS-3189
      "hpmcounter","mcycle","minstret","minstreth", // Permanent: PMC events
      "dcsr","dpc","dscratch0", "dscratch1" // Permanent: Debug events

    };

    std::istringstream iss(FLAGS_cosim_resynch_csr);
    std::string token;
    while (std::getline(iss, token, ',')) {
        cosim_resynch_csr_defaults.push_back(token);
    }
    previous_cycle_ = 0;
    auto platform = cvm::topology::get_from_type("PLATFORM", 0);
    cvm::registry::messenger.connect<rv_tester::terminate_called>(platform, [this] (const auto& v) { return this->process(v); });
    if(FLAGS_random_imsic_intr){
       FLAGS_max_cycle = 2*FLAGS_max_cycle;
       print(cvm::LOW, "Doubling max_cycles for sim run to {}\n",FLAGS_max_cycle );
    }
    int32_t nharts = cvm::topology::attr(platform, "NHARTS").second;

    for(int32_t i = 0 ; i < nharts ; i++) {
      int unsigned location = cvm::topology::get_from_type("CORE", i);
      cvm::registry::messenger.connect<uint64_t>(location , [this] (const auto& payload) { return this->store_cbo_inv_addr(payload); });
    }

    if((FLAGS_max_stall_cycle < (20000 + (nharts-1)*2000)) && (FLAGS_max_stall_cycle != 0)){
        FLAGS_max_stall_cycle = (20000 + (nharts-1)*2000);
        print(cvm::LOW, "Overwriting max_stall_cycle to {} cycles\n",FLAGS_max_stall_cycle );
    }
    if((FLAGS_max_cycle < static_cast<gflags::uint64>(1000000 + (nharts - 1) * 75000)) && (FLAGS_max_cycle != 0) && (nharts != 1)){
        FLAGS_max_cycle = (1000000 + (nharts-1)*75000);
        print(cvm::LOW, "Overwriting max_cycle to {} cycles\n",FLAGS_max_cycle );
    }
    if((FLAGS_max_instr < static_cast<gflags::uint64>(100000 + (nharts - 1) * 20000)) && (FLAGS_max_instr != 0) && (FLAGS_eot != "max_instr") && (nharts != 1)){
        FLAGS_max_instr = (100000 + (nharts-1)*20000);
        print(cvm::LOW, "Overwriting max_instr to {} cycles\n",FLAGS_max_instr );
    }
}

// Destructor
bridge::~bridge() {}

void bridge::reset() {

  if (!cvm::registry::messenger.call<memmap::getRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.MEMMAP", 0), memmap_)) {
    error("Getting Memmap failed");
    return;

  }
  cac_.Reset();
  assert(cac_.SetVlen(vlen_));

  if (id_ == 0 && cvm::registry::messenger.call<whisperClient<uint64_t>::whisperConnectRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0)) != 0) {
    error("Hart {}: Failed whisper_connect\n", id_);
    return;
  }

  // Init csr reset values in cac
  csr_init();

  // Write num_harts to boot mem
  bool valid;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), id_, 0, 'm', memmap_.at("boot").base + boot_num_harts_offset, FLAGS_num_harts, valid) || !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed to poke boot memory\n", id_);
    return;
  }
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), id_, 0, 'm', memmap_.at("boot").base + boot_hart_sync_en_offset, FLAGS_hart_sync_en, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed to poke boot memory\n", id_);
    return;
  }

  if(FLAGS_enable_sp_init){ //only poke num ways when sp_init is required
    uint64_t poke_data = uint64_t(FLAGS_enable_sp_init);
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', memmap_.at("boot").base + boot_sp_init_offset, 8, poke_data, valid)|| !valid) && FLAGS_whisper_client_check){
      error("Hart {}: Failed to poke boot memory\n", id_);
      return;
    }
    poke_data = uint64_t(FLAGS_num_sp_ways);
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', memmap_.at("boot").base + boot_sp_ways_offset, 8, poke_data, valid)|| !valid) && FLAGS_whisper_client_check){
       error("Hart {}: Failed to poke boot memory\n", id_);
       return;
    }
  }
  if (FLAGS_matp_swid) {
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', memmap_.at("boot").base + boot_matp_swid_offset, 8, uint64_t(FLAGS_matp_swid), valid)|| !valid) && FLAGS_whisper_client_check){
      error("Hart {}: Failed to poke boot memory to write matp\n", id_);
      return;
    }

  }

  cvm::registry::messenger.signal<uint64_t>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0), uint64_t(0));
}

void bridge::store_cbo_inv_addr(const uint64_t& payload) {
  curr_cbo_inv_addr_ = payload;
  print(cvm::FULL, "CBO INVAL ADDRESS from CBO_inval_monitor : {:#x}\n",curr_cbo_inv_addr_);
}

void bridge::get_gp_reg(uint32_t reg, uint64_t& data)
{
    if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekGprRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), id_, reg, data)) {
        error("Hart {}: Failed to peek GP {}\n", id_,reg);
    }
}
void bridge::get_fp_reg(uint32_t reg, uint64_t& data)
{
    if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekFprRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), id_, reg, data)) {
        error("Hart {}: Failed to peek FP {}\n", id_,reg);
    }
}

void bridge::get_vec_reg(uint32_t reg, std::array<std::uint8_t, 32>& data)
{
    if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekVprRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), id_, reg, data)) {
        error("Hart {}: Failed to peek VEC {}\n", id_,reg);
    }
}

void bridge::csr_init() {
  bool valid;
  uint64_t data, mask, poke_mask, read_mask;
  for (const auto& csr_: csrs) {
    auto csr = csr_.second;
    if (csr.nonzero_reset) {
      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), id_, csr.addr, data, mask, poke_mask, read_mask, valid)|| !valid) && FLAGS_whisper_client_check)
        error("Hart {}: Failed to peek csr : {:#x} in csr_int()\n", id_ ,csr.addr);
      uint64_t cac_mask = 0xffffffffffffffff;
      update_csr(id_, src_t::dut, csr.addr, data, cac_mask);
      update_csr(id_, src_t::iss, csr.addr, data, cac_mask);
      csr_cac_.Step(id_, false);
    }
  }

  // CSR rename
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), id_, C_FECFG2, data, mask, poke_mask, read_mask, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed to peek csr : C_FECFG2\n", id_);
  }
  csr_rename_en_ = !((data & 0x200) >> 9);
  csr_rd_opt_ = !((data & 0x4) >> 2);
}

void bridge::process_compare_gp_regs(hart_id_t hart, uint64_t cycle, const std::array<std::uint64_t, 32>& array) {
    if (!FLAGS_whisper_exec) {
       return;
    }
    for(int i=0;i<32;i++) {
       uint64_t data;
       pd_.gpr.emplace_back(true, i, array[i]);
       get_gp_reg(i, data);
       if (!FLAGS_psc_compare_only) {
          update_regs(hart, src_t::dut, resource_t::int_reg, i, {array[i]});
          update_regs(hart, src_t::iss, resource_t::int_reg, i, {data});
       }
       else {
          if (data != array[i]) {
            error("cycle={} hart={}: GP[{}] MISMATCH: DUT={:#x} ISS={:#x}\n", cycle,hart,i,array[i],data);
          }
       }
    }
    if (!FLAGS_psc_compare_only) {
        compare_dut_whisper_state(hart, pw_, pd_);
    }
    else {
       print(cvm::HIGH, "Hart {}:PSC GP compare only mode\n", hart);
    }
}
void bridge::process_compare_fp_regs(hart_id_t hart, uint64_t cycle, const std::array<std::uint64_t, 32>& array) {
    if (!FLAGS_whisper_exec) {
       return;
    }
    for(int i=0;i<32;i++) {
        uint64_t data;
        pd_.fpr.emplace_back(true, i, array[i]);
        get_fp_reg(i, data);

        if (!FLAGS_psc_compare_only) {
            update_regs(hart, src_t::dut, resource_t::fp_reg, i, {array[i]});
            update_regs(hart, src_t::iss, resource_t::fp_reg, i, {data});
        }
        else {
           if (data != array[i]) {
             error("cycle={} hart={}: FP[{}] MISMATCH: DUT={:#x} ISS={:#x}\n", cycle,hart,i,array[i],data);
           }
        }
    }
    if (!FLAGS_psc_compare_only) {
        compare_dut_whisper_state(hart, pw_, pd_);
    }
    else {
       print(cvm::HIGH, "Hart {}:PSC FP compare only mode\n", hart);
    }
}

void bridge::process_compare_vc_regs(hart_id_t hart, uint64_t cycle, const std::array<std::uint64_t, 32>& array) {
    std::array<std::bitset<256>, 32> data;
    for(int i=0;i<32;i++) {
        data[i] = array[i];
    }
    process_compare_vc_regs(hart,cycle, data);
}

void bridge::process_compare_vc_regs(hart_id_t hart, uint64_t cycle, const std::array<std::bitset<256>, 32>& array) {
    if (!FLAGS_whisper_exec || !FLAGS_vec_check) {
       return;
    }
    for(int i=1;i<32;i++) {
        std::array<std::uint8_t, 32> data8;
        std::vector<uint64_t> data64;
        std::vector<uint64_t> dut64;
        pd_.vr.emplace_back(true, i, array[i]);
        get_vec_reg(i, data8);
        for(int j=0;j<4;j++) {
            uint64_t q = 0;
            for (int k = 0; k < 8; k++) {
                q = q | uint64_t(data8[8*j + k]) << (k*8);
            }
            data64.push_back(q);
        }
        if (!FLAGS_psc_compare_only) {
            update_regs(hart, src_t::dut, resource_t::vec_reg, i, create_dword_vec(array[i]));
            update_regs(hart, src_t::iss, resource_t::vec_reg, i, std::move(data64));
        }
        else {
            dut64 = create_dword_vec(array[i]);
            for (int k = 0; k < 3; k++) {
               if (dut64[k] != data64[k]) {
                   error("cycle={} hart={}: VEC[{}][{}:{}] mismatch: dut={:#x} iss={:#x}\n", cycle,hart,i,k*32+31,k*32,dut64[k],dut64[k]);
               }
            }
        }
    }
    if (!FLAGS_psc_compare_only) {
        compare_dut_whisper_state(hart, pw_, pd_);
    }
    else {
        print(cvm::HIGH, "Hart {}:PSC VEC compare only mode\n", hart);
    }
}

// DUT interface callback: Step Whisper
void bridge::process_steps(hart_id_t hart, uint32_t n_retire, uint64_t cycle, uint64_t steps, uint64_t skips, uint64_t final_steps) {

  print(cvm::HIGH, "process_steps:: hart={}, cycle={}, steps={}, skips={}, final_steps={}\n", hart,cycle,steps,skips,final_steps);

  psc_stepping_ = false;

  if (((skips >> 63) & 0x1) == 1) {
     skips = 0;
  }

  whisper_state_t w {
    .tag =  pw_.tag,
    .time = pw_.time,
  };


  end_time_ = std::chrono::high_resolution_clock::now();
  if (first_call_ == false) {
  }
  if (first_call_) {
      start_of_test_ = end_time_;
  }

  previous_cycle_ = cycle;
  first_call_ = false;

  //----------------------------------------------------------------------
  // Process the MISSING (or dropped steps accumulated so far)
  //----------------------------------------------------------------------
  for(uint64_t s=0;s<steps;s++) {
      uint64_t t;
      w.tag++;
      t = w.tag;
      if (FLAGS_mcm) {
         print(cvm::HIGH, "process_steps:: hart={}: MCM enabled ... checking tag={} \n", hart,t);
         while(mcm_orders_.find(t) != mcm_orders_.end()) {       // MCM sent this order
            print(cvm::HIGH, "process_steps:: hart={}: need to skip tag={} as it has an mcm tag associated with it\n", hart,t);
            w.tag++;                                                 // skip it so we don't confuse whisper
            t = w.tag;
            if (skips > 0) {
               skips--;                                                 // decrement the skip count
            }
         }
      }
      //----------------------------------------------------------------------------------------------------------------------------------------
      // create pseudo-time-stamp by advancing the timestamp ever Nth whisper step/tag
      // ex: 20 steps = 3 timestamps of T,T+1,T+2  with retire counts of 8,8,4  respectively if CPU can retire max of 8 instructions per clock
      //     We dont' know the real timestamps as that was lost
      //----------------------------------------------------------------------------------------------------------------------------------------
      if ((s % n_retire) == 0) {
         w.time++;
      }

      if (FLAGS_whisper_exec) {
          auto stime = std::chrono::high_resolution_clock::now();
          step(hart, w);
          auto etime = std::chrono::high_resolution_clock::now();
          whisper_time_ = whisper_time_ + (duration_cast<std::chrono::microseconds>(etime - stime).count());
      }

      // Increment step count
      step_++;
  }


  //-------------------------------------------------------
  // Add skips caused by out-of-order tag bits
  //-------------------------------------------------------
  w.tag += skips;

  //------------------------------------------------------
  // now set time to current sim time
  //------------------------------------------------------
  w.time = cycle;

  //------------------------------------------------------
  // Process FINAL steps of the RVFI packet if needed
  //------------------------------------------------------
  for(uint64_t s=0;s<final_steps;s++) {
      w.tag++;
      //----------------------------------------------------------------------------------------------------------------------------------------
      // create pseudo-time-stamp by advancing the timestamp ever Nth whisper step/tag
      // ex: 20 steps = 3 timestamps of T,T+1,T+2  with retire counts of 8,8,4  respectively if CPU can retire max of 8 instructions per clock
      //     We dont' know the real timestamps as that was lost
      //----------------------------------------------------------------------------------------------------------------------------------------
      if ((s % n_retire) == 0) {
         w.time++;
      }

      if (FLAGS_whisper_exec) {
          auto stime = std::chrono::high_resolution_clock::now();
          step(hart, w);
          auto etime = std::chrono::high_resolution_clock::now();
          //print(cvm::HIGH, "process_steps:: hart={}, cycle={}, FINAL STEP: w.tag={}  w.time={}{\n", hart,cycle,w.tag,w.time);
          whisper_time_ = whisper_time_ + (duration_cast<std::chrono::microseconds>(etime - stime).count());
      }

      // Increment step count
      step_++;
  }


  ppw_ = pw_;
  pw_ = w;
  pd_ = rv_instr_t{};
  psc_stepping_ = false;
}

// DUT interface callback: Instruction Retire
void bridge::process_dut_instr_retire(hart_id_t hart, rv_instr_t& d) {

  print(cvm::HIGH, "process_dut_instr_retire:: hart={}, d.cycle={}, d.pc={:#x}, d.tag={}, d.excp={}, d.ecause={}, d.opcode={:#x}, d.disasm={}\n", hart,d.cycle,d.pc.pc_rdata,d.tag,d.excp,d.ecause,d.opcode,d.disasm);
  print(cvm::HIGH, "                        :: mip_={}, deferred_intr_={} patch_mode_={} trap={}\n", mip_.to_ullong(),deferred_intr_,patch_mode_,d.trap);
  for (const auto& gpr : d.gpr) {
    print(cvm::HIGH, "                        :: grd_addr={}, grd_wdata={:#x}\n", gpr.rd_addr,gpr.rd_wdata);
  }

  if ((d.cycle >= FLAGS_bridge_debug_cycle) & (FLAGS_bridge_debug_cycle > 0) & !debug_on_) {
     print(cvm::MEDIUM,"Setting debug_on_ = true\n");
     debug_on_ = true;
  }
  if ((d.cycle >= FLAGS_bridge_cvm_debug_cycle) & (FLAGS_bridge_cvm_debug_cycle > 0) & !cvm_debug_) {
     print(cvm::MEDIUM,"Setting CVM verbosity to DEBUG\n");
     cvm::logger::set_verbosity(cvm::DEBUG);
     cvm_debug_ = true;
  }

  is_priv_debug_mode_ = ((d.priv == 6) || (d.priv ==7));

  if (FLAGS_mcm & (FLAGS_cosim_period > 0))
    mcm_orders_.erase(d.tag);

  twoStage_ = false;

  whisper_state_t w {
    .tag = d.tag,
    .time = d.cycle
  };

  //------------------------------------------------------------------------------------------------------------
  // advance the instruction steps missed when using state-compare method and not sending rvfi packets
  //   - we only advance the missing steps on the FIRST rvfi packet sent on this time-stamp (cycle)
  //------------------------------------------------------------------------------------------------------------
  rvfi_calls_++;

  if (!FLAGS_whisper_exec)
   return;
  w.tag  = d.tag;
  w.time = d.cycle;

  // Handle debug interrupt
  IF_DEBUG("check dut interrupt");
  if (d.intr && (d.icause == 0)){
    IF_DEBUG("dut has interrupt cause=0");
    return;
  }

  IF_DEBUG("check dut single step");
  if (d.excp && (d.ecause == 31)){
    IF_DEBUG("dut single step excp cause=31");
    return;
  }

  // Handle pre-step condition - Debug
  if (debug_mode_) {
    if (FLAGS_emulate_debug_mode) {
      pre_step_debug_poke(hart, d);
    } else {
      return;
    }
  }

  // Handle pre-step condition - Exceptions
  pre_step_exception_poke(hart, d);

  // Handle pre-step condition - Interrupts
  pre_step_nmi_poke(hart, d, w);
  pre_step_interrupt_poke(hart, d, w);
  lrsc_fail_ = false;

  // Handle pre-step condition - LR/SC fail
  pre_step_lrsc_poke(hart, d);

  // Step whisper
  w_.clear();

  if (patch_mode_ == NO_PATCH || ((patch_mode_ == EXIT_PATCH) )) {
    auto stime = std::chrono::high_resolution_clock::now();
    IF_DEBUG("STEP now:  either no-patch or exit-patch");
    step(hart, w);
    step_++;
    auto etime = std::chrono::high_resolution_clock::now();
    whisper_time_ = whisper_time_ + (duration_cast<std::chrono::microseconds>(etime - stime).count());
  }
  // Update cac with whisper state
  if (!psc_stepping_) {
    if (patch_mode_ == NO_PATCH || patch_mode_ == EXIT_PATCH) {
      IF_DEBUG("updating whisper state");
      update_whisper_state(hart, w, d.comp);
    }

    // Update cac with dut state
    IF_DEBUG("updating dut state");
    update_dut_state(hart, d);
  }

  arch_state(w);

  // Fail right away if unexpected instruction as per plusarg
  // Don't fail on instruction page faults
  std::string instr = cosim_util::get_nth_word(w.disasm, 1);
  if (found_in_list(instr, FLAGS_cosim_error_instr)) {
    if (instr == "illegal" && d.excp && d.ecause == INSN_PAGE_FAULT) {
      IF_DEBUG("skipping illegal instruction error");
      // Skip the error
    } else {
      error("Hart {}: Unexpected instruction: +cosim_error_instr {}\n", hart, instr);
      return;
    }
  }

  // Handle post-step conditions
  if (d.pc.pc_rdata == FLAGS_debug_exit_pc) {
    if (nmi_poke_pending_ && nmi_poke_in_debug_mode_) {
      clear_nmi(hart, d.cycle); // it will be later poked when DUT takes NMI
      nmi_poke_in_debug_mode_ = false;
    }
    if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperExitDebugRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart))
      error("Hart {}: Failed to exit debug mode\n", id_);
  }

  post_step_debug_poke(hart, d);
  post_step_nmi_check(hart, d, w);
  post_step_interrupt_check(hart, d, w);
  post_step_exception_check(hart, d, w);
  post_step_satp_write_poke(hart, d, w);

  if (excp_in_debug_mode) {
    IF_DEBUG("excp_in_debug_mode==1 ..reset status and return");
    cac_.ResetStatus(hart);
    return;
  }
  IF_DEBUG("no excp in debug mode...keep going");

  // Save whisper state
  if (patch_mode_ == NO_PATCH || patch_mode_ == EXIT_PATCH) {
    ppw_ = pw_;
    pw_ = w;
    pd_ = d;
    // Check dut vs whisper
    compare_dut_whisper_state(hart, w, d);
    // Save whisper state
    // Interrupt state
    prev_nmi_ = nmi_;
    prev_hw_mip_ = hw_mip_;
    prev_e_mip_ = e_mip_;
    hw_mip_age_++;
    e_mip_age_++;
    msi_.clear();
  }

  // TLB checks
  if (patch_mode_ == NO_PATCH)
    translation_check(hart, d, w);

  if (patch_mode_ == ENTER_PATCH)
    patch_mode_ = IN_PATCH;

  if (patch_mode_ == EXIT_PATCH)
    patch_mode_ = NO_PATCH;
}

void bridge::resynch_whisper_on_patch(hart_id_t hart, rv_instr_t& d, const std::string&, const whisper_state_t& w){
  if (w.is_load) {
    uint64_t pa = 0;
    if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperGetLastLdStAddressRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, pa))
      error("Hart {}: Failed to get last Load Store Address\n", hart);
    if (resynch_on_pa(pa, d.cycle)) {
      bool valid;
      uint64_t rd = (w.opcode & 0xf80)>>7;
      resource_id_t rid = resource_id_t{
        .resource = resource_t::int_reg,
        .offset = rd
      };
      std::vector<bool> bool_vec;
      if (!cac_.GetResource(hart, src_t::dut, rid, bool_vec))
        error("Hart {}: CAC: Failed to GetResource: Register\n", hart);
      uint64_t dword_vec = cac::CreateSizedVec<uint64_t>(bool_vec)[0];
      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, d.cycle, 'r', rd, dword_vec, valid)|| !valid) && FLAGS_whisper_client_check)
      error("Hart {}: Failed to poke Whisper\n", hart);
    }
  }
}

void bridge::compare_dut_whisper_state(hart_id_t hart, const whisper_state_t& w, rv_instr_t& d) {

  const auto cac_status_verbosity = cvm::HIGH;
  cac_.Step(hart, cvm::logger::check_verbosity(cac_status_verbosity));
  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "{}", cac_.GetStatusStr(hart));

  std::string resource = cac_.GetResourceStr(hart);
  std::string instr = cosim_util::get_nth_word(w.disasm, 1);
  if (instr.substr(0,3) == "csr") {
    instr = "csr:" + cosim_util::get_nth_word(w.disasm, 3);
  }

  // error on mismatch
  if (!cac_.GetStatus(hart)) {
    IF_DEBUG("CaC compare failed...here");
    cac_.ResetStatus(hart);
    // Resynch whisper with dut state if needed
    // to continue without failing
    if (FLAGS_cosim_resynch) {
      if (FLAGS_bridge_log)
        bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[+cosim_resynch]\n", d.cycle);
      resynch(hart, d);
    // One of many other reasons checked
    } else if (resynch_needed(hart, d, instr, w)) {
      IF_DEBUG("matched condition for a resynch here");
      resynch(hart, d);
    } else if (patch_mode_ == EXIT_PATCH) {
      if (FLAGS_bridge_log)
        bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[exit_patch]\n", d.cycle);
      resynch_whisper_on_patch(hart, d, instr, w);
    } else {
      print_instr_stdout(hart, w);
      print(cvm::NONE, "{}", cac_.GetStatusStr(hart));
      error("Hart {}: Core Arch Checker Mismatch - {} - {}\n", hart, resource,  instr);
      return;
    }
  }
  else {
    //------------------------------------------------------------------------------------------------------------
    // State compares:
    //  - for periodic mode, registers are only updated periodically and so disable GP/FP/VEC comparisons
    //    therefore we could miscompare still from a a device memory read
    //------------------------------------------------------------------------------------------------------------
    if (FLAGS_cosim_period != 0) {
      IF_DEBUG("CaC compared... but in PSC mode we still need to check");
      bool atomic_op = false;
      if (instr.substr(0,3) == "amo") {
        atomic_op = true;
      }
      if (instr.substr(0,3) == "csr") {
        instr = "csr:" + cosim_util::get_nth_word(w.disasm, 3);
      }
      if (resynch_needed(hart, d, instr, w) & !atomic_op) {
        IF_DEBUG("found condition for resynch");

        if (!(unsupported_csr_access(instr))) {
           for (auto& csr : d.csr) {
              csr.valid = 0;
           }
        }
        resynch(hart, d);
        cac_.ResetStatus(hart);
      }
    }
  }
}

void bridge::process_dut_csr_hw_update(hart_id_t hart, csr_t& c) {
  if (c.csr_addr == MIP)
    return;

  uint64_t mask = c.csr_wmask & static_cast<uint64_t>(get_csr_poke_mask(hart, c.csr_addr));
  update_csr(hart, src_t::dut, c.csr_addr, c.csr_wdata, mask);
}

void bridge::process_dut_instr_group_retire(hart_id_t hart, rv_instr_group_t& d) {
  if (patch_mode_)
    return;
  if (!FLAGS_csr_wr_check)
    return;

  const auto cac_status_verbosity = cvm::HIGH;
  // Step csr cac
  csr_cac_.Step(hart, cvm::logger::check_verbosity(cac_status_verbosity));
  std::string status = csr_cac_.GetStatusStr(hart);
  if (FLAGS_bridge_log && status != "")
    bridge_log_(cac_status_verbosity, "CSR {}", status);

  // error on mismatch
  if (!csr_cac_.GetStatus(hart)) {
    std::string csr = get_csr_name(csr_cac_.GetResourceStr(hart).substr(2));
    csr_cac_.ResetStatus(hart);
    if (resynch_csr_) {
      if (FLAGS_bridge_log)
        bridge_log_(cvm::MEDIUM, "<{}> CSR HW Resynch: Reason=[csr_sw_resynch?]\n", d.cycle);
      resynch_csr_ = false;
    } else {
      for (const auto& token_csr : cosim_resynch_csr_defaults) {
        if (csr.find(token_csr) != std::string::npos){
          if (FLAGS_bridge_log)
            bridge_log_(cvm::FULL, "<{}> CSR HW Check Skip: Reason=[found in resynch_csr_defaults list]\n", d.cycle);
          return;
        }
      }
      for (auto & i : d.instrs)
        print_instr_stdout(hart, i);
      print(cvm::NONE, "{}", csr_cac_.GetStatusStr(hart));
      if(FLAGS_whisper_client_check)
        error("Hart {}: CSR Write Mismatch - {}\n", hart, csr);
      return;
    }
  }

}

void bridge::update_dut_state(hart_id_t hart, rv_instr_t& d) {
  if (FLAGS_pc_check && (patch_mode_ == NO_PATCH || patch_mode_ == ENTER_PATCH)) {
    update_pc(hart, src_t::dut, d.pc.pc_rdata);
  }
  if (FLAGS_priv_check && (patch_mode_ == NO_PATCH || patch_mode_ == ENTER_PATCH)) {
    if (d.priv == DP)
      d.priv = DE;
    update_priv(hart, src_t::dut, d.priv);
  }
  if (FLAGS_insn_check && !d.comp && !d.ucode && !is_vector(d.disasm) && !(d.disasm.substr(0,7)=="illegal") && !d.csr_renamed && (patch_mode_ == NO_PATCH  || patch_mode_ == ENTER_PATCH)) {
    update_insn(hart, src_t::dut, d.opcode);
  }
  if (FLAGS_flags_check && (d.flags != 0)) {
    update_flags(hart, src_t::dut, d.flags);
  }
  if (!d.gpr.empty() || !d.fpr.empty() || !d.vr.empty() || !d.csr.empty()) {
    update_regs(hart, d);
  }
  if (FLAGS_memattr_check && d.mem_read.valid &&  (!is_vector(d.disasm)) && !lrsc_fail_ && patch_mode_ == NO_PATCH) {
      update_mem_attr(hart, src_t::dut, d.mem_read.attr);
  }
  if (FLAGS_memattr_check && d.mem_write.valid && (!is_vector(d.disasm)) && !lrsc_fail_ && patch_mode_ == NO_PATCH) {
      update_mem_attr(hart, src_t::dut, d.mem_write.attr);
  }
}


void bridge::post_step_debug_poke(hart_id_t hart, const rv_instr_t& instr) {
  if (instr.comp && !w_.comp) { // few cases RTL auto expands a compressed OP to 4-byte
    auto client = cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0);
    uint64_t pc;
    bool valid;
    if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekPcRPC>(client, hart, pc) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed to peek PC in debug mode\n", hart);
      return;
    }
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(client, hart, instr.cycle, 'p', 0/*addr*/,  pc-2, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed to poke PC in debug mode\n", hart);
      return;
    }
  }
}

void bridge::pre_step_debug_poke(hart_id_t hart, const rv_instr_t& instr) {
  print(cvm::MEDIUM, "Debug pre step poking instruction in Debug mode\n", hart);
  bool valid;
  uint32_t opcode;
  if (instr.pc.pc_rdata == FLAGS_debug_exit_pc) {
    opcode = opcode_nop;
  }
  else if(instr.excp && (instr.ecause == 3)) { // This is to exit the abstract cmd routine to Park loop at the end of abstract command completion
    opcode = opcode_ebreak; //E-break opcode
  }
  else if (instr.excp) { // In case of other exceptions since RVFI only get's u-op codes, can't poke whisper valid opcode to hit exception. Thus we poke illegal opcode to mimic an exception.
    opcode = 0x0; //Illegal opcode
  }
  else {
    opcode = instr.opcode;
  }

  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, 0, 'm', instr.pc.pc_rdata, 4 /*Size*/, opcode, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed to poke memory\n", hart);
    return;
  }
  return;
}

void bridge::pre_step_exception_poke(hart_id_t hart, const rv_instr_t& d) {
  if (!d.excp) {
    IF_DEBUG("d.excp==0");
    return;
  }

  if (found_in_list(std::to_string(d.ecause), FLAGS_cosim_resynch_excp)) {
    bool valid;
    bool is_load = (d.ecause == LD_ACCESS_FAULT) || (d.ecause == HARDWARE_ERROR);
    bridge_log_(cvm::MEDIUM, "<{}> Inject Exception with code:{} is_load: {}\n", d.cycle, d.ecause, is_load);
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperInjectExceptionRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0),
      hart, is_load, d.ecause, 0, valid) || !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed whisper API InjectException\n", hart);
    }
    return;
  }
}

void bridge::pre_step_lrsc_poke(hart_id_t hart, const rv_instr_t& d) {
  // https://en.wikipedia.org/wiki/Load-link/store-conditional
  if ((d.disasm.find("sc.w") != std::string::npos) ||
      (d.disasm.find("sc.d") != std::string::npos)) {
    // Check if Store-Conditional (SC) failed
    uint64_t fail_code = 1;
    if (d.mem_read.data == fail_code) {
      lrsc_fail_ = true;
      bool valid;
      // Cancel Load-Reserved (LR)
      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperCancelLrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, valid)|| !valid) && FLAGS_whisper_client_check) {
        error("Hart {}: Failed to CancelLr\n", hart);
      }
    }
  }
}

void bridge::pre_step_nmi_poke(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {
  if (!nmi_poke_pending_ && !d.nmi)
    return;

  if (!d.nmi && nmi_poke_pending_) {
    nmi_age_++;
    if (FLAGS_bridge_log)
      bridge_log_(cvm::HIGH, "<{}> nmi_age++={}\n", w.time, nmi_age_);
    if ((nmi_age_ > FLAGS_max_pend_nmi_age) && !FLAGS_cosim_resynch && !FLAGS_intr_timeout_resynch) {
      error("Hart {}: Whisper wants to take NMI, DUT does not. timeout: [{}] retires\n",
        hart, FLAGS_max_pend_nmi_age);
    }
    return;
  }

  // Timing sensitive resynch cases
  // 1. DUT took nmi that deasserted before retire
  if (d.nmi && !nmi_poke_pending_ && (prev_nmi_.valid != nmi_.valid)) {
    if (FLAGS_bridge_log)
      bridge_log_(cvm::MEDIUM, "<{}> DUT took NMI, Whisper does not want to. cause:[{}] (Timing sensitive mismatch: Resynch and keep going)\n", w.time, prev_nmi_.cause);
    poke_nmi(hart, d.cycle, prev_nmi_.cause);
    nmi_poke_pending_ = false;
    return;
  }

  nmi_age_ = 0;
  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> NMI taken by DUT. dcause:[{}]\n", w.time, nmi_.cause);

  // Poke nmi into whisper
  poke_nmi(hart, nmi_.cycle, nmi_.cause);
  nmi_poke_pending_ = false;
  nmi_taken_count_++;
}

void bridge::pre_step_interrupt_poke(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {
  // Set mip ages for resynch cases
  if (prev_hw_mip_ != hw_mip_)
    hw_mip_age_ = 0;
  if (prev_e_mip_ != e_mip_)
    e_mip_age_ = 0;

  // Proceed only if DUT takes interrupt
  if (hw_mip_ == 0 && prev_hw_mip_ == 0 && !d.intr) {
    IF_DEBUG("hw_mip_==0  and prev_hw_mip_==0 ... return");
    return;
  }

  bool w_intr;
  uint64_t w_cause;
  check_interrupt(hart, d.cycle, w_intr, w_cause);

  if (!d.intr && !w_intr) {
    IF_DEBUG("no dut intr and no whisper intr....return");
    return;
  }

  if (!d.intr && w_intr) {
    IF_DEBUG("no dut intr ... but whisper has intr");
    intr_age_[w_cause]++;
    if (FLAGS_bridge_log)
      bridge_log_(cvm::HIGH, "<{}> intr_age_[{}][{}]++={}\n", w.time, hart, w_cause, intr_age_[w_cause]);

    // Check that interrupt age is not beyond threshold
    if ((intr_age_[w_cause] > FLAGS_max_pend_intr_age) && !FLAGS_cosim_resynch && !FLAGS_intr_timeout_resynch) {
      error("Hart {}: Whisper wants to take interrupt, DUT does not. wcause: [{}], timeout: [{}] retires\n",
        hart, w_cause, FLAGS_max_pend_intr_age);
    }
    return;
  }

  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> Interrupt taken by DUT. dcause:[{}] wcause:[{}], d_intr:[{}] w_intr:[{}]\n", w.time, d.icause, w_cause, d.intr, w_intr);

  // Currently for interrupts taken to VS mode, w_cause and d.icause differ by 1
  // We will calculate next privilige mode to address cause mismatch issue and also for printing interrupt stats

  bool valid;
  uint64_t hideleg = 0, mideleg;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, 'c', MIDELEG, mideleg, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed to peek mip\n", hart);
    return;
  }
  if (hyp_enabled()) {
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, 'c', HIDELEG, hideleg, valid) || !valid) && FLAGS_whisper_client_check) {
      print(cvm::MEDIUM,"Trying to peek hideleg\n");
      error("Hart {}: Failed to peek mip\n", hart);
      return;
    }
  }
  bool hdel = hideleg & (1ull << w_cause);
  bool mdel = mideleg & (1ull << w_cause);
  if (d.priv == M) { intrtopriv_ = M;}
  else if (d.priv == HS || d.priv == U)  { intrtopriv_ = mdel ? HS : M;}
  else if (d.priv == VS || d.priv == VU) { intrtopriv_ = mdel ? (hdel ? VS : HS) : M;}

  if (intrtopriv_ == VS || intrtopriv_ == VU) {w_cause--;}

  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> Interrupt to privilege {} \n", w.time, intrtopriv_);

  // Timing sensitive resynch cases
  // 1. DUT took older interrupt that deasserted before retire
  if (d.intr && !w_intr && !FLAGS_cosim_resynch) {
    IF_DEBUG("dut intr==1 and whisper intr==0");
    if (FLAGS_bridge_log)
      bridge_log_(cvm::MEDIUM, "<{}> DUT took interrupt, Whisper did not. dcause:[{}] prev_mip:{}\n", w.time, d.icause, prev_hw_mip_[d.icause]);
    if (prev_hw_mip_[d.icause]) {
      bridge_log_(cvm::MEDIUM, "<{}> cause:[{}] (Timing sensitive mismatch: Resynch and keep going)\n", w.time, d.icause);
      if(d.icause != 9)
        poke_mip(hart, w.time, (uint64_t)1 << d.icause);
      resynch_icause_ = d.icause;
      // Undefer all interrupts
      if (deferred_intr_) {
        defer_interrupt(hart, w.time, 0);
        deferred_intr_ = false;
      }
    }
    return;
  }

  // 2. DUT took older interrupt but a newer one asserted before retire
  if (d.icause != w_cause) {
    IF_DEBUG("dut cause != whisper cause");
    if (FLAGS_bridge_log)
      bridge_log_(cvm::MEDIUM, "<{}> DUT vs Whisper interrupt cause mismatch [{},{}] age [{},{}] \n",
        w.time, d.icause, w_cause, intr_age_[d.icause], intr_age_[w_cause]);
    if (prev_hw_mip_[d.icause]) {
      std::bitset<64> timing_case_w_mip;
      bridge_log_(cvm::MEDIUM, "<{}> cause: [{}] (Timing sensitive mismatch: Resynch and keep going)\n",
        w.time, d.icause);
      peek_mip(hart, w.time, timing_case_w_mip);
      if(d.icause != 9)
        poke_mip(hart, w.time, timing_case_w_mip.to_ullong() | (uint64_t)1 << d.icause); // Combination of case 1 and 2 where whisper is not seeing the interrupt currently being serviced by DUT and there is another interrupt also pending in both DUT and whisper.
      defer_interrupt(hart, w.time, mip_.to_ullong() & ~((uint64_t)1 << d.icause));
      timing_case2 = 1;
    }
    return;
  }

  // Undefer all interrupts
  if (deferred_intr_) {
    IF_DEBUG("deferred intr == 1");
    defer_interrupt(hart, w.time, 0);
    deferred_intr_ = false;
  }

  if (FLAGS_retire_ucode_trap) {
    IF_DEBUG("FLAG retire_ucode_trap == 1 ... return");
    return;
  }

  step(hart, w);
  IF_DEBUG("add an extra 'step' to whisper");
  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Extra step due to interrupt\n", w.time, step_);
}

void bridge::post_step_interrupt_check(hart_id_t hart, const rv_instr_t& d, const whisper_state_t& w) {

  // FIXME if (FLAGS_intr_defer_spcl) {
  // FIXME   IF_DEBUG("FLAG intr_defer_spcl==1");
  // FIXME   if (w.disasm.find("vstimecmp") != std::string::npos && !w_.excp)  {
  // FIXME     IF_DEBUG("VSTIMECMP instruction");
  // FIXME     if (!vstimecmppoked_) resetsstc_poke(hart,d.cycle, VSTIMECMP); else setsstc_poke(hart,d.cycle, VSTIMECMP);
  // FIXME   } else if (w.disasm.find("stimecmp") != std::string::npos && !w_.excp) {
  // FIXME     IF_DEBUG("STIMECMP instruction");
  // FIXME     if (w.priv_mode == VS) {if (!vstimecmppoked_) resetsstc_poke(hart,d.cycle, VSTIMECMP); else setsstc_poke(hart,d.cycle, VSTIMECMP);}
  // FIXME     else if (!stimecmppoked_)  resetsstc_poke(hart,d.cycle, STIMECMP); else setsstc_poke(hart,d.cycle, STIMECMP);
  // FIXME   }
  // FIXME }


  if (!d.intr && !w_.intr) {
    IF_DEBUG("d.intr==0 and w.intr==0  .. return");
    return;
  }

  if (intr_age_[w_.icause] > max_pend_intr_age_)
    max_pend_intr_age_ = intr_age_[w_.icause];

  intr_age_[w_.icause] = 0;

  // If interrupt asserted via csr write, we don't need to defer
  // DUT is expected to take at retire boundary if whisper takes the undeferred interrupt
  if (w_.intr && !d.intr && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    error("Hart {}: Whisper took interrupt, DUT did not. wcause:[{}]\n", hart, 
      intr_to_string.count(static_cast<intr>(w_.icause)) ? intr_to_string.at(static_cast<intr>(w_.icause)) : std::to_string(w_.icause));
    return;
  }

  if (d.intr && !w_.intr && !FLAGS_cosim_resynch) {
    IF_DEBUG("d.intr==1 and w.intr==0  .. return IF d.cause==0");
    // If Debug mode intterupt is seen, don't flag an error, Whisper gets poked based on PC fetches
    if (d.icause == 0)
      return;

    print_instr_stdout(hart, w);
    error("Hart {}: DUT took interrupt, Whisper did not. dcause:[{}]\n", hart,
      intr_to_string.count(static_cast<intr>(d.icause)) ? intr_to_string.at(static_cast<intr>(d.icause)) : std::to_string(d.icause));
    return;
  }

  // DUT cause should match whisper cause
  if ((d.icause != w_.icause) && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    error("Hart {}: DUT vs Whisper interrupt cause mismatch. dcause:[{}] wcause:[{}]\n", hart, 
      intr_to_string.count(static_cast<intr>(d.icause)) ? intr_to_string.at(static_cast<intr>(d.icause)) : std::to_string(d.icause),
      intr_to_string.count(static_cast<intr>(w_.icause)) ? intr_to_string.at(static_cast<intr>(w_.icause)) : std::to_string(w_.icause));
    return;
  }

  num_taken_interrupts_[intrtopriv_][w_.icause]++;

  // Timing sensitive mismatch cases
  if (resynch_icause_) {
    IF_DEBUG("resynch_icause_==1");
    std::bitset<64> resynch_mip_mask, resynch_mip;
    resynch_mip_mask = (1 << resynch_icause_);
    resynch_icause_ = 0;
    peek_mip(hart, d.cycle, resynch_mip);
    resynch_mip &= ~resynch_mip_mask;
    if (FLAGS_bridge_log)
      bridge_log_(cvm::MEDIUM, "<{}> Poking mip de assertion due to resynch in previous step {} \n", d.cycle, resynch_mip.to_ullong());
    poke_mip(hart, d.cycle, resynch_mip);
  }

  if(timing_case2){
    defer_interrupt(hart, w.time, 0);
    timing_case2 = 0;
  }
}

void bridge::post_step_nmi_check(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {
  if (!d.nmi && !w_.nmi)
    return;

  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> NMI detected. dut:[{}, {}] whisper:[{}, {}]\n", w.time, d.nmi, d.ncause, w_.nmi, w_.ncause);

  if (d.nmi && !w_.nmi && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    error("Hart {}: DUT took NMI, Whisper did not. Cause: {}\n", hart,
      nmi_to_string.count(static_cast<nmi>(d.ncause)) ? nmi_to_string.at(static_cast<nmi>(d.ncause)) : std::to_string(d.ncause));
    return;
  }

  if (!d.nmi && w_.nmi && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    error("Hart {}: Whisper took NMI, DUT did not. Cause: {}\n", hart,
      nmi_to_string.count(static_cast<nmi>(w_.ncause)) ? nmi_to_string.at(static_cast<nmi>(w_.ncause)) : std::to_string(w_.ncause));
    return;
  }

  if (d.nmi && w_.nmi && (d.ncause != w_.ncause) && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    error("Hart {}: DUT vs Whisper NMI cause mismatch. Dut: {}, Whisper: {}\n", hart,
      nmi_to_string.count(static_cast<nmi>(d.ncause)) ? nmi_to_string.at(static_cast<nmi>(d.ncause)) : std::to_string(d.ncause),
      nmi_to_string.count(static_cast<nmi>(w_.ncause)) ? nmi_to_string.at(static_cast<nmi>(w_.ncause)) : std::to_string(w_.ncause));
    return;
  }

  // Clear nmi on first step after taking timing sensitive nmi
  if (d.nmi && !nmi_.valid && prev_nmi_.valid) {
    clear_nmi(hart, d.cycle);
  }
}

void bridge::post_step_exception_check(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {

  if (patch_mode_ != NO_PATCH)
    return;

  if(debug_mode_ && FLAGS_emulate_debug_mode && (d.excp )){
    excp_in_debug_mode = true;
    return;
  }else{
    excp_in_debug_mode = false;
  }

  if (!d.excp && !w_.excp) {
    IF_DEBUG("d.excp==0 and w.excp==0");
    return;
  }

  if (d.excp && found_in_list(std::to_string(d.ecause), FLAGS_cosim_error_excp)) {
    error("Hart {}: Unexpected exception: +cosim_error_excp {} ({})\n", hart, d.ecause,
      excp_to_string.count(static_cast<excp>(d.ecause)) ? excp_to_string.at(static_cast<excp>(d.ecause)) : std::to_string(d.ecause));
    return;
  }

  if (d.excp && is_custom_excp(d.ecause)) {
    IF_DEBUG("Exception found");
    if (FLAGS_bridge_log)
      bridge_log_(cvm::MEDIUM, "<{}> Custom exception detected: {}  {:#x}\n", d.cycle, d.ecause, d.pc.pc_rdata);
    // Vector conservative mode
    if (d.ecause == 55) {
      IF_DEBUG("resynch because excp 55");
      resynch(hart, d);
    } else if (d.ecause == 33) { // custom debug mode enter exception
      IF_DEBUG("Exception caused debug mode entry");
      rv_debug_t debug;
      debug.cycle = d.cycle;
      debug.enter = true;
      debug.exit  = false;
      debug.hart  = d.hart;
      enter_debug_mode(debug);
      if (FLAGS_emulate_debug_mode)
        excp_in_debug_mode = true;
    }
    return;
  }

  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> Exception detected. dut:[{}, {}] whisper:[{}, {}]\n", w.time, d.excp, d.ecause, w_.excp, w_.ecause);

  if (d.excp && !w_.excp && !FLAGS_cosim_resynch) {
    IF_DEBUG("d.excp==1 and w.excp==0 ... return");
    print_instr_stdout(hart, w);
    error("Hart {}: DUT took exception, Whisper did not. Cause: {}\n", hart,
      excp_to_string.count(static_cast<excp>(d.ecause)) ? excp_to_string.at(static_cast<excp>(d.ecause)) : std::to_string(d.ecause));
    return;
  }

  if (w_.excp && !d.excp && !FLAGS_cosim_resynch) {
    IF_DEBUG("d.excp==0 and w.excp==1 ... return");
    print_instr_stdout(hart, w);
    error("Hart {}: Whisper took exception, DUT did not. Cause: {}\n", hart,
      excp_to_string.count(static_cast<excp>(w_.ecause)) ? excp_to_string.at(static_cast<excp>(w_.ecause)) : std::to_string(w_.ecause));
    return;
  }

  if (d.excp && w_.excp && (d.ecause != w_.ecause) && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    error("Hart {}: DUT vs Whisper exception cause mismatch. Dut: {}, Whisper: {}\n", hart,
      excp_to_string.count(static_cast<excp>(d.ecause)) ? excp_to_string.at(static_cast<excp>(d.ecause)) : std::to_string(d.ecause),
      excp_to_string.count(static_cast<excp>(w_.ecause)) ? excp_to_string.at(static_cast<excp>(w_.ecause)) : std::to_string(w_.ecause));
    return;
  }

  num_exceptions_++;
  if (w_.ecause == 3 && w_.disasm.find("ebreak") == std::string::npos)
    num_trig_breakpoint_++;

  // If DUT indicates retire on ucode trap handler, extra step not needed
  if (FLAGS_retire_ucode_trap) {
    IF_DEBUG("FLAGS_retire_ucode_trap==1 ... return");
    return;
  }


  step(hart, w);
  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Extra step due to exception\n", w.time, step_);
  update_whisper_state(hart,w, d.comp);
}

bool bridge::is_custom_excp(uint64_t cause) {
  return (cause >= 25 && cause <= 55);
}

void bridge::post_step_satp_write_poke(hart_id_t hart, const rv_instr_t& d, const whisper_state_t& w) {
  if (!FLAGS_delay_satp_update)
    return;

  // Save satp updates and apply only when sfence.vma is seen
  if (w.disasm.find("satp") != std::string::npos) {
    for (auto& c : w_.csr) {
      if (c.csr_addr == SATP) {
        new_satp_ = c.csr_wdata;

        uint16_t new_mode_asid = (new_satp_ >> 44) & 0xffff;
        uint16_t mode_asid = (satp_ >> 44) & 0xffff;
        if (new_mode_asid != mode_asid) {
          satp_ = new_satp_;
          return;
        }

        if (FLAGS_bridge_log)
          bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: SATP write, don't apply till sfence.vma\n", w.time, step_);

        bool valid = false;
        if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, d.cycle, 'c', SATP, satp_, valid) || !valid) && FLAGS_whisper_client_check) {
          error("Hart {}: Failed to poke SATP\n", hart);
          return;
        }
      }
    }
  }

  if (w.disasm.find("sfence.vma") != std::string::npos) {
    if (satp_ == new_satp_)
      return;

    satp_ = new_satp_;

    if (FLAGS_bridge_log)
      bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: sfence.vma, apply SATP write\n", w.time, step_);

    bool valid = false;
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, w.time, 'c', SATP, new_satp_, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed to poke new SATP\n", hart);
      return;
    }
  }
}

void bridge::update_whisper_state(hart_id_t hart, whisper_state_t& w, bool dut_is_compressed) {

  IF_DEBUG("function called");
  w_.valid = true;
  w_.cycle = w.time;
  w_.tag = w.tag;
  w_.priv = w.priv_mode;
  w_.opcode = w.opcode;
  w_.trap = w.trap;
  w_.comp = is_compressed(w.disasm);
  w_.ucode = is_ucode(w.disasm) || w.trap; // system opcode
  w_.mem_read.valid = w.is_load;
  w_.pc.valid = true;
  w_.pc.pc_rdata = w.pc;

  zicbom_ = false;
  if (((w.opcode & 0x7fff) == 0x200f) && (((w.opcode>>20) & 0xfff) <= 2)){ // cbo - inval, clean , flush
    zicbom_ = true;
    if (!FLAGS_mcm && (w.opcode>>20 == 0)) { // cbo.inval and no mcm RVDE-18801
      uint64_t addr;
      if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekGprRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, (w.opcode>>15) & 0x1f, addr)) {
          error("Hart {}: Failed to peek GPR {}\n", hart, (w.opcode>>15) & 0x1f);
      }
    IF_DEBUG("cbo.inval");
    cvm::registry::messenger.signal<cbo_inval_nomcm_s>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0), cbo_inval_nomcm_s(addr));
    }
  }

  if (FLAGS_pc_check)
    update_pc(hart, src_t::iss, w.pc);

  if (FLAGS_priv_check)
    update_priv(hart, src_t::iss, w.priv_mode);

  // FIXME Instruction byte checking disabled for vectors till we find a way to
  // differentiate cracked instructions
  if (FLAGS_insn_check && !(w_.comp||dut_is_compressed) && !w_.ucode && !is_vector(w.disasm) && !(w.disasm.substr(0,7)=="illegal") && !is_renamed_csr(w.disasm) && (patch_mode_ == NO_PATCH))
    update_insn(hart, src_t::iss, w.opcode);

  if (FLAGS_flags_check && (w.fp_flags != 0))
    update_flags(hart, src_t::iss, w.fp_flags);

  for (auto i = 0u; i < w.change_count; i++) {
    if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperChangeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, w.resource, w.address, w.value,
        w.valid)) {
      error("Hart {}: Failed to get whisper changes\n", hart);
      return;
    }
    if (FLAGS_bridge_log) {
      print_resource(hart, w);
    }
    // Populate w_ with bridge_if struct
    if (w.resource == 'r') {
      w_.gpr.emplace_back(true, w.address, w.value);
      update_regs(hart, w);
    }
    if (w.resource == 'f') {
      w_.fpr.emplace_back(true, w.address, w.value);
      update_regs(hart, w);
    }
    if (w.resource == 'v') {
      // w_.vr.valid = true;
      // w_.vr.vrd_addr = w.address;
      // w_.vr.vrd_wdata = w.value;
      update_regs(hart, w, i);
    }
    if (w.resource == 'c') {
      csr_t c;
      c.valid = true;
      c.csr_addr = w.address & 0xfff;
      c.csr_wdata = w.value;
      w_.csr.push_back(c);
      update_regs(hart, w);
    }
    if (w.resource == 'm') {
      w_.mem_write.valid = true;
      w_.mem_write.va = w.address;
      w_.mem_write.data = w.value;
      if ((w.address<0x64000000) && (w.address>=0x60000000) && FLAGS_enable_sp_init)
           num_sp_accesses_++;
    }
  }

  // Mem attributes
  // Disabling mem_attr checks for vectors currently
  if (FLAGS_memattr_check && !w_.trap && !is_vector(w.disasm) && (w_.mem_read.valid || w_.mem_write.valid || zicbom_) && patch_mode_ == NO_PATCH) {
    bool valid;
    uint64_t eff_mem_attr;
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, 's', WhisperSpecialResource::EffMemAttr, eff_mem_attr, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed whisper API call - whisperEffMemAttr\n", hart);
      return;
    }

    update_mem_attr(hart, src_t::iss, eff_mem_attr);
  }

  // Interrupts/Exceptions
  if (w_.trap) {
    bool cause_valid = false;
    uint64_t cause = 0;
    uint64_t ncause = 0;
    for (auto& c : w_.csr) {
      if (c.csr_addr == MNCAUSE) {
        cause_valid = true;
        ncause = c.csr_wdata;
      }
      if ((c.csr_addr == MCAUSE) || (c.csr_addr == SCAUSE) || (((w.priv_mode == VU) || (w.priv_mode == VS)) && (c.csr_addr == VSCAUSE))) {
        cause_valid = true;
        cause = c.csr_wdata;
      }
    }
    if (!cause_valid)
      return;

    if ((ncause >> 63) & 0x1) {
      w_.nmi = true;
      w_.ncause = (ncause & 0x3);
    } else if ((cause >> 63) & 0x1) {
      w_.intr = true;
      w_.icause = (cause & 0x3f);
    } else {
      w_.excp = true;
      w_.ecause = (cause & 0xff);
    }
  }

}

// Print functions
void bridge::print_instr_stdout(hart_id_t hart, const rv_instr_t& d) {
  print(cvm::MEDIUM, "<{}> Instr Group Step #{}: [Hart={}, Mode={}, Tag={}, Trap={}, PC={:#x}, Opcode={:#x}, Disasm={}]\n",
    d.cycle, step_, hart, d.priv, d.tag, d.trap, d.pc.pc_rdata, d.opcode, d.disasm);
}

void bridge::print_instr(hart_id_t hart, const whisper_state_t& w) {
  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, Trap={}, ChangeCount={}, PC={:#x}, Opcode={:#x}, Disasm={}]\n",
      w.time, step_, hart, w.priv_mode, w.tag, w.trap, w.change_count, w.pc, w.opcode, w.disasm);
}

void bridge::print_instr_stdout(hart_id_t hart, const whisper_state_t& w) {
  print(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, Trap={}, ChangeCount={}, PC={:#x}, Opcode={:#x}, Disasm={}]\n",
    w.time, step_, hart, w.priv_mode, w.tag, w.trap, w.change_count, w.pc, w.opcode, w.disasm);
}

void bridge::print_resource(hart_id_t hart, const whisper_state_t& w) {
  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, Resource={}, Addr={:#x}, Data={:#x}]\n",
      w.time, step_, hart, w.priv_mode, w.tag, (char)w.resource, w.address, w.value);
}

bool bridge::is_indirect_reg(const std::string& instr) {
  if ((instr.find("csrr") != std::string::npos) && (instr.find("ireg") != std::string::npos)) {
    return true;
  }
  return false;
}

void bridge::step(hart_id_t hart, whisper_state_t& w) {
  bool valid;
  IF_DEBUG("function called");
  if (((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperStepRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, w.time, w.tag,  w.pc, w.opcode, w.change_count, w.disasm,
      w.priv_mode, w.fp_flags, w.trap, w.stop, w.is_load, valid)) || !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed to step whisper\n", hart);
    return;
  }
  // Print instruction
  if (FLAGS_bridge_log) {
    print_instr(hart, w);
  }
}

// Push DUT register state to cac
void bridge::update_regs(hart_id_t hart, const rv_instr_t& d) {
  // GPR -- disable this checking in PSC mode
  if ((FLAGS_gpr_check) & (FLAGS_cosim_period == 0)) {
    for (const auto& gpr: d.gpr) {
      if (gpr.valid) {
        update_regs(hart, src_t::dut, resource_t::int_reg, gpr.rd_addr, {gpr.rd_wdata});
        if (patch_mode_ != NO_PATCH) {
          uint64_t data;
          get_gp_reg(gpr.rd_addr, data);
          update_regs(hart, src_t::iss, resource_t::int_reg, gpr.rd_addr, {data});
        }
      }
    }
  }
  // FPR -- disable this checking in PSC mode
  if ((FLAGS_fpr_check) & (FLAGS_cosim_period == 0)) {
    for (const auto& fpr: d.fpr) {
      if (fpr.valid) {
        update_regs(hart, src_t::dut, resource_t::fp_reg, fpr.frd_addr, {fpr.frd_wdata});
        if (patch_mode_ != NO_PATCH) {
          uint64_t data;
          get_fp_reg(fpr.frd_addr, data);
          update_regs(hart, src_t::iss, resource_t::fp_reg, fpr.frd_addr, {data});
        }
      }
    }
  }

  // VR -- disable this checking in PSC mode
  if ((FLAGS_vec_check) & (FLAGS_cosim_period == 0)) {
    for (auto & vr : d.vr) {
      if (vr.valid){
        update_regs(hart, src_t::dut, resource_t::vec_reg, vr.vrd_addr, create_dword_vec(vr.vrd_wdata));
        if (patch_mode_ != NO_PATCH) {
          std::array<std::uint8_t, 32> data;
          std::vector<uint64_t> data64;
          get_vec_reg(vr.vrd_addr, data);
          for(int j=0; j<4; j++) {
            uint64_t q = 0;
            for (int k=0; k<8; k++)
              q = q | uint64_t(data[8*j + k]) << (k*8);
            data64.push_back(q);
          }
          update_regs(hart, src_t::iss, resource_t::vec_reg, vr.vrd_addr, std::move(data64));
        }
      }
    }
  }

  // CSR
  for (auto & c : d.csr) {
    uint64_t data = modify_csr_data(hart, c.csr_addr, c.csr_wdata);
    uint64_t mask = modify_csr_mask(hart, c.csr_addr, c.csr_wdata, c.csr_wmask);
    if (FLAGS_csr_rd_check) {
      if ((hypervisor_csr_map_.find(c.csr_addr) != hypervisor_csr_map_.end()) && (!hyp_enabled())) {
      } else {
        update_csr(hart, src_t::dut, c.csr_addr, data, mask);
      }
      if (patch_mode_ != NO_PATCH) {
        bool valid;
        uint64_t w_data, w_mask, w_poke_mask, w_read_mask;
        if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, c.csr_addr, w_data, w_mask, w_poke_mask, w_read_mask, valid)|| !valid) && FLAGS_whisper_client_check) {
          error("Hart {}: Failed to peek csr : {:#x} in step()\n", hart, c.csr_addr);
        }
        update_csr(hart, src_t::iss, c.csr_addr, data, mask);
      }
      if (c.csr_addr == FFLAGS)
        update_csr(hart, src_t::dut, FCSR, data, mask);
      else if (c.csr_addr == FRM) {
        data = data << 5;
        mask = mask << 5;
        update_csr(hart, src_t::dut, FCSR, data, mask, false, false);
      } else if (c.csr_addr == FCSR) {
        uint64_t mask_fcsr = mask;
        mask = mask_fcsr & 0x1f;
        update_csr(hart, src_t::dut, FFLAGS, data, mask, false, false);
        data = data >> 5;
        mask = (mask_fcsr >> 5) & 0x7;
        update_csr(hart, src_t::dut, FRM, data, mask, false, false);
      }
      else if (c.csr_addr == MISA) {  // On misa.H update, update mideleg
        if (c.csr_wmask & 0x80) {
          if (c.csr_wdata & 0x80) {
            mask = 0x1444;
            update_csr(hart, src_t::dut, MIDELEG, 0x1444, mask, false, false);
          } else {
            mask = 0xF00400;
            update_csr(hart, src_t::dut, MEDELEG, 0, mask, false, false);
            mask = 0x1444;
            update_csr(hart, src_t::dut, MIDELEG, 0, mask, false, false);
          }
        }
      }
    }
  }
}

// Push DUT mem attr to cac
// Currently disabling mem_attr checks for vectors
void bridge::update_mem_attr(hart_id_t hart, src_t src, uint32_t data) {
  resource_id_t mem_attr = resource_id_t{
    .resource = resource_t::mem_attr,
    .offset = 0
  };
  // Supported sttributes - [type:11, cacheability:12]
  uint32_t masked_data = data & 0x1800;
  if (!cac_.SetResource(hart, src, mem_attr, std::move(cac::CreateBitVec<uint64_t>(masked_data)))) {
    error("Hart {}: CAC: Failed to SetResource {}\n", hart, mem_attr.ToString());
  }
}

std::bitset<256> create_bitset(uint64_t dword_vec_array [vlen/64]) {
    std::bitset<256> result;

    // Iterate through the array and concatenate each value to the result bitset
    for (size_t i = 0; i < vlen/64; ++i) {
        for (size_t j = 0; j < 64; ++j) {
            size_t bit_index = i * 64 + j;
            bool bit_value = (dword_vec_array[i] & (uint64_t(1) << j)) != 0;
            result[bit_index] = bit_value;
        }
    }

    return result;
}

// Push whisper register state to cac
void bridge::update_regs(hart_id_t hart, const whisper_state_t& w, uint32_t vec_slice_index) {
  // Register changes - r, f, v,
  // uint64_t dword_vec_array [vlen/64] = {0};
  uint32_t vec_slices = vlen/64;
  std::vector<csr> csrsupdatingmip = {SIP, SIREG, MIREG, VSIREG, MTOPEI, VSTOPEI, STOPEI, MENVCFG, STIMECMP, VSTIMECMP};

  switch(w.resource) {
    case 'r':
      if ((FLAGS_gpr_check) & (FLAGS_cosim_period==0))
        update_regs(hart, src_t::iss, resource_t::int_reg, w.address, {w.value});
      break;
    case 'f':
      if ((FLAGS_fpr_check) & (FLAGS_cosim_period==0))
        update_regs(hart, src_t::iss, resource_t::fp_reg, w.address, {w.value});
      break;
    case 'v':
      if ((FLAGS_vec_check) & (FLAGS_cosim_period==0)) {
        dword_vec_array [vec_slice_index % vec_slices] = w.value;
        if ((vec_slice_index % vec_slices) == (vec_slices - 1)){
          update_regs(hart, src_t::iss, resource_t::vec_reg, w.address, std::vector<uint64_t>(dword_vec_array, dword_vec_array + sizeof(dword_vec_array)/sizeof(dword_vec_array[0])));
          vr_t vr;
          vr.valid = true;
          vr.vrd_addr = w.address;
          vr.vrd_wdata = create_bitset(dword_vec_array);
          w_.vr.push_back(vr);
        }
      }
      break;
    case 'c':
      if (FLAGS_csr_rd_check){
        if (!is_indirect_reg(w.disasm) || nmi_.valid){
          // Check if PMP entry is locked
          if (w.address >= PMPADDR0 && w.address < PMPADDR16) {
            bool valid = false;
            uint64_t pmpcfg, mask, reset, read_mask;
            uint64_t i, pmp_cfg_reg, pmp_cfg_index;
            // For PMP addresses, which bits of the pmpcfgs to look for
            i = w.address - PMPADDR0;
            pmp_cfg_reg = ((i*8) / 64) * 2;
            pmp_cfg_index = (i*8) % 64;
            if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, PMACFG0 + pmp_cfg_reg, pmpcfg, mask, reset, read_mask, valid)|| !valid) && FLAGS_whisper_client_check) {
            error("Hart {}: Failed to peek CSR : PMACFG0\n", hart);
            }
            if((pmpcfg >> (pmp_cfg_index + 7)) & 0x1) {
              break;
            }
          }
          update_csr(hart, src_t::iss, w.address & 0xfff, w.value);
        }
      }
      break;
    default:
      break;
  }
}

// Utility functions
void bridge::update_pc(hart_id_t hart, src_t src, uint64_t data) {
  resource_id_t pc = resource_id_t{
    .resource = resource_t::pc_reg,
    .offset = 0
  };
  if (!cac_.SetResource(hart, src, pc, std::move(cac::CreateBitVec<uint64_t>(data)))) {
    error("Hart {}: CAC: Failed to SetResource {}\n", hart, pc.ToString());
  }
}

void bridge::update_insn(hart_id_t hart, src_t src, uint32_t data) {
  resource_id_t insn = resource_id_t{
    .resource = resource_t::insn_bytes,
    .offset = 0
  };
  if (!cac_.SetResource(hart, src, insn, std::move(cac::CreateBitVec<uint64_t>(data)))) {
    error("Hart {}: CAC: Failed to SetResource {}\n", hart, insn.ToString());
  }
}

void bridge::update_flags(hart_id_t hart, src_t src, uint32_t data) {
  resource_id_t flags = resource_id_t{
    .resource = resource_t::flags,
    .offset = 0
  };
  if (!cac_.SetResource(hart, src, flags, std::move(cac::CreateBitVec<uint64_t>(data)))) {
    error("Hart {}: CAC: Failed to SetResource {}\n", hart, flags.ToString());
  }
}

void bridge::update_priv(hart_id_t hart, src_t src, uint32_t data) {
  resource_id_t priv = resource_id_t{
    .resource = resource_t::priv_mode,
    .offset = 0
  };
  if (!cac_.SetResource(hart, src, priv, std::move(cac::CreateBitVec<uint64_t>(data)))) {
    error("Hart {}: CAC: Failed to SetResource {}\n", hart, priv.ToString());
  }
}

void bridge::update_regs(hart_id_t hart, src_t src, resource_t resource, uint64_t addr, const std::vector<uint64_t>&& dword_vec) {
  if ((src == src_t::dut) && (resource == resource_t::int_reg) && (addr == 0)) {
    return;
  }
  resource_id_t rid = resource_id_t{
    .resource = resource,
    .offset = addr
  };
  if (!cac_.SetResource(hart, src, rid, std::move(cac::CreateBitVec<uint64_t>(dword_vec)))) {
    error("Hart {}: CAC: Failed to SetResource {}\n", hart, rid.ToString());
  }
}

bool bridge::disable_pa_check_vec(hart_id_t hart) {
  bool valid = false;
  uint64_t data, mask, poke_mask, read_mask;
  uint64_t vl = 0;
  uint64_t vtype ;
  uint64_t vlmax = 0;

  if((cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, VTYPE, data, mask, poke_mask, read_mask, valid)|| !valid) && FLAGS_whisper_client_check) {

  vtype = data & mask; // getting the vtype csr
  int sew_enc = (vtype & 0x38) >> 3; // encoded sew
  int sew;

  if (sew_enc == 0) sew = 8;
  if (sew_enc == 1) sew = 16;
  if (sew_enc == 2) sew = 32;
  if (sew_enc == 3) sew = 64;

  int vlmul_enc = (vtype & 0x7);

  if (vlmul_enc == 0)
    vlmax = 256/sew ;
  if (vlmul_enc == 1)
    vlmax = 512/sew ;
  if (vlmul_enc == 2)
    vlmax = 1024/sew ;
  if (vlmul_enc == 3)
    vlmax = 2048/sew ;
  if (vlmul_enc == 5)
    vlmax = 256/(8*sew);
  if (vlmul_enc == 6)
    vlmax = 256/(4*sew);
  if (vlmul_enc == 7)
    vlmax = 256/(2*sew);

}

if((cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, VL, data, mask, poke_mask, read_mask, valid)|| !valid) && FLAGS_whisper_client_check)
  vl = data & mask;

if(vl < vlmax)
  return true;
return false;

}

void bridge::arch_state(whisper_state_t& w) {

  if (w.resource == 'c') {
    if (w.address == MSTATUS) {
      if (w.value & 0x20000) {
        mprv_ = 1;
        mpp_ = ((w.value) & 0x1800) >> 11;
        mpv_ = ((w.value) & 0x8000000000) >> 39;
      }
      else {
        mprv_ = 0;
      }
    }
    if (w.address == C_FECFG2) {
      csr_rename_en_ = !((w.value & 0x200) >> 9);
      csr_rd_opt_ = !((w.value & 0x4) >> 2);
    }
  }

  if (!prev_csr_rd_opt_ && csr_rd_opt_) {
    FLAGS_mip_resynch_threshold = FLAGS_mip_resynch_threshold * 4;
  }
  prev_csr_rd_opt_ = csr_rd_opt_;
}


bool bridge::is_vector(const std::string& instr) {
  if (instr.substr(0,1) == "v")
    return true;
  return false;
}

bool bridge::is_compressed(const std::string& instr) {
  if (instr.substr(0,2) == "c.")
    return true;
  return false;
}

bool bridge::is_ucode(const std::string& instr) {
  if ((instr.find("mret") != std::string::npos) ||
      (instr.find("mnret") != std::string::npos) ||
      (instr.find("sret") != std::string::npos) ||
      (instr.find("dret") != std::string::npos) ||
      (instr.find("ecall") != std::string::npos) ||
      (instr.find("ebreak") != std::string::npos) ||
      (FLAGS_fcvt_cracked && ((instr.find("fcvt.d.l") != std::string::npos) ||
      (instr.find("fcvt.d.w") != std::string::npos))) ||
      (FLAGS_scalar_fp64_er && 
      ((instr.find("fnmadd.d") != std::string::npos) ||
      (instr.find("fmadd.d") != std::string::npos) ||
      (instr.find("fmsub.d") != std::string::npos) ||
      (instr.find("fnmsub.d") != std::string::npos) ||
      (instr.find("fadd.d") != std::string::npos) ||
      (instr.find("fsub.d") != std::string::npos) ||
      (instr.find("fmul.d") != std::string::npos) ||
      (instr.find("fdiv.d") != std::string::npos) ||
      (instr.find("fsqrt.d") != std::string::npos) ||
      (instr.find("fsgnj.d") != std::string::npos) ||
      (instr.find("fsgnjn.d") != std::string::npos) ||
      (instr.find("fsgnjx.d") != std::string::npos) ||
      (instr.find("fmin.d") != std::string::npos) ||
      (instr.find("fmax.d") != std::string::npos) ||
      (instr.find("fcvt.d.s") != std::string::npos) ||
      (instr.find("fli.d") != std::string::npos) ||
      (instr.find("fminm.d") != std::string::npos) ||
      (instr.find("fmaxm.d") != std::string::npos) ||
      (instr.find("fround.d") != std::string::npos) ||
      (instr.find("froundnx.d") != std::string::npos)
      )))
    return true;
  return false;
}

bool bridge::is_renamed_csr(const std::string& instr) {
  if (csr_rename_en_ &&
      ((instr.find("mscratch") != std::string::npos) ||
       (instr.find("sscratch") != std::string::npos) ||
       (instr.find("vsscratch") != std::string::npos)))
    return true;
  return false;
}

bool bridge::resynch_needed(const hart_id_t&, const rv_instr_t& d, const std::string& instr, const whisper_state_t&) {

  if (d.mem_read.valid && resynch_on_pa(d.mem_read.pa, d.cycle)) {
    IF_DEBUG("resynch on pa match");
    return true;
  }

  if (resynch_on_instr(instr, d.cycle) && (instr != "illegal")) {
    IF_DEBUG1("resynch on instr match ",instr);
    return true;
  }

  if (d.intr && d.icause==0) {
    IF_DEBUG("intr condition");
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[Debug Mode Interrupt]\n", d.cycle);
    return true;
  }

  if (d.pc.pc_rdata == FLAGS_debug_exit_pc) {
    IF_DEBUG("debug exit condition");
    if (FLAGS_bridge_log)
      bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[debug exit]\n", d.cycle);
    return true;
  }
  return false;
}


bool bridge::resynch_on_pa(const uint64_t& pa, const uint64_t& cycle) {

  if (clint_read(pa)) {
    IF_DEBUG("clint_read condition");
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[clint_read]\n", cycle);
    return true;
  }
  if (htif_read(pa)) {
    IF_DEBUG("htif_read condition");
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[htif_read]\n", cycle);
    return true;
  }
  if (debug_mem_access(pa)) {
    IF_DEBUG("debug_mem_access condition");
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[debug mem access]\n", cycle);
    return true;
  }
  if (boot_read(pa)) {
    IF_DEBUG("boot_read condition");
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[boot_read]\n", cycle);
    return true;
  }
  if (tbox_read(pa)) {
    IF_DEBUG("tbox_read condition");
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[tbox_read]\n", cycle);
    return true;
  }
  if(cbo_inv_access(pa)) {
    print(cvm::FULL, "CBO Inval address detected in bridge -> RESYNCH\n");
    return true;
  }
  if (uart_access(pa)) {
    IF_DEBUG("uart_access condition");
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[uart_access]\n", cycle);
    return true;
  }
  if (sc_slice_status(pa)) {
    IF_DEBUG("sc slice status condition");
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[sc slice status]\n", cycle);
    return true;
  }
  if (unsupported_mmr_access(pa)) {
    IF_DEBUG("unsupport_mmr_access condition");
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[mmr_access]\n", cycle);
    return true;
  }
  return false;
}

bool bridge::resynch_on_instr(const std::string& instr, const uint64_t& cycle) {

  if (hpm_counter_read(instr)) {
    IF_DEBUG("hpm_counter_read condition");
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[hpm_counter_read]\n", cycle);
    return true;
  }
  if (FLAGS_mip_resynch && mip_mismatch(instr)) {
    IF_DEBUG("mip condition");
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[mip_mismatch] [age={}]\n", cycle, hw_mip_age_);
    return true;
  }
  if (FLAGS_topi_resynch && topi_mismatch(instr)) {
    IF_DEBUG("topi condition");
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[topi_mismatch]\n", cycle);
    return true;
  }
  if (FLAGS_topei_resynch && topei_mismatch(instr)) {
    IF_DEBUG("topei condition");
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[topei_mismatch]\n", cycle);
    return true;
  }
  if (unsupported_csr_access(instr)) {
    IF_DEBUG("csr condition");
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[unsupported_csr_access]\n", cycle);
    return true;
  }
  if (FLAGS_cosim_resynch_instr != "") {
    IF_DEBUG("checking cosim resynch instr condition");
    std::stringstream ss(FLAGS_cosim_resynch_instr);
    while(ss.good()) {
      std::string s;
      std::getline(ss, s, ',' );
      if ((instr.find(s) != std::string::npos) && (s != "")) {
        IF_DEBUG1("found resynch instr condition",s);
        bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[+cosim_resynch_instr={} for instr={}]\n", cycle, FLAGS_cosim_resynch_instr, instr);
        return true;
      }
    }
  }
  return false;
}

bool bridge::clint_read(const uint64_t& pa) {
  for (const auto& s : {"clint", "aclint"}) {
    auto it = memmap_.find(s);
    if (it != memmap_.end() && pa >= it->second.base && pa < it->second.end)
      return true;
  }
  return false;
}

bool bridge::tbox_read(const uint64_t& pa) {
  auto it = memmap_.find("trickbox");
  if (it != memmap_.end() && pa >= it->second.base && pa < it->second.end)
    return true;
  return false;
}

bool bridge::boot_read(const uint64_t& pa) {
  if (pa >= memmap_.at("boot").base && pa < memmap_.at("boot").end)
    return true;
  return false;
}

bool bridge::mip_mismatch(const std::string& instr) {
  if ((instr.find("mip") != std::string::npos) &&
      (hw_mip_age_ < FLAGS_mip_resynch_threshold))
    return true;
  return false;
}

bool bridge::topi_mismatch(const std::string& instr) {
  if ((instr.find("topi") != std::string::npos) &&
      (hw_mip_age_ < FLAGS_mip_resynch_threshold))
    return true;
  return false;
}

bool bridge::topei_mismatch(const std::string& instr) {
  if ((instr.find("topei") != std::string::npos) &&
      (e_mip_age_ < FLAGS_mip_resynch_threshold))
    return true;
  return false;
}

bool bridge::debug_mem_access(const uint64_t& pa){
  if (debug_mode_ && pa >= FLAGS_debug_mem_base && pa < (FLAGS_debug_mem_base + FLAGS_debug_mem_size))
    return true;
  return false;
}

bool bridge::unsupported_mmr_access(const uint64_t& pa){
  if ( pa >= mmr_lo_addr && pa < mmr_hi_addr)
    return true;
  return false;
}

bool bridge::htif_read(const uint64_t& pa) {
  if ( pa >= (memmap_.at("htif").base) && pa < (memmap_.at("htif").end))
    return true;
  return false;
}

bool bridge::hpm_counter_read(const std::string& instr) {
  if ((instr.find("hpmcounter") != std::string::npos) ||
      (instr.find("instret") != std::string::npos) ||
      (instr.find("time") != std::string::npos) ||
      (instr.find("stimecmp") != std::string::npos) ||
      (instr.find("vstimecmp") != std::string::npos) ||
      (instr.find("hpmevent") != std::string::npos) || //FIXME: poke events to whisper
      (instr.find("scountovf") != std::string::npos) ||//FIXME: poke events to whisper
      (instr.find("cycle") != std::string::npos))
    return true;
  return false;
}

bool bridge::unsupported_csr_access(const std::string& instr) {
  if ((instr.find("fe_dbg_mux_sel") != std::string::npos) ||
      ((instr.find("c_") != std::string::npos) && !(is_csr_allowlist(instr)))) {
    IF_DEBUG("CSR instruction") ;
    return true;
  }
  return false;
}

bool bridge::cbo_inv_access(const uint64_t& pa) {
  if((pa >> 6) == (curr_cbo_inv_addr_ >> 6))
    return true;
  return false;
}

bool bridge::uart_access(const uint64_t& pa) {
  if (memmap_.find("uart0") != memmap_.end() && pa >= (memmap_.at("uart0").base) && pa < (memmap_.at("uart0").end))
    return true;
  return false;
}

bool bridge::sc_slice_status(const uint64_t& pa) {
   if ((pa & 0xffffffffffff0fff) == sc_slice_base_)
    return true;
  return false;
}

bool bridge::found_in_list(const std::string& num, const std::string& list) {
  if (list == "")
    return false;

  std::stringstream ss(list);

  while(ss.good()) {
    std::string s;
    std::getline(ss, s, ',' );

    if (num == s)
      return true;
  }
  return false;
}

// Poke resources in whisper
void bridge::resynch(hart_id_t hart, const rv_instr_t& d) {
  bool valid = false;

  if (d.pc.pc_rdata != w_.pc.pc_rdata) {
    if (FLAGS_bridge_log)
      bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: PC={:#x}\n", d.cycle, step_, d.pc.pc_rdata);

    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, d.cycle, 'p', 0, d.pc.pc_rdata, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed to resynch PC\n", hart);
      return;
    }
  }

  for (const auto& gpr : d.gpr) {
    if (gpr.valid) {
      if (FLAGS_bridge_log)
        bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: X{}={:#x}\n", d.cycle, step_, gpr.rd_addr, gpr.rd_wdata);

      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, d.cycle, 'r', gpr.rd_addr, gpr.rd_wdata, valid)|| !valid) && FLAGS_whisper_client_check) {
        error("Hart {}: Failed to resynch GPR\n", hart);
        return;
      }
    }
  }

  for (const auto& fpr : d.fpr) {
    if (fpr.valid) {
      if (FLAGS_bridge_log)
        bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: F{}={:#x}\n", d.cycle, step_, fpr.frd_addr, fpr.frd_wdata);

      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, d.cycle, 'f', fpr.frd_addr, fpr.frd_wdata, valid)|| !valid) && FLAGS_whisper_client_check) {
        error("Hart {}: Failed to resynch FP\n", hart);
        return;
      }
    }
  }

  if (d.mem_write.valid) {
    uint64_t pa = translate(hart, d.mem_write.va, w_.priv, memclass_t::write);
    if (FLAGS_bridge_log)
      bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: M[{:#x}]={:#x}\n", d.cycle, step_, pa, d.mem_write.data);

    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, d.cycle, 'm', pa, d.mem_write.size, d.mem_write.data, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed to resynch memory\n", hart);
      return;
    }
  }

  for (auto& csr : d.csr) {
    if (csr.valid) {
      resynch_csr_ = true;
      // Special case: Resynch for topei cases
      if (csr.csr_addr == STOPEI || csr.csr_addr == VSTOPEI || csr.csr_addr == MTOPEI) {
        topei_resynch(hart, d, csr);
        continue;
      }
      if (FLAGS_bridge_log)
        bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: C[{:#x}]={:#x}\n", d.cycle, step_, csr.csr_addr, get_csr(hart, src_t::dut, csr.csr_addr));

      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, d.cycle, 'c', csr.csr_addr, get_csr(hart, src_t::dut, csr.csr_addr), valid)|| !valid) && FLAGS_whisper_client_check) {
        error("Hart {}: Failed to resynch CSRs\n", hart);
        return;
      }
    }
  }
}

void bridge::topei_resynch(hart_id_t hart, const rv_instr_t& d, const csr_t& csr) {
  bridge_log_(cvm::MEDIUM, "<{}> topei resynch\n", d.cycle);

  // First, replay the csr operation to claim the same old MSI IID as DUT
  uint64_t data = 0;
  uint64_t count = 0;
  while ((!d.gpr.empty() && (data != (d.gpr[0].rd_wdata))) && (count < FLAGS_topei_claim_threshold)) {
    peek_resource(hart, 'c', csr.csr_addr, data);
    poke_resource(hart, d.cycle, 'c', csr.csr_addr, data);
    count++;
    bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: topei claim csr:{:#x} w.data={:#x} d.data={:#x}\n", d.cycle, step_, csr.csr_addr, data, d.gpr[0].rd_wdata);
  }

  // Then, inject the in-flight new MSI IIDs to get the state identical to DUT
  for (const auto &m : msi_) {
    if (FLAGS_bridge_log)
      bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: Mpoke[{:#x}]={:#x}\n", d.cycle, step_, m.pa, m.data);

    bool valid;
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, d.cycle, 'm', m.pa, m.size, m.data, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed to resynch memory\n", hart);
      return;
    }
    process_imsic_msi(hart, m);
  }
}

void bridge::resynch(hart_id_t hart, const rv_instr_group_t& d) {
  bool valid = false;
  for (auto& csr : d.csrs) {
    if (csr.valid) {
      if (FLAGS_bridge_log)
        bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: C[{:#x}]={:#x}\n", d.cycle, step_, csr.csr_addr,
          csr.csr_wdata);

      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, d.cycle, 'c', csr.csr_addr, csr.csr_wdata, valid)|| !valid) && FLAGS_whisper_client_check) {
        error("Hart {}: Failed to resynch CSRs\n", hart);
        return;
      }
    }
  }
}

// Process mem accesses - load resolves
void bridge::process_dut_mcm_read(hart_id_t hart, mem_t& m) {
  bool valid = false;
  if (FLAGS_cosim_period > 0) {
     if (mcm_orders_.find(m.tag) == mcm_orders_.end()) {
        print(cvm::HIGH, "process_dut_mcm_read: [Hart={} adding tag={} to mcm_orders\n",hart,m.tag);
        mcm_orders_.insert( std::pair<uint64_t,int>(m.tag,1) );
     }
  }
  if (debug_mode_) {
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, m.cycle, 'm', m.pa, m.size, m.data, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed to poke memory\n", hart);
      return;
    }
  }
  if (m.v_ext){
    std::vector<uint64_t> data_vec = create_dword_vec(m.data_vec);
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmVecReadRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, m.cycle, m.tag, m.pa, m.size, data_vec, m.elem_idx, m.field, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed mcm vec load\n", hart);
      return;
    }
  } else {
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmReadRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, m.cycle, m.tag, m.pa, m.size, m.data, m.elem_idx, m.field, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed mcm load\n", hart);
      return;
    }
  }

  if (FLAGS_bridge_log)
    bridge_log_(cvm::HIGH, "<{}> mcm_read [valid={}, tag={}, addr={:#x}, size={}, data={:#x}]\n",
      m.cycle, valid, m.tag, m.pa, m.size, m.data);
}

// Process mem accesses - store inserts
void bridge::process_dut_mcm_insert(hart_id_t hart, mem_t& m) {
  bool valid = false;
  if (FLAGS_cosim_period > 0) {
     if (mcm_orders_.find(m.tag) == mcm_orders_.end()) {
        print(cvm::HIGH, "process_dut_mcm_insert: [Hart={} adding tag={} to mcm_orders\n",hart,m.tag);
        mcm_orders_.insert( std::pair<uint64_t,int>(m.tag,1) );
     }
  }
  if (m.v_ext){
    std::vector<uint64_t> data_vec = create_dword_vec(m.data_vec);
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmVecInsertRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, m.cycle, m.tag, m.pa, m.size, data_vec, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed mcm load insert\n", hart);
      return;
    }
  } else {
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmInsertRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, m.cycle, m.tag, m.pa, m.size, m.data, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed mcm load insert\n", hart);
      return;
    }
  }

  if (FLAGS_bridge_log)
    bridge_log_(cvm::HIGH, "<{}> mcm_insert [valid={}, tag={}, addr={:#x}, size={}, data={:#x}]\n",
      m.cycle, valid, m.tag, m.pa, m.size, m.data);
}

// Process mem accesses - store bypass_writes
void bridge::process_dut_mcm_bypass(hart_id_t hart, mem_t& m) {
  bool valid = false;
  if (FLAGS_cosim_period > 0) {
     if (mcm_orders_.find(m.tag) == mcm_orders_.end()) {
        print(cvm::HIGH, "process_dut_mcm_bypass: [Hart={} adding tag={} to mcm_orders\n",hart,m.tag);
        mcm_orders_.insert( std::pair<uint64_t,int>(m.tag,1) );
     }
  }

  if (m.v_ext){
    std::vector<uint64_t> data_vec = create_dword_vec(m.data_vec);
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmVecBypassRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, m.cycle, m.tag, m.pa, m.size, data_vec, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed mcm store bypass\n", hart);
      return;
    }
  } else {
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmBypassRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, m.cycle, m.tag, m.pa, m.size, m.data, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed mcm store bypass\n", hart);
      return;
    }
  }

  if (FLAGS_bridge_log)
    bridge_log_(cvm::HIGH, "<{}> mcm_bypass [valid={}, tag={}, vec={}, addr={:#x}, size={}, data={:#x}]\n",
      m.cycle, valid, m.tag, m.v_ext, m.pa, m.size, m.data);
 }

// Process mem accesses - store drains
void bridge::process_dut_mcm_write(hart_id_t hart, mem_cl_t& m) {
  uint8_t data[64] = {0};
  for (unsigned i=0; i<64; i++) {
    data[i] = (uint8_t)((m.data >> (i*8)) & std::bitset<512>(0xff)).to_ulong();
  }
  bool valid = false;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmWriteRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, m.cycle, m.pa, 64, data, m.mask, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed mcm store drain\n", hart);
    return;
  }

  if (FLAGS_bridge_log) {
    std::string log_str;
    log_str += fmt::format("<{}> mcm_write [valid={}, addr={:#x}, mask={:016x}, data=",
      m.cycle, valid, m.pa, m.mask);
    for (int i=63; i>=0; i--)
      log_str += fmt::format("{:02x}", data[i]);
    log_str += fmt::format("]\n");
    bridge_log_(cvm::HIGH, fmt::to_string(log_str));
  }
}

// Process inst fetches
void bridge::process_dut_mcm_ifetch(hart_id_t hart, mem_t& m) {
  bool valid = false;

  if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmIFetchRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, m.cycle, m.pa, valid)) {
    error("Hart {}: Failed mcm ifetch\n", hart);
    return;
  }

  if (FLAGS_bridge_log)
    bridge_log_(cvm::HIGH, "<{}> mcm_ifetch [valid={}, addr={:#x}]\n", m.cycle, valid, m.pa);
}

// Process inst evicts
void bridge::process_dut_mcm_ievict(hart_id_t hart, mem_t& m) {
  bool valid = false;

  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmIEvictRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, m.cycle, m.pa, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed mcm ievict\n", hart);
    return;
  }

  if (FLAGS_bridge_log)
    bridge_log_(cvm::HIGH, "<{}> mcm_ievict [valid={}, addr={:#x}]\n", m.cycle, valid, m.pa);
}

uint64_t bridge::translate(hart_id_t hart, uint64_t va, uint8_t priv, memclass_t memclass) {
  uint64_t pa = va;

  if (priv == M)
    return pa;

  bool valid = false;
  bool r = (memclass == memclass_t::read);
  bool w = (memclass == memclass_t::write);
  bool x = (memclass == memclass_t::fetch);
  bool sup = (priv == HS);

  if (twoStage_ == true)
    sup = false;

  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperTranslateRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, va, r, w, x, twoStage_, sup, pa, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed VA translation\n", hart);
    return -1;
  }
  return pa;
}

// LS Translation check
void bridge::translation_check(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w){

  if (!FLAGS_translation_check)
    return;

  if (d.mem_va == 0)
  return;

  uint64_t pa;
  uint64_t va = d.mem_va;
  uint64_t bit57 = va & (1ull << 56);
  va &= ((1ull << 57) - 1);             // Clear all bits to the left of 57th bit
  if (bit57) {  va |= (~0ull) << 57; } // sign extend the 57th bit to [63:58]

  // Conditions for two stage translation
  // When V = 1
  if (w.priv_mode == VU)
    twoStage_ = true;

  //  All flavours of Hypervisor Loads and Stores
  if((w.opcode & 0x7f) == 0x73)
  {
    // lower 7 bit opcode , upper 4 bits of funct7 and funct3 to differentiate from HFENCE and HINVAL
    if(((w.opcode & 0xf0000000) == 0x60000000) && ((w.opcode & 0x7000) == 0x4000))
      twoStage_ = true;
  }

  // 3.) When MPRV = 1 and MPV = 1 (Table 9.5 in Hypervisor spec)
  if(mprv_ == 1 & mpv_ == 1)
    twoStage_ = true;

  if((mprv_ == 1) && w.priv_mode == 3)
    pa = translate(hart, va, mpp_, memclass_t::read);
  else
    pa = translate(hart, va, w.priv_mode, memclass_t::read);

  if (pa != d.mem_pa) {
    print(cvm::NONE, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, PC={:#x}, VA={:#x}, RTL-PA={:#x}, ISS-PA={:#x}]\n", w.time, step_-1, hart, w.priv_mode, w.tag, w.pc, d.mem_va, d.mem_pa, pa);
    if (!(is_vector(d.disasm) && disable_pa_check_vec(hart))) {
      error("Hart {}: PA MISMATCH !! :\n", hart);
      return;
    };
  } else {
    bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, PC={:#x}, VA={:#x}, PA={:#x}]\n", w.time, step_-1, hart, w.priv_mode, w.tag, w.pc, d.mem_va, pa);
  }
}

// Interrupts
void bridge::process_dut_nmi(hart_id_t hart, rv_nmi_t& n) {
  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> NMI: Hart {} valid: {}\n", n.cycle, hart, n.valid);

  if (n.valid) {
    nmi_poke_pending_ = true;
    bridge_log_(cvm::HIGH, "Valud of is_priv_debug_mode_ is {}", is_priv_debug_mode_);
    if (debug_mode_ || is_priv_debug_mode_ || debug_haltreq_asserted) {
      poke_nmi(hart, nmi_.cycle, nmi_.cause);
      nmi_poke_in_debug_mode_ = true;
    }
  } else {
    clear_nmi(hart, nmi_.cycle);
    nmi_poke_pending_ = false;
  }
  nmi_ = n;
}

std::string bridge::to_string(rv_intr_t& i) {
  std::string mip_str;
  for (const auto& [k,v] : intr_to_string) {
    if (k == DEBUG)
      continue;
    if (i.mip_set[k])
      mip_str += fmt::format("{}+,", v);
    else if (i.mip_clr[k])
      mip_str += fmt::format("{}-,", v);
    else if (i.mip[k])
      mip_str += fmt::format("{},", v);
  }
  return mip_str;
}

void bridge::process_dut_interrupt(hart_id_t hart, rv_intr_t& i) {
  // Store for tracking
  mip_ = i.mip;
  hw_mip_ = i.hw ? i.mip : hw_mip_;
  e_mip_ = (i.mip[MEI] << MEI) | ((i.mip[SEI] | i.seip) << SEI);

  // Handling needed only for hw interrupts
  if (!i.hw)
    return;

  // Handle each interrupt category
  // External
  if (i.mip_set[MEI] || i.mip_set[SEI] || i.mip_set[VSEI] || i.mip_set[SGEI] || i.seip_set) {
    if (FLAGS_bridge_log)
      bridge_log_(cvm::MEDIUM, "<{}> External interrupt set: mip={} seip={}\n", i.cycle, to_string(i), i.seip ? 1 : 0);
    // Nothing to do here since the external interrupt was visible to whisper in process_dut_imsic_msi
  }

  // Timer
  if (i.mip_set[MTI] || i.mip_set[STI] || i.mip_set[VSTI]) {
    if (FLAGS_bridge_log)
      bridge_log_(cvm::MEDIUM, "<{}> Timer interrupt set: mip={} time={:#x}\n", i.cycle, to_string(i), i.mtime);

    std::bitset<64> t_mip = i.mip_set[MTI] << MTI | i.mip_set[STI] << STI | i.mip_set[VSTI] << VSTI;
    poke_timer(hart, i.cycle, t_mip, i.mtime + 16); // FIXME: Workaround to get mtime over the line. Investigate why it's needed.
  }

  // Default behavior is to poke the time csr to handle timer interrupts
  // But +poke_mip_timer can override to poke mip bits instead
  if (FLAGS_poke_mip_timer && (i.mip_clr[MTI] || i.mip_clr[STI] || i.mip_clr[VSTI])) {
    if (FLAGS_bridge_log)
      bridge_log_(cvm::MEDIUM, "<{}> Timer interrupt cleared: mip={} time={:#x}\n", i.cycle, to_string(i), i.mtime);
    poke_mip(hart, i.cycle, mip_);
  }

  // Local
  if (i.mip_set[LCOFI]) {
    if (FLAGS_bridge_log)
      bridge_log_(cvm::MEDIUM, "<{}> Local interrupt set: mip={}\n", i.cycle, to_string(i));

    std::bitset<64> l_mip = i.mip_set[LCOFI] << LCOFI;
    poke_local_interrupt(hart, i.cycle, l_mip);
  }
}

void bridge::process_dut_timer(hart_id_t hart, rv_intr_t& i) {
  poke_timer(hart, i.cycle, i.mip, i.mtime);
}

void bridge::poke_timer(hart_id_t hart, uint64_t cycle, std::bitset<64> t_mip, uint64_t mtime) {
  // Default behavior is to poke the time csr to handle timer interrupts
  // But +poke_mip_timer can override to poke mip bits instead
  if (FLAGS_poke_mip_timer) {
    poke_mip(hart, cycle, mip_);
  } else {
    poke_resource(hart, cycle, 'c', time_csr, mtime);
  }

  check_and_defer_interrupt(hart, cycle, t_mip);
}

void bridge::poke_local_interrupt(hart_id_t hart, uint64_t cycle, std::bitset<64> l_mip) {
  poke_mip(hart, cycle, mip_);
  check_and_defer_interrupt(hart, cycle, l_mip);
}

void bridge::process_dut_imsic_msi(hart_id_t hart, mem_t& m) {
  msi_.push_back(m);
  process_imsic_msi(hart, m);
}

void bridge::process_imsic_msi(hart_id_t hart, const mem_t& m) {
  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> IMSIC write: [addr={:#x} data={:#x}]\n", m.cycle, m.pa, m.data);

  // Poke imsic write into whisper memory
  bool valid;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, m.cycle, 'm', m.pa, 4, m.data, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed to poke memory\n", hart);
    return;
  }

  // Peek mip to check if expected to be taken
  std::bitset<64> w_mip;
  bool w_seip;
  peek_mip(hart, m.cycle, w_mip);
  peek_seip(hart, m.cycle, w_seip);
  e_mip_ = (w_mip[MEI] << MEI) | ((w_mip[SEI] | w_seip) << SEI);

  if (FLAGS_bridge_log) {
    bridge_log_(cvm::MEDIUM, "<{}> IMSIC write: mip[MEI]={} mip[SEI]={} seip={}\n", m.cycle, w_mip[MEI], w_mip[SEI], w_seip);
  }

  // Record possible new update in mip_
  mip_ |= e_mip_;

  if (e_mip_ != prev_e_mip_) {
    check_and_defer_interrupt(hart, m.cycle, e_mip_);
  }
}

void bridge::check_and_defer_interrupt(hart_id_t hart, uint64_t time, std::bitset<64> mip) {
  bool w_intr;
  uint64_t w_cause;
  check_interrupt(hart, time, w_intr, w_cause);

  if (!w_intr)
    return;

  uint64_t w_defer_mip;
  bool valid;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, 's', WhisperSpecialResource::DeferredInterrupts, w_defer_mip, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed whisper API call - whisperGetDeferredInterrupts\n", hart);
    return;
  }

   defer_interrupt(hart, time, mip.to_ullong() | w_defer_mip);
}

void bridge::defer_interrupt(hart_id_t hart, uint64_t cycle, uint64_t mip) {
  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> Whisper defer_interrupt: mip={:#x}\n", cycle, mip);

  bool valid;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, cycle, 's', WhisperSpecialResource::DeferredInterrupts, mip, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed to poke DeferredInterrupts\n", hart);
    return;
  }

  deferred_intr_ = (mip != 0) ? true : false;
}

void bridge::check_interrupt(hart_id_t hart, uint64_t cycle, bool& taken, uint64_t& cause) {
  if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperCheckInterruptRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, taken, cause)) {
    error("Hart {}: Failed whisper API call - whisperCheckInterrupt\n", hart);
    return;
  }

  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> Whisper check_interrupt: taken={} cause={}\n", cycle, taken, intr_to_string.at(static_cast<intr>(cause)));
}

void bridge::poke_nmi(hart_id_t hart, uint64_t time, uint64_t cause) {
  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> Whisper poke: nmi: {:#x}\n", time, cause);

  if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperNmiRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, time, cause)) {
    error("Hart {}: Failed to poke nmi\n", hart);
    return;
  }
}

void bridge::clear_nmi(hart_id_t hart, uint64_t time) {
  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> Whisper clear nmi\n", time);

  if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperClearNmiRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, time)) {
    error("Hart {}: Failed to clear nmi\n", hart);
    return;
  }
}

void bridge::poke_mip(hart_id_t hart, uint64_t time, std::bitset<64> mip) {
  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> Whisper poke: mip={:#x}\n", time, mip.to_ullong());

  bool valid;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, time, 'c', MIP, mip.to_ullong(), valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed to poke mip csr\n", hart);
    return;
  }
}

void bridge::peek_mip(hart_id_t hart, uint64_t time, std::bitset<64>& mip) {
  bool valid;
  uint64_t w_mip;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, 'c', MIP, w_mip, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed to peek mip\n", hart);
    return;
  }

  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> Whisper peek: mip: {:#x}\n", time, w_mip);

  mip = std::bitset<64>(w_mip);
}

void bridge::peek_seip(hart_id_t hart, uint64_t time, bool& seip) {
  uint64_t w_seip;
  if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperGetSeiPinRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, w_seip)) {
    error("Hart {}: Failed to peek seip\n", hart);
    return;
  }

  if (FLAGS_bridge_log)
    bridge_log_(cvm::MEDIUM, "<{}> Whisper peek: seip: {}\n", time, w_seip);

  seip = w_seip ? true : false;
}

void bridge::process_debug_haltreq(bool haltreq) {
  debug_haltreq_asserted = haltreq;
}

// Debug Mode
void bridge::enter_debug_mode(rv_debug_t& d) {
  uint64_t debugROM[26] = {
    0x7b2000737b202473,
    0x7b30257310852823,
    0xf1402473a41ff06f,
    0x7b2024737b302573,
    0x10052423f9dff06f,
    0x7b04307304046413,
    0x000474337b042073,
    0x1804641300047433,
    0x001000737b202473,
    0x7b30257310052c23,
    0x00c5151300c55513,
    0x00000517fd5ff06f,
    0xf8041ee300247413,
    0x4004440300a40433,
    0xf140247304041a63,
    0x0014741340044403,
    0x00a4043310852023,
    0xf140247304041863,
    0x0044741340044403,
    0x00a40433f1402473,
    0x00c5151300c55513,
    0x000005177b351073,
    0x7b2410730ff0000f,
    0x000000130640006f,
    0x000000130b40006f,
    0x000000130180006f
  };
  if (FLAGS_bridge_log)
    bridge_log_(cvm::NONE, "<{}> Enter debug mode\n", d.cycle);
  if (!debug_mode_) {
    IF_DEBUG("Sending message to whisper to enable debug mode");
    if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperEnterDebugRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), d.hart)) {
      error("Hart {}: Failed to enter debug mode\n", id_);
      return;
    }
  }

  debug_mode_ = true;

  bool valid;
  for(int i=25; i>=0; i--) {
    uint64_t debugROM_loc = FLAGS_debug_entry_pc + (25-i)*8;
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), d.hart, 0, 'm', debugROM_loc, 8, debugROM[i], valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed to poke debug memory\n", d.hart);
      return;
    }
  }
}

void bridge::exit_debug_mode(rv_debug_t& d) {
  if (FLAGS_bridge_log)
    bridge_log_(cvm::NONE, "<{}> Exit debug mode\n", d.cycle);
  debug_mode_ = false;
}

uint64_t bridge::modify_csr_data(hart_id_t hart, uint64_t addr, uint64_t data) {
  uint64_t result = data;
  if (addr >= PMPADDR0 && addr < PMPADDR16) {
    bool valid;
    uint64_t pmpcfg, mask, reset, read_mask;
    uint64_t i, pmp_cfg_reg, pmp_cfg_index;
    // For PMP addresses, which bits of the pmpcfgs to look for
    i = addr - PMPADDR0;
    pmp_cfg_reg = ((i*8) / 64) * 2;
    pmp_cfg_index = (i*8) % 64;
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, PMACFG0 + pmp_cfg_reg, pmpcfg, mask, reset, read_mask, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed to peek CSR : {:#x} in modify_csr_data()\n", hart, (PMPADDR0 + pmp_cfg_reg));
    };
    if((pmpcfg >> (pmp_cfg_index + 4)) & 0x1) {
      result = data | 0x1ff;
    } else {
      result = data & 0xfffffffffffffc00;
    }
  }
  if ((addr == HSTATEEN0) || (addr == SSTATEEN0)) {
    bool valid;
    uint64_t mstateen, mask_iss, reset, read_mask;
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, MSTATEEN0, mstateen, mask_iss, reset, read_mask, valid) || !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed to peek CSR : MSTATEEN0\n", hart);
    }
    result = result & mstateen;
  }
  return result;
}

uint64_t bridge::modify_csr_mask(hart_id_t hart, uint64_t addr, uint64_t data, uint64_t mask) {
  uint64_t result = mask;
  if (addr == VL) result = mask;
  else if((hypervisor_csr_map_.find(addr) != hypervisor_csr_map_.end()) && (!hyp_enabled())) {
    // Do not peek Hypervisor CSRs when MISA.H = 0
  }
  else
    result = mask & get_csr_mask(hart, addr);
  if (addr >= PMPADDR0 && addr < PMPADDR16) {
    bool valid;
    uint64_t pmpcfg, mask_iss, reset, read_mask;
    uint64_t i, pmp_cfg_reg, pmp_cfg_index;
    // For PMP addresses, which bits of the pmpcfgs to look for
    i = addr - PMPADDR0;
    pmp_cfg_reg = ((i*8) / 64) * 2;
    pmp_cfg_index = (i*8) % 64;
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, PMPADDR0 + pmp_cfg_reg, pmpcfg, mask_iss, reset, read_mask, valid)|| !valid) && FLAGS_whisper_client_check) {
      error("Hart {}: Failed to peek CSR : {:#x} in modify_csr_mask()\n", hart, (PMACFG0 + pmp_cfg_reg));
    }
    if((pmpcfg >> (pmp_cfg_index + 4)) & 0x1) {
      result = result | 0x1ff;
    } else {
      result = result | 0x3ff;
    }
  }
  if (addr == HGATP) {
    uint64_t mode = (get_csr(id_, src_t::dut, HGATP) | (data & mask)) >> 60;
    bool valid_mode = false;
    for (uint16_t hgatp_valid_mode : hgatp_valid_modes) {
      if (mode == hgatp_valid_mode) {
          valid_mode = true;
          break;
      }
    }
    if (!valid_mode) {
      result = result & 0xfffffffffffffffULL;
    } else {
      result = result & ((mode << 60) | 0xfffffffffffffffULL);
    }
  }

  // Handle PMM legal values during write, valid PMM values are 2'b00 and 2'b10, rvde-19017
  if ((addr == MSECCFG) || (addr == MENVCFG) || (addr == HENVCFG) || (addr == SENVCFG) || (addr == HSTATUS)) {
    bool pmm_legal = false;
    if (addr == HSTATUS) {
      uint16_t pmm = (((data & result) >> pmm_hstatus_mask_lo) & ((1 << pmm_mask_size) - 1));
      for (uint16_t pmm_legal_value : pmm_legal_values) {
        if (pmm == pmm_legal_value) {
          pmm_legal = true;
          break;
        }
      }
      if (!pmm_legal){
        uint64_t bitmask = ((1ULL << pmm_mask_size) - 1) << pmm_hstatus_mask_lo; // Create a mask for bits to clear
        result = result & ~bitmask;
      }
    } else {
      uint16_t pmm = (((data & result) >> pmm_cfgs_mask_lo) & ((1 << pmm_mask_size) - 1));
      for (uint16_t pmm_legal_value : pmm_legal_values) {
        if (pmm == pmm_legal_value) {
          pmm_legal = true;
          break;
        }
      }
      if (!pmm_legal){
        uint64_t bitmask = ((1ULL << pmm_mask_size) - 1) << pmm_cfgs_mask_lo; // Create a mask for bits to clear
        result = result & ~bitmask;
      }
    }
  }

  // Handle PMM legal values during write, valid PMM values are 2'b00 and 2'b10
  if ((addr == MSECCFG) || (addr == MENVCFG) || (addr == HENVCFG) || (addr == SENVCFG) || (addr == HSTATUS)) {
    if (addr == HSTATUS) {
      uint16_t pmm = (((data & result) >> 48) & 0x3);
      if (!(pmm == 0 || pmm == 2)){
        result = result & 0xfffcffffffffffffULL;
      }
    } else {
      uint16_t pmm = (((data & result) >> 32) & 0x3);
      if (!(pmm == 0 || pmm == 2)){
        result = result & 0xfffffffcffffffffULL;
      }
    }
  }
  return result;
}

bool bridge::is_custom_csr(uint64_t addr) {
  return ((addr >= 0x5C0 && addr <= 0x5FF) ||
          (addr >= 0x6C0 && addr <= 0x6FF) ||
          (addr >= 0x7C0 && addr <= 0x7FF) ||
          (addr >= 0x800 && addr <= 0x8FF) ||
          (addr >= 0x9C0 && addr <= 0x9FF) ||
          (addr >= 0xAC0 && addr <= 0xAFF) ||
          (addr >= 0xBC0 && addr <= 0xBFF));
}

bool bridge::is_pmacfg_csr(uint64_t addr) {
  return (addr >= PMACFG0 && addr <= PMACFG15);
}

bool bridge::is_csr_allowlist(uint64_t addr) {
  return csrs[addr].allowlist_custom_csr; // perform core arch checks for these allowlisted custom CSRs
}

bool bridge::is_csr_allowlist(const std::string& csr_name) {
    for (const auto& [addr, csr] : csrs) {
        if (csr_name.find(csr.name) != std::string::npos) {
            return csr.allowlist_custom_csr;
        }
    }
    throw std::invalid_argument("Invalid CSR name: " + csr_name);
}

bool bridge::is_chicken_bit_csr(uint64_t addr) {
  return (addr >= C_FECFG && addr <= C_LSCFG15);
}

bool bridge::is_mtimecmp_mmr(uint64_t addr) {
  return (addr == (mtimecmp_mmr + (id_ * 8)));
}

bool bridge::is_mtime_mmr(uint64_t addr) {
  return (addr == mtime_mmr);
}

void bridge::update_csr(hart_id_t hart, src_t src, uint64_t addr, uint64_t data, cac::optional_const_ref<uint64_t> mask_ref, bool shadow_csr, bool check_en) {
  if (is_custom_csr(addr) &&
      !is_pmacfg_csr(addr) &&
      !is_csr_allowlist(addr) &&
      !is_chicken_bit_csr(addr))
    return;

  bool check = true;
  if (is_chicken_bit_csr(addr) && !is_csr_allowlist(addr))
    check = false; // FIXME: Reset values in json
  else
    check = check_en;

  resource_id_t csr_resource = resource_id_t{
    .resource = resource_t::csr_reg,
    .offset = addr
  };
  cac::mask_t mask = cac::CreateBitVec<uint64_t>(std::numeric_limits<uint64_t>::max());
  if (mask_ref != std::nullopt) {
    mask = cac::CreateBitVec<uint64_t>(mask_ref.value());
  }
  if (!csr_cac_.SetResource(hart, src, csr_resource, std::move(cac::CreateBitVec<uint64_t>({data})), mask, check)) {
    error("Hart {}: CAC: Failed to SetResource {}\n", hart, csr_resource.ToString());
  }

  // Also update shadow csr if applicable ex: mstatus/sstatus
  if (!shadow_csr && csrs[addr].shadow_csr) {
    shadow_csr = csrs[csrs[addr].shadow_csr].addr;
    uint64_t alias_mask;
    if (src == src_t::dut) {
      if (mask_ref)
        alias_mask = mask_ref.value() & get_csr_poke_mask(hart, shadow_csr);
      else
        alias_mask = get_csr_poke_mask(hart, shadow_csr);

    } else {
      uint64_t mask, poke_mask, read_mask;
      bool valid;
      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, shadow_csr, data, mask, poke_mask, read_mask, valid)|| !valid) && FLAGS_whisper_client_check) {
        error("Hart {}: Failed to peek csr : {:#x} in update_csr()\n", hart, shadow_csr);
      }
      alias_mask = get_csr_poke_mask(hart, shadow_csr);
    }
    update_csr(hart, src, shadow_csr, data, alias_mask, true);
  }
}

uint64_t bridge::get_csr(hart_id_t hart, src_t src, uint64_t addr) {

  // Special handling for mip
  if (addr == MIP)
    return mip_.to_ullong();

  std::vector<bool> bool_vec;
  std::vector<uint64_t> dword_vec;
  resource_id_t csr_resource = resource_id_t{
    .resource = resource_t::csr_reg,
    .offset = addr
  };
  assert(csr_cac_.GetResource(hart, src, csr_resource, bool_vec));
  dword_vec = cac::CreateSizedVec<uint64_t>(bool_vec);
  return dword_vec[0];
}

uint64_t bridge::get_csr_mask(hart_id_t hart, uint64_t addr) {
  bool valid;
  uint64_t data, mask, poke_mask, read_mask;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, addr, data, mask, poke_mask, read_mask, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed to peek csr : {:#x} in get_csr()\n", hart, addr);
  }
  if (debug_mode_ && addr == DCSR)
    return poke_mask;
  return mask & read_mask;
}

uint64_t bridge::get_csr_poke_mask(hart_id_t hart, uint64_t addr) {
  bool valid;
  uint64_t data, mask, poke_mask, read_mask;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekCsrRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, addr, data, mask, poke_mask, read_mask, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed to peek csr : {:#x} in get_csr_poke_mask()\n", hart, addr);
  }
  return poke_mask;
}

void bridge::peek_resource(hart_id_t hart, char resource, uint64_t addr, uint64_t& data) {
  bool valid;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, resource, addr, data, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed to peek resource {} [addr={:#x} data={:#x}]\n", id_, resource, addr, data);
    return;
  }
}

void bridge::poke_resource(hart_id_t hart, uint64_t cycle, char resource, uint64_t addr, uint64_t data) {
  bool valid;
  if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), hart, cycle, resource, addr, data, valid)|| !valid) && FLAGS_whisper_client_check) {
    error("Hart {}: Failed to poke resource {} [addr={:#x} data={:#x}]\n", id_, resource, addr, data);
    return;
  }
}

std::string bridge::get_csr_name(const std::string& csr_addr) {
  unsigned int addr;
  try {
    addr = std::stoul(csr_addr, nullptr, 16);
  }
  catch (...) {
    return csr_addr;
  }
  return csrs[addr].name;
}

void bridge::final_phase() {
  // report_metrics();
}

void bridge::process(const rv_tester::terminate_called&) {
  if (terminated_)
    return;
  report_metrics();
  terminated_ = true;
}

void bridge::report_metrics() {
  if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperConnectedRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0)))
    return;

  whisper_state_t w;
  if (FLAGS_mcm) {
    bool valid;
    if (!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperMcmEndRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), id_, pw_.time, valid) || !valid) {
      error("Hart {}: Failed to disable MCM\n", id_);
    }
    w = { .tag = pw_.tag+1, .time = pw_.time+1 };
  }
  else {
    w = { .tag = step_+1, .time = pw_.time+1 };
  }
  if (!FLAGS_metrics)
    return;
  print(cvm::NONE, "[COSIM] Report metrics...\n");
  const auto& prev_whisp_state = pw_;
  const auto& prev_prev_whisp_state = ppw_;
  const int instructions = cac_.GetStep(id_);
  const auto& cpu_cycles = prev_whisp_state.time;
  const double ipc = cpu_cycles ? static_cast<double>(instructions) / static_cast<double>(cpu_cycles) : 0.0;
  const auto& instr = prev_whisp_state.disasm;
  const auto& mode = prev_whisp_state.priv_mode;
  const auto& trap = prev_whisp_state.trap;
  const auto& num_dest = prev_whisp_state.change_count;
  bool rfcm = (prev_whisp_state.resource == 'r' || prev_whisp_state.resource == 'f' || prev_whisp_state.resource == 'c' || prev_whisp_state.resource == 'm');
  const std::string dest = (rfcm ? std::string(1, static_cast<char>(prev_whisp_state.resource)) : "none");
  const std::string dest_addr = (rfcm ? fmt::format("0x{:x}", prev_whisp_state.address) : "none");
  const std::string dest_data = (rfcm ? fmt::format("0x{:x}", prev_whisp_state.value) : "none");
  const auto& src_addr = pd_.mem_read.valid ? pd_.mem_read.pa : 0;
  const auto& prev_instr = prev_prev_whisp_state.disasm;
  const auto& prev_mode = prev_prev_whisp_state.priv_mode;
  const auto& prev_trap = prev_prev_whisp_state.trap;
  const auto& prev_num_dest = prev_prev_whisp_state.change_count;
  const auto test_time = duration_cast<std::chrono::milliseconds>(end_time_ - start_of_test_).count();

  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_num_instructions\": {}}}\n", id_, instructions);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_num_cycles\": {}}}\n", id_, cpu_cycles);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_rvfi_calls\": {}}}\n", id_, rvfi_calls_);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_exec_time_ms\": {}}}\n", id_, test_time);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_whisper_time_ms\": {}}}\n", id_, whisper_time_/1000);
  if (test_time != 0) {
    print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_inst_per_sec\": {}}}\n", id_, instructions*1000/test_time);
    print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_clks_per_sec\": {}}}\n", id_, cpu_cycles*1000/test_time);
  }
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_exceptions\": {}}}\n", id_, num_exceptions_);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_trigger_breakpoint\": {}}}\n", id_, num_trig_breakpoint_);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_ipc\": {:.2f}}}\n", id_, ipc);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_instr\": \"{}\"}}\n", id_, instr);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_mode\": {}}}\n", id_, mode);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_trap\": {}}}\n", id_, trap);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_num_dest\": {}}}\n", id_, num_dest);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_dest\": \"{}\"}}\n", id_, dest);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_dest_addr\": \"{}\"}}\n", id_, dest_addr);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_src_addr\": \"{:#x}\"}}\n", id_, src_addr);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_dest_data\": \"{}\"}}\n", id_, dest_data);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_prev_instr\": \"{}\"}}\n", id_, prev_instr);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_prev_mode\": {}}}\n", id_, prev_mode);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_prev_trap\": {}}}\n", id_, prev_trap);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_prev_num_dest\": {}}}\n", id_, prev_num_dest);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_max_pend_intr_age\": {}}}\n", id_, max_pend_intr_age_);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_scratchpad_accesses\": {}}}\n", id_, num_sp_accesses_);

  // Whisper csr values
  bool valid;
  for (auto& csr_ : csrs) {
    auto csr = csr_.second;
    if (!csr.metric)
      continue;
    uint64_t csr_data;
    if((hypervisor_csr_map_.find(csr.addr) != hypervisor_csr_map_.end()) && (!hyp_enabled())) {
    }
    else {
      if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPeekRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), id_, 'c', csr.addr, csr_data, valid)|| !valid) && FLAGS_whisper_client_check) {
        error("Hart {}: Failed to peek CSR values : {:#x} in report_metrics()\n", id_, csr.addr);
      }
    print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_iss_csr_{}\": \"0x{:x}\"}}\n", id_, csr.name, csr_data);
    }
  }

  // DUT csr values
  for (auto& csr_ : csrs) {
    auto csr = csr_.second;
    if (!csr.metric)
      continue;
    uint64_t csr_data = get_csr(id_, src_t::dut, csr.addr);
    print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_dut_csr_{}\": \"0x{:x}\"}}\n", id_, csr.name, csr_data);
  }

  // Interrupts taken count
  for (size_t i = 0; i < num_taken_interrupts_.size(); i++) {
    for (size_t j = 0; j < num_taken_interrupts_[i].size(); j++) {
        if (num_taken_interrupts_[i][j] != 0) {
            print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_taken_interrupt_count_{}_{}\": {}}}\n", id_, intr_to_string.at(static_cast<intr>(j)), priv_to_string.at(static_cast<priv>(i)), num_taken_interrupts_[i][j]);
        }
    }
  }
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_nmi_taken_count\": \"{}\"}}\n", id_, nmi_taken_count_);

  // Step one final time to collect metrics for next instruction
  step(id_, w);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_next_instr\": \"{}\"}}\n", id_, w.disasm);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_next_mode\": {}}}\n", id_, w.priv_mode);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_next_trap\": {}}}\n", id_, w.trap);
  print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_next_num_dest\": {}}}\n", id_, w.change_count);
  // Regression level metrics from hart 0
  if (id_ == 0) 
    print(cvm::NONE, "INFO_PASS_REGR_METRIC:{{\"name\": \"ipc\", \"value\": {:.2f}, \"type\": \"d\", \"action\": \"avg\"}}\n", ipc); // Average ipc
}
