#pragma once

#include <mutex>
#include <memory>
#include "device.h"
#include "svdpi.h"
#include "memmap.h"
#include "transactor.h"
#include "clint/clint.h"
#include "htif/htif.h"
#include "cvm/topology.hpp"

class sysmod {

  public:

    sysmod(cvm::topology::loc_t loc, unsigned id);

    ~sysmod();

    device& dev(uint64_t addr);
    device& dev(const std::string& tag);

    void write(uint64_t addr, size_t length, const device::data_t& data, const device::strb_t& strb);
    void read(uint64_t addr, size_t length, device::data_t& data);

    struct scope_t {
      svScope scope;
    };

    struct tick_t {
      uint64_t advance;
    };

    struct flush_t {};

    void set_scope(svScope s) { scope_ = s; }
    void tick(uint64_t advance);
    void compose();
    void load_prog(const std::string& hex, const std::string& load);
    std::string tag() { return "sysmod" + std::to_string(id()); }

  protected:

    void timer_interrupt(clint::timer_t t);
    void sw_interrupt(clint::sw_t s);
    void terminate(htif::terminate_t t);

  private:

    void reset();

    svScope scope() { return scope_; }
    unsigned id() { return id_; }

    svScope scope_;
    cvm::topology::loc_t loc_;
    unsigned id_;

    mutable std::mutex sys_m;
    std::vector<std::unique_ptr<device> > devices_;

    // Memmap
    memmap::memmap_t memmap_;
};
