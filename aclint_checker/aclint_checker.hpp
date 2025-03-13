#ifndef _RISCV_DEBUG_MODULE_H
#define _RISCV_DEBUG_MODULE_H
#include <iostream>

#include <stdint.h>
#include <set>
#include <vector>
#include <cassert>
#include <unordered_set>
#include <queue>
#include "cvm/logger.hpp"
#include "cvm/topology.hpp"
#include "rv_tester_transactions.hpp"
#include "aclint_checker_param.hpp"
#include <unordered_map>

DECLARE_bool(aclint);

#define max_hartid 1 // Define the maximum number of harts in the system
#define halt_on_reset false

inline uint64_t insterClusterId(uint64_t inaddr);

typedef uint64_t reg_t;
typedef struct {
    uint64_t addr;
    uint64_t data;
    uint64_t mask;
    uint64_t order;
    bool datavalid;
}
MmrWr;

// Equality operator for MmrWr
bool operator==(const MmrWr& lhs, const MmrWr& rhs) {
    return lhs.addr == rhs.addr &&
           lhs.data == rhs.data &&
           lhs.mask == rhs.mask;
}

struct mmr {
    std::string name;      // Name of the mmr
    uint32_t address;      // Address of the mmr
    uint8_t size;          // Size of the mmr in bytes
    uint64_t reset_value;  // Reset value of the mmr
    uint64_t data;         // Current data stored in the mmr
    uint64_t write_mask = -1;   // Mask for mmr write checks
    uint64_t read_mask = -1;    // Mask for mmr read checks
    uint64_t lock_bit = 0;

    // Default constructor
    mmr() : name(""), address(0), size(0), reset_value(0), data(0) {}

    // Constructor for easier initialization
    mmr(const std::string& name, uint32_t addr, uint8_t sz, uint64_t reset)
        : name(name), address(addr), size(sz), reset_value(reset), data(reset) {}

    // Constructor with masks
    mmr(const std::string& name, uint32_t addr, uint8_t sz, uint64_t reset, uint64_t write_mask, uint64_t read_mask)
        : name(name), address(addr), size(sz), reset_value(reset), data(reset), write_mask(write_mask), read_mask(read_mask) {}

    // Write new data to the mmr
    void write(uint64_t new_data) {
        data = new_data;
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] MMR write: [{} = {:#x}]\n", name, data);
    }

    // Write new data to the mmr
    void write(uint64_t new_data, size_t sz) {
        uint64_t sz_mask = (sz == 3) ? ~uint64_t(0) : ((uint64_t)1 << ((1<<sz)*8)) - 1;
        if ((sz == 2 || sz == 3) && (lock_bit == 0)) { // update modeled data only for 4B and 8B writes
            data = (data & ~sz_mask) | (new_data & sz_mask);
            cvm::log(cvm::HIGH, "[ACLINT CHECKER] ACLINT MMR write: [{} = {:#x}(size = {})]\n", name, data, sz);
        }
    }

    // Read the current value of the mmr
    uint64_t read() const {
        return data;
    }
};

typedef enum : size_t { SZ_4B = 4, SZ_8B = 8 } sz_t;

typedef enum : uint64_t {
    AC_MTIMECMP0       = 0x4218'8000,
    AC_MTIMECMP1       = 0x4218'8008,
    AC_MTIMECMP2       = 0x4218'8010,
    AC_MTIMECMP3       = 0x4218'8018,
    AC_MTIMECMP4       = 0x4218'8020,
    AC_MTIMECMP5       = 0x4218'8028,
    AC_MTIMECMP6       = 0x4218'8030,
    AC_MTIMECMP7       = 0x4218'8038,
    AC_MTIMECMP8       = 0x4218'8040,
    AC_MTIME           = 0x4218'0000,
    AC_TIMESYNC        = 0x4218'0018,
    AC_CLUSTERFUSE     = 0x4218'FFF8,
    CR_WTIME           = 0x4200'0000,
    CR_CTIME           = 0x4200'0008
} aclint_addr;

std::unordered_map<aclint_addr, mmr> aclint_mmrs = {
    {AC_MTIMECMP0,   {"AC_MTIMECMP0", 0x4218'8000, 8, 0xffffffff}},
    {AC_MTIMECMP1,   {"AC_MTIMECMP1", 0x4218'8008, 8, 0xffffffff}},
    {AC_MTIMECMP2,   {"AC_MTIMECMP2", 0x4218'8010, 8, 0xffffffff}},
    {AC_MTIMECMP3,   {"AC_MTIMECMP3", 0x4218'8018, 8, 0xffffffff}},
    {AC_MTIMECMP4,   {"AC_MTIMECMP4", 0x4218'8020, 8, 0xffffffff}},
    {AC_MTIMECMP5,   {"AC_MTIMECMP5", 0x4218'8028, 8, 0xffffffff}},
    {AC_MTIMECMP6,   {"AC_MTIMECMP6", 0x4218'8030, 8, 0xffffffff}},
    {AC_MTIMECMP7,   {"AC_MTIMECMP7", 0x4218'8038, 8, 0xffffffff}},
    {AC_MTIMECMP8,   {"AC_MTIMECMP8", 0x4218'8040, 8, 0xffffffff}},
    {AC_MTIME,       {"AC_MTIME", 0x4218'0000, 8, 0x0, 0x0, 0xffffffffffffffff}},
    {AC_CLUSTERFUSE, {"AC_CLUSTERFUSE", 0x4218'FFF8, 8, 0x0, 0xffffffffffffffff, 0xffffffffffffffff}},
    {AC_TIMESYNC,    {"AC_TIMESYNC", 0x4218'0018, 8, 0x0, 0xffffffffffffffff, 0x0}}, 
    {CR_WTIME,       {"CR_WTIME", 0x4200'0000, 8, 0xffffffff, 0x0, 0xffffffffffffffff}},
    {CR_CTIME,       {"CR_CTIME", 0x4200'0008, 8, 0x0, 0x0, 0xffffffffffffffff}}
};

std::unordered_map<aclint_addr, std::pair<bool, bool>> mmrReadReqFlag = {
    {AC_MTIMECMP0   , {false, false}},
    {AC_MTIMECMP1   , {false, false}},
    {AC_MTIMECMP2   , {false, false}},
    {AC_MTIMECMP3   , {false, false}},
    {AC_MTIMECMP4   , {false, false}},
    {AC_MTIMECMP5   , {false, false}},
    {AC_MTIMECMP6   , {false, false}},
    {AC_MTIMECMP7   , {false, false}},
    {AC_MTIMECMP8   , {false, false}},
    {AC_MTIME       , {false, false}},
    {AC_TIMESYNC    , {false, false}},
    {AC_CLUSTERFUSE , {false, false}},
    {CR_WTIME       , {false, false}},
    {CR_CTIME       , {false, false}}
};


class aclint_checker {
    public: aclint_checker(cvm::topology::loc_t, unsigned);

    // Called for every cycle the JTAG TAP spends in Run-Test/Idle.
    // void run_test_idle();

    // Called when one of the attached harts was reset.
    //void proc_reset(unsigned id);

    void process(const rv_tester_transactions::aclint_checker::cr_ac_mmrwrite < > & cr_ac_mmrwrite);
    void process(const rv_tester_transactions::aclint_checker::axi_ac_write < > & axi_ac_write);
    void process(const smc_write_pkt & w);
    void process(const smc_read_pkt & r);
    void process(const smc_req_pkt & read_req);
    
    void popifpossible(uint64_t srcid);
    void popifpossible(const MmrWr& m);
    void initializevqueue(std::vector < std::queue < MmrWr >> & q, int size);
    void initializevhash(std::vector < std::unordered_map < int, MmrWr >> & q, int size);
    void check_outstanding_transactions(uint64_t signal);
    void set_scope(svScope scope);
    private:
        // cvm::file_logger log;
        void reset();

    std::unordered_map < int, std::queue < MmrWr >> cr_ac_mmr_q_;
    std::unordered_map < int, std::queue < MmrWr >> axi_ac_mmr_q_;
    std::vector < MmrWr > smc_ac_mmr_v_;
    const uint64_t cluster_id_end_ = 25;
    const uint64_t cluster_id_start_ = 21;
    const uint64_t mmr_base_start_ = 27;
    int unsigned smc_monitor_loc = cvm::topology::get_from_type("PLATFORM", 0);
    svScope aclint_checker_scope_;
};

#endif
