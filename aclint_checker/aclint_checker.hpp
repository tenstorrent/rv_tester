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
        cvm::log(cvm::HIGH, "[cluster] mmr write: [{} = {:#x}]\n", name, data);
    }

    // Write new data to the mmr
    void write(uint64_t new_data, size_t sz) {
        uint64_t sz_mask = (sz == 3) ? ~uint64_t(0) : ((uint64_t)1 << ((1<<sz)*8)) - 1;
            if (sz == 2 || sz == 3) { // update modeled data only for 4B and 8B writes
            data = (data & ~sz_mask) | (new_data & sz_mask);
        }
        cvm::log(cvm::MEDIUM, "ACLINT MMR write: [{} = {:#x}(size = {})]\n", name, data, sz);
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
    CR_WTIME           = 0x4200'0000,
    CR_CTIME           = 0x4200'0008
} aclint_addr;

std::unordered_map<aclint_addr, mmr> aclint_mmrs = {
    {AC_MTIMECMP0,  {"aclintmmr_mtimecmp0", 0x4218'8000, 8, 0xffffffff}},
    {AC_MTIMECMP1,  {"aclintmmr_mtimecmp1", 0x4218'8008, 8, 0xffffffff}},
    {AC_MTIMECMP2,  {"aclintmmr_mtimecmp2", 0x4218'8010, 8, 0xffffffff}},
    {AC_MTIMECMP3,  {"aclintmmr_mtimecmp3", 0x4218'8018, 8, 0xffffffff}},
    {AC_MTIMECMP4,  {"aclintmmr_mtimecmp4", 0x4218'8020, 8, 0xffffffff}},
    {AC_MTIMECMP5,  {"aclintmmr_mtimecmp5", 0x4218'8028, 8, 0xffffffff}},
    {AC_MTIMECMP6,  {"aclintmmr_mtimecmp6", 0x4218'8030, 8, 0xffffffff}},
    {AC_MTIMECMP7,  {"aclintmmr_mtimecmp7", 0x4218'8038, 8, 0xffffffff}},
    {AC_MTIMECMP8,  {"aclintmmr_mtimecmp8", 0x4218'8040, 8, 0xffffffff}},
    {AC_MTIME,      {"aclintmmr_mtime", 0x4218'0000, 8, 0x0, 0x0, 0x0}},
    {AC_TIMESYNC,   {"aclintmmr_timesync", 0x4218'0018, 8, 0x0, 0xffffffffffffffff, 0x0}}, 
    {CR_WTIME,      {"aclintmmr_wtime", 0x4200'0000, 8, 0xffffffff, 0x0, 0x0}},
    {CR_CTIME,      {"aclintmmr_ctime", 0x4200'0008, 8, 0x0, 0x0, 0x0}}
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
    
    void popifpossible(uint64_t hart);
    void initializevqueue(std::vector < std::queue < MmrWr >> & q, int size);
    void initializevhash(std::vector < std::unordered_map < int, MmrWr >> & q, int size);
    void check_outstanding_transactions(uint64_t signal);
    
    private:
        // cvm::file_logger log;
        void reset();

    std::unordered_map < int, std::queue < MmrWr >> cr_ac_mmr_q_;
    std::unordered_map < int, std::queue < MmrWr >> axi_ac_mmr_q_;
    std::queue < MmrWr > smc_ac_mmr_q_;
    const uint64_t cluster_id_end_ = 25;
    const uint64_t cluster_id_start_ = 21;
    const uint64_t mmr_base_start_ = 27;
    int unsigned smc_monitor_loc = cvm::topology::get_from_type("PLATFORM", 0);
};

#endif
