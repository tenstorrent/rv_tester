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

// internal flags
DEFINE_string(hex, "", "hex file (program) to load into memory");
DEFINE_string(load, "", "elf file (program) to load into memory");
DEFINE_string(load_io, "", "load specified io dev with content from memory");
DEFINE_bool(sysmod_tick_async, true, "Asynchronous sysmod_tick calls");

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

  cvm::registry::messenger.connect<rv_tester_transactions::sysmod::tick>(
      loc_,
      [this](const rv_tester_transactions::sysmod::tick& t) { return this->tick(t.advance); });

  auto sources = cvm::topology::get_from_type("PLATFORM_TRANSACTOR");

  for (const auto& source : sources) {
    cvm::registry::messenger.connect<transactor::write_t>(
        source,
        [this](transactor::write_t w) {
            return this->write(w.addr, w.length, w.data, w.strb);
        });

    auto* l = +[](cvm::topology::loc_t source, sysmod* sm) -> cvm::messenger::task<void> {
                  auto channel = cvm::registry::messenger.channel<transactor::read_t>(source);

                  while (1) {
                      auto r = co_await cvm::registry::messenger.wait<transactor::read_t>(channel);
                      device::data_t data(r.length, 0);
                      co_await sm->read(r.addr, r.length, data);
                      cvm::registry::messenger.signal(source, transactor::read_response_t{std::move(data)});
                  }

                  co_return;
              };
    cvm::registry::messenger.fork(l, source, this);
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
  cvm::registry::callbacks.push(
      scope(),
      [t]() {
        if (t.terminate) sysmod_terminate();
      });
}

void
sysmod::reset() {
  compose();
  load_prog(FLAGS_hex, FLAGS_load);
  load_io(FLAGS_load_io);
}

void
sysmod::compose()
{
  devices_.clear();

  // Load memmap
  memmap::get(memmap_);


  try {
    for(const auto& d : memmap_) {
      const auto base = d.second.base;
      const auto size = d.second.size;
      const auto type = d.second.type;
      const auto tag  = d.second.tag;

      device* device = nullptr;

      if (type == "memory") {
        device = new sysmod_mem(tag, base, size);
      } else if (type == "io_dev") {
        device = new io_dev(tag, base, size);
      } else if (type == "null_dev") {
        device = new null_dev(tag, base, size);
      } else if (type == "htif") {
        device = new htif(tag, base, loc_);
        cvm::registry::messenger.connect<htif::terminate_t>(
            loc_,
            [&](htif::terminate_t t) { return this->terminate(t); });
      }else if (type == "dm") {
        auto axi_mst_loc = cvm::topology::get_from_type("PLATFORM_TRANSACTOR_MST");
        device = new dm(tag, base, size,loc_,axi_mst_loc[0]);
      }else if (type == "clint") {
        device = new clint(tag, base, 1, loc_);
        cvm::registry::messenger.connect<clint::timer_t>(
            loc_,
            [&](clint::timer_t t) { return this->timer_interrupt(t); });
        cvm::registry::messenger.connect<clint::sw_t>(
            loc_,
            [&](clint::sw_t s) { return this->sw_interrupt(s); });
      } else if (type == "trickbox") {
        device = new trickbox(tag, base, 1, loc_);
        cvm::registry::messenger.connect<interrupter::interrupt_t>(
            loc_,
            [&](interrupter::interrupt_t i) { return this->tbox_interrupt(i); });
        cvm::registry::messenger.connect<debugger::dmi_data_t>(
            loc_,
            [&](debugger::dmi_data_t i) { return this->dmi_write(i); });
      } else
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
  while(ss.good()) {
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
      dev(tag)->write(dev(tag)->addr()+offset, 8, data, strb);
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
sysmod::write(uint64_t addr, size_t length, const device::data_t& data, const device::strb_t& strb)
{
  //std::cout << std::hex << "write req at: " << addr << '\n';
  auto d = dev(addr);

  if (d == nullptr)
    return;

  d->write(addr, length, data, strb);
}

cvm::messenger::task<void>
sysmod::read(uint64_t addr, size_t length, device::data_t& data)
{
  //std::cout << std::hex << "read req at: " << addr << '\n';
  auto d = dev(addr);

  if (d == nullptr)
    co_return;

  co_await d->read(addr, length, data);
  co_return;
}

void
sysmod::tick(uint64_t advance)
{
  for (auto& d : devices_) {
      d->tick(advance);
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
