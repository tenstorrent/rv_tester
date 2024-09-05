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
#include "cla_cfg/cla_cfg.h"
#include "pm_nw_xtor/pm_nw_xtor.h"
#include "cvm/topology.hpp"
#include "sysmod_params.hpp"

#include <string>

#include "rv_tester_transactions.hpp"

class sysmod {

  public:
    struct backdoor_read_t {
      uint64_t address;
      std::atomic<bool>* flag;
      uint64_t* out_data;
    };

    struct backdoor_write_t {
      uint64_t address;
      uint64_t data;
      int size;
      std::atomic<bool>* flag;
    };

    sysmod(cvm::topology::loc_t loc, unsigned id);

    ~sysmod();

    void configure_plusargs();
    void core_harvest_plusargs();
    void sc_harvest_plusargs();
    uint32_t get_rand_mask(uint32_t n, uint32_t max);
    std::string get_rand_id(uint32_t mask, uint32_t ncores);
    std::string get_id(uint32_t mask, uint32_t ncores);
    int32_t get_rand_ways_mask(int32_t n, int32_t max);
    int32_t get_rand_dis_ways(int32_t max);
    int32_t get_rand_sp_ways(int32_t max);

    device* dev(uint64_t addr);
    device* dev(const std::string& tag);

    void write(uint64_t addr, size_t length, const device::data_t& data, const device::strb_t& strb);
    cvm::messenger::task<void> read(uint64_t addr, size_t length, device::data_t& data);

    void set_scope(svScope s) { scope_ = s; }
    void tick(uint64_t advance);
    void jtag_tick(uint64_t advance);
    void overlay_tick(uint64_t advance);
    void jtag_resp(std::bitset<70> rdata);
    void compose();
    void load_boot(const std::string& boot);
    void load_cplfw(const std::string& cplfw);
    void load_prog(const std::string& hex, const std::string& load, const std::string& lz4);
    void load_csr_mmr_boot(uint64_t);
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

      std::bitset<70> bitset_shifted = bitset>>2;

      //jtag rx -> jtag.op_Data , we are shifting only by 2 since from jtag_xtor for each tap point we shift accordingly but all of them are shifted by 2
      //std::cout<<"[JTAG RESP] original = " <<bitset<<" shifted = "<<bitset_shifted<<"\n";
      std::vector<uint64_t> ulongArray(arraySize);

      for (size_t i = 0; i < bitsetSize; i += ulongSize) {
        size_t ulongIndex = i / ulongSize;
        uint64_t value = 0;
        for (size_t j = 0; j < ulongSize && (i + j) < bitsetSize; ++j)
          value |= (bitset_shifted[i + j] ? 1UL : 0UL) << j;

        ulongArray[ulongIndex] = value;
      }
      return ulongArray;
    }
    void store_dm_randpc();
  protected:
    void trace_info_handler(trace_cfg::trace_info_t i);
    void cla_info_handler(cla_cfg::cla_info_t i);
    void pm_nw_info_handler(pm_nw_xtor::pm_nw_info_t i);
    void timer_interrupt(clint::timer_t t);
    void sw_interrupt(clint::sw_t s);
    void dmi_write(debugger::dmi_data_t s);
    void jtag_req(jtag_driver::jtag_data_t i);
    void tbox_interrupt(interrupter::interrupt_t i);
    void tboxtrig_updatemem(uint64_t addr, uint64_t data);
    void aplic_interrupt(aplic_driver::aplic_driver_write_t i);
    void uc_helper_backdoor_write(uc_helper::uc_helper_write_t w);
    void uc_helper_backdoor_read(uc_helper::uc_helper_read_req_t w);
    void trace_cfg_read_req_router(trace_cfg::trace_cfg_read_t r);
    void scratchpad_xtor_read_req_router(scratchpad_xtor::scratchpad_xtor_read_t r);
    void terminate(htif::terminate_t t);
    cvm::messenger::task<uint64_t> backdoor_read(uint64_t address);
    cvm::messenger::task<uint64_t> backdoor_write(backdoor_write_t);

  private:

    void reset();

    void process(const rv_tester_transactions::sysmod::tick<>& tick);
    void store_inval_load(const inval_load_s& payload);
    void store_inval_crsp(const inval_crsp_s& payld);

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

    inval_load_s inval_load_;
    inval_crsp_s inval_crsp_;

    std::uint64_t ticks_ = 0;
    std::uint64_t jtag_ticks_ = 0;
    std::uint64_t overlay_ticks_ = 0;
    //remote_bitbang_t remote_bitbang();
};
