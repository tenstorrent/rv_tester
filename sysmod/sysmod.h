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
#include "trickbox/aplic_driver.h"
#include "trickbox/debugger.h"
#include "trickbox/jtag_driver.h"
#include "trace_cfg/trace_cfg.h"
#include "smc_xtor/smc_xtor.h"
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
    void jtag_resp(uint64_t rdata);
    void compose();
    void load_boot(const std::string& boot);
    void load_prog(const std::string& hex, const std::string& load);
    void load_io(const std::string& io);

  protected:
    void trace_info_handler(trace_cfg::trace_info_t i);
    void smc_info_handler(smc_xtor::smc_info_t i);
    void timer_interrupt(clint::timer_t t);
    void sw_interrupt(clint::sw_t s);
    void dmi_write(debugger::dmi_data_t s);
    void jtag_req(jtag_driver::jtag_data_t i);
    void tbox_interrupt(interrupter::interrupt_t i);
    void aplic_interrupt(aplic_driver::aplic_driver_write_t i);
    void uc_helper_backdoor_write(uc_helper::uc_helper_write_t w);
    void uc_helper_backdoor_read(uc_helper::uc_helper_read_req_t w);
    void trace_cfg_read_req_router(trace_cfg::trace_cfg_read_t r);
    void terminate(htif::terminate_t t);

  private:

    void reset();

    void process(const rv_tester_transactions::sysmod::tick<>& tick);

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
