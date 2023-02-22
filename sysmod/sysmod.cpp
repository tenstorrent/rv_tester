#include <iostream>
#include <thread>
#include <cassert>
#include <unordered_map>
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "sysmod.h"
#include "mem/sysmod_mem.h"
#include "clint/clint.h"
// #include "io_dev/io_dev.h"
// #include "null_dev/null_dev.h"
#include "htif/htif.h"
// #include "trickbox/trickbox.h"

// internal flags
DEFINE_string(hex, "", "hex file (program) to load into memory");
DEFINE_string(load, "", "elf file (program) to load into memory");

REGISTRY_register(sysmod, platform, 0);

extern "C" {
  void sysmod_timer_interrupt(unsigned hartid, unsigned val);
  void sysmod_sw_interrupt(unsigned hartid, unsigned val);
  // used by TRICKBOX to assert/deassert  interrupt
  void sysmod_tbox_interrupt(unsigned hartid, unsigned val, unsigned int_val);
  void sysmod_terminate(uint8_t call_finish);
}

sysmod::sysmod(cvm::topology::loc_t loc, unsigned id)
  : scope_(nullptr), loc_(loc), id_(id)
{
  cvm::registry::messenger.connect<scope_t>(
      loc_,
      [&](scope_t s) { return this->set_scope(s.scope); });

  cvm::registry::messenger.connect<tick_t>(
      loc_,
      [&](tick_t t) { return this->tick(t.advance); });

  cvm::registry::messenger.connect<transactor::write_t>(
      loc_,
      [&](transactor::write_t w) {
        return this->write(w.addr, w.length, w.data, w.strb);
      });

  cvm::registry::messenger.connect<transactor::read_t>(
      loc_,
      [&](transactor::read_t r) {
        return this->read(r.addr, r.length, r.data);
      });

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
                    sysmod_timer_interrupt(t.hart, t.flag);
                  });
}

void
sysmod::sw_interrupt(clint::sw_t s) {
  cvm::registry::callbacks.push(
                  scope(),
                  [s]() {
                    sysmod_sw_interrupt(s.hart, s.flag);
                  });
}

void
sysmod::terminate(htif::terminate_t t) {
  cvm::registry::callbacks.push(
                  scope(),
                  [t]() {
                    sysmod_terminate(t.terminate);
                  });
}

void
sysmod::reset() {
  compose();
  load_prog(FLAGS_hex, FLAGS_load);
}

void
sysmod::compose()
{
  std::lock_guard<std::mutex> lock(sys_m);
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
        device = new sysmod_mem(tag, type, base, size);
      } else if (type == "io_dev") {
        // device = new io_dev(tag, type, base, size);
      } else if (type == "null_dev") {
        // device = new null_dev(tag, type, base, size);
      } else if (type == "htif") {
        device = new htif(tag, type, base, loc_);
        cvm::registry::messenger.connect<htif::terminate_t>(
            loc_,
            [&](htif::terminate_t t) { return this->terminate(t); });
      } else if (type == "clint") {
        device = new clint(tag, type, base, 1, loc_);
        cvm::registry::messenger.connect<clint::timer_t>(
            loc_,
            [&](clint::timer_t t) { return this->timer_interrupt(t); });
        cvm::registry::messenger.connect<clint::sw_t>(
            loc_,
            [&](clint::sw_t s) { return this->sw_interrupt(s); });
      } else if (type == "trickbox") {
        // device = new trickbox(tag, base, 1);
      } else {
        std::cerr << "Error: unknown type " << type << "\n";
        assert(false);
      }

      devices_.emplace_back(device);
    }
  }
  catch (std::exception& e) {
    std::cerr << "Error: Memmap access exception.\n" << "  Message: " << e.what() << "\n";
  }
}

device&
sysmod::dev(uint64_t addr)
{
  for (auto& d : devices_) {
    if (d->has_addr(addr))
      return *d;
  }
  std::cerr << "bus error: address not mapped: " << std::hex << addr << '\n';
  assert(false);
}

device&
sysmod::dev(const std::string& tag)
{
  for (auto& d : devices_) {
    if (d->tag() == tag)
      return *d;
  }
  std::cerr << "bus error: tag not mapped: " << tag << '\n';
  assert(false);
}

void
sysmod::load_prog(const std::string& hex, const std::string& load)
{
  std::lock_guard<std::mutex> lock(sys_m);
  if (load != "") {
    std::cout << "loading " << load << "\n";
    if (not dynamic_cast<sysmod_mem&>(dev("memory")).init_elf(load))
      assert(false);
  }
  if (hex != "") {
    std::cout << "loading " << hex << "\n";
    if (not dynamic_cast<sysmod_mem&>(dev("memory")).init_hex(hex))
      assert(false);
  }
  
  for (auto& d : devices_) {
    if(d->type() == "io_dev"){
      if (FLAGS_load != "") {
        std::cout << "loading " << FLAGS_load << "\n";
        //if (not dynamic_cast<sysmod_mem&>(dev("memory")).init_elf(FLAGS_load))
        if (not dynamic_cast<io_dev&>(*d).init_elf(FLAGS_load))
          exit(1);
      }
    }
  }
}

void
sysmod::write(uint64_t addr, size_t length, const device::data_t& data, const device::strb_t& strb)
{
  std::lock_guard<std::mutex> lock(sys_m);
  //std::cout << std::hex << "write req at: " << addr << '\n';
  auto& d = dev(addr);
  d.write(addr, length, data, strb);
}

void
sysmod::read(uint64_t addr, size_t length, device::data_t& data)
{
  std::lock_guard<std::mutex> lock(sys_m);
  //std::cout << std::hex << "read req at: " << addr << '\n';
  auto& d = dev(addr);
  d.read(addr, length, data);
}

void
sysmod::tick(uint64_t advance)
{
  std::lock_guard<std::mutex> lock(sys_m);
  for (auto& d : devices_) {
      d->tick(advance);
  }
}

extern "C" {

  void sysmod_set_scope(cvm::topology::loc_t loc) {
    typedef sysmod sm;
    svScope scope = svGetScope();
    cvm::registry::messenger.signal<sm::scope_t>(
        loc,
        sm::scope_t{scope});
  }

  void sysmod_tick(cvm::topology::loc_t loc, uint64_t new_clock) {
    typedef sysmod sm;
    cvm::registry::messenger.signal<sm::tick_t>(
        loc,
        sm::tick_t{new_clock});
  }
}
