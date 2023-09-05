#pragma once

#include <mutex>
#include <memory>
#include "device.h"
#include "svdpi.h"
#include "memmap.h"
#include "transactor.h"
#include "clint/clint.h"
#include "htif/htif.h"
#include "trickbox/interrupter.h"
#include "trickbox/uc_helper.h"
#include "trickbox/debugger.h"
#include "cvm/topology.hpp"
//#include "SimJTAG.cc"
#include <string>

#include "rv_tester_transactions.hpp"

class sysmod {

  public:

    sysmod(cvm::topology::loc_t loc, unsigned id);

    ~sysmod();

    device* dev(uint64_t addr);
    device* dev(const std::string& tag);

    void write(uint64_t addr, size_t length, const device::data_t& data, const device::strb_t& strb);
    cvm::messenger::task<void> read(uint64_t addr, size_t length, device::data_t& data);

    void set_scope(svScope s) { scope_ = s; }
    void tick(uint64_t advance);
    void compose();
    void load_prog(const std::string& hex, const std::string& load);
    void load_io(const std::string& io);

  protected:

    void timer_interrupt(clint::timer_t t);
    void sw_interrupt(clint::sw_t s);
    void dmi_write(debugger::dmi_data_t s);
    void tbox_interrupt(interrupter::interrupt_t i);
    void uc_helper_backdoor_write(uc_helper::uc_helper_write_t w);
    void uc_helper_backdoor_write(interrupter::interrupt_t i);
    void terminate(htif::terminate_t t);

  private:

    void reset();

    void process(const rv_tester_transactions::sysmod::tick& tick);

    svScope scope() { return scope_; }
    unsigned id() { return id_; }

    svScope scope_;
    cvm::topology::loc_t loc_;
    unsigned id_;

    std::vector<std::unique_ptr<device> > devices_;

    // Memmap
    memmap::memmap_t memmap_;
    std::string hostname = "localhost";
    int port = 50001;

    std::uint64_t ticks_ = 0;
    //remote_bitbang_t remote_bitbang();
};
