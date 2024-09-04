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
#include "whisper_client_decl.h"
#include "whisper_decoder.h"
#include "rv_tester/rv_tester_plusargs.h"
#include "sysmod/trickbox/interrupter.h"
#include "sysmod/trickbox/imsic_driver.h"
#include "cosim/dut_if/rvfi/rvfi_plusargs.h"
#include "sysmod/sysmod_plusargs.h"
#include "cosim/utils/eot/eot_plusargs.h"

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
DEFINE_string(whisper_json_path, "", "Path to whisper json config");
DEFINE_bool(cosim_resynch, false, "Resynch whisper with dut state on every instruction");
DEFINE_string(cosim_resynch_instr, "", "List of instruction mnemonics to resynch whisper with dut state");
DEFINE_string(cosim_resynch_prev_instr, "", "List of instruction mnemonics to resynch whisper with dut state");
DEFINE_string(cosim_resynch_csr, "", "List of csr mnemonics to resynch whisper with dut state"); 
DEFINE_bool(mip_resynch, true, "Resynch whisper with dut state on mip mismatch condition");
DEFINE_bool(topi_resynch, true, "Resynch whisper with dut state on topi mismatch condition");
DEFINE_bool(topei_resynch, true, "Resynch whisper with dut state on topei mismatch condition");
DEFINE_bool(intr_defer_spcl, true, "Defer all interrupts in special cases");
DEFINE_bool(intr_timeout_resynch, true, "Ignore whisper timeout error condition");
DEFINE_bool(retire_ucode_trap, true, "DUT indicates retire on a trap after executing the ucode trap handler");
DEFINE_bool(pc_check, true, "Enable cosim checks on pc");
DEFINE_bool(priv_check, true, "Enable cosim checks on priv mode");
DEFINE_bool(insn_check, true, "Enable cosim checks on insn bytes");
DEFINE_bool(gpr_check, true, "Enable cosim checks on gprs");
DEFINE_bool(fpr_check, true, "Enable cosim checks on fprs");
DEFINE_bool(vec_check, true, "Enable cosim checks on vector regs");
DEFINE_bool(csr_rd_check, true, "Enable cosim checks on csr reads");
DEFINE_bool(csr_wr_check, true, "Enable cosim checks on csr writes");
DEFINE_bool(memattr_check, true, "Enable cosim checks on mem attributes");
DEFINE_bool(flags_check, true, "Enable cosim checks on fflags");
DEFINE_uint64(max_cycle, 1000000, "Max cycle limit to terminate the sim");
DEFINE_int32(debug_excp_mcause, 24, "MCAUSE value for debug exception");
DEFINE_bool(translation_check, false, "Do VA-PA translation check");
DEFINE_bool(emulate_debug_mode, true, "Emulate debug mode by forcing whisper to be in sync with DUT");
DEFINE_bool(delay_satp_update, false, "Delay satp update till next sfence.vma");
DEFINE_bool(cov, false, "Enable Arch coverage");
DEFINE_string(archsample_lib_path, "", "Path to libarchsample.so");
DEFINE_bool(standalone, true, "Enable whisper standalone run at beginning of sim");
DEFINE_bool(metrics, true, "Enable printing metrics in log file");
DEFINE_uint32(max_pend_intr_age, 128, "Number of instructions allowed to retire before a pending interrupt should be taken");
DEFINE_bool(whisper_log, true, "Enable whisper logging to iss_cosim.log and iss_cmd.log");
DEFINE_bool(whisper_cosim_log, false, "Enable whisper logging to iss_cosim.log");
DEFINE_bool(whisper_cmd_log, false, "Enable whisper logging to iss_cmd.log");
DEFINE_bool(whisper_stdin_null, false, "Redirect whisoer stdin to null");
DEFINE_bool(whisper_stdout_null, false, "Redirect whisoer stdout to null");
DEFINE_bool(preload, false, "Whisper preload");

DEFINE_int32(mcmi_poke_enables, 0, "MCM interface poke enables");
DEFINE_bool(psc_compare_only, true, "Peridoic COSIM will only compare current register states preload");
DEFINE_uint64(debug_cycle, 0, "enabled debug");
DEFINE_uint64(cosim_period, 0, "COSIM periodic mode enable");

//#define IF_DEBUG(str) if (debug_on_)  print(cvm::NONE, "DEBUG::line={: <5}::{: <30} ::{}\n",__LINE__,__FUNCTION__,str);
#define IF_DEBUG(str) if (0)  print(cvm::NONE, "DEBUG::line={: <5}::{: <30} ::{}\n",__LINE__,__FUNCTION__,str);

std::shared_ptr<whisperClient<uint64_t>> client_;
//std::unique_ptr<whisperClient<uint64_t>> client_;

#define log \
#   error "Don't use cvm::log, use print() instead. This will cause errors to be reported to rvfi and stop further cosim checking."

static std::vector<bridge::size_8_bytes_t> create_dword_vec(const std::bitset<256>& input) {
    // Calculate the number of 8-byte chunks needed for the 256-bit input
    size_t num_chunks = (256 + 63) / 64; // Round up division

    // Create a vector to store the chunks
    std::vector<bridge::size_8_bytes_t> dword_vec(num_chunks);

    // Convert and store each chunk of 64 bits (8 bytes)
    for (size_t i = 0; i < num_chunks; ++i) {
        bridge::size_8_bytes_t chunk = 0;
        for (size_t j = 0; j < 64; ++j) {
            size_t bit_index = i * 64 + j;
            if (bit_index < 256 && input[bit_index]) {
                chunk |= (bridge::size_8_bytes_t(1) << j);
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
        print(cvm::ERROR, "Error: COSIM periodic-state-check mode enabled with cosim_resynch=1\n");
      }
      if (FLAGS_cosim_resynch_instr != "") {
        print(cvm::ERROR, "Error: COSIM periodic-state-check mode enabled with cosim_resynch_instr being used\n");
      }
      if (FLAGS_mcm == 1) {
        print(cvm::ERROR, "Error: COSIM periodic-state-check mode enabled with mcm=1 .. not yet validated\n");
      }
    }

    std::string traceFile  = (FLAGS_whisper_log || FLAGS_whisper_cosim_log) ? "iss_cosim.log" : "";
    std::string commandLog = (FLAGS_whisper_log || FLAGS_whisper_cmd_log  ) ? "iss_cmd.log" : "";
    cosim_resynch_csr_defaults = {

      //"htval","mtval2", // RVDE-10043
      "mtinst","htinst", // RVDE-10005
      "vstart","vxsat","vxrm","vcsr", // Unimplemented
      "sstatus","mstatus","hstatus","mie","hie","vsie","sie", // RVDE-11840
      "tselect","tdata1","tdata2","tdata3","mcontext","tinfo", // Unimplemented: RVDE-7518, RVTOOLS-3124
      "fflags","fcsr", // Unimplemented
      "menvcfg","senvcfg","henvcfg", // FIXME: pointer masking change
      "pma","pmp", // FIXME: Performant NC change
      "vtype", // Permanent: Vector vtype will not be implemented
      "mip","hip","vsip","hvip","sip","mireg","sireg","vsireg","mtopei","stopei","vstopei", // Permanent: Interrupts
      "mtopi", "stopi", "vstopi", // RVTOOLS-3189
      "hpmcounter","hpmevent","scountovf","mcycle","minstret","minstreth", // Permanent: PMC events
      "dcsr","dpc","dscratch0", "dscratch1" // Permanent: Debug events

    };
    std::istringstream iss(FLAGS_cosim_resynch_csr);
    std::string token;
    while (std::getline(iss, token, ',')) {
        cosim_resynch_csr_defaults.push_back(token);
    }
    previous_cycle_ = 0;
    client_ = std::make_shared<whisperClient<uint64_t>>(traceFile, commandLog);
    auto platform = cvm::topology::get_from_type("PLATFORM", 0);
    cvm::registry::messenger.connect<rv_tester::terminate_called>(platform, [this] (const auto& v) { return this->process(v); });
    if(FLAGS_random_intr | FLAGS_random_imsic_intr){
       FLAGS_max_cycle = 2*FLAGS_max_cycle;
       print(cvm::LOW, "Doubling max_cycles for sim run to {}\n",FLAGS_max_cycle );
    }
    int32_t nharts = cvm::topology::attr(platform, "NHARTS").second;
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
bridge::~bridge() {
  report_metrics();
  client_->whisperQuit();
}

void bridge::reset() {

  memmap::get(memmap_);

  cac_.Reset();
  assert(cac_.SetVlen(vlen_));

  if (client_->whisperConnect(num_harts_) != 0) {
    print(cvm::ERROR, "Error: Hart {}: Failed whisper_connect\n", id_);
    return;
  }

  // Init csr reset values in cac
  csr_init();

  // Write num_harts to boot mem
  bool valid;
  if (!client_->whisperPoke(id_, 0, 'm', memmap_.at("boot").base + 0x9000, FLAGS_num_harts, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to poke boot memory\n", id_);
    return;
  }
  if (!client_->whisperPoke(id_, 0, 'm', memmap_.at("boot").base + 0x9018, FLAGS_hart_sync_en, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to poke boot memory\n", id_);
    return;
  }



  if(FLAGS_enable_sp_init){ //only poke num ways when sp_init is required
    uint64_t poke_data = uint64_t(FLAGS_enable_sp_init);
    if (!client_->whisperPokeMem(0, 0, 'm', memmap_.at("boot").base + 0x9008, 8, poke_data, valid)){
      print(cvm::ERROR, "Error: Hart {}: Failed to poke boot memory\n", id_);
      return;
    }
    poke_data = uint64_t(FLAGS_num_sp_ways);
    if (!client_->whisperPokeMem(0, 0, 'm', memmap_.at("boot").base + 0x9010, 8, poke_data, valid)){
       print(cvm::ERROR, "Error: Hart {}: Failed to poke boot memory\n", id_);
       return;
    }
  }

  cvm::registry::messenger.signal<uint64_t>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0), uint64_t(0)); // sysmod needs whisper client
  cvm::registry::messenger.signal<uint64_t>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0), uint64_t(1));
  resetsstc_poke(id_,0,0x14d);
  resetsstc_poke(id_,0,0x24d);
}

void bridge::get_gp_reg(uint32_t reg, uint64_t& data)
{
    if (!client_->whisperPeekGpr(id_, reg, data)) {
        print(cvm::ERROR, "Error: Hart {}: Failed to peek GP {}\n", id_,reg);
    }
}
void bridge::get_fp_reg(uint32_t reg, uint64_t& data)
{
    if (!client_->whisperPeekFpr(id_, reg, data)) {
        print(cvm::ERROR, "Error: Hart {}: Failed to peek FP {}\n", id_,reg);
    }
}

void bridge::get_vec_reg(uint32_t reg, std::array<std::uint8_t, 32>& data)
{
    if (!client_->whisperPeekVpr(id_, reg, data)) {
        print(cvm::ERROR, "Error: Hart {}: Failed to peek VEC {}\n", id_,reg);
    }
}

void bridge::csr_init() {
  bool valid;
  uint64_t data, mask, poke_mask, read_mask;
  for (const auto& csr: nonzero_reset_csrs) {
    if (!client_->whisperPeekCsr(id_, csr.address, data, mask, poke_mask, read_mask, valid)) {
      print(cvm::ERROR, "Error: Hart {}: Failed to peek csr\n", id_);
    }
    size_8_bytes_t cac_mask = 0xffffffffffffffff;
    update_csr(id_, src_t::dut, csr.address, data, cac_mask);
    update_csr(id_, src_t::iss, csr.address, data, cac_mask);
    csr_cac_.Step(id_, false);
  }

  // CSR rename
  if (!client_->whisperPeekCsr(id_, 0xBC2, data, mask, poke_mask, read_mask, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to peek csr\n", id_);
  }
  csr_rename_en_ = !((data & 0x200) >> 9);
}

void bridge::setsstc_poke(hart_id_t hart, uint64_t cycle, uint64_t csr) {
  bool valid;
  if (!client_->whisperPoke(hart, cycle, 'c', csr, 0, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to poke timecmp csr\n", id_);
    return;
  }
}
void bridge::resetsstc_poke(hart_id_t hart, uint64_t cycle, uint64_t csr) {
  bool valid;
  if (!client_->whisperPoke(hart, cycle, 'c', csr, 0xffffffffffffffff, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to poke timecmp csr\n", id_);
    return;
  }
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
            print(cvm::ERROR, "Error: cycle={} hart={}: GP[{}] MISMATCH: DUT={:#x} ISS={:#x}\n", cycle,hart,i,array[i],data);
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
             print(cvm::ERROR, "Error: cycle={} hart={}: FP[{}] MISMATCH: DUT={:#x} ISS={:#x}\n", cycle,hart,i,array[i],data);
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
        std::vector<bridge::size_8_bytes_t> data64;
        std::vector<bridge::size_8_bytes_t> dut64;
        pd_.vr.emplace_back(true, i, array[i]);
        get_vec_reg(i, data8);
        for(int j=0;j<4;j++) {
            bridge::size_8_bytes_t q = 0;
            for (int k = 0; k < 8; k++) {
                q = q | bridge::size_8_bytes_t(data8[8*j + k]) << (k*8);
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
                   print(cvm::ERROR, "Error: cycle={} hart={}: VEC[{}][{}:{}] mismatch: dut={:#x} iss={:#x}\n", cycle,hart,i,k*32+31,k*32,dut64[k],dut64[k]);
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

  print(cvm::HIGH, "process_dut_instr_retire:: hart={}, d.cycle={}, d.pc={:#x}, d.tag={}, d.opcode={:#x}, d.disasm={}\n", hart,d.cycle,d.pc.pc_rdata,d.tag,d.opcode,d.disasm);
  print(cvm::HIGH, "                        :: mip_={}, prev_sync_intr_={}, deferred_intr_={}\n", mip_,prev_sync_intr_,deferred_intr_);
  for (const auto& gpr : d.gpr) {
    print(cvm::HIGH, "                        :: grd_addr={}, grd_wdata={:#x}\n", gpr.rd_addr,gpr.rd_wdata);
  }

  if ((d.cycle >= FLAGS_debug_cycle) & (FLAGS_debug_cycle > 0) & !debug_on_) {
     print(cvm::MEDIUM,"Setting debug_on_ = true\n");
     cvm::logger::set_verbosity(cvm::HIGH);
     debug_on_ = true;
  }

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

  if (!FLAGS_whisper_exec) {
     return;
  }

  w.tag  = d.tag;
  w.time = d.cycle;

  // Handle debug interrupt
  IF_DEBUG("check dut interrupt");
  if (d.intr && (d.icause == 0)){
    IF_DEBUG("dut has interrupt cause=0");
    return;
  }

  // Handle pre-step condition - Debug
  if (debug_mode_) {
    if (FLAGS_cosim_period != 0) {
      print(cvm::ERROR, "Error: COSIM periodic-state-check mode enabled with test utilizing debug_mode\n");
    }
    if (FLAGS_emulate_debug_mode) {
      pre_step_debug_poke(hart, d);
    } else {
      return;
    }
  }
  // Handle pre-step condition - Interrupts
  pre_step_interrupt_poke(hart, d, w);
  lrsc_fail_ = false;

  // Handle pre-step condition - LR/SC fail
  pre_step_lrsc_poke(hart, d);

  // Step whisper
  w_.clear();

  if (patch_mode_ <= 1) {
    auto stime = std::chrono::high_resolution_clock::now();
    step(hart, w);
    auto etime = std::chrono::high_resolution_clock::now();
    whisper_time_ = whisper_time_ + (duration_cast<std::chrono::microseconds>(etime - stime).count());
  }
  if (patch_mode_) {
    //patch_mode_++;
    cac_.ResetStatus(hart);
    patch_mode_ = 2;
    return;
  }

  // Update cac with whisper state
  if (!psc_stepping_) {
    IF_DEBUG("updating whispter state");
    update_whisper_state(hart, w);

    // Update cac with dut state
    IF_DEBUG("updating dut state");
    update_dut_state(hart, d);
  }

  arch_state(w);

  // Handle post-step conditions
  post_step_interrupt_poke(hart, d, w);
  post_step_exception_poke(hart, d, w);
  //}
  post_step_satp_write_poke(hart, d, w);

  if (excp_in_debug_mode) {
    IF_DEBUG("excp_in_debug_mode==1 ..reset status and return");
    cac_.ResetStatus(hart);
    return;
  }
  IF_DEBUG("no excp in debug mode...keep going");

  // Increment step count
  step_++;

  // Save whisper state
  ppw_ = pw_;
  pw_ = w;
  pd_ = d;


  // Check dut vs whisper
  compare_dut_whisper_state(hart, w, d);

  // Save whisper state
  prev_mip_ = mip_;
  prev_e_mip_ = e_mip_;
  mem_poke_.clear();

  // TLB checks
  translation_check(hart, d, w);
}

void bridge::compare_dut_whisper_state(hart_id_t hart, const whisper_state_t& w, rv_instr_t& d) {

  const auto cac_status_verbosity = cvm::HIGH;
  cac_.Step(hart, cvm::logger::check_verbosity(cac_status_verbosity));

  // Error on mismatch
  if (!cac_.GetStatus(hart)) {
    IF_DEBUG("CaC compare failed...");
    cac_.ResetStatus(hart);
    if (FLAGS_cosim_resynch) {
      if (FLAGS_bridge_log) {
        print_instr(hart, w);
        bridge_log_(cvm::MEDIUM, "{}", cac_.GetStatusStr(hart));
      }
      resynch(hart, d);
    } else {
      std::string instr = cosim_util::get_nth_word(w.disasm, 1);
      std::string resource = cac_.GetResourceStr(hart);
      if (instr.substr(0,3) == "csr") {
        instr = "csr:" + cosim_util::get_nth_word(w.disasm, 3);
      }
      // Resynch whisper with dut state if needed
      // to continue without failing
      if (does_instr_match_resynch_list(d, instr) ||
          does_instr_match_resynch_condition(d, instr)) {
        IF_DEBUG("matched condition for a resynch");
        resynch(hart, d);
        cac_.ResetStatus(hart);
      } else {
        print_instr_stdout(hart, w);
        print(cvm::NONE, "{}", cac_.GetStatusStr(hart));
        print(cvm::ERROR, "Error: Hart {}: Core Arch Checker Mismatch - {} - {}\n", hart, resource,  instr);
        return;
      }
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
      std::string instr = cosim_util::get_nth_word(w.disasm, 1);
      std::string resource = cac_.GetResourceStr(hart);
      if (instr.substr(0,3) == "csr") {
        instr = "csr:" + cosim_util::get_nth_word(w.disasm, 3);
      }
      if (does_instr_match_resynch_list(d, instr) ||
         does_instr_match_resynch_condition(d, instr)) {
        IF_DEBUG("found condition for resynch");
        for (auto& csr : d.csr) {
           csr.valid = 0;
        }
        resynch(hart, d);
        cac_.ResetStatus(hart);
      }
    }
    bridge_log_(cac_status_verbosity, "{}", cac_.GetStatusStr(hart));
  }
}

void bridge::process_dut_csr_hw_update(hart_id_t hart, csr_t& c) {
  // MIP updates handled in process_dut_interrupt
  if (c.csr_addr == 0x344)
    return;

  size_8_bytes_t mask = c.csr_wmask & static_cast<size_8_bytes_t>(get_csr_poke_mask(hart, c.csr_addr));
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

  if (resynch_csr_) {
    csr_cac_.ResetStatus(hart);
    resynch_csr_ = false;
  }

  // Error on mismatch
  if (!csr_cac_.GetStatus(hart)) {
    std::string csr = get_csr_name(csr_cac_.GetResourceStr(hart).substr(2));
    csr_cac_.ResetStatus(hart);
    if (FLAGS_cosim_resynch) {
      resynch(hart, d);
    } else {
      for (const auto& token_csr : cosim_resynch_csr_defaults) {
        if (csr.find(token_csr) != std::string::npos){
          return;
        }
      }
      for (auto & i : d.instrs)
        print_instr_stdout(hart, i);
      print(cvm::NONE, "{}", csr_cac_.GetStatusStr(hart));
      print(cvm::ERROR, "Error: Hart {}: CSR Write Mismatch - {}\n", hart, csr);
      return;
    }
  }
  else {
      bridge_log_(cac_status_verbosity, "{}", csr_cac_.GetStatusStr(hart));
  }

}

void bridge::update_dut_state(hart_id_t hart, rv_instr_t& d) {
  if (FLAGS_pc_check) {
    update_pc(hart, src_t::dut, d.pc.pc_rdata);
  }
  if (FLAGS_priv_check) {
    update_priv(hart, src_t::dut, d.priv);
  }
  if (FLAGS_insn_check && !d.comp && !d.ucode && !is_vector(d.disasm) && !(d.disasm.substr(0,7)=="illegal") && !d.csr_renamed) {
    update_insn(hart, src_t::dut, d.opcode);
  }
  if (FLAGS_flags_check && (d.flags != 0)) {
    update_flags(hart, src_t::dut, d.flags);
  }
  if (!d.gpr.empty() || !d.fpr.empty() || !d.vr.empty() || !d.csr.empty()) {
    update_regs(hart, d);
  }
  if (FLAGS_memattr_check && d.mem_read.valid && (!is_vector(d.disasm)) && !lrsc_fail_) {
      update_mem_attr(hart, src_t::dut, d.mem_read.attr);
  }
  if (FLAGS_memattr_check && d.mem_write.valid && (!is_vector(d.disasm)) && !lrsc_fail_) {
      update_mem_attr(hart, src_t::dut, d.mem_write.attr);
  }
}


void bridge::pre_step_debug_poke(hart_id_t hart, const rv_instr_t& instr) {
  print(cvm::MEDIUM, "Debug pre step poking instruction in Debug mode\n", hart); 
  bool valid;
  uint32_t opcode;
  if (instr.pc.pc_rdata == FLAGS_debug_exit_pc) {
    opcode = 0x7b200073; // Dret instruction opcode
  }
  else if(instr.excp) {
    opcode = 0x00100073; //E-break opcode
  }
  else {
    opcode = instr.opcode;
  }

  if (!client_->whisperPoke(hart, 0, 'm', instr.pc.pc_rdata, opcode, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to poke memory\n", hart);
    return;
  }
  return;
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
      if (!client_->whisperCancelLr(hart, valid)) {
        print(cvm::ERROR, "Error: Hart {}: Failed to CancelLr\n", hart);
      }
    }
  }
}

void bridge::pre_step_interrupt_poke(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {
// FIXME We are deferring all interrupts, if new interrupt was made possible due to execution of a csr op previously
  if (FLAGS_intr_defer_spcl) {
    IF_DEBUG("FLAGS_intr_defer_spcl==1");
    if (d.disasm.find("csr") != std::string::npos) {
      IF_DEBUG("CSR instruction");
      bool valid;
      if (!client_->whisperPeek(hart, 's', WhisperSpecialResource::DeferredInterrupts, deferred_mip_, valid)) {
        print(cvm::ERROR, "Error: Hart {}: Failed whisper API call - whisperGetDeferredInterrupts\n", hart);
        return;
      }
      if (prev_sync_intr_) {
        IF_DEBUG("prev_sync_intr==1");
        bridge_log_(cvm::MEDIUM, "<{}> All interrupts Defer\n", d.cycle);
        all_interrupts_defer_ = true;
        pre_csr_defermip_ = deferred_mip_;
        deferred_intr_ = true;
        defer_interrupt(hart, d.cycle, mip_);
      }
      prev_sync_intr_ = 0;
      uint64_t undeferred_mip = mip_ & ~ deferred_mip_;
      uint64_t undeferred_w_cause;
      check_interrupt(hart, undeferred_mip, pre_undeferred_intr_, undeferred_w_cause);
    }
  }

  if (!mip_ && !prev_mip_) {
    IF_DEBUG("mip_==0  and prev_mip_==0 ... return");
    return;
  }

  bool w_intr;
  uint64_t w_cause;
  check_interrupt(hart, mip_, w_intr, w_cause);

  if (!d.intr && !w_intr) {
    IF_DEBUG("no dut intr and no whisper intr....return");
    return;
  }

  if (!d.intr && w_intr) {
    IF_DEBUG("no dut intr ... but whisper has intr");
    intr_age_[w_cause]++;
    bridge_log_(cvm::HIGH, "<{}> intr_age_[{}][{}]++={}\n", w.time, hart, w_cause, intr_age_[w_cause]);

    // Check that interrupt age is not beyond threshold
    if ((intr_age_[w_cause] > FLAGS_max_pend_intr_age) && !FLAGS_cosim_resynch && !FLAGS_intr_timeout_resynch) {
      print(cvm::ERROR, "Error: Hart {}: Whisper wants to take interrupt, DUT does not. cause: [{}], timeout: [{}] retires\n",
        hart, w_cause, FLAGS_max_pend_intr_age);
    }
    return;
  }

  if (FLAGS_bridge_log) {
    bridge_log_(cvm::MEDIUM, "<{}> Interrupt taken by DUT. dcause:[{}] wcause:[{}]\n", w.time, d.icause, w_cause);
  }

  // Currently for interrupts taken to VS mode, w_cause and d.icause differ by 1
  // We will calculate next privilige mode to address cause mismatch issue and also for printing interrupt stats

  bool valid;
  uint64_t hideleg, mideleg;
  if (!client_->whisperPeek(hart, 'c', 0x303, mideleg, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to peek mip\n", hart);
    return;
  }
  if (!client_->whisperPeek(hart, 'c', 0x603, hideleg, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to peek mip\n", hart);
    return;
  }
  bool hdel = hideleg & (1ull << w_cause);
  bool mdel = mideleg & (1ull << w_cause);
  if(d.priv == 3) {intrtopriv_ = 3;}                                                 // M mode
  else if (d.priv == 1 || d.priv == 0) { intrtopriv_ = mdel ? 1 : 3;}                // HS or U mode
  else if (d.priv == 9 || d.priv == 8) { intrtopriv_ = mdel ? (hdel ? 9 : 1) : 3;}   // VS or VU mode

  if (intrtopriv_ == 9 || intrtopriv_ == 8) {w_cause--;}

  bridge_log_(cvm::MEDIUM, "<{}> Interrupt to privilege {} \n", w.time, intrtopriv_);

  // Timing sensitive resynch cases
  // 1. DUT took older interrupt that deasserted before retire
  if (d.intr && !w_intr && !FLAGS_cosim_resynch) {
    IF_DEBUG("dut intr==1 and whisper intr==0");
    check_interrupt(hart, prev_mip_, w_intr, w_cause);
    if (w_intr && (w_cause == d.icause)) {
      bridge_log_(cvm::MEDIUM, "<{}> DUT took interrupt, Whisper did not. cause:[{}] (Timing sensitive mismatch: Resynch and keep going)\n", w.time, d.icause);
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
    check_interrupt(hart, prev_mip_, w_intr, w_cause);
    if (w_intr && (w_cause == d.icause)) {
      bridge_log_(cvm::MEDIUM, "<{}> DUT vs Whisper interrupt cause mismatch [{},{}] age [{},{}] (Timing sensitive mismatch: Resynch and keep going)\n",
        w.time, d.icause, w_cause, intr_age_[d.icause], intr_age_[w_cause]);
      defer_interrupt(hart, w.time, mip_ & ~((uint64_t)1 << d.icause));
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
  if (FLAGS_bridge_log) {
    bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Extra step due to interrupt\n", w.time, step_);
  }
}

void bridge::post_step_interrupt_poke(hart_id_t hart, const rv_instr_t& d, const whisper_state_t& w) {

  if (FLAGS_intr_defer_spcl) {
    IF_DEBUG("FLAG intr_defer_spcl==1");
    if (d.disasm.find("csr") != std::string::npos) {
       IF_DEBUG("CSR instruction");
       uint64_t undeferred_mip = mip_ & ~ deferred_mip_;
       uint64_t undeferred_w_cause;
       check_interrupt(hart, undeferred_mip, post_undeferred_intr_, undeferred_w_cause);
       prev_sync_intr_ = post_undeferred_intr_ && !pre_undeferred_intr_;
    }

    if (all_interrupts_defer_) {
      IF_DEBUG("all_interrupts_defer==1 .. defer and clear this flag");
      defer_interrupt(hart, d.cycle, pre_csr_defermip_);
      all_interrupts_defer_ = false;
    }

    if ((w.disasm.find("mret") != std::string::npos) || (w.disasm.find("sret") != std::string::npos)) {
      IF_DEBUG("MRET instruction.. set flag prev_sync_intr=1 ");
      if(prev_mip_ != mip_) {
        IF_DEBUG("prev_mip != mip_ .. check and defer");
        check_and_defer_interrupt(hart, d.cycle, ~prev_mip_ & mip_);
      }
      prev_sync_intr_ = true; // This will waive cases when after execution of mret there exists a csr operation which needs to be interrupted.
    }

    if (w.disasm.find("vsstimecmp") != std::string::npos)  {
      IF_DEBUG("VSSTIMECMP instruction");
      if (!vstimecmppoked_) resetsstc_poke(hart,d.cycle, 0x24d); else setsstc_poke(hart,d.cycle, 0x24d);
    } else if (w.disasm.find("stimecmp") != std::string::npos) {
      IF_DEBUG("STIMECMP instruction");
      if (w.priv_mode == 9) {if (!vstimecmppoked_) resetsstc_poke(hart,d.cycle, 0x24d); else setsstc_poke(hart,d.cycle, 0x24d);}
      else if (!stimecmppoked_)  resetsstc_poke(hart,d.cycle, 0x14d); else setsstc_poke(hart,d.cycle, 0x14d);
    }
  }


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
    print(cvm::ERROR, "Error: Hart {}: Whisper took interrupt, DUT did not. cause:[{}]\n", hart, w_.icause);
    return;
  }

  if (d.intr && !w_.intr && !FLAGS_cosim_resynch) {
    IF_DEBUG("d.intr==1 and w.intr==0  .. return IF d.cause==0");
    // If Debug mode intterupt is seen, don't flag an error, Whisper gets poked based on PC fetches
    if (d.icause == 0) 
      return;

    print_instr_stdout(hart, w);
    print(cvm::ERROR, "Error: Hart {}: DUT took interrupt, Whisper did not. cause:[{}]\n", hart, d.icause);
    return;
  }

  // DUT cause should match whisper cause
  if ((d.icause != w_.icause) && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    print(cvm::ERROR, "Error: Hart {}: DUT vs Whisper interrupt cause mismatch [dut:{},whisper:{}]\n", hart, d.icause, w_.icause);
    return;
  }
  if (resynch_icause_) {
    IF_DEBUG("resynch_icause_==1");
    uint64_t resynch_mip_mask, resynch_mip;
    resynch_mip_mask = (1 << resynch_icause_);
    resynch_icause_ = 0;
    peek_mip(hart, d.cycle, resynch_mip);
    resynch_mip &= ~resynch_mip_mask;
    bridge_log_(cvm::MEDIUM, "<{}> Poking mip de assertion due to resynch in previous step {} \n", d.cycle, resynch_mip);
    poke_mip(hart, d.cycle, resynch_mip);
  }

  num_taken_interrupts_[intrtopriv_][w_.icause]++;
}

void bridge::post_step_exception_poke(hart_id_t hart, const rv_instr_t& d, whisper_state_t& w) {


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

  if (d.excp && is_custom_excp(d.ecause)) {
    bridge_log_(cvm::MEDIUM, "<{}> Custom exception detected: {}  {:#x}\n", d.cycle, d.ecause, d.pc.pc_rdata);
    // Vector conservative mode
    if (d.ecause == 55) {
      resynch(hart, d);
    } else if (d.ecause == 33) { // custom debug mode enter exception
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
  

  bridge_log_(cvm::MEDIUM, "<{}> Exception detected. dut:[{}, {}] whisper:[{}, {}]\n", w.time, d.excp, d.ecause, w_.excp, w_.ecause);

  if (d.excp && !w_.excp && !FLAGS_cosim_resynch) {
    IF_DEBUG("d.excp==1 and w.excp==0 ... return");
    print_instr_stdout(hart, w);
    print(cvm::ERROR, "Error: Hart {}: DUT took exception, Whisper did not. Cause: {}\n", hart,
      excp_to_string.count(static_cast<excp>(d.ecause)) ? excp_to_string.at(static_cast<excp>(d.ecause)) : std::to_string(d.ecause));
    return;
  }
  
  if (w_.excp && !d.excp && !FLAGS_cosim_resynch) {
    IF_DEBUG("d.excp==0 and w.excp==1 ... return");
    print_instr_stdout(hart, w);
    print(cvm::ERROR, "Error: Hart {}: Whisper took exception, DUT did not. Cause: {}\n", hart,
      excp_to_string.count(static_cast<excp>(w_.ecause)) ? excp_to_string.at(static_cast<excp>(w_.ecause)) : std::to_string(w_.ecause));
    return;
  }

  if (d.excp && w_.excp && (d.ecause != w_.ecause) && !FLAGS_cosim_resynch) {
    print_instr_stdout(hart, w);
    print(cvm::ERROR, "Error: Hart {}: DUT vs Whisper exception cause mismatch. Dut: {}, Whisper: {}\n", hart,
      excp_to_string.count(static_cast<excp>(d.ecause)) ? excp_to_string.at(static_cast<excp>(d.ecause)) : std::to_string(d.ecause),
      excp_to_string.count(static_cast<excp>(w_.ecause)) ? excp_to_string.at(static_cast<excp>(w_.ecause)) : std::to_string(w_.ecause));
    return;
  }

  num_exceptions_++;

  // If DUT indicates retire on ucode trap handler, extra step not needed
  if (FLAGS_retire_ucode_trap) {
    IF_DEBUG("FLAGS_retire_ucode_trap==1 ... return");
    return;
  }


  step(hart, w);
  if (FLAGS_bridge_log) {
    bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Extra step due to exception\n", w.time, step_);
  }
  update_whisper_state(hart,w);
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
      if (c.csr_addr == 0x180) {
        new_satp_ = c.csr_wdata;

        uint16_t new_mode_asid = (new_satp_ >> 44) & 0xffff;
        uint16_t mode_asid = (satp_ >> 44) & 0xffff;
        if (new_mode_asid != mode_asid) {
          satp_ = new_satp_;
          return;
        }

        if (FLAGS_bridge_log) {
          bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: SATP write, don't apply till sfence.vma\n", w.time, step_);
        }
        bool valid = false;
        if (!client_->whisperPoke(hart, d.cycle, 'c', 0x180, satp_, valid)) {
          print(cvm::ERROR, "Error: Hart {}: Failed to poke SATP\n", hart);
          return;
        }
      }
    }
  }

  if (w.disasm.find("sfence.vma") != std::string::npos) {
    if (satp_ == new_satp_)
      return;

    satp_ = new_satp_;

    if (FLAGS_bridge_log) {
      bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: sfence.vma, apply SATP write\n", w.time, step_);
    }
    bool valid = false;
    if (!client_->whisperPoke(hart, w.time, 'c', 0x180, new_satp_, valid)) {
      print(cvm::ERROR, "Error: Hart {}: Failed to poke new SATP\n", hart);
      return;
    }
  }
}

void bridge::update_whisper_state(hart_id_t hart, whisper_state_t& w) {

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
  if(((w.opcode & 0x7fff) == 0x200f) && (((w.opcode>>20)==0)||((w.opcode>>20)==1)||((w.opcode>>20)==2))) // cbo - inval, clean , flush
    zicbom_ = true;

  if (FLAGS_pc_check)
    update_pc(hart, src_t::iss, w.pc);

  if (FLAGS_priv_check)
    update_priv(hart, src_t::iss, w.priv_mode);

  // FIXME Instruction byte checking disabled for vectors till we find a way to
  // differentiate cracked instructions
  if (FLAGS_insn_check && !w_.comp && !w_.ucode && !is_vector(w.disasm) && !(w.disasm.substr(0,7)=="illegal") && !is_renamed_csr(w.disasm))
    update_insn(hart, src_t::iss, w.opcode);

  if (FLAGS_flags_check && (w.fp_flags != 0))
    update_flags(hart, src_t::iss, w.fp_flags);

  for (auto i = 0u; i < w.change_count; i++) {
    if (!client_->whisperChange(hart, w.resource, w.address, w.value,
        w.valid)) {
      print(cvm::ERROR, "Error: Hart {}: Failed to get whisper changes\n", hart);
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
      if((w.address<0x64000000) && (w.address>=0x60000000) && FLAGS_enable_sp_init){
           num_sp_accesses_++;
      }

    }
    
  }

  // Mem attributes
  // Disabling mem_attr checks for vectors currently
  if (FLAGS_memattr_check && !w_.trap && !is_vector(w.disasm) && (w_.mem_read.valid || w_.mem_write.valid || zicbom_)) {
    bool valid; 
    uint64_t eff_mem_attr;
    if (!client_->whisperPeek(hart, 's', WhisperSpecialResource::EffMemAttr, eff_mem_attr, valid)) {
      print(cvm::ERROR, "Error: Hart {}: Failed whisper API call - whisperEffMemAttr\n", hart);
      return;
    }
  
    update_mem_attr(hart, src_t::iss, eff_mem_attr);
  }

  // Interrupts/Exceptions
  if (w_.trap) {
    uint64_t cause = 0;
    for (auto& c : w_.csr) {
      if ((c.csr_addr == 0x342) || (c.csr_addr == 0x142) || (((w.priv_mode == 0x8) || (w.priv_mode == 0x9)) && (c.csr_addr == 0x242)))
        cause = c.csr_wdata;
    }
    if ((cause >> 63) & 0x1) {
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
  bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, Trap={}, ChangeCount={}, PC={:#x}, Opcode={:#x}, Disasm={}]\n",
    w.time, step_, hart, w.priv_mode, w.tag, w.trap, w.change_count, w.pc, w.opcode, w.disasm);
}

void bridge::print_instr_stdout(hart_id_t hart, const whisper_state_t& w) {
  print(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, Trap={}, ChangeCount={}, PC={:#x}, Opcode={:#x}, Disasm={}]\n",
    w.time, step_, hart, w.priv_mode, w.tag, w.trap, w.change_count, w.pc, w.opcode, w.disasm);
}

void bridge::print_resource(hart_id_t hart, const whisper_state_t& w) {
  bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, Resource={}, Addr={:#x}, Data={:#x}]\n",
    w.time, step_, hart, w.priv_mode, w.tag, (char)w.resource, w.address, w.value);
}

void bridge::step(hart_id_t hart, whisper_state_t& w) {
  if (!client_->whisperStep(hart, w.time, w.tag,  w.pc, w.opcode, w.change_count, w.disasm,
      w.priv_mode, w.fp_flags, w.trap, w.stop, w.is_load)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to step whisper\n", hart);
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
      }
    }
  }
  // FPR -- disable this checking in PSC mode
  if ((FLAGS_fpr_check) & (FLAGS_cosim_period == 0)) {
    for (const auto& fpr: d.fpr) {
      if (fpr.valid) {
        update_regs(hart, src_t::dut, resource_t::fp_reg, fpr.frd_addr, {fpr.frd_wdata});
      }
    }
  }

  // VR -- disable this checking in PSC mode
  if ((FLAGS_vec_check) & (FLAGS_cosim_period == 0)) {
    for (auto & vr : d.vr) {
      if (vr.valid){
        update_regs(hart, src_t::dut, resource_t::vec_reg, vr.vrd_addr, create_dword_vec(vr.vrd_wdata));
      }
    }
  }

  // CSR
  for (auto & c : d.csr) {
    uint64_t data = modify_csr_data(hart, c.csr_addr, c.csr_wdata);
    size_8_bytes_t mask = modify_csr_mask(hart, c.csr_addr, c.csr_wdata, c.csr_wmask);
    if (FLAGS_csr_rd_check) {
      update_csr(hart, src_t::dut, c.csr_addr, data, mask);
      if (c.csr_addr == 0x001) update_csr(hart, src_t::dut, 0x003, data, mask); // On fflags update, update fcsr
      else if (c.csr_addr == 0x002) { // On frm update, update fcsr
        data = data << 5;
        mask = mask << 5;
        update_csr(hart, src_t::dut, 0x003, data, mask, false, false);
      }
      else if (c.csr_addr == 0x003){ // On fcsr update, update fflags,frm
          bridge::size_8_bytes_t mask_fcsr = mask;
        mask = mask_fcsr & 0x1f;
        update_csr(hart, src_t::dut, 0x001, data, mask, false, false);
        data = data >> 5;
        mask = (mask_fcsr >> 5) & 0x7;
        update_csr(hart, src_t::dut, 0x002, data, mask, false, false);
      }
      else if (c.csr_addr == 0x301){ // On misa.H update, update mideleg
        if ((c.csr_wmask >> 7) & 0x1) {
          if ((c.csr_wdata >> 7) & 0x1) {
            mask = 0x1444;
            update_csr(hart, src_t::dut, 0x303, 0x1444, mask, false, false);
          }
          else {
            mask = 0xF00000;
            update_csr(hart, src_t::dut, 0x302, 0, mask, false, false);
            mask = 0x1444;
            update_csr(hart, src_t::dut, 0x303, 0, mask, false, false);
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
    print(cvm::ERROR, "Error: Hart {}: CAC: Failed to SetResource {}\n", hart, mem_attr.ToString());
  }
}

std::bitset<256> create_bitset(bridge::size_8_bytes_t dword_vec_array [vlen/64]) {
    std::bitset<256> result;

    // Iterate through the array and concatenate each value to the result bitset
    for (size_t i = 0; i < vlen/64; ++i) {
        for (size_t j = 0; j < 64; ++j) {
            size_t bit_index = i * 64 + j;
            bool bit_value = (dword_vec_array[i] & (bridge::size_8_bytes_t(1) << j)) != 0;
            result[bit_index] = bit_value;
        }
    }

    return result;
}

// Push whisper register state to cac
void bridge::update_regs(hart_id_t hart, const whisper_state_t& w, uint32_t vec_slice_index) {
  // Register changes - r, f, v,
  // size_8_bytes_t dword_vec_array [vlen/64] = {0};
  uint32_t vec_slices = vlen/64;
  std::vector<uint64_t> csrsupdatingmip = {0x144, 0x351, 0x251, 0x151, 0x35c, 0x25c, 0x15c, 0x30a};

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
          update_regs(hart, src_t::iss, resource_t::vec_reg, w.address, std::vector<bridge::size_8_bytes_t>(dword_vec_array, dword_vec_array + sizeof(dword_vec_array)/sizeof(dword_vec_array[0])));
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
        // Check if PMP entry is locked
        if (w.address >= 0x3B0 && w.address < 0x3C0) {
          bool valid;
          uint64_t pmpcfg, mask, reset, read_mask;
          uint64_t i, pmp_cfg_reg, pmp_cfg_index;
          // For PMP addresses, which bits of the pmpcfgs to look for 
          i = w.address - 0x3B0;
          pmp_cfg_reg = ((i*8) / 64) * 2;
          pmp_cfg_index = (i*8) % 64;
          client_->whisperPeekCsr(hart, 0x3A0 + pmp_cfg_reg, pmpcfg, mask, reset, read_mask, valid);
          if((pmpcfg >> (pmp_cfg_index + 7)) & 0x1) {
            break;
          }
        }
        update_csr(hart, src_t::iss, w.address & 0xfff, w.value);
      }

      if (w.address == 0x344) {
        uint64_t w_seip;
        peek_seip(hart, w.time, w_seip);
        mip_ = w.value | w_seip << 9;
        e_mip_ = mip_ & 0x1e00;
        bridge_log_(cvm::MEDIUM, "<{}> Zicsr write based interrupt: mip {:#x}\n", w.time, w.value);
      }
      // Whisper is not doing recordwrite of mip if change happens to it through sip, *ireg, *topei
      for (size_t i = 0; i < csrsupdatingmip.size(); ++i) {
        if (csrsupdatingmip[i] == w.address) {
            uint64_t w_seip;
            peek_seip(hart, w.time, w_seip);
            peek_mip(hart, w.time , mip_);
            mip_ |= w_seip << 9;
            e_mip_ = mip_ & 0x1e00;
            bridge_log_(cvm::MEDIUM, "<{}> Zicsr write based interrupt: shadow update to mip {:#x}\n", w.time, mip_);
            break;
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
    print(cvm::ERROR, "Error: Hart {}: CAC: Failed to SetResource {}\n", hart, pc.ToString());
  }
}

void bridge::update_insn(hart_id_t hart, src_t src, uint32_t data) {
  resource_id_t insn = resource_id_t{
    .resource = resource_t::insn_bytes,
    .offset = 0
  };
  if (!cac_.SetResource(hart, src, insn, std::move(cac::CreateBitVec<uint64_t>(data)))) {
    print(cvm::ERROR, "Error: Hart {}: CAC: Failed to SetResource {}\n", hart, insn.ToString());
  }
}

void bridge::update_flags(hart_id_t hart, src_t src, uint32_t data) {
  resource_id_t flags = resource_id_t{
    .resource = resource_t::flags,
    .offset = 0
  };
  if (!cac_.SetResource(hart, src, flags, std::move(cac::CreateBitVec<uint64_t>(data)))) {
    print(cvm::ERROR, "Error: Hart {}: CAC: Failed to SetResource {}\n", hart, flags.ToString());
  }
}

void bridge::update_priv(hart_id_t hart, src_t src, uint32_t data) {
  resource_id_t priv = resource_id_t{
    .resource = resource_t::priv_mode,
    .offset = 0
  };
  if (!cac_.SetResource(hart, src, priv, std::move(cac::CreateBitVec<uint64_t>(data)))) {
    print(cvm::ERROR, "Error: Hart {}: CAC: Failed to SetResource {}\n", hart, priv.ToString());
  }
}

void bridge::update_regs(hart_id_t hart, src_t src, resource_t resource, uint64_t addr, const std::vector<size_8_bytes_t>&& dword_vec) {
  if ((src == src_t::dut) && (resource == resource_t::int_reg) && (addr == 0)) {
    return;
  }
  resource_id_t rid = resource_id_t{
    .resource = resource,
    .offset = addr
  };
  if (!cac_.SetResource(hart, src, rid, std::move(cac::CreateBitVec<size_8_bytes_t>(dword_vec)))) {
    print(cvm::ERROR, "Error: Hart {}: CAC: Failed to SetResource {}\n", hart, rid.ToString());
  }
}

bool bridge::disable_pa_check_vec(hart_id_t hart) {
  bool valid;
  uint64_t data, mask, poke_mask, read_mask;
  uint64_t vl = 0;
  uint64_t vtype ;
  uint64_t vlmax = 0;

  if(client_->whisperPeekCsr(hart,0xc21, data, mask, poke_mask, read_mask, valid)) {
  
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

if(client_->whisperPeekCsr(hart,0xc20, data, mask, poke_mask, read_mask, valid)) 
  vl = data & mask;

if(vl < vlmax)
  return true;  
return false;

}

void bridge::arch_state(whisper_state_t& w) {

  if (w.resource == 'c') {
      if(w.address == 0x300) {
          if(((w.value) & 0x20000) != 0) {
              mprv_ = 1;
              mpp_ = ((w.value) & 0x1800) >> 11; 
              mpv_ = ((w.value) & 0x8000000000) >> 39;
            }
            else
              mprv_ = 0;
        }
      }
      if (w.address == 0xBC2) {
        csr_rename_en_ = !((w.value & 0x200) >> 9);
      }
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
      (instr.find("sret") != std::string::npos) ||
      (instr.find("dret") != std::string::npos) ||
      (instr.find("ecall") != std::string::npos) ||
      (instr.find("ebreak") != std::string::npos))
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

bool bridge::does_instr_match_resynch_condition(const rv_instr_t& d, const std::string& instr) {
  // Case #1
  if (clint_read(d)) {
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[clint_read]\n", d.cycle);
    return true;
  }
  if (tbox_read(d)) {
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[tbox_read]\n", d.cycle);
    return true;
  }
  // Case #2
  if (htif_read(d)) {
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[htif_read]\n", d.cycle);
    return true;
  }
  // Case #3
  if (hpm_counter_read(instr)) {
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[hpm_counter_read]\n", d.cycle);
    return true;
  }
  // Case #4
  if (debug_mem_access(d)) {
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[debug mem access]\n", d.cycle);
    return true;
  }
  // Case #5
  if (boot_read(d)) {
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[boot_read]\n", d.cycle);
    return true;
  }
  // Case #6
  if (FLAGS_mip_resynch && mip_mismatch(instr)) {
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[mip_mismatch]\n", d.cycle);
    return true;
  }
  // Case #7
  if (FLAGS_topi_resynch && topi_mismatch(instr)) {
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[topi_mismatch]\n", d.cycle);
    return true;
  }
  // Case #8
  if (FLAGS_topei_resynch && topei_mismatch(instr)) {
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[topei_mismatch]\n", d.cycle);
    return true;
  }
  // Case #9
  if (unsupported_mmr_access(d)) {
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[mmr_access]\n", d.cycle);
    return true;
  }
  // Case #10
  if (d.intr && (d.icause == 0)){
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[Debug Mode Interrupt]\n", d.cycle);
   return true;
  }
  // Case #11
  if (unsupported_csr_access(instr)) {
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[unsupported_csr_access]\n", d.cycle);
    return true;
  }
  // Case #12
  if (cpl_smc_access(d)) {
    bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[cpl_smc_access]\n", d.cycle);
    return true;
  }
  return false;
}

bool bridge::clint_read(const rv_instr_t& d) {
  if (d.mem_read.valid) {
    for (const auto& s : {"clint", "aclint"}) {
      auto it = memmap_.find(s);
      if (it != memmap_.end()) {
        if (d.mem_read.pa >= it->second.base && d.mem_read.pa < it->second.end) {
          return true;
        }
      }
    }
  }
  return false;
}
bool bridge::tbox_read(const rv_instr_t& d) {
  if (d.mem_read.valid) {
    for (const auto& s : {"trickbox"}) {
      auto it = memmap_.find(s);
      if (it != memmap_.end()) {
        if (d.mem_read.pa >= it->second.base && d.mem_read.pa < it->second.end) {
          return true;
        }
      }
    }
  }
  return false;
}

bool bridge::boot_read(const rv_instr_t& d) {
  if (d.mem_read.valid &&
      d.mem_read.pa >= memmap_.at("boot").base &&
      d.mem_read.pa < memmap_.at("boot").end)
    return true;
  return false;
}

bool bridge::mip_mismatch(const std::string& instr) {
  if ((instr.find("mip") != std::string::npos) &&
      (mip_ != prev_mip_))
    return true;
  return false;
}

bool bridge::topi_mismatch(const std::string& instr) {
  if ((instr.find("topi") != std::string::npos) &&
      (mip_ != prev_mip_ || mem_poke_.size() != 0))
    return true;
  return false;
}

bool bridge::topei_mismatch(const std::string& instr) {
  if ((instr.find("topei") != std::string::npos) &&
      (e_mip_ != prev_e_mip_ || mem_poke_.size() != 0))
    return true;
  return false;
}

bool bridge::debug_mem_access(const rv_instr_t& d){
  print(cvm::NONE, "<{}> debug_mem_access: valid={} for pa={}]\n", d.cycle, d.mem_read.valid, d.mem_read.pa);
  if (d.mem_read.valid && debug_mode_ &&
      (d.mem_read.pa >= FLAGS_debug_mem_base) && (d.mem_read.pa < (FLAGS_debug_mem_base + FLAGS_debug_mem_size)))
    return true;
  return false;
}

bool bridge::unsupported_mmr_access(const rv_instr_t& d){
  if (d.mem_read.valid &&
      d.mem_read.pa >= mmr_lo_addr &&
      d.mem_read.pa < mmr_hi_addr)
    return true;
  return false;
}

bool bridge::htif_read(const rv_instr_t& d) {
  if (d.mem_read.valid &&
      d.mem_read.pa >= (memmap_.at("htif").base) &&
      d.mem_read.pa < (memmap_.at("htif").end))
    return true;
  return false;
}

bool bridge::hpm_counter_read(const std::string& instr) {
  if ((instr.find("hpmcounter") != std::string::npos) ||
      (instr.find("instret") != std::string::npos) ||
      (instr.find("time") != std::string::npos) ||
      (instr.find("stimecmp") != std::string::npos) ||
      (instr.find("hpmevent") != std::string::npos) || //FIXME: poke events to whisper
      (instr.find("scountovf") != std::string::npos) ||//FIXME: poke events to whisper
      (instr.find("cycle") != std::string::npos))
    return true;
  return false;
}

bool bridge::unsupported_csr_access(const std::string& instr) {
  if ((instr.find("fe_dbg_mux_sel") != std::string::npos) ||
      (instr.find("c_") != std::string::npos)) {
    IF_DEBUG("CSR instruction") ;
    return true;
  }
  return false;
}

bool bridge::cpl_smc_access(const rv_instr_t& d){
  if (d.mem_read.valid &&
      d.mem_read.pa >= smc_lo_addr &&
      d.mem_read.pa < smc_hi_addr)
    return true;
  return false;
}

bool bridge::does_instr_match_resynch_list(const rv_instr_t& d, const std::string& instr) {
  if (FLAGS_cosim_resynch_instr == "")
    return false;

  std::stringstream ss(FLAGS_cosim_resynch_instr);

  while(ss.good()) {
    std::string s;
    std::getline(ss, s, ',' );

    if (instr.find(s) != std::string::npos) {
      bridge_log_(cvm::MEDIUM, "<{}> Resynch: Reason=[+cosim_resynch_instr={} for instr={}]\n", d.cycle, FLAGS_cosim_resynch_instr, instr);
      return true;
    }
  }
  return false;
}

// Poke resources in whisper
void bridge::resynch(hart_id_t hart, const rv_instr_t& d) {
  bool valid = false;

  if (d.pc.pc_rdata != w_.pc.pc_rdata) {
    if (FLAGS_bridge_log) {
      bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: PC={:#x}\n", d.cycle, step_, d.pc.pc_rdata);
    }
    if (!client_->whisperPoke(hart, d.cycle, 'p', 0, d.pc.pc_rdata, valid)) {
      print(cvm::ERROR, "Error: Hart {}: Failed to resynch PC\n", hart);
      return;
    }
  }

  for (const auto& gpr : d.gpr) {
    if (gpr.valid) {
      if (FLAGS_bridge_log) {
        bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: X{}={:#x}\n", d.cycle, step_, gpr.rd_addr,
          gpr.rd_wdata);
      }
      if (!client_->whisperPoke(hart, d.cycle, 'r', gpr.rd_addr, gpr.rd_wdata, valid)) {
        print(cvm::ERROR, "Error: Hart {}: Failed to resynch GPR\n", hart);
        return;
      }
    }
  }

  for (const auto& fpr : d.fpr) {
    if (fpr.valid) {
      if (FLAGS_bridge_log) {
        bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: F{}={:#x}\n", d.cycle, step_, fpr.frd_addr,
          fpr.frd_wdata);
      }
      if (!client_->whisperPoke(hart, d.cycle, 'f', fpr.frd_addr, fpr.frd_wdata, valid)) {
        print(cvm::ERROR, "Error: Hart {}: Failed to resynch FP\n", hart);
        return;
      }
    }
  }

  if (d.mem_write.valid) {
    uint64_t pa = translate(hart, d.mem_write.va, w_.priv, memclass_t::write);
    if (FLAGS_bridge_log) {
      bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: M[{:#x}]={:#x}\n", d.cycle, step_, pa,
        d.mem_write.data);
    }
    if (!client_->whisperPokeMem(hart, d.cycle, 'm', pa, d.mem_write.size, d.mem_write.data, valid)) {
      print(cvm::ERROR, "Error: Hart {}: Failed to resynch memory\n", hart);
      return;
    }
  }

  for (auto& csr : d.csr) {
    if (csr.valid) {
      resynch_csr_ = true;
      // Resynch msi poke for topi/topei cases
      if (csr.csr_addr==0x15c || csr.csr_addr==0x25c || csr.csr_addr==0x35c ||
          csr.csr_addr==0xdb0 || csr.csr_addr==0xfb0) {
        bridge_log_(cvm::MEDIUM, "<{}> topi/topei resynch\n", d.cycle);
        for (const auto &m : mem_poke_) {
          if (FLAGS_bridge_log) {
            bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: Mpoke[{:#x}]={:#x}\n", d.cycle, step_, m.pa, m.data);
          }
          if (!client_->whisperPokeMem(hart, d.cycle, 'm', m.pa, m.size, m.data, valid)) {
            print(cvm::ERROR, "Error: Hart {}: Failed to resynch memory\n", hart);
            return;
          }
          process_imsic_msi(hart, m);
        }
        continue;
      }
      if (FLAGS_bridge_log) {
        bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: C[{:#x}]={:#x}\n", d.cycle, step_, csr.csr_addr, 
          get_csr(hart, src_t::dut, csr.csr_addr));
      }
      if (!client_->whisperPoke(hart, d.cycle, 'c', csr.csr_addr, get_csr(hart, src_t::dut, csr.csr_addr), valid)) {
        print(cvm::ERROR, "Error: Hart {}: Failed to resynch CSRs\n", hart);
        return;
      }
    }
  }

}

void bridge::resynch(hart_id_t hart, const rv_instr_group_t& d) {
  bool valid = false;
  for (auto& csr : d.csrs) {
    if (csr.valid) {
      if (FLAGS_bridge_log) {
        bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: Resynch: C[{:#x}]={:#x}\n", d.cycle, step_, csr.csr_addr,
          csr.csr_wdata);
      }
      if (!client_->whisperPoke(hart, d.cycle, 'c', csr.csr_addr, csr.csr_wdata, valid)) {
        print(cvm::ERROR, "Error: Hart {}: Failed to resynch CSRs\n", hart);
        return;
      }
    }
  }
}

// Process mem accesses - load resolves
void bridge::process_dut_mcm_read(hart_id_t hart, mem_t& m) {
  bool valid = false;
  if (debug_mode_) {
    if (!client_->whisperPokeMem(hart, m.cycle, 'm', m.pa, m.size, m.data, valid)) {
      print(cvm::ERROR, "Error: Hart {}: Failed to poke memory\n", hart);
      return;
    }
  }
  if (m.v_ext){
    std::vector<bridge::size_8_bytes_t> data_vec = create_dword_vec(m.data_vec);
    if (!client_->whisperMcmVecRead(hart, m.cycle, m.tag, m.pa, m.size, data_vec, valid)) {
      print(cvm::ERROR, "Error: Hart {}: Failed mcm vec load\n", hart);
      return;
    }
  } else {
    if (!client_->whisperMcmRead(hart, m.cycle, m.tag, m.pa, m.size, m.data, valid)) {
      print(cvm::ERROR, "Error: Hart {}: Failed mcm load\n", hart);
      return;
    }
  }
  bridge_log_(cvm::HIGH, "<{}> mcm_read [valid={}, tag={}, addr={:#x}, size={}, data={:#x}]\n",
    m.cycle, valid, m.tag, m.pa, m.size, m.data);
}

// Process mem accesses - store inserts
void bridge::process_dut_mcm_insert(hart_id_t hart, mem_t& m) {
  bool valid = false;
  if (m.v_ext){
    std::vector<bridge::size_8_bytes_t> data_vec = create_dword_vec(m.data_vec);
    if (!client_->whisperMcmVecInsert(hart, m.cycle, m.tag, m.pa, m.size, data_vec, valid)) {
      print(cvm::ERROR, "Error: Hart {}: Failed mcm load insert\n", hart);
      return;
    }
  } else {
    if (!client_->whisperMcmInsert(hart, m.cycle, m.tag, m.pa, m.size, m.data, valid)) {
      print(cvm::ERROR, "Error: Hart {}: Failed mcm load insert\n", hart);
      return;
    }
  }
  bridge_log_(cvm::HIGH, "<{}> mcm_insert [valid={}, tag={}, addr={:#x}, size={}, data={:#x}]\n",
    m.cycle, valid, m.tag, m.pa, m.size, m.data);
}

// Process mem accesses - store bypass_writes
void bridge::process_dut_mcm_bypass(hart_id_t hart, mem_t& m) {
  bool valid = false;

  if (!client_->whisperMcmBypass(hart, m.cycle, m.tag, m.pa, m.size, m.data, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed mcm store bypass\n", hart);
    return;
  }
  bridge_log_(cvm::HIGH, "<{}> mcm_bypass [valid={}, tag={}, addr={:#x}, size={}, data={:#x}]\n",
    m.cycle, valid, m.tag, m.pa, m.size, m.data);
}

// Process mem accesses - store drains
void bridge::process_dut_mcm_write(hart_id_t hart, mem_cl_t& m) {
  uint8_t data[64] = {0};
  for (unsigned i=0; i<64; i++) {
    data[i] = (uint8_t)((m.data >> (i*8)) & std::bitset<512>(0xff)).to_ulong();
  }
  bool valid = false;
  if (!client_->whisperMcmWrite(hart, m.cycle, m.pa, 64, data, m.mask, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed mcm store drain\n", hart);
    return;
  }
  std::string log_str;
  log_str += fmt::format("<{}> mcm_write [valid={}, addr={:#x}, mask={:016x}, data=",
    m.cycle, valid, m.pa, m.mask);
  for (int i=63; i>=0; i--)
    log_str += fmt::format("{:02x}", data[i]);
  log_str += fmt::format("]\n");
  bridge_log_(cvm::HIGH, fmt::to_string(log_str));
}

// Process inst fetches
void bridge::process_dut_mcm_ifetch(hart_id_t hart, mem_t& m) {
  bool valid = false;

  if (!client_->whisperMcmIFetch(hart, m.cycle, m.pa, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed mcm ifetch\n", hart);
    return;
  }
  bridge_log_(cvm::HIGH, "<{}> mcm_ifetch [valid={}, addr={:#x}]\n", m.cycle, valid, m.pa);
}

// Process inst evicts
void bridge::process_dut_mcm_ievict(hart_id_t hart, mem_t& m) {
  bool valid = false;

  if (!client_->whisperMcmIEvict(hart, m.cycle, m.pa, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed mcm ievict\n", hart);
    return;
  }
  bridge_log_(cvm::HIGH, "<{}> mcm_ievict [valid={}, addr={:#x}]\n", m.cycle, valid, m.pa);
}

uint64_t bridge::translate(hart_id_t hart, uint64_t va, uint8_t priv, memclass_t memclass) {
  uint64_t pa = va;

  if (priv == 0x3)
    return pa;

  bool valid;
  bool r = (memclass == memclass_t::read);
  bool w = (memclass == memclass_t::write);
  bool x = (memclass == memclass_t::fetch);
  bool sup = ((priv & 0x11) == 0x1); // made a change here
  
if(twoStage_ == true)
  sup = false;

if (!client_->whisperTranslate(hart, va, r, w, x, twoStage_, sup, pa, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed VA translation\n", hart);
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
  if((w.priv_mode & 0x8) != 0)
  {
    twoStage_ = true;
  }
  // 2.) All flavours of Hypervisor Loads and Stores
  if((w.opcode & 0x7f) == 0x73) 
  {
    // lower 7 bit opcode , upper 4 bits of funct7 and funct3 to differentiate from HFENCE and HINVAL
    if(((w.opcode & 0xf0000000) == 0x60000000) && ((w.opcode & 0x7000) == 0x4000))
    {
      twoStage_ = true;
    }
  }
  // 3.) When MPRV = 1 and MPV = 1 (Table 9.5 in Hypervisor spec)
  if(mprv_ == 1 & mpv_ == 1)
  {
    twoStage_ = true;
  }


  if((mprv_ == 1) && w.priv_mode == 3)
  {
    pa = translate(hart, va, mpp_, memclass_t::read);
  }
  else
    pa = translate(hart, va, w.priv_mode, memclass_t::read);
  
  if (pa != d.mem_pa){
    print(cvm::NONE, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, PC={:#x}, VA={:#x}, RTL-PA={:#x}, ISS-PA={:#x}]\n", w.time, step_-1, hart, w.priv_mode, w.tag, w.pc, d.mem_va, d.mem_pa, pa);
      //print(cvm::ERROR, "Error: Hart {}: PA MISMATCH !! :\n", hart);
    if(is_vector(d.disasm) && disable_pa_check_vec(hart));
    
    else {
    print(cvm::ERROR, "Error: Hart {}: PA MISMATCH !! :\n", hart);
    }

    return;
  }
  else {
    bridge_log_(cvm::MEDIUM, "<{}> Whisper Step #{}: [Hart={}, Mode={}, Tag={}, PC={:#x}, VA={:#x}, PA={:#x}]\n", w.time, step_-1, hart, w.priv_mode, w.tag, w.pc, d.mem_va, pa);
  }

}

// Interrupts
void bridge::process_dut_interrupt(hart_id_t hart, rv_intr_t& i) {
  if (i.mip_mask & 0x1e00) {
    process_external_interrupt(hart, i);
  } 
  if (i.mip_mask & 0x20ee) {
    process_local_interrupt(hart, i);
  }
}

void bridge::process_external_interrupt(hart_id_t hart, rv_intr_t& i) {
    mip_ = (i.mip & i.mip_mask) | (mip_ & ~i.mip_mask);
    e_mip_ = mip_ & 0x1e00;
    check_and_defer_interrupt(hart, i.cycle, i.mip_assert);
  bridge_log_(cvm::MEDIUM, "<{}> External interrupt: Hart {} mip {:#x} mask {:#x} assert {:#x}\n", i.cycle, hart, i.mip, i.mip_mask, i.mip_assert);
}

void bridge::process_local_interrupt(hart_id_t hart, rv_intr_t& i) {
  bridge_log_(cvm::MEDIUM, "<{}> Timer/Sw interrupt: mip {:#x} mask {:#x} assert {:#x}\n", i.cycle, i.mip, i.mip_mask, i.mip_assert);

  // POKE 0x14d 0x24d
  if(i.mip & i.mip_assert & 0x20) { setsstc_poke(hart, i.cycle, 0x14d); stimecmppoked_ = true; }
  if(i.mip & i.mip_assert & 0x40) { setsstc_poke(hart, i.cycle, 0x24d); vstimecmppoked_ = true;}

  // POKE 0x14d 0x24d
  if( ~i.mip_assert & i.mip_mask & 0x20) { resetsstc_poke(hart, i.cycle, 0x14d); stimecmppoked_ = false; }
  if( ~i.mip_assert & i.mip_mask & 0x40) { resetsstc_poke(hart, i.cycle, 0x24d); vstimecmppoked_ = false; }

  // Ideally, would have liked to poke mip with a mask
  // Since we can't, doing a rmw instead
  uint64_t mip;
  peek_mip(hart, i.cycle, mip);
  mip_ = (mip & 0x3e66) | (i.mip & ~0x1e00);
  poke_mip(hart, i.cycle, mip_);

  uint64_t w_seip;
  peek_seip(hart, i.cycle, w_seip);
  mip_ |= w_seip << 9;

  // Defer interrupt only on 0->1 transition
  check_and_defer_interrupt(hart, i.cycle, i.mip_assert);
}

void bridge::process_dut_imsic_msi(hart_id_t hart, mem_t& m) {
  mem_poke_.push_back(m);
  bridge_log_(cvm::MEDIUM, "<{}> poke_mem:: push: [addr={:#x} data={:#x}]\n", m.cycle, m.pa, m.data);
  for (const auto &p : mem_poke_) {
    bridge_log_(cvm::MEDIUM, "<{}> poke_mem: [addr={:#x} data={:#x}]\n", p.cycle, p.pa, p.data);
  }

  process_imsic_msi(hart, m);
}

void bridge::process_imsic_msi(hart_id_t hart, const mem_t& m) {
  bridge_log_(cvm::MEDIUM, "<{}> IMSIC interrupt: [addr={:#x} data={:#x}]\n", m.cycle, m.pa, m.data);

  // Poke imsic write into whisper memory
  bool valid;
  if (!client_->whisperPokeMem(hart, m.cycle, 'm', m.pa, 4, m.data, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to poke memory\n", hart);
    return;
  }

  // Peek mip to check if expected to be taken
  peek_mip(hart, m.cycle, mip_);
  uint64_t w_seip;
  peek_seip(hart, m.cycle, w_seip);
  mip_ |= w_seip << 9;
  e_mip_ = mip_ & 0x1e00;
  
  // Defer interrupt only on 0->1 transition
  bool meip = (e_mip_ >> 11) & 0x1;
  bool seip = (e_mip_ >> 9) & 0x1;
  bool prev_meip = (prev_e_mip_ >> 11) & 0x1;
  bool prev_seip = (prev_e_mip_ >> 9) & 0x1;
  bool meip_assert = (meip != prev_meip);
  bool seip_assert = (seip != prev_seip);
  uint64_t mip_assert = (meip_assert << 11) | (seip_assert << 9);
  check_and_defer_interrupt(hart, m.cycle, mip_assert);
}

void bridge::check_and_defer_interrupt(hart_id_t hart, uint64_t time, uint64_t mip) {
  bool w_intr;
  uint64_t w_cause;
  uint64_t deferredmip;
  bool valid;
  if (!client_->whisperPeek(hart, 's', WhisperSpecialResource::DeferredInterrupts, deferredmip, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed whisper API call - whisperGetDeferredInterrupts\n", hart);
    return;
  }
  uint64_t defer_mip = mip | deferredmip;
  check_interrupt(hart, mip, w_intr, w_cause);
  if (w_intr) {
    deferred_intr_ = true;
    defer_interrupt(hart, time, defer_mip);
  }
}

void bridge::defer_interrupt(hart_id_t hart, uint64_t cycle, uint64_t mip) {
  bridge_log_(cvm::MEDIUM, "<{}> Interrupt defer mip status {:#x}\n", cycle, mip);
  bool valid;
  if (!client_->whisperPoke(hart, cycle, 's', WhisperSpecialResource::DeferredInterrupts, mip, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to poke DeferredInterrupts\n", hart);
    return;
  }
}

void bridge::check_interrupt(hart_id_t hart, uint64_t mip, bool& taken, uint64_t& cause) {
  if (!client_->whisperCheckInterrupt(hart, mip, taken, cause)) {
    print(cvm::ERROR, "Error: Hart {}: Failed whisper API call - whisperCheckInterrupt\n", hart);
    return;
  }
  bridge_log_(cvm::MEDIUM, "<> Whisper check_interrupt: mip: {:#x} taken: {} cause: {}\n", mip, taken, cause);
}

void bridge::poke_mip(hart_id_t hart, uint64_t time, uint64_t mip) {
  bool valid;
  if (!client_->whisperPoke(hart, time, 'c', 0x344, mip, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to poke mip csr\n", hart);
    return;
  }
  bridge_log_(cvm::MEDIUM, "<{}> Whisper poke: mip: {:#x}\n", time, mip);
}

void bridge::peek_mip(hart_id_t hart, uint64_t time, uint64_t& mip) {
  bool valid;
  if (!client_->whisperPeek(hart, 'c', 0x344, mip, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to peek mip\n", hart);
    return;
  }
  bridge_log_(cvm::MEDIUM, "<{}> Whisper peek: mip: {:#x}\n", time, mip);
}

void bridge::peek_seip(hart_id_t hart, uint64_t time, uint64_t& val) {
  if (!client_->whisperGetSeiPin(hart, val)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to peek seip\n", hart);
    return;
  }
  bridge_log_(cvm::MEDIUM, "<{}> Whisper peek: seip: {}\n", time, val);
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
  bridge_log_(cvm::NONE, "<{}> Enter debug mode\n", d.cycle);
  if (!debug_mode_) {
    if (!client_->whisperEnterDebug(d.hart)) {
      print(cvm::ERROR, "Error: Hart {}: Failed to enter debug mode\n", id_);
      return;
    }
  }

  debug_mode_ = true;

  bool valid;
  for(int i=25; i>=0; i--) {
    uint64_t debugROM_loc = FLAGS_debug_entry_pc + (25-i)*8;
    if (!client_->whisperPokeMem(d.hart, 0, 'm', debugROM_loc, 8, debugROM[i], valid)) {
      print(cvm::ERROR, "Error: Hart {}: Failed to poke debug memory\n", d.hart);
      return;
    }
  }
}

void bridge::exit_debug_mode(rv_debug_t& d) {
  bridge_log_(cvm::NONE, "<{}> Exit debug mode\n", d.cycle);
  debug_mode_ = false;
}

uint64_t bridge::modify_csr_data(hart_id_t hart, uint64_t addr, uint64_t data) {
  uint64_t result = data;
  // pmpaddr
  // Spec section...
  if (addr >= 0x3B0 && addr < 0x3C0) {
    bool valid;
    uint64_t pmpcfg, mask, reset, read_mask;
    uint64_t i, pmp_cfg_reg, pmp_cfg_index;
    // For PMP addresses, which bits of the pmpcfgs to look for 
    i = addr - 0x3B0;
    pmp_cfg_reg = ((i*8) / 64) * 2;
    pmp_cfg_index = (i*8) % 64;
    client_->whisperPeekCsr(hart, 0x3A0 + pmp_cfg_reg, pmpcfg, mask, reset, read_mask, valid);
    if((pmpcfg >> (pmp_cfg_index + 4)) & 0x1) {
      result = data | 0x1ff;
    } else {
      result = data & 0xfffffffffffffc00;
    }
  }
  return result;
}

bridge::size_8_bytes_t bridge::modify_csr_mask(hart_id_t hart, uint64_t addr, uint64_t data, size_8_bytes_t mask) {
  size_8_bytes_t result = mask;
  // pmpaddr
  // Spec section...
  if (addr == 0xC20) result = mask;
  else result = mask & get_csr_mask(hart, addr);
  if (addr >= 0x3B0 && addr < 0x3C0) {
    bool valid;
    uint64_t pmpcfg, mask_iss, reset, read_mask;
    uint64_t i, pmp_cfg_reg, pmp_cfg_index;
    // For PMP addresses, which bits of the pmpcfgs to look for 
    i = addr - 0x3B0;
    pmp_cfg_reg = ((i*8) / 64) * 2;
    pmp_cfg_index = (i*8) % 64;
    client_->whisperPeekCsr(hart, 0x3A0 + pmp_cfg_reg, pmpcfg, mask_iss, reset, read_mask, valid);
    if((pmpcfg >> (pmp_cfg_index + 4)) & 0x1) {
      result = result | 0x1ff;
    } else {
      result = result | 0x3ff;
    }
  }
  if (addr == 0x680) {
    uint16_t mode = (data & mask) >> 60;
    constexpr uint16_t valid_modes[] = {0, 8, 9, 10};
    bool valid_mode = false;
    for (uint16_t valid_mode_value : valid_modes) {
      if (mode == valid_mode_value) {
          valid_mode = true;
          break;
      }
    }
    if (!valid_mode) {
      result = result & 0xfffffffffffffffULL;
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
  return (addr >= 0x7E0 && addr <= 0x7EF);
}

bool bridge::is_chicken_bit_csr(uint64_t addr) {
  return (addr >= 0xBC0 && addr <= 0xBDF);
}

void bridge::update_csr(hart_id_t hart, src_t src, uint64_t addr, uint64_t data, cac::optional_const_ref<size_8_bytes_t> mask_ref, bool shadow_csr, bool check_en) {
  if (is_custom_csr(addr) &&
      !is_pmacfg_csr(addr) &&
      !is_chicken_bit_csr(addr))
    return;

  bool check = true;
  if (is_chicken_bit_csr(addr))
    check = false; // FIXME: Reset values in json
  else
    check = check_en;

  resource_id_t csr_resource = resource_id_t{
    .resource = resource_t::csr_reg,
    .offset = addr
  };
  cac::mask_t mask = cac::CreateBitVec<size_8_bytes_t>(std::numeric_limits<size_8_bytes_t>::max());
  if (mask_ref != std::nullopt) {
    mask = cac::CreateBitVec<size_8_bytes_t>(mask_ref.value());
  }
  if (!csr_cac_.SetResource(hart, src, csr_resource, std::move(cac::CreateBitVec<size_8_bytes_t>({data})), mask, check)) {
    print(cvm::ERROR, "Error: Hart {}: CAC: Failed to SetResource {}\n", hart, csr_resource.ToString());
  }

  // Also update shadow csr if applicable ex: mstatus/sstatus
  if (!shadow_csr && shadow_csrs.count(addr)) {
    auto range = shadow_csrs.equal_range(addr);
    for (auto shadow_csr = range.first; shadow_csr != range.second; ++shadow_csr) {
        size_8_bytes_t alias_mask;
      if (src == src_t::dut){
        if (mask_ref)
          alias_mask = mask_ref.value() & get_csr_poke_mask(hart, shadow_csr->second);
        else
          alias_mask = get_csr_poke_mask(hart, shadow_csr->second);
      }
      else {
        uint64_t mask, poke_mask, read_mask;
        bool valid;
        if (!client_->whisperPeekCsr(hart, shadow_csr->second, data, mask, poke_mask, read_mask, valid)) {
          print(cvm::ERROR, "Error: Hart {}: Failed to peek csr\n", hart);
        }
        alias_mask = get_csr_poke_mask(hart, shadow_csr->second);
      }
      update_csr(hart, src, shadow_csr->second, data, alias_mask, true);
    }
  }
}

uint64_t bridge::get_csr(hart_id_t hart, src_t src, uint64_t addr) {
  // Special handling for mip
  if (addr == 0x344)
    return mip_;

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
  if (!client_->whisperPeekCsr(hart, addr, data, mask, poke_mask, read_mask, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to peek csr\n", hart);
  }
  if (debug_mode_ && addr == 0x7b0) //TODO: this list may need to be extended for all CSRs accessible only via Debug mode
    return poke_mask;
  return mask & read_mask;
}

uint64_t bridge::get_csr_poke_mask(hart_id_t hart, uint64_t addr) {
  bool valid;
  uint64_t data, mask, poke_mask, read_mask;
  if (!client_->whisperPeekCsr(hart, addr, data, mask, poke_mask, read_mask, valid)) {
    print(cvm::ERROR, "Error: Hart {}: Failed to peek csr\n", hart);
  }
  return poke_mask;
}

std::string bridge::get_csr_name(const std::string& csr_addr) {
  unsigned int addr;
  try {
    addr = std::stoul(csr_addr, nullptr, 16);
  }
  catch (...) {
    return csr_addr;
  }

  for (const auto& csr : csrs) {
    if (csr.address == addr) {
        return csr.name;
    }
  }
  return csr_addr;
}

void bridge::final_phase() {
  //report_metrics();
}

void bridge::process(const rv_tester::terminate_called&) {
  terminated_ = true;
}

void bridge::report_metrics() {
  if (!FLAGS_metrics || !client_->whisperConnected())
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
  for (auto& csr : metrics_csrs) {
    uint64_t csr_data;
    bool valid;
    if (!client_->whisperPeek(id_, 'c', csr.address, csr_data, valid)) {
      print(cvm::ERROR, "Error: Hart {}: Failed to peek CSR values\n", id_);
    }
    print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_iss_csr_{}\": \"0x{:x}\"}}\n", id_, csr.name, csr_data);
  }

  // DUT csr values
  for (auto& csr : metrics_csrs) {
    uint64_t csr_data = get_csr(id_, src_t::dut, csr.address);
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

  if (!terminated_) {
    // Step one final time to collect metrics for next instruction
    whisper_state_t w;
    if (FLAGS_mcm) {
      client_->whisperDisableMcm();
      w = { .tag = prev_whisp_state.tag+1, .time = prev_whisp_state.time+1 };
    }
    else {
      w = { .tag = step_+1, .time = prev_whisp_state.time+1 };
    }
    step(id_, w);
    const auto& next_instr = w.disasm;
    const auto& next_mode = w.priv_mode;
    const auto& next_trap = w.trap;
    const auto& next_num_dest = w.change_count;

    print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_next_instr\": \"{}\"}}\n", id_, next_instr);
    print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_next_mode\": {}}}\n", id_, next_mode);
    print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_next_trap\": {}}}\n", id_, next_trap);
    print(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_next_num_dest\": {}}}\n", id_, next_num_dest);
  }
  // Regression level metrics from hart 0
  if (id_ == 0) {
    // Average ipc
    print(cvm::NONE, "INFO_PASS_REGR_METRIC:{{\"name\": \"ipc\", \"value\": {:.2f}, \"type\": \"d\", \"action\": \"average\"}}\n", ipc);
  }
}
