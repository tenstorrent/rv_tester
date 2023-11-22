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
#include "dm/dm.h"
#include "io_dev/io_dev.h"
#include "null_dev/null_dev.h"
#include "htif/htif.h"
#include "trickbox/trickbox.h"
#include "rv_tester/rv_tester_structs.h"

// internal flags
DEFINE_string(hex, "", "hex file (program) to load into memory");
DEFINE_string(load, "", "elf file (program) to load into memory");
DEFINE_string(bootrom_path, "", "Path to bootrom object file");
DEFINE_string(load_io, "", "load specified io dev with content from memory");
DEFINE_bool(sysmod_tick_async, true, "Asynchronous sysmod_tick calls");
DEFINE_uint64(sysmod_tick_update_threshold, 1, "Slow down tick update frequency by this factor. The tick is still eventually advanced the same cumulative amount, just not as often. Useful for emulation where the clock counts much faster but tests setup interrupts to happen very soon for simulation. They git hit by an interrupt storm and are stuck in the interrupt handler forever.");
DEFINE_uint64(hart_enable_mask, 0x1, "Hart enable mask. Ex: To enable 2 harts in a 8-hart system, use 0x3.");

REGISTRY_register(sysmod, TOP.PLATFORM.SYSMOD, 0);

extern "C" {
  void sysmod_timer_interrupt(unsigned hartid, unsigned val);
  void sysmod_sw_interrupt(unsigned hartid, unsigned val);
  void sysmod_tbox_interrupt(unsigned hartid, unsigned val, unsigned int_val);
  void sysmod_dmi_write(unsigned hartid, unsigned upper_val, unsigned lower_val);
  void sysmod_terminate();
}

sysmod::sysmod(cvm::topology::loc_t loc, unsigned id)
  : scope_(nullptr), loc_(loc), id_(id)
{
  cvm::registry::messenger.connect<svScope>(
      loc_,
      [this](svScope s) { return this->set_scope(s); });

  cvm::registry::messenger.connect<rv_tester_transactions::sysmod::tick<>>(
      loc_,
      [this](const rv_tester_transactions::sysmod::tick<>& t) { return this->tick(t.advance); });

  auto sources = cvm::topology::get_from_type("PLATFORM_TRANSACTOR");
    for (const auto& source : sources) {
        cvm::registry::messenger.connect<transactor::write_t>(
            source,
            [this](const auto& w) {
                // unnecessary but better for catching bugs
                cvm::log(cvm::DEBUG, "new write request at {:#x}", w.addr);
                if (this->dev(w.addr))
                    cvm::registry::messenger.signal<device::write_t>(this->loc_, {w});
            });
        cvm::registry::messenger.connect<transactor::read_t>(
            source,
            [this, source](const auto& r) {
                cvm::log(cvm::DEBUG, "new read request at {:#x}", r.addr);
                if (this->dev(r.addr)){
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
          cvm::log(cvm::FULL, "[SYSMOD] clint::mti = {}\n", t.flag);
          sysmod_timer_interrupt(t.hart, t.flag);
        });
}

void
sysmod::sw_interrupt(clint::sw_t s) {
  cvm::registry::callbacks.push(
      scope(),
      [s]() {
        cvm::log(cvm::FULL, "[SYSMOD] clint::msi = {}\n", s.flag);
        sysmod_sw_interrupt(s.hart, s.flag);
      });
}

void
sysmod::tbox_interrupt(interrupter::interrupt_t i) {
  cvm::registry::callbacks.push(
      scope(),
      [i]() {
        cvm::log(cvm::FULL, "[SYSMOD] trickbox::intr.(sel,val) = {:#x}, {:#x}\n", i.intr_select, i.intr_value);
        sysmod_tbox_interrupt(i.hart, i.intr_select, i.intr_value);
      });
}

void
sysmod::uc_helper_backdoor_write(uc_helper::uc_helper_write_t w) {
    cvm::log(cvm::HIGH,"[SYSMOD] uc_helper_backdoor_write addr {:#x} \n",w.addr);
    cvm::log(cvm::HIGH,"[SYSMOD] uc_helper_backdoor_write len {} \n",(unsigned)w.length);
    cvm::log(cvm::HIGH,"[SYSMOD] uc_helper_backdoor_write data-vec : \n");
     for (auto i: w.data){
         //std::cout << (unsigned )i << ' ';
         cvm::log(cvm::HIGH," {:#x} ",(unsigned)i);
      }

    //cvm::log(cvm::HIGH,"[SYSMOD] uc_helper_backdoor_write strb {} \n",w.strb);
    cvm::log(cvm::FULL, "[SYSMOD] uc_helper_backdoor:write sysmem for addr {:#x}  \n", w.addr);
    transactor::write_t wt;
    wt.addr = w.addr;
    //wt.length = w.length;
    wt.length = 1;
    wt.data = w.data;
    wt.strb = w.strb;
   // dynamic_cast<sysmod_mem&>(*dev("memory")).write(wt);


    cvm::log(cvm::HIGH, "new backdoor prt write request at {:#x}", wt.addr);
                if (this->dev(wt.addr))
                    cvm::registry::messenger.signal<device::write_t>(this->loc_, {wt});
}

void
sysmod::uc_helper_backdoor_read(uc_helper::uc_helper_read_req_t r) {
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
sysmod::terminate(htif::terminate_t) {
  cvm::registry::messenger.signal<rv_tester::terminate_called>(cvm::topology::get_from_type("PLATFORM", 0), rv_tester::terminate_called{});
  cvm::registry::callbacks.push(
      scope(),
      sysmod_terminate
  );
}

void
sysmod::reset() {
  compose();
  load_boot(FLAGS_bootrom_path);
  load_prog(FLAGS_hex, FLAGS_load);
  load_io(FLAGS_load_io);
}

void
sysmod::compose()
{

  devices_.clear();

  // Load memmap
  memmap::get(memmap_);

  auto masters = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST");
  auto platform_loc = cvm::topology::get_from_type("PLATFORM", 0);
  auto nharts = cvm::topology::attr(platform_loc, "NHARTS").second;

  try {
    for(const auto& d : memmap_) {
      const auto base = d.second.base;
      const auto size = d.second.size;
      const auto type = d.second.type;
      const auto tag  = d.second.tag;

      device* device = nullptr;

      if (type == "memory") {
        device = new sysmod_mem(tag, base, size, loc_);
      }
      else if (type == "boot") {
        device = new sysmod_mem(tag, base, size, loc_);
      }
      else if (type == "io_dev") {
        device = new io_dev(tag, base, size, loc_);
      }
      else if (type == "null_dev") {
        device = new null_dev(tag, base, size, loc_);
      }
      else if (type == "htif") {
        device = new htif(tag, base, loc_);
        cvm::registry::messenger.connect<htif::terminate_t>(
            loc_,
            [&](htif::terminate_t t) { return this->terminate(t); });
      }
      else if (type == "dm") {
        // TODO: cvm::ERROR
        assert(masters.size() > 0);
        device = new dm(tag, base, size, loc_, masters[0]);
      }
      else if (type == "clint") {
        device = new clint(tag, base, nharts, loc_);
        cvm::registry::messenger.connect<clint::timer_t>(
            loc_,
            [&](clint::timer_t t) { return this->timer_interrupt(t); });
        cvm::registry::messenger.connect<clint::sw_t>(
            loc_,
            [&](clint::sw_t s) { return this->sw_interrupt(s); });
      }
      else if (type == "trickbox") {
        device = new trickbox(tag, base, nharts, loc_,masters[1]);
        cvm::registry::messenger.connect<interrupter::interrupt_t>(
            loc_,
            [&](interrupter::interrupt_t i) { return this->tbox_interrupt(i); });
        cvm::registry::messenger.connect<debugger::dmi_data_t>(
            loc_,
            [&](debugger::dmi_data_t i) { return this->dmi_write(i); });
        cvm::registry::messenger.connect<uc_helper::uc_helper_write_t>(
            loc_,
            [&](uc_helper::uc_helper_write_t i) { return this->uc_helper_backdoor_write(i); });
        cvm::registry::messenger.connect<uc_helper::uc_helper_read_req_t>(
            loc_,
            [&](uc_helper::uc_helper_read_req_t i) { return this->uc_helper_backdoor_read(i); });
      }
      else
        cvm::log(cvm::ERROR, "Error: unknown type %s", type);

      devices_.emplace_back(device);
    }
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
sysmod::load_prog(const std::string& hex, const std::string& load)
{
  if (load != "") {
    std::cout << "loading " << load << "\n";
    for (const auto& d : memmap_) {
      const auto type = d.second.type;
      const auto tag  = d.second.tag;
      if (type == "memory") {
        if (not dev(tag) or not dynamic_cast<sysmod_mem&>(*dev(tag)).init_elf(load)) {
          cvm::log(cvm::ERROR, "Failed to load program");
          return;
        }
      }
    }
  }
  if (hex != "") {
    std::cout << "loading " << hex << "\n";
    if (not dev("memory") or not dynamic_cast<sysmod_mem&>(*dev("memory")).init_hex(hex)) {
      cvm::log(cvm::ERROR, "No memory defined");
      return;
    }
  }
}

void
sysmod::load_boot(const std::string& boot)
{
  if (boot != "") {
    std::cout << "loading " << boot << "\n";
    if (not dev("boot") or not dynamic_cast<sysmod_mem&>(*dev("boot")).init_elf(boot)) {
      cvm::log(cvm::ERROR, "No boot defined");
      return;
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

extern "C" {

  void sysmod_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();
    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }
}
