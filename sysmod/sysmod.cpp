#include <iostream>
#include <thread>
#include <unordered_map>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "sysmod.h"
#include "mem/sysmod_mem.h"
#include "clint/clint.h"
#include "aclint/aclint.h"
#include "dm/dm.h"
#include "trace_cfg/trace_cfg.h"
#include "smc_xtor/smc_xtor.h"
#include "pll_xtor/pll_xtor.h"
#include "pm_nw_xtor/pm_nw_xtor.h"
#include "aplic_mmr/aplic_mmr.h"
#include "io_dev/io_dev.h"
#include "null_dev/null_dev.h"
#include "heartbeat/heartbeat.h"
#include "htif/htif.h"
#include "uart8250/uart8250.h"
#include "trickbox/trickbox.h"
#include "rv_tester/rv_tester_structs.h"
#include "rv_tester/rv_tester_plusargs.h"
#include "cosim/bridge_if/bridge_params.h"
#include "cosim/dut_if/rvfi/rvfi_plusargs.h"

// internal flags
DEFINE_string(hex, "", "hex file (program) to load into memory");
DEFINE_string(load, "", "elf file (program) to load into memory");
DEFINE_string(load_lz4, "", "lz4 compressed file (program) to load into memory");
DEFINE_bool(bootrom, true, "Load bootrom before test");
DEFINE_string(bootrom_path, "", "Path to bootrom object file");
DEFINE_string(cplfw_path, "", "Path to cpl firmware object file");
DEFINE_string(load_io, "", "load specified io dev with content from memory");
DEFINE_bool(sysmod_tick_async, true, "Asynchronous sysmod_tick calls");
DEFINE_uint64(sysmod_tick_update_threshold, 1, "Slow down tick update frequency by this factor. The tick is still eventually advanced the same cumulative amount, just not as often. Useful for emulation where the clock counts much faster but tests setup interrupts to happen very soon for simulation. They git hit by an interrupt storm and are stuck in the interrupt handler forever.");
DEFINE_uint64(hart_enable_mask, 0x1, "Hart enable mask. Ex: To enable 2 harts in a 8-hart system, use 0x3.");
DEFINE_string(set_csr, "", "+set_csr=<csr_num>:<value>,<num2>:<val2> ");
DEFINE_int32(seed, 1, "Simulation seed passed down for randomization");
REGISTRY_register(sysmod, TOP.PLATFORM.SYSMOD, 0);

extern "C" {
  void sysmod_timer_interrupt(unsigned hartid, unsigned val);
  void sysmod_sw_interrupt(unsigned hartid, unsigned val);
  void sysmod_tbox_interrupt(unsigned hartid, unsigned val, unsigned int_val);
  void sysmod_trace_info(unsigned trace_info_s);
  void sysmod_aplic_dir_interrupt(unsigned long* i) ;
  void sysmod_aplic_rnd_interrupt(unsigned hartid, unsigned val, unsigned int_val);
  void sysmod_dmi_write(unsigned hartid, unsigned upper_val, unsigned lower_val);
  void sysmod_jtag_req(unsigned cmd,unsigned long upper_val, unsigned long lower_val, unsigned length, unsigned quit);
  void sysmod_terminate();
}

sysmod::sysmod(cvm::topology::loc_t loc, unsigned id)
  : scope_(nullptr), loc_(loc), id_(id)
{
  cvm::registry::messenger.connect<svScope>(
      loc_,
      [this](svScope s) { return this->set_scope(s); });
  cvm::registry::messenger.connect<uint64_t>(
      loc_,
      [this](const uint64_t& t) { return this->load_csr_boot(t); });
  cvm::registry::messenger.connect<rv_tester_transactions::sysmod::tick<>>(
      loc_,
      [this](const rv_tester_transactions::sysmod::tick<>& t) { return this->tick(t.advance); });
  cvm::registry::messenger.connect<rv_tester_transactions::sysmod::jtag_tick<>>(
      loc_,
      [this](const rv_tester_transactions::sysmod::jtag_tick<>& t) { return this->jtag_tick(t.advance); });
  cvm::registry::messenger.connect<rv_tester_transactions::sysmod::jtag_rdata<>>(
      loc_,
      [this](const rv_tester_transactions::sysmod::jtag_rdata<>& t) { return this->jtag_resp(t.rdata); });

  auto sources = cvm::topology::get_from_type("PLATFORM_TRANSACTOR");
    for (const auto& source : sources) {
        cvm::registry::messenger.connect<transactor::write_t>(
            source,
            [this](const auto& w) {
                // unnecessary but better for catching bugs
                cvm::log(cvm::DEBUG, "new write request at {:#x}\n", w.addr);
                if (this->dev(w.addr))
                    cvm::registry::messenger.signal<device::write_t>(this->loc_, {w});
            });
        cvm::registry::messenger.connect<transactor::read_t>(
            source,
            [this, source](const auto& r) {
                cvm::log(cvm::DEBUG, "new read request at {:#x}\n", r.addr);
                if (this->dev(r.addr)){
                    cvm::log(cvm::FULL, "[sysmod] read: src={} id={}, addr={:#x}, len={}\n", source, r.id, r.addr, r.length);
                    cvm::registry::messenger.signal<device::read_t>(this->loc_, {r, source});

		    }

            });
  }

  reset();
}

sysmod::~sysmod()
{
}

// forwarding functions for devices
void
sysmod::timer_interrupt(clint::timer_t t) {
      cvm::registry::callbacks.push(
        scope(),
        [t]() {
          cvm::log(cvm::FULL, "[SYSMOD] timer_interrupt [hart={}, mti={}]\n", t.hart, t.flag);
          sysmod_timer_interrupt(t.hart, t.flag);
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

void
sysmod::trace_info_handler(trace_cfg::trace_info_t i) {
  cvm::registry::callbacks.push(
      scope(),
      [i]() {
        cvm::log(cvm::HIGH, "[SYSMOD] trace_info \n");
        sysmod_trace_info(i.trace_quiesced);
      });
}

void
sysmod::smc_info_handler(smc_xtor::smc_info_t i) {
        cvm::log(cvm::HIGH, "[SYSMOD] trace_info {} \n",i.smc_quiesced);
 // cvm::registry::callbacks.push(
 //     scope(),
 //     [i]() {
 //       cvm::log(cvm::HIGH, "[SYSMOD] smc_info \n");
 //       sysmod_trace_info(i.trace_quiesced);
 //     });
}

void
sysmod::pll_info_handler(pll_xtor::pll_info_t i) {
        cvm::log(cvm::HIGH, "[SYSMOD] trace_info {} \n",i.pll_quiesced);
 // cvm::registry::callbacks.push(
 //     scope(),
 //     [i]() {
 //       cvm::log(cvm::HIGH, "[SYSMOD] smc_info \n");
 //       sysmod_trace_info(i.trace_quiesced);
 //     });
}

void
sysmod::pm_nw_info_handler(pm_nw_xtor::pm_nw_info_t i) {
        cvm::log(cvm::HIGH, "[SYSMOD] trace_info {} \n",i.pm_nw_quiesced);
 // cvm::registry::callbacks.push(
 //     scope(),
 //     [i]() {
 //       cvm::log(cvm::HIGH, "[SYSMOD] smc_info \n");
 //       sysmod_trace_info(i.trace_quiesced);
 //     });
}
void
sysmod::aplic_interrupt(aplic_driver::aplic_driver_write_t i) {
  cvm::registry::callbacks.push(
      scope(),
      [i]() {
        unsigned long arr[16];
        for (int j = 0; j < 16; j++) {
        arr[j] = i.aplic_pin_values_vec[j];
        }
        sysmod_aplic_dir_interrupt(arr);
      });
}

void
sysmod::trace_cfg_read_req_router(trace_cfg::trace_cfg_read_t r) {

    transactor::read_t rd;
    rd.addr = r.addr;
    rd.length = r.length;
    rd.id =  r.id;

    auto sources = cvm::topology::get_from_type("PLATFORM_TRANSACTOR");

    if (this->dev(r.addr)){
        cvm::registry::messenger.signal<device::read_t>(this->loc_, {rd, sources[0]});
    }

}

void
sysmod::smc_read_req_router(smc_xtor::smc_xtor_read_t r) {

    transactor::read_t rd;
    rd.addr = r.addr;
    rd.length = r.length;
    rd.id =  r.id;

    auto sources = cvm::topology::get_from_type("PLATFORM_TRANSACTOR");

    if (this->dev(r.addr)){
        cvm::registry::messenger.signal<device::read_t>(this->loc_, {rd, sources[0]});
    }

}

void
sysmod::scratchpad_xtor_read_req_router(scratchpad_xtor::scratchpad_xtor_read_t r) {

    transactor::read_t rd; 
    rd.addr = r.addr;
    rd.length = r.length;
    rd.id =  r.id;
    cvm::log(cvm::FULL, "[SYSMOD] SCRATCHPAD_XTOR ROUTER - addr={:#x} \n", rd.addr);

    auto sources = cvm::topology::get_from_type("PLATFORM_TRANSACTOR");

    if (this->dev(r.addr)){
    cvm::log(cvm::FULL, "[SYSMOD] SCRATCHPAD_XTOR ROUTER  send to device - addr={:#x} \n", rd.addr);
        cvm::registry::messenger.signal<device::read_t>(this->loc_, {rd, sources[0]});
    }

}
void
sysmod::uc_helper_backdoor_write(uc_helper::uc_helper_write_t w) {

    if(!FLAGS_bypass_cache && !FLAGS_bypass_mem){
        cvm::log(cvm::ERROR, "Error: [SYSMOD] uc_helper_backdoor_write: caching is enabled in rv_tester and it does not receive DMAs, so the test could fail if the CPU does a read to this address as it will receive the stale cached data");
    }

    cvm::log(cvm::FULL,"[SYSMOD] uc_helper_backdoor_write addr {:#x} \n",w.addr);
    cvm::log(cvm::FULL,"[SYSMOD] uc_helper_backdoor_write len {} \n",(unsigned)w.length);
    cvm::log(cvm::FULL,"[SYSMOD] uc_helper_backdoor_write data-vec : \n");
     for (auto i: w.data){
         cvm::log(cvm::HIGH," {:#x} ",(unsigned)i);
      }

    cvm::log(cvm::FULL, "[SYSMOD] uc_helper_backdoor:write sysmem for addr {:#x}  \n", w.addr);
    transactor::write_t wt;
    wt.addr = w.addr;
    //wt.length = w.length;
    wt.length = 1;
    wt.data = w.data;
    wt.strb = w.strb;
   // dynamic_cast<sysmod_mem&>(*dev("memory")).write(wt);


    cvm::log(cvm::FULL, "[UC_HELPER] new backdoor write request at {:#x}", wt.addr);
                if (this->dev(wt.addr))
                    cvm::registry::messenger.signal<device::write_t>(this->loc_, {wt});

}

void
sysmod::uc_helper_backdoor_read(uc_helper::uc_helper_read_req_t r) {
    if(!FLAGS_bypass_cache && !FLAGS_bypass_mem){
        cvm::log(cvm::ERROR, "Error: [SYSMOD] uc_helper_backdoor_read: caching is enabled in rv_tester and it does not receive DMAs, so this backdoor read might receive stale data");
    }

    cvm::log(cvm::HIGH,"[SYSMOD] uc_helper_backdoor_read addr {:#x} \n",r.addr);
    cvm::log(cvm::HIGH,"[SYSMOD] uc_helper_backdoor_read len {} \n",(unsigned)r.length);
    cvm::log(cvm::HIGH, "new PRT BACKDOOR read request at {:#x}", r.addr);
    device::data_t data(8);
    std::vector<uint8_t> data_trickbox(8);
    std::vector<bool> strb(8);
    for (size_t i = 0; i < 8; i++) strb[i] = true;
      // Read from memory and write to requested dev tag
      if (not dev("memory"))
        return;
      dev("memory")->backdoor_read(r.addr, r.length, data);
      for (size_t i = 0; i < 8; i++) {
        data_trickbox[i] = (uint8_t)data[i];
      };

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
sysmod::jtag_req(jtag_driver::jtag_data_t i) {
  cvm::registry::callbacks.push(
      scope(),
      [i]() {
        cvm::log(cvm::FULL, "[SYSMOD] trickbox jtag_driver::dmi.(upper,lower) = {:#x}, {:#x}\n",i.upper_jtag_data, i.lower_jtag_data, i.jtag_length_data);
        sysmod_jtag_req(i.jtag_cmd, i.upper_jtag_data, i.lower_jtag_data,i.jtag_length_data,i.jtag_quit);
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
  if (t.low_priority_based) {
      cvm::registry::messenger.signal<rv_tester::terminate_called>(cvm::topology::get_from_type("PLATFORM", 0), rv_tester::terminate_called{});
  } else {
      cvm::registry::messenger.signal_async<rv_tester::terminate_called>(cvm::topology::get_from_type("PLATFORM", 0), rv_tester::terminate_called{}, prio);
  }
  cvm::registry::callbacks.push(
      scope(),
      sysmod_terminate
  );
}

void
sysmod::reset() {
  compose();
  load_prog(FLAGS_hex, FLAGS_load, FLAGS_load_lz4);
  load_io(FLAGS_load_io);
  load_boot(FLAGS_bootrom_path);
  if (!FLAGS_cosim)
    load_csr_boot(0);
  load_cplfw(FLAGS_cplfw_path);
}

void
sysmod::compose()
{

  devices_.clear();

  // Load memmap
  memmap::get(memmap_);

  auto mmr_master = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MMR_MST");
  auto smc_master = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_SMC_MST");
  auto pll_master = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_PLL_MST");
  auto pm_nw_master = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_PM_NW_MST");
  auto masters = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST");
  auto platform_loc = cvm::topology::get_from_type("PLATFORM", 0);
  auto nharts = cvm::topology::attr(platform_loc, "NHARTS").second;

  std::shared_ptr<mem_manager> mm = std::make_shared<mem_manager>();

  try {
    for(const auto& d : memmap_) {
      const auto base = d.second.base;
      const auto size = d.second.size;
      const auto type = d.second.type;
      const auto tag  = d.second.tag;

      std::unique_ptr<device> device;

      if (type == "memory") {
        device = std::make_unique<sysmod_mem>(tag, base, size, loc_, mm);
      }
      else if (type == "io_dev") {
        device = std::make_unique<io_dev>(tag, base, size, loc_);
      }
      else if (type == "null_dev") {
        device = std::make_unique<null_dev>(tag, base, size, loc_);
      }
      else if (type == "htif") {
        device = std::make_unique<htif>(tag, base, loc_);
        cvm::registry::messenger.connect<htif::terminate_t>(
            loc_,
            [&](htif::terminate_t t) { return this->terminate(t); });
      }
      else if (type == "uart8250") {
        device = std::make_unique<uart8250>(tag, base, loc_);
      }
      else if (type == "dm") {
        // TODO: cvm::ERROR
       // assert(masters.size() > 0);
       // device = std::make_unique<dm>(tag, base, size, loc_, masters[0]);
      }
      else if (type == "trace_cfg") {
        // TODO: cvm::ERROR
        cvm::registry::messenger.connect<trace_cfg::trace_info_t>(
            loc_,
            [&](trace_cfg::trace_info_t i) { return this->trace_info_handler(i); });
        assert(masters.size() > 0);
        device = std::make_unique<trace_cfg>(tag, base, size, loc_, masters[0]);
      }
      else if (type == "pll_xtor") {
        // TODO: cvm::ERROR
        cvm::registry::messenger.connect<pll_xtor::pll_info_t>(
            loc_,
            [&](pll_xtor::pll_info_t i) { return this->pll_info_handler(i); });
        assert(masters.size() > 0);
        device = std::make_unique<pll_xtor>(tag, base, size, loc_, pll_master[0]);
      }
      else if (type == "pm_nw_xtor") {
        // TODO: cvm::ERROR
        cvm::registry::messenger.connect<pm_nw_xtor::pm_nw_info_t>(
            loc_,
            [&](pm_nw_xtor::pm_nw_info_t i) { return this->pm_nw_info_handler(i); });
        assert(masters.size() > 0);
        device = std::make_unique<pm_nw_xtor>(tag, base, size, loc_, pm_nw_master[0]);
      }
      else if (type == "scratchpad_xtor") {
        // TODO: cvm::ERROR
        assert(masters.size() > 0);
        device = std::make_unique<scratchpad_xtor>(tag, base, size, loc_, masters[0]);
        cvm::registry::messenger.connect<scratchpad_xtor::scratchpad_xtor_read_t>(
            loc_,
            [&](scratchpad_xtor::scratchpad_xtor_read_t i) { return this->scratchpad_xtor_read_req_router(i); });
      }
      else if (type == "smc_xtor") {
        // TODO: cvm::ERROR
        cvm::registry::messenger.connect<smc_xtor::smc_info_t>(
            loc_,
            [&](smc_xtor::smc_info_t i) { return this->smc_info_handler(i); });
        assert(masters.size() > 0);
        device = std::make_unique<smc_xtor>(tag, base, size, loc_, smc_master[0]);
        cvm::registry::messenger.connect<smc_xtor::smc_xtor_read_t>(
            loc_,
            [&](smc_xtor::smc_xtor_read_t i) { return this->smc_read_req_router(i); });
      }
      else if (type == "aplic_mmr") {
        // TODO: cvm::ERROR
        assert(mmr_master.size() > 0);
        device = std::make_unique<aplic_mmr>(tag, base, size, loc_, mmr_master[0]);
      }
      else if (type == "clint") {
        device = std::make_unique<clint>(tag, base, nharts, loc_);
        cvm::registry::messenger.connect<clint::timer_t>(
            loc_,
            [&](clint::timer_t t) { return this->timer_interrupt(t); });
        cvm::registry::messenger.connect<clint::sw_t>(
            loc_,
            [&](clint::sw_t s) { return this->sw_interrupt(s); });
      }
      else if (type == "aclint") {
        device = std::make_unique<aclint>(tag, base, nharts, loc_);
        cvm::registry::messenger.connect<clint::timer_t>(
            loc_,
            [&](clint::timer_t t) { return this->timer_interrupt(t); });
      }
      else if (type == "trickbox") {
        device = std::make_unique<trickbox>(tag, base, nharts, loc_,masters[0]);
        cvm::registry::messenger.connect<interrupter::interrupt_t>(
            loc_,
            [&](interrupter::interrupt_t i) { return this->tbox_interrupt(i); });
	cvm::registry::messenger.connect<aplic_driver::aplic_driver_write_t>(
            loc_,
            [&](aplic_driver::aplic_driver_write_t i) { return this->aplic_interrupt(i); });
        cvm::registry::messenger.connect<debugger::dmi_data_t>(
            loc_,
            [&](debugger::dmi_data_t i) { return this->dmi_write(i); });
        cvm::registry::messenger.connect<jtag_driver::jtag_data_t>(
            loc_,
            [&](jtag_driver::jtag_data_t i) { return this->jtag_req(i); });
        cvm::registry::messenger.connect<uc_helper::uc_helper_write_t>(
            loc_,
            [&](uc_helper::uc_helper_write_t i) { return this->uc_helper_backdoor_write(i); });
        cvm::registry::messenger.connect<uc_helper::uc_helper_read_req_t>(
            loc_,
            [&](uc_helper::uc_helper_read_req_t i) { return this->uc_helper_backdoor_read(i); });
        cvm::registry::messenger.connect<trace_cfg::trace_cfg_read_t>(
            loc_,
            [&](trace_cfg::trace_cfg_read_t i) { return this->trace_cfg_read_req_router(i); });
      }
      else
        cvm::log(cvm::ERROR, "Error: unknown type %s", type);

      devices_.emplace_back(std::move(device));
    }

    devices_.emplace_back(std::make_unique<heartbeat>("heartbeat", 0, 0, loc_));
  }
  catch (std::exception& e) {
    std::cerr << "Error: Memmap access exception.\n" << "  Message: " << e.what() << "\n";
  }
}

device*
sysmod::dev(uint64_t addr)
{
  for (auto& d : devices_) {
    if (d->has_addr(addr))
      return d.get();
  }
  cvm::log(cvm::ERROR, "Error: Address not mapped: {:#x}\n", addr);
  return nullptr;
}

device*
sysmod::dev(const std::string& tag)
{
  for (auto& d : devices_) {
    if (d->tag() == tag)
      return d.get();
  }
  cvm::log(cvm::ERROR, "Error: Tag not mapped: {}\n", tag);
  return nullptr;
}

void
sysmod::load_io(const std::string& io)
{
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

void
sysmod::load_prog(const std::string& hex, const std::string& load, const std::string& lz4)
{
  for (const auto& d : memmap_) {
    const auto type = d.second.type;
    const auto tag  = d.second.tag;

    if (type != "memory") continue;

    if (load != "") {
      cvm::log(cvm::MEDIUM, "Loading {}\n", load);
      if (not dev(tag) or not dynamic_cast<sysmod_mem&>(*dev(tag)).init_elf(load)) {
        cvm::log(cvm::ERROR, "Failed to load program");
        return;
      }
      cvm::log(cvm::MEDIUM, "Loading {} complete\n", load);
    }

    if (hex != "") {
      cvm::log(cvm::MEDIUM, "Loading {}\n", hex);
      if (not dev(tag) or not dynamic_cast<sysmod_mem&>(*dev(tag)).init_hex(hex)) {
        cvm::log(cvm::ERROR, "No memory defined");
        return;
      }
      cvm::log(cvm::MEDIUM, "Loading {} complete\n", hex);
    }

    if (lz4 != "") {
      cvm::log(cvm::MEDIUM, "Loading {}\n", lz4);
      if (not dev("memory") or not dynamic_cast<sysmod_mem&>(*dev("memory")).init_lz4(lz4)) {
        cvm::log(cvm::ERROR, "No memory defined");
        return;
      }
      cvm::log(cvm::MEDIUM, "Loading {} complete\n", lz4);
    }

    // all memories share the same backing mem manaager
    return;
  }

  cvm::log(cvm::ERROR, "No memory found");
}

void
sysmod::load_boot(const std::string& boot)
{
  if (FLAGS_bootrom && boot != "") {
    cvm::log(cvm::MEDIUM, "Loading {}\n", boot);
    if (boot.substr(boot.length() - 3) == "elf") {
      if (not dev("boot") or not dynamic_cast<sysmod_mem&>(*dev("boot")).init_elf(boot)) {
        cvm::log(cvm::ERROR, "No boot defined");
        return;
      }
    }
    if (boot.substr(boot.length() - 3) == "hex") {
      if (not dev("boot") or not dynamic_cast<sysmod_mem&>(*dev("boot")).init_hex(boot)) {
        cvm::log(cvm::ERROR, "No boot defined");
        return;
      }
    }
    // Write hart_enable_mask for bootrom to access
    device::data_t data(8);
    for (size_t i = 0; i < 8; i++) data[i] = FLAGS_hart_enable_mask >> 8*i;
    device::strb_t strb(8);
    for (size_t i = 0; i < 8; i++) strb[i] = true;
    dev("boot")->backdoor_write(dev("boot")->addr() + 0x9000, 8, data, strb);
  }
}
void
sysmod::load_csr_boot(uint64_t dummy)
{
  cvm::log(cvm::HIGH, "{}", dummy);
  auto split_string = [] (const std::string& input, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    for (char c : input) {
        if (c != delimiter)
            token += c;
        else {
            tokens.push_back(token);
            token.clear();
        }
    }
    tokens.push_back(token);
    return tokens;
  };

  if (FLAGS_bootrom && FLAGS_set_csr!="") {

    std::map<uint64_t,    uint64_t> csr_data;
    std::map<std::string, uint64_t> csr_map;
    for (const auto& csr : csrs) {
        csr_map[csr.name] = csr.address;
    }
    try { // parse the +set_csr and report any errors
      char delimiter = ',';
      std::vector<std::string> csr_num_val = split_string(FLAGS_set_csr, delimiter);
      for (const auto& entry : csr_num_val) {
        delimiter = ':';
        std::vector<std::string> num_val = split_string(entry, delimiter);
        auto csr = num_val.at(0); // expect both csr address("0x301") as well as string("misa")
        auto value = std::stoull(num_val.at(1), nullptr, 0);
        if (csr_map.count(csr))
          csr_data[csr_map[csr]] = value;
        else {
          char* p;
          uint64_t csrn = std::strtoul(csr.c_str(), &p, 0);
          if (*p == 0)
            csr_data[csrn] = value;
          else {
            cvm::log(cvm::ERROR, "Error: csr_name:{} undefined see +set_csr switch\n", csr);
            return;
          }
        }
      }
    }
    catch (...) {
      cvm::log(cvm::ERROR, "Error: unable to parse +set_csr={}\n", FLAGS_set_csr);
      return;
    }
    int addr = dev("boot")->addr() + 0x8000;
    int temp_gpr = 4, temp_gpr2 = 3, temp_gpr3 = 28;
    int csr_opcode = 0x73, lui_opcode = 0x37, op_imm_opcode = 0x13, or_opcode = 0x33;

    auto add_to_mem = [&addr,this] (const uint32_t op) { //simple lambda to poke to memory
      device::strb_t strb(4);
      for (size_t i = 0; i<4; i++) strb[i] = true;
      cvm::log(cvm::HIGH, "Address: {:#x} OPCODE: {:#x}\n",addr, op);
      bool valid = true;
      device::data_t data(4);
      for (size_t i=0; i<4; i++) data[i] = op >> 8*i;
      dev("boot")->backdoor_write(addr, 4, data, strb);
      if (client_ != nullptr && !client_->whisperPoke(0, 0, 'm', addr, op, valid))
        cvm::log(cvm::ERROR, "Error: Failed to poke whisper memory\n");
      addr += 4;
    };

    cvm::log(cvm::HIGH, "Backdoor bootrom CSR writes to memory\n");
    for (auto const& [csr_num, value] : csr_data) {
      uint32_t csr_op = 0;
      if (value < 32)
        csr_op = csr_opcode + (0/*x0*/<<7) + (5<<12) + (value<<15) + (csr_num<<20); // csrwi csrnum, x4 (csrwi can accomodate [4:0] immediate otherwise, use GPR)
      else {
        uint32_t lui_op = lui_opcode + (temp_gpr<<7) + (((value & 0xfffff000)>>12)<<12);
        add_to_mem(lui_op);

        if (!(value & 0x800)) {
          uint32_t ori_op = op_imm_opcode + (temp_gpr<<7) + (0b110<<12) + (temp_gpr<<15) + ((value & 0xfff)<<20);
          add_to_mem(ori_op);
        } else {
            lui_op = lui_opcode + (temp_gpr3<<7) + (0x1000);
            add_to_mem(lui_op);
            uint32_t addw_op = (0x1b /*addw*/) + (temp_gpr3<<7) + (temp_gpr3<<15) + (((value & 0xfff) - 0x1000)<<20);
            add_to_mem(addw_op);
            uint32_t or_op = or_opcode + (temp_gpr<<7/*rd*/) + (0b110<<12) + (temp_gpr3<<15) + (temp_gpr<<20);
            add_to_mem(or_op);
        }

        if (value & 0x80000000) { // data gets sign extended, shl and shr to correct it
          uint32_t slli_op = op_imm_opcode + (temp_gpr<<7) + (0b001<<12) + (temp_gpr<<15) + (32<<20); // slli x4, x4, 32
          add_to_mem(slli_op);
          uint32_t srli_op = op_imm_opcode + (temp_gpr<<7) + (0b101<<12) + (temp_gpr<<15) + (32<<20); // srli x4, x4, 32
          add_to_mem(srli_op);
        }
        if (value > uint64_t(0xffffffff)) {
          // data is greater than 32-bits (another opcode, another temporary register)
          uint32_t lui_op = lui_opcode + (temp_gpr2<<7) + ((((value>>32) & 0xfffff000) >> 12)<<12);
          add_to_mem(lui_op);

          if (!((value>>32) & 0x800)) {
            uint32_t ori_op = 0x13 + (temp_gpr2<<7) + (0b110<<12) /*funct3*/ + (temp_gpr2<<15) + (((value>>32) & 0xfff)<<20);
            add_to_mem(ori_op);
          } else {
            lui_op = lui_opcode + (temp_gpr3<<7) + (0x1000);
            add_to_mem(lui_op);
            uint32_t addw_op = (0x1b /*addw*/) + (temp_gpr3<<7) + (temp_gpr3<<15) + ((((value>>32) & 0xfff) - 0x1000)<<20);
            add_to_mem(addw_op);
            uint32_t or_op = or_opcode + (temp_gpr2<<7) + (0b110<<12) + (temp_gpr3<<15) + (temp_gpr2<<20);
            add_to_mem(or_op);
          }

          uint32_t slli_op = 0x13 + (temp_gpr2<<7) + (0b001<<12) + (temp_gpr2<<15) + (32<<20);
          add_to_mem(slli_op);

          uint32_t or_op = or_opcode + (temp_gpr<<7) + (0b110<<12) + (temp_gpr2<<15) + (temp_gpr<<20);
          add_to_mem(or_op);
        }
        csr_op = csr_opcode + (0/*x0*/<<7) + (1<<12) + (temp_gpr<<15) + (csr_num<<20); // csrw csrnum, x4
      }
      add_to_mem(csr_op);
    }
    add_to_mem(0x8067/*ret*/);
  }
}

void
sysmod::load_cplfw(const std::string& cplfw)
{
  if (cplfw != "") {
    cvm::log(cvm::MEDIUM, "Loading {}\n", cplfw);
    if (cplfw.substr(cplfw.length() - 3) == "elf") {
      if (not dev("memory") or not dynamic_cast<sysmod_mem&>(*dev("memory")).init_elf(cplfw)) {
        cvm::log(cvm::ERROR, "No cpl firmware defined");
        return;
      }
    }
    if (cplfw.substr(cplfw.length() - 3) == "hex") {
      if (not dev("memory") or not dynamic_cast<sysmod_mem&>(*dev("memory")).init_hex(cplfw)) {
        cvm::log(cvm::ERROR, "No cpl firmware defined");
        return;
      }
    }
  }
}

void sysmod::jtag_resp(std::bitset<70> rdata){
  auto tbox_loc = cvm::topology::get_from_type("TRICKBOX", 0);
  std::vector<uint64_t> convertedArray = bitsetToUint64Array(rdata);
  cvm::log(cvm::FULL, "[SYSMOD.CPP] In JTAG RESP converted array size = {}\n", convertedArray.size());
  
  for (uint64_t num : convertedArray) {
        cvm::log(cvm::FULL, "[SYSMOD.CPP] In JTAG RESP converted array element = {}\n", num);
  }
  
  cvm::registry::messenger.signal(tbox_loc, jtag_driver::jtag_req_t{0, 0,0,convertedArray[0],0});

}
void
sysmod::tick(uint64_t advance)
{

  ticks_ += advance;

  advance = 0;
  if (ticks_ >= FLAGS_sysmod_tick_update_threshold)  {
      auto rem = ticks_ % FLAGS_sysmod_tick_update_threshold;
      advance  = ticks_ - rem;
      ticks_   = rem;
  }

  if (advance) {
      for (auto& d : devices_) {
          d->tick(advance);
      }
  }
}

void
sysmod::jtag_tick(uint64_t advance)
{

  jtag_ticks_ += advance;

   if (advance) {
       for (auto& d : devices_) {
           d->jtag_tick(advance);
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
}
