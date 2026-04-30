#include <iostream>
#include <thread>
#include <unordered_map>
#include <set>
#include <regex>
#include <sstream>
#include <vector>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include <fmt/ranges.h>
#include "cvm/random.hpp"
#include "sysmod.h"
#include "sysmod_rpc.h"
#include "mem/sysmod_mem.h"
#include "clint/clint.h"
#include "aclint/aclint.h"
#include "dm/dm.h"
#include "io_dev/io_dev.h"
#include "null_dev/null_dev.h"
#include "heartbeat/heartbeat.h"
#include "htif/htif.h"
#include "aplic/aplic_device.h"
#include "Uart8250.hpp"
#include "io_device.h"
#include "trickbox/trickbox.h"
#include "sep_entropy_fifo/sep_entropy_fifo.h"
#include "rv_tester/rv_tester_plusargs.h"
#include "cosim/bridge_if/bridge_params.h"
#include "cosim/whisper_if/whisper_client_plusargs.h"
#include "cosim/dut_if/rvfi/rvfi_plusargs.h"
#include "pmu/pmu_plusargs.h"
#include "cosim/utils/general/util.h"
#include "sysmod_params.hpp"
#include "cosim/bridge/bridge_plusargs.h"
#include "rv_tester_transactions.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include "csr_param.hpp"
using namespace CSR;

// internal flags
DEFINE_uint64(seed, 1, "Simulation seed passed down for randomization");
DEFINE_string(hex, "", "hex file (program) to load into memory");
DEFINE_string(load, "", "elf file (program) to load into memory");
DEFINE_string(load_lz4, "", "lz4 compressed file (program) to load into memory. If there's a colon, the number after the colon is interpreted as the offset to load the image into memory");
DEFINE_string(load_bin, "", "Binary file (program) to load into memory. If there's a colon, the number after the colon is interpreted as the offset to load the image into memory");
DEFINE_bool(bootrom, true, "Load bootrom before test");
DEFINE_string(bootrom_path, "", "Path to bootrom object file");
DEFINE_bool(debugrom, false, "Load debug ROM ELF before test");
DEFINE_string(debugrom_path, "", "Path to debug ROM ELF");
DEFINE_bool(cplfw, false, "Load cpl firmware before test");
DEFINE_string(cplfw_path, "", "Path to cpl firmware object file");
DEFINE_string(load_io, "", "load specified io dev with content from memory");
DEFINE_bool(sysmod_tick_async, true, "Asynchronous sysmod_tick calls");
DEFINE_uint64(sysmod_tick_update_threshold, 1, "Slow down tick update frequency by this factor. The tick is still eventually advanced the same cumulative amount, just not as often. Useful for emulation where the clock counts much faster but tests setup interrupts to happen very soon for simulation. They git hit by an interrupt storm and are stuck in the interrupt handler forever.");
// DCLS
DEFINE_bool(dcls_en, false, "Enable DCLS i.e. Dual core lockstep mode");
//core harvesting
//FIXME: Move these defines to cluster_rv_tester/harvesting_agent.cpp once all rv_tester deps are moved
DEFINE_bool(rand_core_harvest, false, "Randomize core harvest options");
DEFINE_uint32(num_harts, cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second , "Number of enabled harts - upto 8");
DEFINE_uint32(hart_enable_mask, 0, "Hart enable mask. Ex: With 2 enabled harts in a 8-hart system, could ie 0x18. Should match num_harts.");
DEFINE_string(hart_enable_id, "", "Hart id sequence corresponding to physical cores. Ex: With 2 enabled harts in a 8-hart system, could be 4,3 i.e. hart0=core4, hart1=core3.");
// SC harvesting
//FIXME: Move these defines to cluster_rv_tester/harvesting_agent.cpp once all rv_tester deps are moved
DEFINE_int32(sc_dis_ways_mask, -1, "SC way enable mask. Ex: With 20 enabled ways out of 24, could be 0xF0_FFFF.");
//Fuse
DEFINE_uint32(debug_enable, 3, "Debug enable fuse");
DEFINE_bool(ntrace_enable, true, "Trace enable fuse");
DEFINE_bool(dst_enable, true, "Enable DST fuse");
DEFINE_bool(cla_enable, true, "Enable CLA fuse");
DEFINE_bool(io_coherency_disable, false, "Disable io coherency");
DEFINE_int32(strobe_type,4, "strobe type need to be driven for random access");
DEFINE_uint32(overlay_num_times,3, "Maximum number of debug snippets to be driven");
DEFINE_int32(overlay_idle,5, "Number of idle cycles between each transfer");
DEFINE_int32(start_overlay_access,10, "Start tick point for starting overlay access");
DEFINE_bool(export_control_en, false, "Enable export control to reduce FP double precision");
DEFINE_uint32(mem_manager_page_size, 4096, "Mem manager internal page size");
// Uninitialized memory read callback configuration
DEFINE_bool(sysmod_mem_random, false, "Return random data for uninitialized memory reads");
DEFINE_uint64(sysmod_mem_default_64, 0, "Return specific 64-bit pattern for uninitialized memory reads (0 = disabled)");
// STEE
DEFINE_string(stee_secure_region, "", "colon separated pair of number (same as whisper's --steesr)");
DEFINE_uint64(pa_mask, 0x0080000000000000, "address bit(s) that act as STEE distinction");
DEFINE_bool(sysmod_terminate, true, "Set to false for offline DPI mode");
// APLIC
DEFINE_uint32(aplic_sources, 127, "Number of APLIC interrupt sources");
// Uart8250
DEFINE_bool(uart8250_sock, false, "Enable uart8250 spawning a unix domain socket server for I/O");
DEFINE_uint32(uart8250_iid, 1, "Interrupt identity of the uart8250 device");
DEFINE_uint64(dm_rand_addr, 0x9080500, "(Trickbox) Random address for DM: PC/Load/Store");;
// Overlay MMR Checker
DEFINE_bool(overlay_mmr_check, true, "Enable Scratchpad MMR read/write data checks from Overlay port");
// Enable NTrace during boot
DEFINE_bool(enable_ntrace_in_boot, false, "Enable NTrace during boot");

REGISTRY_register(sysmod, TOP.PLATFORM.SYSMOD, 0);

extern "C" {
  void sysmod_timer_interrupt(unsigned hartid, unsigned val, unsigned long mtime_val);
  void sysmod_sw_interrupt(unsigned hartid, unsigned val);
  void sysmod_tbox_interrupt(unsigned hartid, unsigned val, unsigned int_val);
  void sysmod_trace_info(unsigned trace_info_s);
  void sysmod_dmi_write(unsigned hartid, unsigned upper_val, unsigned lower_val);
  void sysmod_jtag_req(unsigned cmd,unsigned long upper_val, unsigned long lower_val, unsigned length, unsigned quit,unsigned tap_cfg_sel);
  void sysmod_terminate();
}

sysmod::sysmod(cvm::topology::loc_t loc, unsigned id)
  : scope_(nullptr), loc_(loc), id_(id)
{
  // Whisper client location
  wc_loc_ = cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0);

  cvm::registry::messenger.connect<svScope>(
      loc_,
      [this](svScope s) { return this->set_scope(s); });
  cvm::registry::messenger.connect<rv_tester::whisper_connected>(
      wc_loc_,
      [this](const auto&) {
      this->store_dm_rand();
      cosim_init_ = true;
      return;
      });
  cvm::registry::messenger.procedure<sysmod_eot>(loc, [this] (uint64_t addr, size_t length, device::data_t& data){ return this->dev("memory")->backdoor_read(addr, length, data);});
  cvm::registry::messenger.procedure<sysmod_secure_mem>(loc, [this] (uint64_t& start, uint64_t& end) {
    start = secure_region_start_;
    end = secure_region_end_;
  });
  cvm::registry::messenger.procedure<sysmod_backdoor_write_boot>(loc, [this] (uint64_t addr, uint32_t size, const device::data_t& data, const device::strb_t& strb) {
    device::data_t data_copy = data;
    device::strb_t strb_copy = strb;
    cvm::log(cvm::NONE, "[sysmod] Writing boot memory: addr={:#x}, size={}, data={}\n", addr, size, fmt::join(data_copy, ", "));
    this->dev("boot")->backdoor_write(addr, size, data_copy, strb_copy);
  });
  cvm::registry::messenger.procedure<sysmod_get_boot_addr>(loc, [this] () -> uint64_t {
    return this->dev("boot")->addr();
  });

  cvm::registry::messenger.connect<rv_tester_transactions::sysmod::tick<>>(
      loc_,
      [this](const rv_tester_transactions::sysmod::tick<>& t) { return this->tick(t.advance); });
  cvm::registry::messenger.connect<rv_tester_transactions::sysmod::tick<>>(
      loc_,
      [this](const rv_tester_transactions::sysmod::tick<>& t) { return this->is_dut_reset_req(t.dut_reset_req,t.clocks,t.divisor); });
 // cvm::registry::messenger.connect<rv_tester_transactions::sysmod::jtag_tick<>>(
 //     loc_,
 //     [this](const rv_tester_transactions::sysmod::jtag_tick<>& t) { return this->jtag_tick(t.advance); });
  cvm::registry::messenger.connect<rv_tester_transactions::sysmod::overlay_tick<>>(
      loc_,
      [this](const rv_tester_transactions::sysmod::overlay_tick<>& t) { return this->overlay_tick(t.advance); });
  //cvm::registry::messenger.connect<rv_tester_transactions::sysmod::jtag_rdata<>>(
  //    loc_,
  //    [this](const rv_tester_transactions::sysmod::jtag_rdata<>& t) { return this->jtag_resp(t.rdata); });
  cvm::registry::messenger.connect<rv_tester_transactions::sysmod::event_triggers<>>(
      loc_,
      [this](const rv_tester_transactions::sysmod::event_triggers<>& t) { return this->tboxtrig_updatemem(t.addr,t.data); });
  cvm::registry::messenger.connect<sysmod::backdoor_read_t>(
      loc_,
      [this](sysmod::backdoor_read_t t) {
      auto *task = +[] (sysmod* m, backdoor_read_t w) -> cvm::messenger::task<void> {
          *w.out_data = co_await m->backdoor_read(w.address);
          *w.flag = true;
          w.flag->notify_one();
          co_return;
      };
      cvm::registry::messenger.fork(task, this, std::move(t));
      }
      );

  uint32_t num_harts = cvm::topology::attr(cvm::topology::get_from_type("PLATFORM", 0), "NHARTS").second;

  if (FLAGS_cosim) {
    for(uint32_t i = 0 ; i < num_harts ; i++) {
      int unsigned location = cvm::topology::get_from_type("CORE", i);
      cvm::registry::messenger.connect<inval_load_s>(location , [this] (const auto& payload) { return this->store_inval_load(payload); });
      cvm::registry::messenger.connect<inval_crsp_s>(location , [this] (const auto& payld) { return this->store_inval_crsp(payld, 1 /*mcm*/); });
    }
  }

  cvm::registry::messenger.connect<cbo_inval_nomcm_s>(loc_, [this] (const auto& cbo_inval_nomcm) {
    inval_crsp_s payload;
    payload.address = cbo_inval_nomcm.address;
    return this->store_inval_crsp(payload, 0/*nomcm*/);
  });

  cvm::registry::messenger.connect<sysmod::backdoor_write_t>(
      loc_,
      [this](sysmod::backdoor_write_t t) {
      auto *task = +[] (sysmod* m, backdoor_write_t w) -> cvm::messenger::task<void> {
          co_await m->backdoor_write(w);
          *w.flag = true;
          w.flag->notify_one();
          co_return;
      };
      cvm::registry::messenger.fork(task, this, std::move(t));
      }
      );


  auto sources = cvm::topology::get_from_type("PLATFORM_TRANSACTOR");
    for (const auto& source : sources) {
        cvm::registry::messenger.connect<transactor::write_t>(
            source,
            [this, source](const auto& w) {
                if (this->dev(w.addr)) {
                    std::string d;
                    if (cvm::logger::check_verbosity(cvm::FULL)) {
                      for (int i=w.data.size()-1; i>=0; i--) {
                        d += fmt::format("{:02x}", w.data[i]);
                      }
                    }
                    cvm::log(cvm::HIGH, "[sysmod] write: src={} addr={:#x}, data={}\n", source, w.addr,d);
                    transactor::write_t w_pkt;
                    w_pkt = w;
                    w_pkt.addr = w.addr & ~FLAGS_pa_mask; // STEE : RVDE-24052
                    cvm::registry::messenger.signal<device::write_t>(this->loc_, {w_pkt});
                }
        });
        cvm::registry::messenger.connect<transactor::read_t>(
            source,
            [this, source](const auto& r) {
                if (this->dev(r.addr)){
                    cvm::log(cvm::HIGH, "[sysmod] read: src={} id={}, addr={:#x}, len={}\n", source, r.id, r.addr, r.length);
                    cvm::registry::messenger.signal<device::read_t>(this->loc_, {{r.id, r.addr & ~FLAGS_pa_mask, r.length}, source});
                }
        });
  }
 auto snoop_gen_loc = cvm::topology::get_from_hierarchy("TOP.PLATFORM.SNOOP_GEN", 0);
 auto platform_loc = cvm::topology::get_from_hierarchy("TOP.PLATFORM", 0);
 cvm::registry::messenger.connect<transactor::write_t>(snoop_gen_loc, [this] (transactor::write_t w)  { return this->eot_backdoor_write(w);});
 cvm::registry::messenger.connect<htif::terminate_t>(  platform_loc,  [this] (htif::terminate_t t)    { return this->terminate(t);});
 cvm::registry::messenger.connect<rv_tester::test_started>(platform_loc, [this] (rv_tester::test_started t) { return this->actual_test_started(t);});
}

void sysmod::eot_backdoor_write(transactor::write_t& w) {
  device::strb_t strb(8);
  device::data_t data(8);
  for(size_t i=0; i<64; i+=8) {
    for (size_t j=0; j<8; j++) {
      data[j] = w.data[i+j];
      strb[j] = true;
    }
    this->dev("memory")->backdoor_write(((w.addr + i)& ~FLAGS_pa_mask), 8, data, strb);
  }
}

void sysmod::configure()
{
  if (FLAGS_cosim)
    cosim_init_ = false; // this will be set once bridge is init
  else
    cosim_init_ = true;
  // Harvesting plusargs now handled by harvesting module
  // Reset configuration
  reset();
}

// Harvesting methods removed - now handled by harvesting module


sysmod::~sysmod()
{
}

// forwarding functions for devices
void
sysmod::timer_interrupt(clint::timer_t t) {
      cvm::registry::callbacks.push(
        scope(),
        [t]() {
          cvm::log(cvm::FULL, "[SYSMOD] timer_interrupt [hart={}, mti={} mtime={:#x}]\n", t.hart, t.flag, t.mtime);
          sysmod_timer_interrupt(t.hart, t.flag, t.mtime);
        });
}

void
sysmod::sw_interrupt(clint::sw_t s) {
  cvm::registry::callbacks.push(
      scope(),
      [s]() {
        cvm::log(cvm::FULL, "[SYSMOD] sw_interrupt [hart={}, msi={}]\n", s.hart, s.flag);
        sysmod_sw_interrupt(s.hart, s.flag);
      });
}

void
sysmod::tbox_interrupt(interrupter::interrupt_t i) {
  cvm::registry::callbacks.push(
      scope(),
      [i]() {
        cvm::log(cvm::FULL, "[SYSMOD] tbox_interrupt [hart={}, intr.sel={:#x}, intr.val={:#x}]\n", i.hart, i.intr_select, i.intr_value);
        sysmod_tbox_interrupt(i.hart, i.intr_select, i.intr_value);
      });
}


//void
// sysmod::scratchpad_xtor_read_req_router(scratchpad_xtor::scratchpad_xtor_read_t r) {

//     transactor::read_t rd;
//     rd.addr = r.addr;
//     rd.length = r.length;
//     rd.id =  r.id;
//     cvm::log(cvm::FULL, "[SYSMOD] SCRATCHPAD_XTOR ROUTER - addr={:#x} \n", rd.addr);

//     auto sources = cvm::topology::get_from_type("PLATFORM_TRANSACTOR");

//     if (this->dev(r.addr)){
//     cvm::log(cvm::FULL, "[SYSMOD] SCRATCHPAD_XTOR ROUTER  send to device - addr={:#x} \n", rd.addr);
//         cvm::registry::messenger.signal<device::read_t>(this->loc_, {rd, sources[0]});
//     }

// }
void
sysmod::uc_helper_backdoor_write(uc_helper::uc_helper_write_t w) {

    if (!FLAGS_rv_tester_mem_bypass_cache && FLAGS_rv_tester_enable_llc)
      cvm::log(cvm::ERROR, "Error: [SYSMOD] uc_helper_backdoor_write: caching is enabled in rv_tester and it does not receive DMAs, so the test could fail if the CPU does a read to this address as it will receive the stale cached data");

    cvm::log(cvm::FULL,"[SYSMOD] uc_helper_backdoor_write addr {:#x} \n",w.addr);
    cvm::log(cvm::FULL,"[SYSMOD] uc_helper_backdoor_write len {} \n",(unsigned)w.length);
    cvm::log(cvm::FULL,"[SYSMOD] uc_helper_backdoor_write data-vec : \n");

    for (auto i: w.data)
      cvm::log(cvm::HIGH," {:#x} ",(unsigned)i);

    cvm::log(cvm::FULL, "[SYSMOD] uc_helper_backdoor:write sysmem for addr {:#x}  \n", w.addr);
    transactor::write_t wt;
    wt.addr = w.addr;
    wt.length = 1;
    wt.data = w.data;
    wt.strb = w.strb;

    cvm::log(cvm::FULL, "[UC_HELPER] new backdoor write request at {:#x}", wt.addr);
    if (this->dev(wt.addr))
      cvm::registry::messenger.signal<device::write_t>(this->loc_, {wt});
}

void
sysmod::store_inval_crsp(const inval_crsp_s& payld, bool mcm) {

  cvm::log(cvm::HIGH, "CBO_INVAL_MONITOR::MCM={} FLAGS_MCM={}\n", mcm, FLAGS_mcm );
  if ( mcm and !FLAGS_mcm) return;
  if (!mcm and  FLAGS_mcm) return;
  inval_crsp_ = payld;
  // Compulsive Backdoor write
  uint64_t read_data = 0;
  device::data_t data(8);
  uint64_t ld_addr = ((inval_crsp_.address) >> 6) << 6; // Starting from cacheline base address
  // Performing Whisper Poke for entire Cacheline Granularity
  for (int offset=0; offset<8; offset++) {
    read_data = 0;
    dev("memory")->backdoor_read(ld_addr + (offset*8), 8, data);
    for (int i=0; i<8; ++i)
      read_data |= uint64_t(data[i]) << (i*8);
     bool valid = true;
     cvm::log(cvm::FULL, "[CBO_INVAL_MONITOR - CRSP POKE] Whisper Poke to Address : {:#x}, with data : {:#x}\n",(ld_addr + (offset*8)),read_data);
     if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(wc_loc_, 0, 0, 'm', (ld_addr + (offset*8)), 8, read_data, false, false, valid) || !valid) && FLAGS_whisper_client_check) {
       cvm::log(cvm::ERROR, "Error: store_inval_crsp failed to poke whisper memory");
    }
  }
}

void
sysmod::store_inval_load(const inval_load_s& payload) {
  inval_load_ = payload;
  // Do a backdoor read for the load's address
  device::data_t data(8);
  uint64_t read_data = 0;
  uint64_t ld_addr = inval_load_.address;
  uint8_t byte_mask = inval_load_.size;
  // length = inval_load_.size;
  // int size = (1 << length)/8;
  dev("memory")->backdoor_read(ld_addr, 8, data);
  for (int i=0; i<8; ++i)
    read_data |= uint64_t(data[i]) << (i*8);

  // read_data is a 64 bit payload
  if(inval_load_.amo) {
    // For AMO MB Bypass -> We dont need to check with main memory contents
    bool valid = true;
    cvm::log(cvm::HIGH, "CBO_INVAL_MONITOR :: Whisper Poke with data:{:#x} for AMO MB Bypass to address:{:#x}\n", inval_load_.data, ld_addr);
    for (int i = 0; i < 8; i++) {
      if (byte_mask & (1 << i)) {
        if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', (ld_addr + i), 1, ((inval_load_.data >> (i*8)) & 0xff), false, false, valid)) && FLAGS_whisper_client_check) {
          cvm::log(cvm::ERROR, "Error: store_inval_load failed to poke whisper memory in AMO MB Bypass case\n");
        }
      }
    }
  }
  uint64_t read_bit_mask=0;
  for (int i = 0; i < 8; i++) {
    if (byte_mask & (1 << i)) {
      read_bit_mask |= (0xFFULL << (i * 8));
    }
  }

  if(((inval_load_.data & read_bit_mask) == (read_data & read_bit_mask)) && (!inval_load_.amo)) // Check with main memory for all cases other than AMO mbbypass
  {
    // No need to poke entire cacheline granularity - that will be done after CRSP
    bool valid = true;
    cvm::log(cvm::HIGH, "CBO_INVAL_MONITOR :: Whisper Poke with data:{:#x} for address:{:#x} with read-mask : {:#x}\n", read_data,ld_addr,byte_mask);
    for (int i = 0; i < 8; i++) {
      if (byte_mask & (1 << i)) {
        if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.WHISPER_CLIENT", 0), 0, 0, 'm', (ld_addr + i), 1, (read_data >> (i*8)), false, false, valid) || !valid) && FLAGS_whisper_client_check) {
          cvm::log(cvm::ERROR, "Error: store_inval_load failed to poke whisper memory\n");
        }
      }
    }
  }
}

cvm::messenger::task<uint64_t>
sysmod::backdoor_write(sysmod::backdoor_write_t t) {

  cvm::log(cvm::HIGH, "[BACKDOOR_WRITE] new backdoor write request at {:#x} value:{:#x} size: {:#x}\n", t.address, t.data, t.size);
  device::data_t datax(8);
  device::strb_t strbx(8);

  if (FLAGS_cosim) {
    bool valid = true;
    if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemRPC>(wc_loc_, 0, 0, 'm', t.address, 8, t.data, false, false, valid) || !valid) && FLAGS_whisper_client_check)
      cvm::log(cvm::ERROR, "Error: [sysmod] backdoor_write failed to poke whisper memory\n");
  }

  for (int i=0; i<t.size; ++i, t.data>>=8) {
    datax[i] = t.data & 0xff;
    strbx[i] = true;
  }
  dev("memory")->backdoor_write(t.address, t.size, datax, strbx);
  co_return 0;
}

cvm::messenger::task<uint64_t>
sysmod::backdoor_read(uint64_t address) {

    device::data_t data(8);
    dev("memory")->backdoor_read(address, 8, data);
    uint64_t read_data = 0;
    for (int i=0; i<8; ++i)
      read_data |= uint64_t(data[i]) << (i*8);
    co_return read_data;
}

void
sysmod::uc_helper_backdoor_read(uc_helper::uc_helper_read_req_t r) {
    if (!FLAGS_rv_tester_mem_bypass_cache && FLAGS_rv_tester_enable_llc)
      cvm::log(cvm::ERROR, "Error: [SYSMOD] uc_helper_backdoor_read: caching is enabled in rv_tester and it does not receive DMAs, so this backdoor read might receive stale data");

    cvm::log(cvm::HIGH, "[SYSMOD] uc_helper_backdoor_read addr {:#x} \n", r.addr);
    cvm::log(cvm::HIGH, "[SYSMOD] uc_helper_backdoor_read len {} \n", (unsigned)r.length);
    cvm::log(cvm::HIGH, "New PRT BACKDOOR read request at {:#x}", r.addr);
    device::data_t data(8);
    std::vector<uint8_t> data_trickbox(8);
    std::vector<bool> strb(8);
    for (size_t i = 0; i < 8; i++) strb[i] = true;
      // Read from memory and write to requested dev tag
      if (not dev("memory"))
        return;
      dev("memory")->backdoor_read(r.addr, r.length, data);
      for (size_t i = 0; i < 8; i++)
        data_trickbox[i] = (uint8_t)data[i];

      auto tbox_loc = cvm::topology::get_from_type("TRICKBOX", 0);
      cvm::registry::messenger.signal(tbox_loc, uc_helper::trickbox_mem_req_t{r.addr, r.length, data_trickbox, strb});
}

void
sysmod::dmi_write(debugger::dmi_data_t i) {
  cvm::registry::callbacks.push(
      scope(),
      [i]() {
        cvm::log(cvm::FULL, "[SYSMOD] trickbox::dmi.(upper,lower) = {:#x}, {:#x}\n", i.upper_dmi_data, i.lower_dmi_data);
        sysmod_dmi_write(i.hart,i.upper_dmi_data,i.lower_dmi_data);
      });
}

void
sysmod::terminate(htif::terminate_t t) {
  // fast path for handlers which want to be notified immediately
  cvm::registry::messenger.signal<rv_tester::terminate_called_fast>(cvm::topology::get_from_type("PLATFORM", 0), rv_tester::terminate_called_fast{});
  // we want this to be low prio and async so it goes behind existing rvfi transactions in the queue
  // because of QoS this could have been seen before all rvfi transactions up to this instruction were processed
  // unless the terminator tells us that it came from a low priority transaction
  const auto prio = t.low_priority_based ? cvm::messenger::highest_priority : cvm::messenger::lowest_priority;
  if (t.low_priority_based)
    cvm::registry::messenger.signal<rv_tester::terminate_called>(cvm::topology::get_from_type("PLATFORM", 0), rv_tester::terminate_called{});
  else
    cvm::registry::messenger.signal_async<rv_tester::terminate_called>(cvm::topology::get_from_type("PLATFORM", 0), rv_tester::terminate_called{}, prio);

  if (FLAGS_sysmod_terminate) {
      cvm::registry::callbacks.push(
          scope(),
          sysmod_terminate
      );
  }
}

void
sysmod::actual_test_started(rv_tester::test_started) {
  cvm::log(cvm::HIGH, "[SYSMOD] actual_test_start\n");
  cvm::registry::messenger.signal<rv_tester::actual_test_start>(cvm::topology::get_from_type("PLATFORM", 0), rv_tester::actual_test_start{});
}

void
sysmod::reset() {
  compose();
  load_prog(FLAGS_hex, FLAGS_load, FLAGS_load_lz4, FLAGS_load_bin);
  load_io(FLAGS_load_io);
  load_boot(FLAGS_bootrom_path);
  load_debugrom(FLAGS_debugrom_path);
  load_cplfw(FLAGS_cplfw_path);
  set_secure_region(FLAGS_stee_secure_region);
}
void
sysmod::set_secure_region(std::string region) {
  if (region == "")
    return;
  stee = true;
  std::vector<std::string> secure_region = cosim_util::split_string(region, ":");
  secure_region_start_ = std::stoull(secure_region.at(0), nullptr, 0);
  secure_region_end_   = std::stoull(secure_region.at(1), nullptr, 0);
  cvm::registry::messenger.call<whisperClient<uint64_t>::secureRegionRPC>(wc_loc_, secure_region_start_, secure_region_end_);
}

std::shared_ptr<TT_APLIC::Aplic>
sysmod::create_aplic() const {
  auto masters = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST");
  auto platform_loc = cvm::topology::get_from_type("PLATFORM", 0);
  auto nharts = cvm::topology::attr(platform_loc, "NHARTS").second;

  std::vector<TT_APLIC::DomainParams> domains;

  // Aplic is handled specially because we must look through all the aplic
  // domains before constructing it.
  for (const auto &d : memmap_) {
    const auto type = d.second.type;

    if (type != "aplic_domain")
      continue;

    const auto base = d.second.base;
    const auto size = d.second.size;
    const auto tag  = d.second.tag;
    const auto attributes = d.second.attributes;

    if (attributes.is_null()) {
      throw std::runtime_error("Domain " + tag + " does not have attributes");
    }

    const auto parent_json = attributes.at("parent");
    const std::optional<std::string> parent = parent_json.is_null() ?
      static_cast<std::optional<std::string>>(std::nullopt) :
      parent_json.template get<std::string>();
    const size_t child_index = attributes.at("child_index");
    const auto privilege = attributes.at("is_machine") ?
      TT_APLIC::Privilege::Machine :
      TT_APLIC::Privilege::Supervisor;

    domains.emplace_back(tag, parent, child_index, base, size, privilege);
  }

  if (domains.empty()) {
    return nullptr;
  }

  std::shared_ptr<TT_APLIC::Aplic> aplic =
    std::make_shared<TT_APLIC::Aplic>(nharts, FLAGS_aplic_sources, domains);

  auto axi_mst_loc = masters[0];
  auto msiCallback = [axi_mst_loc] (uint64_t addr, uint32_t data) {
    cvm::log(cvm::DEBUG, "Aplic sent IMSIC interrupt {:#x} (@ {:#x})\n", data, addr);

    std::vector<uint8_t> data_vec(64, 0);
    std::vector<bool> strb(64, false);
    for (int i = 0; i < 4; i++) {
      data_vec[i] = data & 0xff;
      strb[i] = true;
      data >>= 8;
    }

    cvm::registry::messenger.signal(axi_mst_loc,
        transactor::write_request_t{addr, 64, data_vec, strb, false });
    return true;
  };
  aplic->setMsiCallback(msiCallback);

  return aplic;
}


void
sysmod::compose() {

  devices_.clear();
  // Load memmap
  if (!cvm::registry::messenger.call<memmap::getRPC>(cvm::topology::get_from_hierarchy("TOP.PLATFORM.MEMMAP", 0), memmap_))
    return;

  auto masters = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST");
  auto platform_loc = cvm::topology::get_from_type("PLATFORM", 0);
  auto nharts = cvm::topology::attr(platform_loc, "NHARTS").second;

  std::shared_ptr<mem_manager> mm = std::make_shared<mem_manager>(mem_manager::opts{.page_size = FLAGS_mem_manager_page_size});

  try {
    std::shared_ptr<TT_APLIC::Aplic> aplic = create_aplic();

    for (const auto& d : memmap_) {
      const auto base = d.second.base;
      const auto size = d.second.size;
      const auto type = d.second.type;
      const auto tag  = d.second.tag;

      std::unique_ptr<device> device;

      if (type == "memory") {
        device = std::make_unique<sysmod_mem>(tag, base, size, loc_, mm);

      } else if (type == "sep") {
        device = std::make_unique<sep_entropy_fifo>(tag, base, size, loc_);
      } else if (type == "io_dev") {
        device = std::make_unique<io_dev>(tag, base, size, loc_);

      } else if (type == "null_dev") {
        device = std::make_unique<null_dev>(tag, base, size, loc_);

      } else if (type == "htif") {
        device = std::make_unique<htif>(tag, base, loc_);

      } else if (type == "uart8250") {
        std::unique_ptr<WdRiscv::UartChannel> channel;
        if (FLAGS_uart8250_sock) {
          auto absolutePath = std::filesystem::absolute("uart8250.sock").string();
          // Create unix socket at this path and accept connection
          auto sock = socket(AF_UNIX, SOCK_STREAM, 0);
          if (sock < 0) {
            cvm::log(cvm::ERROR, "Error: failed to create uart8250 socket\n");
            return;
          }
          struct sockaddr_un addr;
          memset(&addr, 0, sizeof(addr));
          addr.sun_family = AF_UNIX;
          strncpy(addr.sun_path, absolutePath.c_str(), sizeof(addr.sun_path) - 1);
          if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            cvm::log(cvm::ERROR, "Error: failed to bind uart8250 socket\n");
            return;
          }
          if (listen(sock, 1) < 0) {
            cvm::log(cvm::ERROR, "Error: failed to listen on uart8250 socket\n");
            return;
          }

          cvm::log(cvm::NONE, "TEE_IO:{}\n", absolutePath);
          cvm::log(cvm::NONE, "TTY_RAW\n", absolutePath);
          // Flush to guarantee simtest sees the above output
          std::flush(std::cout);

          channel = std::make_unique<WdRiscv::SocketChannel>(sock);

          if (close(sock) < 0) {
            cvm::log(cvm::ERROR, "Error: failed to close uart8250 socket\n");
            return;
          }
          if (unlink(absolutePath.c_str()) < 0) {
            cvm::log(cvm::ERROR, "Error: failed to unlink uart8250 socket\n");
            return;
          }
        } else {
          channel = std::make_unique<WdRiscv::FDChannel>(STDIN_FILENO, STDOUT_FILENO);
        }

        device = std::make_unique<io_device<WdRiscv::Uart8250>>(tag, loc_,
            base, 32, aplic, FLAGS_uart8250_iid, std::move(channel),
            // We only enable input when using a socket; When using stdio, we only output
            FLAGS_uart8250_sock);
      } else if (type == "dm") {
        // TODO: cvm::ERROR
       // assert(masters.size() > 0);
       // device = std::make_unique<dm>(tag, base, size, loc_, masters[0]);

      // } else if (type == "scratchpad_xtor") {
      //   // TODO: cvm::ERROR
      //   assert(masters.size() > 0);
      //   device = std::make_unique<scratchpad_xtor>(tag, base, size, loc_, masters[0]);
      //   cvm::registry::messenger.connect<scratchpad_xtor::scratchpad_xtor_read_t>(
      //       loc_,
      //       [&](scratchpad_xtor::scratchpad_xtor_read_t i) { return this->scratchpad_xtor_read_req_router(i); });

      } else if (type == "clint") {
        device = std::make_unique<clint>(tag, base, nharts, loc_);
        cvm::registry::messenger.connect<clint::timer_t>(
            loc_,
            [&](clint::timer_t t) { return this->timer_interrupt(t); });
        cvm::registry::messenger.connect<clint::sw_t>(
            loc_,
            [&](clint::sw_t s) { return this->sw_interrupt(s); });

      } else if (type == "aclint") {
        device = std::make_unique<aclint>(tag, base, nharts, loc_);
        cvm::registry::messenger.connect<clint::timer_t>(
            loc_,
            [&](clint::timer_t t) { return this->timer_interrupt(t); });

      } else if (type == "trickbox") {
        device = std::make_unique<trickbox>(tag, base, nharts, loc_,masters[0]);
        cvm::registry::messenger.connect<interrupter::interrupt_t>(
            loc_,
            [&](interrupter::interrupt_t i) { return this->tbox_interrupt(i); });
        cvm::registry::messenger.connect<debugger::dmi_data_t>(
            loc_,
            [&](debugger::dmi_data_t i) { return this->dmi_write(i); });
       // cvm::registry::messenger.connect<jtag_driver::jtag_data_t>(
       //     loc_,
       //     [&](jtag_driver::jtag_data_t i) { return this->jtag_req(i); });
        cvm::registry::messenger.connect<uc_helper::uc_helper_write_t>(
            loc_,
            [&](uc_helper::uc_helper_write_t i) { return this->uc_helper_backdoor_write(i); });
        cvm::registry::messenger.connect<uc_helper::uc_helper_read_req_t>(
            loc_,
            [&](uc_helper::uc_helper_read_req_t i) { return this->uc_helper_backdoor_read(i); });

      } else if (type == "aplic_domain") {
        device = std::make_unique<aplic_device>(tag, base, size, loc_, aplic);
      } else {
        cvm::log(cvm::ERROR, "Error: [sysmod] unknown sysmod type {} \n", type);
      }

      if (device)
        devices_.emplace_back(std::move(device));
    }

    devices_.emplace_back(std::make_unique<heartbeat>("heartbeat", 0, 0, loc_));

    // Create fallback null_dev for unmapped addresses
    fallback_null_dev_ = std::make_unique<null_dev>("fallback_null_dev", 0, 0, loc_);

    assert(masters.size() > 0);

    // Configure uninitialized read callbacks for memory devices
    configure_uninit_read_callbacks();
  }
  catch (std::exception& e) {
    std::cerr << "Error: [sysmod] Memmap access exception.\n" << "  Message: " << e.what() << "\n";
  }
}

device*
sysmod::dev(uint64_t addr) {
  if (stee) {
    if (addr & FLAGS_pa_mask)
      addr = addr & ~FLAGS_pa_mask;
  }
  for (auto& d : devices_) {
    if (d->has_addr(addr)){
      return d.get();
    }
  }
  cvm::log(cvm::ERROR, "Error: [sysmod] Address not mapped: {:#x}\n", addr);
  return fallback_null_dev_.get();
}

device*
sysmod::dev(const std::string& tag) {

  for (auto& d : devices_) {
    if (d->tag() == tag)
      return d.get();
  }
  cvm::log(cvm::ERROR, "Error: [sysmod] Tag not mapped: {}\n", tag);
  return nullptr;
}

void
sysmod::load_io(const std::string& io) {

  if (io == "")
    return;

  std::stringstream ss(io);
  while (ss.good()) {
    std::string io_str;
    std::getline(ss, io_str, ',' );
    std::size_t pos = io_str.find(':');
    if (pos != std::string::npos) {
      std::string tag = io_str.substr(0, pos);
      std::string offset_str = io_str.substr(pos + 1);
      uint64_t offset = std::stoul(offset_str, nullptr, 16);

      device::data_t data(8);
      device::strb_t strb(8);
      for (size_t i = 0; i < 8; i++) strb[i] = true;

      // Read from memory and write to requested dev tag
      if (not dev("memory") or not dev(tag))
        return;

      dev("memory")->backdoor_read(dev(tag)->addr()+offset, 8, data);
      dev(tag)->backdoor_write(dev(tag)->addr()+offset, 8, data, strb);
    }
  }
}

bool
sysmod::bin_load(const std::string load, bool lz4_compressed)
{
  uint64_t offset = 0;
  std::string file = load;

  cvm::log(cvm::MEDIUM, "Loading {}\n", load);
  if (std::size_t pos = load.find(':'); pos != std::string::npos) {
    file = load.substr(0, pos);
    std::string offset_str = load.substr(pos + 1);
    offset = std::stoull(offset_str, nullptr, 0);
  }
  if (not dev("memory") or not
          (lz4_compressed ? dynamic_cast<sysmod_mem&>(*dev("memory")).init_lz4(file, offset) :
                            dynamic_cast<sysmod_mem&>(*dev("memory")).init_bin(file, offset)
          )
     ) {
    cvm::log(cvm::ERROR, "Error: [sysmod] No memory defined\n");
    return false;
  }

  return true;
}

void
sysmod::load_prog(const std::string& hex, const std::string& load, const std::string& lz4, const std::string& bin) {

  for (const auto& d : memmap_) {
    const auto type = d.second.type;
    const auto tag  = d.second.tag;

    if (type != "memory") continue;

    if (load != "") {
      cvm::log(cvm::MEDIUM, "Loading {}\n", load);
      if (not dev(tag) or not dynamic_cast<sysmod_mem&>(*dev(tag)).init_elf(load)) {
        cvm::log(cvm::ERROR, "Error: [sysmod] Failed to load program\n");
        return;
      }
      cvm::log(cvm::MEDIUM, "Loading {} complete\n", load);
    }

    if (hex != "") {
      cvm::log(cvm::MEDIUM, "Loading {}\n", hex);
      if (not dev(tag) or not dynamic_cast<sysmod_mem&>(*dev(tag)).init_hex(hex)) {
        cvm::log(cvm::ERROR, "Error: [sysmod] No memory defined\n");
        return;
      }
      cvm::log(cvm::MEDIUM, "Loading {} complete\n", hex);
    }

    auto parse_bin = [this](const std::string& flag, bool lz4_compressed) {
      std::stringstream ss;

      cvm::log(cvm::MEDIUM, "Parsing {}\n", flag);
      // split string by colon into file path and offset
      // if no colon is found, assume offset is 0

      ss << flag;
      while (ss.good())
      {
        std::string substr;

        getline(ss, substr, ',');
        if (not bin_load(substr, lz4_compressed))
          return;
      }
      cvm::log(cvm::MEDIUM, "Parsing {} complete\n", flag);
    };

    if (lz4 != "") parse_bin(lz4, true);
    if (bin != "") parse_bin(bin, false);

    // all memories share the same backing mem manaager
    return;
  }

  cvm::log(cvm::ERROR, "Error: [sysmod] No memory found\n");
}

void
sysmod::load_boot(const std::string& boot) {

  if (FLAGS_bootrom && boot != "") {
    cvm::log(cvm::MEDIUM, "Loading {}\n", boot);
    if (boot.substr(boot.length() - 3) == "elf") {
      if (not dev("boot") or not dynamic_cast<sysmod_mem&>(*dev("boot")).init_elf(boot)) {
        cvm::log(cvm::ERROR, "Error: [sysmod] No boot defined");
        return;
      }
    }
    if (boot.substr(boot.length() - 3) == "hex") {
      if (not dev("boot") or not dynamic_cast<sysmod_mem&>(*dev("boot")).init_hex(boot)) {
        cvm::log(cvm::ERROR, "Error: [sysmod] No boot defined");
        return;
      }
    }
  }
}


void
sysmod::load_debugrom(const std::string& debugrom) {

  if (FLAGS_debugrom && debugrom != "") {
    cvm::log(cvm::MEDIUM, "Loading debug ROM {}\n", debugrom);
    if (debugrom.substr(debugrom.length() - 3) == "elf") {
      if (not dev("dm") or not dynamic_cast<sysmod_mem&>(*dev("dm")).init_elf(debugrom)) {
        cvm::log(cvm::ERROR, "Error: [sysmod] No dm device defined for debug ROM");
        return;
      }
    }
    if (debugrom.substr(debugrom.length() - 3) == "hex") {
      if (not dev("dm") or not dynamic_cast<sysmod_mem&>(*dev("dm")).init_hex(debugrom)) {
        cvm::log(cvm::ERROR, "Error: [sysmod] No dm device defined for debug ROM");
        return;
      }
    }
  }
}


void
sysmod::load_cplfw(const std::string& cplfw) {

  if (FLAGS_cplfw && cplfw != "") {
    cvm::log(cvm::MEDIUM, "Loading {}\n", cplfw);
    if (cplfw.substr(cplfw.length() - 3) == "elf") {
      if (not dev("memory") or not dynamic_cast<sysmod_mem&>(*dev("memory")).init_elf(cplfw)) {
        cvm::log(cvm::ERROR, "Error: [sysmod] No cpl firmware defined");
        return;
      }
    }
    if (cplfw.substr(cplfw.length() - 3) == "hex") {
      if (not dev("memory") or not dynamic_cast<sysmod_mem&>(*dev("memory")).init_hex(cplfw)) {
        cvm::log(cvm::ERROR, "Error: [sysmod] No cpl firmware defined");
        return;
      }
    }
  }
}

void
sysmod::store_dm_rand() {

  device::data_t dataw(8);
  device::strb_t strb(8);
  for (unsigned h=0; h<FLAGS_num_harts; h++) {
    auto dm_rand_values = cvm::registry::messenger.call<whisperClient<uint64_t>::iss_select_rand_RPC>(wc_loc_, h);
    std::vector<uint64_t> stores, loads, pcs;
    for (const auto &iss: dm_rand_values) {
      switch (iss.type) {
        case 0:
          pcs.push_back(iss.phys_addr);
          break;
        case 1:
          loads.push_back(iss.phys_addr);
          break;
        default:
          stores.push_back(iss.phys_addr);
      }
    }
    std::vector<uint64_t> result;
    result.insert(result.end(), pcs.begin(), pcs.end());
    result.insert(result.end(), loads.begin(), loads.end());
    result.insert(result.end(), stores.begin(), stores.end());
    uint64_t addr = FLAGS_dm_rand_addr + (h * 0x40); // offset per hart (maximum 8 addresses)
    cvm::log(cvm::HIGH, "Backdoor writes to DM rand addresses for Hart:{}, starting address:{:x}\n", h, addr);
    for (const auto &val: result) {
      for (size_t i=0; i<8; i++) {
        dataw[i] = (val >> 8*i) & 0xff;
        strb[i]  = true;
      }
      dev("trickbox")->backdoor_write(addr, 8, dataw, strb);
      addr += 8;
    }
  }
}

void
sysmod::tick(uint64_t advance) {

  ticks_ += advance;
  advance = 0;
  if (ticks_ >= FLAGS_sysmod_tick_update_threshold)  {
      auto rem = ticks_ % FLAGS_sysmod_tick_update_threshold;
      advance  = ticks_ - rem;
      ticks_   = rem;
  }
  if (advance)
    for (auto& d : devices_) {
     if (!cosim_init_ && d->tag() == "trickbox")
         continue; // when in cosim mode, do not initialize ticks of trickbox until Whisper client is set.
      d->tick(advance);
    }
}

void
sysmod::is_dut_reset_req(bool dut_reset_req,uint64_t clocks,uint64_t divisor) {

  cvm::log(cvm::FULL,"Value of dut_reset_req in sysmod is : {}\n",dut_reset_req);
  if (dut_reset_req)
    for (auto& d : devices_)
      d->is_dut_reset_req(dut_reset_req,clocks,divisor);
}

void
sysmod::jtag_tick(uint64_t advance) {

  jtag_ticks_ += advance;
  if (advance)
    for (auto& d : devices_)
      d->jtag_tick(advance);
}

void sysmod::tboxtrig_updatemem(uint64_t addr, uint64_t data) {

    device::data_t dataw(8);
    device::strb_t strb(8);
    for (size_t i = 0; i < 8; i++) strb[i] = true;
    for (size_t i = 0; i < 8; i++) dataw[i] = (data >> 8*i) & 0xff;

    dev("trickbox")->backdoor_write(addr, 8, dataw, strb);
}

void sysmod::overlay_tick(uint64_t advance) {

  overlay_ticks_ += advance;
   if (advance)
     for (auto& d : devices_)
       d->overlay_tick(advance);
}

void sysmod::configure_uninit_read_callbacks()
{
  // Check if any uninitialized read flags are enabled
  if (!FLAGS_sysmod_mem_random && FLAGS_sysmod_mem_default_64 == 0)
    return;

  // Validate that only one mode is enabled
  int modes_enabled = 0;
  if (FLAGS_sysmod_mem_random)
    modes_enabled++;
  if (FLAGS_sysmod_mem_default_64 != 0)
    modes_enabled++;

  if (modes_enabled > 1)
  {
    cvm::log(cvm::ERROR, "Error: [sysmod] Only one uninitialized read mode can be enabled at a time\n");
    return;
  }

  // Create the appropriate callback based on flags
  std::function<std::vector<std::uint8_t>(std::uint64_t, std::uint64_t)> callback;

  if (FLAGS_sysmod_mem_random)
  {
    callback = [this](std::uint64_t addr, std::uint64_t size) -> std::vector<std::uint8_t>
    {
      std::vector<std::uint8_t> data(size);
      auto random_byte_dist = cvm::rand::uniform_dist<uint8_t>();
      std::generate(data.begin(), data.end(), [&random_byte_dist]()
                    { return random_byte_dist(); });

      // Poke whisper with generated data if cosim is enabled
      if (FLAGS_cosim)
      {
        bool valid = true;
        if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemBatchRPC>(wc_loc_, 0, 0, 'm', addr, data, valid) || !valid) && FLAGS_whisper_client_check)
        {
          cvm::log(cvm::ERROR, "Error: [sysmod] configure_uninit_read_callbacks failed to batch poke whisper memory for random data at addr {:#x}, size {}\n", addr, size);
        }
      }

      return data;
    };
  }
  else if (FLAGS_sysmod_mem_default_64 != 0)
  {
    callback = [this](std::uint64_t addr, std::uint64_t size) -> std::vector<std::uint8_t>
    {
      std::vector<std::uint8_t> data(size);

      // Fill data with 64-bit aligned pattern
      for (std::uint64_t i = 0; i < size; ++i)
      {
        auto aligned_offset = cvm::bitmanip::slice(addr + i, 2, 0) * 8;
        data[i] = cvm::bitmanip::slice(FLAGS_sysmod_mem_default_64, aligned_offset + 7, aligned_offset);
      }

      // Poke whisper with generated data if cosim is enabled
      if (FLAGS_cosim)
      {
        bool valid = true;
        if ((!cvm::registry::messenger.call<whisperClient<uint64_t>::whisperPokeMemBatchRPC>(wc_loc_, 0, 0, 'm', addr, data, valid) || !valid) && FLAGS_whisper_client_check)
        {
          cvm::log(cvm::ERROR, "Error: [sysmod] configure_uninit_read_callbacks failed to batch poke whisper memory for pattern data at addr {:#x}, size {}\n", addr, size);
        }
      }

      return data;
    };
  }

  // Apply callback to all memory devices
  for (auto &device : devices_)
  {
    auto *mem_device = dynamic_cast<sysmod_mem *>(device.get());
    if (mem_device)
    {
      mem_device->uninitialized_read_data_cb(callback);
      cvm::log(cvm::MEDIUM, "[sysmod] Configured uninitialized read callback for memory device: {}\n", mem_device->tag());
    }
  }
}

extern "C" {
  void sysmod_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();
    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

  uint64_t backdoor_read(uint64_t address) {
    uint64_t out_data;
    std::atomic<bool> flag{false};
    auto loc_ = cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0);
    cvm::registry::messenger.signal_async<sysmod::backdoor_read_t>(loc_, sysmod::backdoor_read_t{address, &flag, &out_data});
    flag.wait(false);
    return out_data;
  }
  void backdoor_write(uint64_t address, uint64_t data) {
    std::atomic<bool> flag{false};
    auto loc_ = cvm::topology::get_from_hierarchy("TOP.PLATFORM.SYSMOD", 0);
    cvm::registry::messenger.signal_async<sysmod::backdoor_write_t>(loc_, sysmod::backdoor_write_t{address, data, 8, &flag});
    flag.wait(false);
    return;
  }
}
