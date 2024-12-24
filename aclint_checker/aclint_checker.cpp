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
    cvm::registry::messenger.connect < rv_tester_transactions::aclint_checker::axi_ac_write < >> (loc, [this](const auto & v) {
        return this -> process(v);
    });
    cvm::registry::messenger.connect <smc_write_pkt> (smc_monitor_loc, [this](const auto & v) {
        return this -> process(v);
    });
    cvm::registry::messenger.connect <smc_read_pkt> (smc_monitor_loc, [this](const auto & v) {
        return this -> process(v);
    });
    cvm::registry::messenger.connect <uint64_t>(loc, [this](const uint64_t& signal) {
        return this -> check_outstanding_transactions(signal);
    });
    reset();
}

void aclint_checker::process(const rv_tester_transactions::aclint_checker::cr_ac_mmrwrite < > & cr_ac_mmrwrite) {
    MmrWr m;
    uint64_t cluster_mmr_addr = insterClusterId(cr_ac_mmrwrite.addr);
    m.addr = cluster_mmr_addr;
    m.data = cr_ac_mmrwrite.data;
    m.mask = cr_ac_mmrwrite.mask;
    m.order = cr_ac_mmrwrite.order;
    m.datavalid = false;
    uint64_t srcid = cr_ac_mmrwrite.srcid;
    cr_ac_mmr_q_[srcid].push(m);

    cvm::log(cvm::MEDIUM, "[ACLINT CHECKER] AC MMR WRITES: location {} srcid {} order {} addr {:#x} ({:#x}) data {:#x} mask {:#x} \n", cr_ac_mmrwrite.location, cr_ac_mmrwrite.srcid, cr_ac_mmrwrite.order, cr_ac_mmrwrite.addr, cluster_mmr_addr, cr_ac_mmrwrite.data, cr_ac_mmrwrite.mask);
}

void aclint_checker::process(const rv_tester_transactions::aclint_checker::axi_ac_write < > & axi_ac_write) {
    
    uint64_t cluster_mmr_addr = insterClusterId(axi_ac_write.addr);

    MmrWr m;
    m.addr = cluster_mmr_addr;
    m.data = axi_ac_write.data;
    m.mask = axi_ac_write.mask;
    m.order = 0;
    m.datavalid = true;
    uint64_t srcid = axi_ac_write.srcid;
    axi_ac_mmr_q_[srcid].push(m);
    cvm::log(cvm::MEDIUM, "[ACLINT CHECKER] AC AXI WRITES: location {} srcid {} addr {:#x} ({:#x}) data {:#x} mask {:#x} \n", axi_ac_write.location, axi_ac_write.srcid, axi_ac_write.addr, cluster_mmr_addr, axi_ac_write.data, axi_ac_write.mask);

    
    auto mmr_addr = static_cast<aclint_addr>(cluster_mmr_addr);
    if (aclint_mmrs.find(mmr_addr) != aclint_mmrs.end()) {
        size_t sz = (axi_ac_write.mask == 0xFF) ?  3 :
                    (axi_ac_write.mask == 0x0F) ?  2 :
                    (axi_ac_write.mask == 0x03) ?  1 : 0;
        aclint_mmrs[mmr_addr].write(axi_ac_write.data, sz);
    }    

    popifpossible(srcid);
}

void aclint_checker::process(const smc_write_pkt & w) {
    
    auto mmr_addr = static_cast<aclint_addr>(w.addr);
    if (aclint_mmrs.find(mmr_addr) == aclint_mmrs.end()) {
        return;
    }

    MmrWr m;
    m.addr = w.addr;
    m.data = w.data;
    uint64_t sz_mask = (w.size == 3) ?  0xFF :
                       (w.size == 2) ?  0x0F :
                       (w.size == 1) ?  0x03 : 0x01;
    m.mask = sz_mask;
    m.order = 0;
    m.datavalid = false;
    
    smc_ac_mmr_q_.push(m);
    cvm::log(cvm::MEDIUM, "SMC-AC WRITES: addr {:#x} data {:#x} size {:#x} mask {:#x}\n", w.addr, w.data, w.size, sz_mask);
}

void aclint_checker::process(const smc_read_pkt & r) {
    
    auto mmr_addr = static_cast<aclint_addr>(r.addr);
    if (aclint_mmrs.find(mmr_addr) == aclint_mmrs.end()) return;
    
    size_t sz =  1 << r.size;
    uint64_t sz_mask = (sz == SZ_8B) ? ~uint64_t(0) : ((1ULL << (sz * 8)) - 1);
    uint64_t actual = ((r.addr % 8) ? (r.data >> 32) : r.data) & sz_mask;

    uint64_t expected = aclint_mmrs[mmr_addr].read();


    if ((actual & aclint_mmrs[mmr_addr].write_mask) != (expected & sz_mask & aclint_mmrs[mmr_addr].read_mask)) {
      cvm::log(cvm::ERROR, "Error: [SMC-AC] Mismatch:- ACLINT MMR mismatch - Name = {}, Address = {:#x} - Actual: {:#x} Expected: {:#x}\n", aclint_mmrs[mmr_addr].name, aclint_mmrs[mmr_addr].address, actual & aclint_mmrs[mmr_addr].write_mask, expected & sz_mask & aclint_mmrs[mmr_addr].write_mask);
    } else {
      cvm::log(cvm::MEDIUM, "[SMC-AC] ACLINT MMR match - Name = {}, Address = {:#x} - Actual: {:#x} Expected: {:#x}\n", aclint_mmrs[mmr_addr].name, aclint_mmrs[mmr_addr].address, actual & aclint_mmrs[mmr_addr].write_mask, expected & sz_mask & aclint_mmrs[mmr_addr].write_mask);
    }

}

void aclint_checker::reset() {
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Reset \n");
}

void aclint_checker::popifpossible(uint64_t srcid) {
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Came in popifpossible srcid: {} \n", srcid);
    if (cr_ac_mmr_q_.find(srcid) == cr_ac_mmr_q_.end()) return;
    if (cr_ac_mmr_q_[srcid].empty() && smc_ac_mmr_q_.empty()) {
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: CR_AC_MMR srcid {} queue empty and SMC-AC queue empty.\n", srcid);
        return;
    }

    MmrWr & axi_ac_q_front = axi_ac_mmr_q_[srcid].front();
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: AC_AXI srcid {} addr {:#x} data {:#x} mask {:#x} \n", srcid, axi_ac_q_front.addr, axi_ac_q_front.data, axi_ac_q_front.mask);


    MmrWr & smc_ac_q_front = smc_ac_mmr_q_.front();
    MmrWr & cr_ac_q_front = cr_ac_mmr_q_[srcid].front();

    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: CR_AC_MMR srcid {} addr {:#x} data {:#x} mask {:#x} \n", srcid, cr_ac_q_front.addr, cr_ac_q_front.data, cr_ac_q_front.mask);
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: SMC_AC_MMR srcid {} addr {:#x} data {:#x} mask {:#x} \n", srcid, smc_ac_q_front.addr, smc_ac_q_front.data, smc_ac_q_front.mask);
    

    if (cr_ac_q_front == axi_ac_q_front) {
        cr_ac_mmr_q_[srcid].pop();
        axi_ac_mmr_q_[srcid].pop();
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] CR_AC_MMR Popped \n");
    }
    else if (smc_ac_q_front == axi_ac_q_front) {
        // Should we add check here for srcid == 3 for SMC?
        smc_ac_mmr_q_.pop();
        axi_ac_mmr_q_[srcid].pop();
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] SMC_AC_MMR Popped \n");
    }
    else
        cvm::log(cvm::ERROR,"Error: AC_AXI Bad Transaction\n");

    return;
}

inline uint64_t insterClusterId(uint64_t inaddr) {
    // Parameters
    constexpr int ClusterIdStart = 21;
    constexpr int ClusterIdEnd = 25;
    constexpr int MMR_BASE_START = 27;
    constexpr int BASE_ADDR = 0x40000000;
    
    uint64_t upper_mask = ((1ULL << (MMR_BASE_START - ClusterIdStart)) - 1);
    uint64_t lower_mask = ((1ULL << (ClusterIdStart)) - 1);
    
    uint64_t upper = ((inaddr >> ClusterIdStart) & upper_mask) << ClusterIdEnd;
    uint64_t lower = inaddr & lower_mask;
    return  (upper | lower | BASE_ADDR);
}

void aclint_checker::check_outstanding_transactions(uint64_t signal) {
    cvm::log(cvm::MEDIUM, "[ACLINT CHECKER] Checking for outstanding SMC-AC MMR writes...\n");
    if (!signal) return;
    for (const auto& [key, queue] : cr_ac_mmr_q_) {
        if (!queue.empty()) {
            cvm::log(cvm::ERROR, "Error: {} Outstanding AC MMR writes for srcid {}\n", queue.size(), key);
            return; 
        }
    }
    cvm::log(cvm::MEDIUM, "[ACLINT CHECKER] No outstanding AC MMR writes\n");
    
    if (!smc_ac_mmr_q_.empty()) {
        cvm::log(cvm::ERROR, "Error: {} Outstanding SMC-AC MMR writes\n", smc_ac_mmr_q_.size());
        return; 
    }
    cvm::log(cvm::MEDIUM, "[ACLINT CHECKER] No outstanding SMC-AC MMR writes\n");
}

extern "C" void check_outstanding_transactions(cvm::topology::loc_t loc) {
    cvm::registry::messenger.signal<uint64_t>(loc, uint64_t(1));
}

