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
#include <unordered_map>

DECLARE_bool(aclint);

#define max_hartid 1 // Define the maximum number of harts in the system
#define halt_on_reset false

typedef uint64_t reg_t;
typedef struct {
    uint64_t addr;
    uint64_t data;
    uint64_t mask;
    uint64_t order;
    bool datavalid;
}
MmrWr;

class aclint_checker {
    public: aclint_checker(cvm::topology::loc_t, unsigned);

    // Called for every cycle the JTAG TAP spends in Run-Test/Idle.
    // void run_test_idle();

    // Called when one of the attached harts was reset.
    //void proc_reset(unsigned id);

    void process(const rv_tester_transactions::aclint_checker::cr_ac_mmrwrite < > & cr_ac_mmrwrite);
    void process(const rv_tester_transactions::aclint_checker::ac_axi_write < > & ac_axi_write);
    void process(const rv_tester_transactions::aclint_checker::cr_ac_mmrwr_bypass < > & cr_ac_mmrwr_bypass);

    void popifpossible(uint64_t hart);
    void initializevqueue(std::vector < std::queue < MmrWr >> & q, int size);
    void initializevhash(std::vector < std::unordered_map < int, MmrWr >> & q, int size);

    private:
        // cvm::file_logger log;
        void reset();

    std::vector < std::queue < MmrWr >> cr_ac_mmr_q_;
    std::vector < std::queue < MmrWr >> ac_axi_mmr_q_;
    std::vector < std::unordered_map < int,
    MmrWr >> cr_ac_bypass_;
    const uint64_t cluster_id_end_ = 25;
    const uint64_t cluster_id_start_ = 21;
    const uint64_t mmr_base_start_ = 27;

};

#endif