#pragma once

#include <mutex>
#include <memory>
#include "device.h"
#include "svdpi.h"
#include "memmap.h"
#include "transactor.h"
#include "clint/clint.h"
#include "aclint/aclint.h"
#include "htif/htif.h"
#include "trickbox/interrupter.h"
#include "trickbox/uc_helper.h"
#include "trickbox/aplic_driver.h"
#include "trickbox/debugger.h"
#include "trickbox/jtag_driver.h"
#include "scratchpad_xtor/scratchpad_xtor.h"
#include "trace_cfg/trace_cfg.h"
#include "smc_xtor/smc_xtor.h"
#include "pll_xtor/pll_xtor.h"
#include "pm_nw_xtor/pm_nw_xtor.h"
#include "cvm/topology.hpp"

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
    void jtag_tick(uint64_t advance);
    void jtag_resp(std::bitset<70> rdata);
    void compose();
    void load_boot(const std::string& boot);
    void load_cplfw(const std::string& cplfw);
    void load_prog(const std::string& hex, const std::string& load, const std::string& lz4);
    void load_csr_boot(uint64_t);
    void load_io(const std::string& io);
    // Function to convert a bitset to an array of uint64_t
  //   std::vector<uint64_t> bitsetToUint64Array(const std::bitset<70>& bs) {
  //      std::vector<uint64_t> result;
  //      std::bitset<64> temp;
  //      for (int i = 0; i < static_cast<int>(bs.size()); i += 64) {
  //         temp[i] = bs[i*64 +; // Extract 64 bits from the bitset
  //         result.push_back(temp.to_ulong());
  //      }
  //     return result;
  //  }
   std::vector<uint64_t> bitsetToUint64Array(const std::bitset<70>& bitset) {
    const size_t bitsetSize = 64;//70;
    const size_t ulongSize = sizeof(uint64_t) * 8;
    const size_t arraySize = (bitsetSize + ulongSize - 1) / ulongSize;
    
     std::bitset<70> bitset_shifted = bitset>>4;
    std::vector<uint64_t> ulongArray(arraySize);

    for (size_t i = 0; i < bitsetSize; i += ulongSize) {
        size_t ulongIndex = i / ulongSize;
        uint64_t value = 0;

        for (size_t j = 0; j < ulongSize && (i + j) < bitsetSize; ++j) {
            value |= (bitset_shifted[i + j] ? 1UL : 0UL) << j;
        }

        ulongArray[ulongIndex] = value;
    }

    return ulongArray;
}
  protected:
    void trace_info_handler(trace_cfg::trace_info_t i);
    void smc_info_handler(smc_xtor::smc_info_t i);
    void pll_info_handler(pll_xtor::pll_info_t i);
    void pm_nw_info_handler(pm_nw_xtor::pm_nw_info_t i);
    void timer_interrupt(clint::timer_t t);
    void sw_interrupt(clint::sw_t s);
    void dmi_write(debugger::dmi_data_t s);
    void jtag_req(jtag_driver::jtag_data_t i);
    void tbox_interrupt(interrupter::interrupt_t i);
    void aplic_interrupt(aplic_driver::aplic_driver_write_t i);
    void uc_helper_backdoor_write(uc_helper::uc_helper_write_t w);
    void uc_helper_backdoor_read(uc_helper::uc_helper_read_req_t w);
    void trace_cfg_read_req_router(trace_cfg::trace_cfg_read_t r);
    void smc_read_req_router(smc_xtor::smc_xtor_read_t r);
    void scratchpad_xtor_read_req_router(scratchpad_xtor::scratchpad_xtor_read_t r);
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
    std::uint64_t jtag_ticks_ = 0;
    //remote_bitbang_t remote_bitbang();
};
