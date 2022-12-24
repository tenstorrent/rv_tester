#include <iostream>
#include <thread>
#include <cassert>
#include <unordered_map>
#include "cvm/plusargs.hpp"
#include "sysmod.h"
#include "mem/mem.h"
#include "clint/clint.h"
#include "htif/htif.h"

// shared flags
DEFINE_string(memmap_json_path, "", "Path to memory map json");

// internal flags
DEFINE_string(hex, "", "hex file (program) to load into memory");
DEFINE_bool(sysmod_terminate, true, "Call $finish on write to tohost");
DEFINE_bool(sysmod_poll, true, "Poll for sysmod callbacks");

extern "C" {
  // used by CLINT to assert/deassert timer interrupt
  void sysmod_timer_interrupt(unsigned hartid, unsigned val);

  // used by CLINT to assert/deassert sw interrupt
  void sysmod_sw_interrupt(unsigned hartid, unsigned val);

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
  for (auto& d : devices_)
    delete d;
  devices_.clear();
}

void
sysmod::compose()
{
  std::lock_guard<std::mutex> lock(sys_m);
  for (auto& d : devices_)
    delete d;
  devices_.clear();

  // Load memmap
  memmap::load(memmap_);

  try {
    // ariane specifies bootrom/debug module regions we don't have here
    devices_.push_back(new mem("scratch", memmap_.at("scratch").at("base"), memmap_.at("scratch").at("size")));
    devices_.push_back(new mem("memory", memmap_.at("memory0").at("base"), memmap_.at("memory0").at("size"))); 
    devices_.push_back(new htif("htif", memmap_.at("htif").at("base")));
    devices_.push_back(new clint("clint", memmap_.at("clint").at("base"), 1));
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
sysmod::load_prog(const std::string& prog)
{
  std::lock_guard<std::mutex> lock(sys_m);
  if (not dynamic_cast<mem&>(dev("memory")).init_hex(prog))
    exit(0);
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
  // std::cout << std::hex << "write req at: " << addr << '\n';
  auto& d = dev(addr);
  device::cbs_t cbs;
  d.write(addr, length, data, strb, cbs);
  handle_callbacks(cbs);
}

void
sysmod::read(uint64_t addr, size_t length, device::data_t& data)
{
  std::lock_guard<std::mutex> lock(sys_m);
  // std::cout << std::hex << "read req at: " << addr << '\n';
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
      case device::Callback::TERMINATE: sysmod_terminate(FLAGS_sysmod_terminate);
                                        break;
      default: assert(false);
    }
  }

  callbacks_.clear();
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
    std::cout << "loading " << FLAGS_hex << "\n";
    s->load_prog(FLAGS_hex);
  }
}
