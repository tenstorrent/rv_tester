#include <cassert>
#include <cstring>
#include <map>
#include <memory>
#include <vector>
#include <bitset>
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/bitmanip.hpp"
#include "aclint_checker.hpp"
#include <queue>
#include <unordered_map>

REGISTRY_register(aclint_checker, TOP.PLATFORM.ACLINT_CHECKER, 0);
DEFINE_bool(aclint, false, "Enable aclint checks");

aclint_checker::aclint_checker(cvm::topology::loc_t loc, unsigned) {

    cvm::registry::messenger.connect < rv_tester_transactions::aclint_checker::cr_ac_mmrwrite < >> (loc, [this](const auto & v) {
        return this -> process(v);
    });
    cvm::registry::messenger.connect < rv_tester_transactions::aclint_checker::ac_axi_write < >> (loc, [this](const auto & v) {
        return this -> process(v);
    });
    cvm::registry::messenger.connect < rv_tester_transactions::aclint_checker::cr_ac_mmrwr_bypass < >> (loc, [this](const auto & v) {
        return this -> process(v);
    });

    reset();
    initializevqueue(cr_ac_mmr_q_, 10);
    initializevqueue(ac_axi_mmr_q_, 10);
    initializevhash(cr_ac_bypass_, 10);
}

void aclint_checker::process(const rv_tester_transactions::aclint_checker::cr_ac_mmrwrite < > & cr_ac_mmrwrite) {
    MmrWr m;
    m.addr = cr_ac_mmrwrite.addr;
    m.data = cr_ac_mmrwrite.data;
    m.mask = cr_ac_mmrwrite.mask;
    m.order = cr_ac_mmrwrite.order;
    m.datavalid = false;
    uint64_t hart = cr_ac_mmrwrite.hart;
    cr_ac_mmr_q_[hart].push(m);
    popifpossible(hart);
    if (cr_ac_mmr_q_[hart].size() > 5)  cvm::log(cvm::ERROR, "More than 4 outstanding AC MMR writes\n"); 
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] AC MMR WRITES: location {} hart {} order {} addr {:#x} data {:#x} mask {:#x} \n", cr_ac_mmrwrite.location, cr_ac_mmrwrite.hart, cr_ac_mmrwrite.order, cr_ac_mmrwrite.addr, cr_ac_mmrwrite.data, cr_ac_mmrwrite.mask);
}

void aclint_checker::process(const rv_tester_transactions::aclint_checker::cr_ac_mmrwr_bypass < > & cr_ac_mmrwr_bypass) {
    MmrWr m;
    m.addr = 0;
    m.data = cr_ac_mmrwr_bypass.data;
    m.mask = cr_ac_mmrwr_bypass.mask;
    m.order = 0;
    m.datavalid = false;
    uint64_t hart = cr_ac_mmrwr_bypass.hart;
    cr_ac_bypass_[hart][cr_ac_mmrwr_bypass.order] = m;
    popifpossible(hart);
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] CR AC BYPASS: location {} hart {} order {} data {:#x} mask {:#x} \n", cr_ac_mmrwr_bypass.location, cr_ac_mmrwr_bypass.hart, cr_ac_mmrwr_bypass.order, cr_ac_mmrwr_bypass.data, cr_ac_mmrwr_bypass.mask);
}

void aclint_checker::process(const rv_tester_transactions::aclint_checker::ac_axi_write < > & ac_axi_write) {
    MmrWr m;
    m.addr = ac_axi_write.addr;
    m.data = ac_axi_write.data;
    m.mask = ac_axi_write.mask;
    m.order = 0;
    m.datavalid = true;
    uint64_t hart = ac_axi_write.hart;
    ac_axi_mmr_q_[hart].push(m);
    popifpossible(hart);
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] AC AXI WRITES: location {} hart {} addr {:#x} data {:#x} mask {:#x} \n", ac_axi_write.location, ac_axi_write.hart, ac_axi_write.addr, ac_axi_write.data, ac_axi_write.mask);
}

void aclint_checker::reset() {
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Reset \n");
}

void aclint_checker::popifpossible(uint64_t hart) {
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Came in popifpossible {} \n", hart);
    if (ac_axi_mmr_q_[hart].empty()) {
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: AC_AXI hart {} queue empty \n", hart);
        return;
    }

    if (cr_ac_mmr_q_[hart].empty()) {
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: CR_AC_MMR hart {} queue empty \n", hart);
        return;
    }

    bool matched = false;
    while( !ac_axi_mmr_q_[hart].empty() || !matched ) {
    MmrWr & a = ac_axi_mmr_q_[hart].front();
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: AC_AXI hart {} addr {:#x} data {:#x} mask {:#x} \n", hart, a.addr, a.data, a.mask);

    MmrWr & b = cr_ac_mmr_q_[hart].front();
    if (!b.datavalid) {
        uint64_t order = b.order;
        if (cr_ac_bypass_[hart].find(order) == cr_ac_bypass_[hart].end()) {
            cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: CR_AC_BYPASS hart {} order {}  not found\n", hart, order);
            return;
        } else {
            b.data = cr_ac_bypass_[hart][order].data;
            b.mask = cr_ac_bypass_[hart][order].mask;
            b.datavalid = true;
            cr_ac_bypass_[hart].erase(cr_ac_bypass_[hart].find(order));
        }
    }

    uint64_t b_addr = b.addr & ((1 << mmr_base_start_) - 1);
    b_addr = ((b_addr >> cluster_id_end_) << cluster_id_start_) | (b_addr & ((1 << cluster_id_start_) - 1)); // Conversion from 'ha130000 to 'h330000
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: CR_AC_MMR hart {} addr {:#x} data {:#x} mask {:#x} \n", hart, b_addr, b.data, b.mask);

    if (a.addr != b_addr)
        matched = false;

    else if (a.data != b.data)
        matched = false;

    else if (a.mask != b.mask)
        matched = false;

    else matched = true;

    ac_axi_mmr_q_[hart].pop();
    }
    if (matched) {
    cr_ac_mmr_q_[hart].pop();
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Popped \n");
    }

    return;

}

void aclint_checker::initializevqueue(std::vector < std::queue < MmrWr >> & q, int size) {
    // Clear the vector in case it already contains elements
    q.clear();

    // Initialize the vector with 'size' number of empty queues
    q.resize(size, std::queue < MmrWr > ());
}

void aclint_checker::initializevhash(std::vector < std::unordered_map < int, MmrWr >> & q, int size) {
    // Clear the vector in case it already contains elements
    q.clear();

    // Initialize the vector with 'size' number of empty hash
    q.resize(size, {});
}
