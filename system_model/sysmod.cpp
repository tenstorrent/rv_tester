#include <iostream>
#include <thread>
#include <cassert>
#include "sysmod.h"
#include "mem/mem.h"
#include "clint/clint.h"
#include "htif/htif.h"

extern "C" {
  // used by CLINT to assert/deassert timer interrupt
  void sysmod_timer_interrupt(unsigned hartid, unsigned val);

  // used by CLINT to assert/deassert sw interrupt
  void sysmod_sw_interrupt(unsigned hartid, unsigned val);

  // used by HTIF to indicate program end
  void sysmod_terminate();
}

sysmod::sysmod(svScope& scope, const std::string& memmap)
  : scope_(scope)
{
  compose(memmap);
}

sysmod::~sysmod()
{
  for (auto& d : devices_)
    if (d) {
      delete d;
      d = nullptr;
    }
}

void
sysmod::compose(const std::string& memmap)
{
  // ariane specifies bootrom/debug module regions we don't have here
  devices_.push_back(new mem("scratch", 0x8000000, 0x100000));
  // full length in ariane cfg is 0x40000000
  devices_.push_back(new mem("memory", 0x80000000, 0xc000000));
  devices_.push_back(new clint("clint", 0x200000, 1));
  devices_.push_back(new htif("htif", 0x70000000));
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
sysmod::tick(uint64_t new_clock)
{
  std::lock_guard<std::mutex> lock(sys_m);
  for (auto& d : devices_) {
      device::cbs_t cbs;
      d->tick(new_clock, cbs);
      handle_callbacks(cbs);
  }
}

void
sysmod::flush_cbs()
{
  std::lock_guard<std::mutex> lock(sys_m);
  svSetScope(scope_);
  for (auto& res : callbacks_) {
    switch (res.cb) {
      case device::Callback::TIMER_INT: sysmod_timer_interrupt(res.hart, res.val);
                                        break;
      case device::Callback::SW_INT:    sysmod_sw_interrupt(res.hart, res.val);
                                        break;
      case device::Callback::TERMINATE: sysmod_terminate();
                                        break;
      default: assert(false);
    }
  }

  callbacks_.clear();
}

extern "C" {

  sysmod* sysmod_new(const char* memmap) {
    svScope scope = svGetScope();
    return new sysmod(scope, std::string(memmap));
  }

  void sysmod_load_program(sysmod* s, const char* prog) {
    s->load_prog(std::string(prog));
  }

  void sysmod_tick(sysmod* s, uint64_t new_clock) {
    s->tick(new_clock);
  }

  void sysmod_flush_cbs(sysmod* s) {
    s->flush_cbs();
  }
}
