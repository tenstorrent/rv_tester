#include <iostream>
#include <thread>
#include <cassert>
#include <unordered_map>
#include "cvm/plusargs.hpp"
#include "sysmod.h"
#include "mem/sysmod_mem.h"
#include "clint/clint.h"
#include "io_dev/io_dev.h"
#include "null_dev/null_dev.h"
#include "trickbox/trickbox.h"
#include "htif/htif.h"

// shared flags
DEFINE_string(memmap_json_path, "", "Path to memory map json");

// internal flags
DEFINE_string(hex, "", "hex file (program) to load into memory");
DEFINE_string(load, "", "elf file (program) to load into memory");
DEFINE_bool(sysmod_terminate, true, "Call $finish on write to tohost");
DEFINE_bool(sysmod_poll, true, "Poll for sysmod callbacks");

extern "C" {
  // used by CLINT to assert/deassert timer interrupt
  void sysmod_timer_interrupt(unsigned hartid, unsigned val);

  // used by CLINT to assert/deassert sw interrupt
  void sysmod_sw_interrupt(unsigned hartid, unsigned val);
  
  // used by TRICKBOX to assert/deassert  interrupt
  void sysmod_tbox_interrupt(unsigned hartid, unsigned val, unsigned int_val);

  // used by HTIF to indicate program end
  void sysmod_terminate(std::uint8_t call_finish);
}

sysmod::sysmod()
  : scope_(nullptr)
{

    if(!FLAGS_sysmod_poll) {
        std::thread([&] () {
                while(1) {
                    flush_cbs();
                }
                }).detach();
    }

}

sysmod::~sysmod()
{
}

void
sysmod::compose()
{
  std::lock_guard<std::mutex> lock(sys_m);
  devices_.clear();

  // Load memmap
  memmap::get(memmap_);

  try {
    for(auto& d : memmap_) {
      auto& base = d.second.base;
      auto& size = d.second.size;
      auto& type = d.second.type;
      auto& tag  = d.second.tag;

      device* device = nullptr;

      if (type == "memory") {
        device = new sysmod_mem(tag,type, base, size);
      } else if (type == "io_dev") {
        device = new io_dev(tag, type,base, size);
      } else if (type == "htif") {
        device = new htif(tag,type, base);
      } else if (type == "clint") {
        device = new clint(tag, type,base, 1);
      } else if (type == "trickbox") {
        device = new trickbox(tag,type, base, 1);
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
sysmod::load_prog()
{
  std::lock_guard<std::mutex> lock(sys_m);
  if (FLAGS_load != "") {
    std::cout << "loading " << FLAGS_load << "\n";
    if (not dynamic_cast<sysmod_mem&>(dev("memory")).init_elf(FLAGS_load))
      exit(1);
  }
  if (FLAGS_hex != "") {
    std::cout << "loading " << FLAGS_hex << "\n";
    if (not dynamic_cast<sysmod_mem&>(dev("memory")).init_hex(FLAGS_hex))
      exit(1);
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
sysmod::handle_callbacks(const device::cbs_t& cbs)
{
  for (const auto& c : cbs) {
    if (c.cb != device::Callback::NONE)
      callbacks_.push_back(c);
  }
}

void
sysmod::write(uint64_t addr, size_t length, const device::data_t& data, const device::strb_t& strb)
{
  std::lock_guard<std::mutex> lock(sys_m);
  //std::cout << std::hex << "write req at: " << addr << '\n';
  auto& d = dev(addr);
  device::cbs_t cbs;
  d.write(addr, length, data, strb, cbs);
  handle_callbacks(cbs);
}

void
sysmod::read(uint64_t addr, size_t length, device::data_t& data)
{
  std::lock_guard<std::mutex> lock(sys_m);
  //std::cout << std::hex << "read req at: " << addr << '\n';
  auto& d = dev(addr);
  device::cbs_t cbs;
  d.read(addr, length, data, cbs);
  handle_callbacks(cbs);
}

void
sysmod::tick(uint64_t advance)
{
  std::lock_guard<std::mutex> lock(sys_m);
  for (auto& d : devices_) {
      device::cbs_t cbs;
      d->tick(advance, cbs);
      handle_callbacks(cbs);
  }
}

void
sysmod::reset()
{
  std::lock_guard<std::mutex> lock(sys_m);
  std::cout<<"[SYSMOD]: resetting devices\n";
  for (auto& d : devices_) {
      d->reset();
  }
}
void
sysmod::flush_cbs()
{
  std::lock_guard<std::mutex> lock(sys_m);
  if (callbacks_.size()) {
      svSetScope(scope_);
  }
  for (auto& res : callbacks_) {
    switch (res.cb) {
      case device::Callback::TIMER_INT: sysmod_timer_interrupt(res.hart, res.val);
                                        break;
      case device::Callback::SW_INT:    sysmod_sw_interrupt(res.hart, res.val);
                                        break;
      case device::Callback::TRICKBOX_INTR:    sysmod_tbox_interrupt(res.hart, res.intr_select, res.intr_value);
                                        break;
      case device::Callback::TERMINATE: sysmod_terminate(FLAGS_sysmod_terminate);
                                        break;
      default: assert(false);
    }
  }

  callbacks_.clear();
}

void sysmod::add_callback(const device::cb_t& cb) {
  std::lock_guard<std::mutex> lock(sys_m);
  device::cbs_t cbs;
  cbs.push_back(cb);
  handle_callbacks(cbs);
}

extern "C" {

  void sysmod_set_scope(sysmod* s) {
    svScope scope = svGetScope();
    s->set_scope(scope);
  }

  void sysmod_tick(sysmod* s, uint64_t new_clock) {
    s->tick(new_clock);
  }

  void sysmod_flush_cbs(sysmod* s) {
    s->flush_cbs();
  }

  sysmod* sysmod_get(int num) {
      static std::unordered_map<int, sysmod> sysmods;
      auto it = sysmods.find(num);

      if (it == sysmods.end()) {
          it = sysmods.emplace(
                  std::piecewise_construct,
                  std::make_tuple(num),
                  std::make_tuple()
                  ).first;
      }
      return &(it->second);
  }

  void sysmod_reset(sysmod* s) {
    // possibly compose once?
    s->compose();
    s->load_prog();
    s->reset();
  }
}
