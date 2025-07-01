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
#include "trickbox/debugger.h"
#include "cvm/topology.hpp"
#include "rv_tester/rv_tester_structs.h"
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

    void configure();
    void core_harvest_plusargs();
    void sc_harvest_plusargs();
    void set_scope(svScope s) { scope_ = s; }
    void tick(uint64_t advance);
    void is_dut_reset_req(bool dut_reset_req,uint64_t clocks,uint64_t divisor);
    void jtag_tick(uint64_t advance);
    void overlay_tick(uint64_t advance);
    void compose();
    void load_boot(const std::string& boot);
    void load_cplfw(const std::string& cplfw);
    void load_prog(const std::string& hex, const std::string& load, const std::string& lz4, const std::string& bin);
    void load_csr_mmr_boot(uint64_t dut);
    void load_io(const std::string& io);
    void store_dm_rand();

    uint32_t get_rand_mask(uint32_t n, uint32_t max);
    int32_t  get_rand_ways_mask(int32_t n, int32_t max);
    int32_t  get_rand_dis_ways(int32_t max);
    int32_t  get_rand_sp_ways(int32_t max);
    std::string get_rand_id(uint32_t mask, uint32_t ncores);
    std::string get_id(uint32_t mask, uint32_t ncores);

    device* dev(uint64_t addr);
    device* dev(const std::string& tag);

    void write(uint64_t addr, size_t length, const device::data_t& data, const device::strb_t& strb);
    cvm::messenger::task<void> read(uint64_t addr, size_t length, device::data_t& data);

    std::vector<uint64_t> bitsetToUint64Array(const std::bitset<70>& bitset) {

      std::bitset<70> bitset_shifted = bitset>>2;
      const size_t bitsetSize = 64;
      const size_t ulongSize = sizeof(uint64_t) * 8;
      const size_t arraySize = (bitsetSize + ulongSize - 1) / ulongSize;
      std::vector<uint64_t> ulongArray(arraySize);

      for (size_t i = 0; i<bitsetSize; i+=ulongSize) {
        size_t ulongIndex = i / ulongSize;
        uint64_t value = 0;
        for (size_t j = 0; j < ulongSize && (i + j) < bitsetSize; ++j)
          value |= (bitset_shifted[i + j] ? 1UL : 0UL) << j;
        ulongArray[ulongIndex] = value;
      }
      return ulongArray;
    }

  protected:
    void timer_interrupt(clint::timer_t t);
    void sw_interrupt(clint::sw_t s);
    void dmi_write(debugger::dmi_data_t s);
    void tbox_interrupt(interrupter::interrupt_t i);
    void eot_backdoor_write(transactor::write_t& w);
    void tboxtrig_updatemem(uint64_t addr, uint64_t data);
    void uc_helper_backdoor_write(uc_helper::uc_helper_write_t w);
    void uc_helper_backdoor_read(uc_helper::uc_helper_read_req_t w);
    void terminate(htif::terminate_t t);
    void actual_test_started(rv_tester::test_started);
    cvm::messenger::task<uint64_t> backdoor_read(uint64_t address);
    cvm::messenger::task<uint64_t> backdoor_write(backdoor_write_t);

  private:

    bool bin_load(const std::string load, bool lz4_compressed);

    std::shared_ptr<TT_APLIC::Aplic> create_aplic() const;

    std::string hostname = "localhost";
    int port = 50001;
    svScope scope_;
    svScope scope() { return scope_; }

    cvm::topology::loc_t loc_, wc_loc_;
    unsigned id_;
    unsigned id()   { return id_; }
    std::vector<std::unique_ptr<device> > devices_;
    std::map<std::string, memmap_entry_t> memmap_;

    uint64_t ticks_ = 0;
    uint64_t jtag_ticks_ = 0;
    uint64_t overlay_ticks_ = 0;
    bool cosim_init_ = 0;
    bool stee = false;
    uint64_t secure_region_start_, secure_region_end_;

    inval_load_s inval_load_;
    inval_crsp_s inval_crsp_;

    void reset();
    void process(const rv_tester_transactions::sysmod::tick<>& tick);
    void store_inval_load(const inval_load_s& payload);
    void store_inval_crsp(const inval_crsp_s& payld, bool);
    void set_secure_region(std::string region);
    void configure_uninit_read_callbacks();
};
