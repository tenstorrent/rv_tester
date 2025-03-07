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

extern "C" {
    uint64_t get_mtime_value();
    uint64_t get_ctime_value();
    void update_ctime_value(uint64_t value);
}

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
    cvm::registry::messenger.connect <smc_req_pkt> (smc_monitor_loc, [this](const auto & v) {
        return this -> process(v);
    });
    cvm::registry::messenger.connect <uint64_t>(loc, [this](const uint64_t& signal) {
        return this -> check_outstanding_transactions(signal);
    });
    cvm::registry::messenger.connect<svScope>(loc, [this](svScope s) {
        return this->set_scope(s);
    });
    reset();
}

void aclint_checker::process(const rv_tester_transactions::aclint_checker::cr_ac_mmrwrite < > & cr_ac_mmrwrite) {
    MmrWr m;
    
    m.addr = cr_ac_mmrwrite.addr;
    m.data = cr_ac_mmrwrite.data;
    m.mask = cr_ac_mmrwrite.mask;
    m.order = cr_ac_mmrwrite.order;
    m.datavalid = false;
    uint64_t srcid = cr_ac_mmrwrite.srcid;

    cvm::log(cvm::HIGH, "[ACLINT CHECKER] AC MMR WRITES: location {} srcid {} order {} addr {:#x} data {:#x} mask {:#x} \n", cr_ac_mmrwrite.location, cr_ac_mmrwrite.srcid, cr_ac_mmrwrite.order, cr_ac_mmrwrite.addr, cr_ac_mmrwrite.data, cr_ac_mmrwrite.mask);

    auto mmr_addr = static_cast<aclint_addr>(cr_ac_mmrwrite.addr);
    if (mmr_addr == aclint_addr::AC_TIMESYNC) return;
    if (aclint_mmrs.find(mmr_addr) != aclint_mmrs.end()) {
        cr_ac_mmr_q_[srcid].push(m);
    }
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
    uint8_t user = axi_ac_write.user;
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] AC AXI WRITES: location {} srcid {} addr {:#x} ({:#x}) data {:#x} mask {:#x} user {} \n", axi_ac_write.location, axi_ac_write.srcid, axi_ac_write.addr, cluster_mmr_addr, axi_ac_write.data, axi_ac_write.mask, user);

    
    auto mmr_addr = static_cast<aclint_addr>(cluster_mmr_addr);
    if ((mmr_addr == aclint_addr::AC_TIMESYNC || 
         mmr_addr == aclint_addr::AC_CLUSTERFUSE) && 
         user != 3) {
        // Checking for only src
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] Writes to {} are srcid protected\n", aclint_mmrs[mmr_addr].name);
        return;
    }
    if (aclint_mmrs.find(mmr_addr) != aclint_mmrs.end()) {
        size_t sz = (axi_ac_write.mask == 0xFF) ?  3 :
                    (axi_ac_write.mask == 0x0F) ?  2 :
                    (axi_ac_write.mask == 0x03) ?  1 : 0;
        if (mmrReadReqFlag[mmr_addr].first) mmrReadReqFlag[mmr_addr].second = true;
        aclint_mmrs[mmr_addr].write(axi_ac_write.data, sz);
        axi_ac_mmr_q_[srcid].push(m);
        popifpossible(srcid);

        if (mmr_addr == aclint_addr::AC_CLUSTERFUSE && (aclint_mmrs[mmr_addr].lock_bit == 0) &&
            (((aclint_mmrs[mmr_addr].data >> 15) & 1) == 1)){
            // Lock bit set hence write_mask is zero
            aclint_mmrs[mmr_addr].lock_bit = 1;
            cvm::log(cvm::HIGH, "[ACLINT CHECKER] AC_CLUSTERFUSE Locked.\n");
        }
    }    
}

void aclint_checker::process(const smc_write_pkt & w) {
    if (!FLAGS_aclint) return;
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
    
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] SMC-AC WRITE: addr {:#x} data {:#x} size {:#x} mask {:#x}\n", w.addr, w.data, w.size, sz_mask);
    if (!(w.addr == aclint_addr::CR_CTIME || w.addr == aclint_addr::CR_WTIME)) {
        smc_ac_mmr_v_.push_back(m);
        popifpossible(m);
    }
    else {
        aclint_mmrs[mmr_addr].write(w.data, w.size);
        if (w.addr == aclint_addr::CR_CTIME){
            svSetScope(aclint_checker_scope_);
            update_ctime_value(aclint_mmrs[mmr_addr].read());   
        }
    }
}

void aclint_checker::process(const smc_req_pkt & read_req) {
    if (!FLAGS_aclint) return;
    auto mmr_addr = static_cast<aclint_addr>(read_req.addr);
    if (aclint_mmrs.find(mmr_addr) == aclint_mmrs.end()) {
        return;
    }
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] SMC-AC READ REQ: addr {:#x}\n", read_req.addr);
    // If read req is true, check for any writes when true, if write happens then set the second to true
    mmrReadReqFlag[mmr_addr].first = true;
    return;
}

void aclint_checker::process(const smc_read_pkt & r) {
    auto mmr_addr = static_cast<aclint_addr>(r.addr);
    if (aclint_mmrs.find(mmr_addr) == aclint_mmrs.end()) return;
    // Ignore read check if write happened after the read req is issued from smc
    if (mmrReadReqFlag[mmr_addr].first && mmrReadReqFlag[mmr_addr].second) {
        mmrReadReqFlag[mmr_addr] = {false, false};
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] Ignore SMC-AC READ: addr {:#x} data {:#x}, Data overwritten\n", r.addr, r.data);
        return;
    }
    mmrReadReqFlag[mmr_addr].first = false;     // Read carried without in between writes hence deasserting guard
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] SMC-AC READ: addr {:#x} data {:#x} size {:#x}B resp {}\n", r.addr, r.data, (1 << r.size), r.resp);
    if (r.size == 1 || r.size == 0) {
        if (r.data == 0 && r.resp == 3) return;
        else {
            cvm::log(cvm::ERROR, "Error: Non-zero data returned for 1B, 2B read, data {:#x} resp {}\n", r.data, r.resp);
            return;
        }
    }

    svSetScope(aclint_checker_scope_);
    
    size_t sz =  1 << r.size;
    uint64_t sz_mask = (sz == SZ_8B) ? ~uint64_t(0) : ((1ULL << (sz * 8)) - 1);
    uint64_t actual = r.data & sz_mask;

    uint64_t expected = aclint_mmrs[mmr_addr].read();

    if (mmr_addr == aclint_addr::AC_MTIME && FLAGS_aclint) {
        uint64_t mtime_expected = get_mtime_value() & sz_mask & aclint_mmrs[mmr_addr].read_mask;
        uint64_t mtime_actual = (actual & aclint_mmrs[mmr_addr].read_mask);
        if ((mtime_actual < mtime_expected) &&
            (mtime_actual >= (mtime_expected - 60))){
            cvm::log(cvm::HIGH, "[SMC-AC] ACLINT MMR match - Name = MTIME, Address = {:#x} - Actual: {:#x} Expected: {:#x}\n", aclint_mmrs[mmr_addr].address, mtime_actual, mtime_expected);
        } else {
            cvm::log(cvm::ERROR, "Error: [SMC-AC] Mismatch:- ACLINT MMR mismatch - Address = {:#x} - Actual: {:#x} Expected: {:#x}\n", aclint_mmrs[mmr_addr].address, mtime_actual, mtime_expected);
        }
    } 
    else if (mmr_addr == aclint_addr::CR_CTIME && FLAGS_aclint){
        // Get modelled copy of CTIME from aclint_checker.sv using DPI-C
        uint64_t ctime_model = get_ctime_value();
        uint64_t ctime_range_lower = (ctime_model < (ctime_model - 100)) ? 0 : (ctime_model - 100);             // Overflow on lower limit
        uint64_t ctime_range_upper = (ctime_model > (ctime_model + 100)) ? (-1) : (ctime_model + 100);          // Overflow of upper limit
        
        uint64_t ctime_expect = ctime_model & sz_mask & aclint_mmrs[mmr_addr].read_mask;
        uint64_t ctime_actual = (actual & aclint_mmrs[mmr_addr].read_mask);
        
        if ((r.data <= ctime_range_upper) && (r.data >= ctime_range_lower)) {
            cvm::log(cvm::HIGH, "[SMC-AC] ACLINT MMR match - Name = CTIME, Address = {:#x} - Actual: {:#x} Expected: {:#x}\n", aclint_mmrs[mmr_addr].address, ctime_actual, ctime_expect);
        } else {
            cvm::log(cvm::ERROR, "Error: [SMC-AC] Mismatch:- ACLINT MMR mismatch - Address = {:#x} - Actual: {:#x} Expected: {:#x}\n", aclint_mmrs[mmr_addr].address, ctime_actual, ctime_expect);
        }
        
    }
    else if (FLAGS_aclint) { 
        if ((actual & aclint_mmrs[mmr_addr].read_mask) != (expected & sz_mask & aclint_mmrs[mmr_addr].read_mask)) {
            cvm::log(cvm::ERROR, "Error: [SMC-AC] Mismatch:- ACLINT MMR mismatch - Address = {:#x} - Actual: {:#x} Expected: {:#x}\n", aclint_mmrs[mmr_addr].address, actual & aclint_mmrs[mmr_addr].write_mask, expected & sz_mask & aclint_mmrs[mmr_addr].write_mask);
        } else {
            cvm::log(cvm::HIGH, "[ACLINT CHECKER] ACLINT MMR match - Name = {}, Address = {:#x} - Actual: {:#x} Expected: {:#x}\n", aclint_mmrs[mmr_addr].name, aclint_mmrs[mmr_addr].address, actual & aclint_mmrs[mmr_addr].read_mask, expected & sz_mask & aclint_mmrs[mmr_addr].read_mask);
        }
    }
}

void aclint_checker::reset() {
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Reset \n");
}

void aclint_checker::popifpossible(uint64_t srcid) {
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Came in popifpossible srcid: {} \n", srcid);

    MmrWr & axi_ac_q_front = axi_ac_mmr_q_[srcid].front();
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: AC_AXI srcid {} addr {:#x} data {:#x} mask {:#x} \n", srcid, axi_ac_q_front.addr, axi_ac_q_front.data, axi_ac_q_front.mask);

    if (!cr_ac_mmr_q_[srcid].empty()) {
        MmrWr & cr_ac_q_front = cr_ac_mmr_q_[srcid].front();
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: CR_AC_MMR srcid {} addr {:#x} data {:#x} mask {:#x} \n", srcid, cr_ac_q_front.addr, cr_ac_q_front.data, cr_ac_q_front.mask);
        
        if (cr_ac_q_front == axi_ac_q_front) {
            cr_ac_mmr_q_[srcid].pop();
            axi_ac_mmr_q_[srcid].pop();
            cvm::log(cvm::HIGH, "[ACLINT CHECKER] CR_AC_MMR Popped \n");
        }
    }
    else if (!smc_ac_mmr_v_.empty()) {
        for (auto it = smc_ac_mmr_v_.begin(); it != smc_ac_mmr_v_.end(); ) {
            if (*it == axi_ac_q_front) {
                cvm::log(cvm::HIGH, "[ACLINT CHECKER] SMC_AC_MMR Popped\n");
                it = smc_ac_mmr_v_.erase(it); // Erase the element and update the iterator
            } else {
                ++it; // Move to the next element
            }
        }
    }
    return;
}

void aclint_checker::popifpossible(const MmrWr& m) {
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Came in popifpossible for SMC-AC Addr - {:#x} \n", m.addr);

    for (auto& [srcid, queue] : axi_ac_mmr_q_) {
        if (!queue.empty()) {
            MmrWr & axi_ac_q_front = queue.front();
            if (m == axi_ac_q_front){
                cvm::log(cvm::HIGH, "[ACLINT CHECKER] Match found: AC_AXI srcid {} addr {:#x} data {:#x} mask {:#x} \n", srcid, axi_ac_q_front.addr, axi_ac_q_front.data, axi_ac_q_front.mask);
                cvm::log(cvm::HIGH, "[ACLINT CHECKER] SMC_AC_MMR Popped\n");
                queue.pop();
                smc_ac_mmr_v_.pop_back();
                return;
            }
        }
    }
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

void aclint_checker::set_scope(svScope s) {
    aclint_checker_scope_ = s;
}

void aclint_checker::check_outstanding_transactions(uint64_t signal) {
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Checking for outstanding MMR writes...\n");
    if (!signal) return;
    for (const auto& [key, queue] : cr_ac_mmr_q_) {
        if (!queue.empty()) {
            cvm::log(cvm::ERROR, "Error: {} Outstanding CORE-ACLINT MMR writes for srcid {}\n", queue.size(), key);
            return; 
        }
    }
    cvm::log(cvm::MEDIUM, "[ACLINT CHECKER] No outstanding CORE-ACLINT MMR writes\n");
    
    if (!smc_ac_mmr_v_.empty()) {
        cvm::log(cvm::ERROR, "Error: {} Outstanding SMC-ACLINT MMR writes\n", smc_ac_mmr_v_.size());
        return; 
    }
    cvm::log(cvm::MEDIUM, "[ACLINT CHECKER] No outstanding SMC-ACLINT MMR writes\n");
}

extern "C" void check_outstanding_transactions(cvm::topology::loc_t loc) {
    cvm::registry::messenger.signal<uint64_t>(loc, uint64_t(1));
}

extern "C" void aclint_checker_scope(cvm::topology::loc_t loc) {
    cvm::log(cvm::HIGH, "Getting ACLINT CHECKER scope\n");
    svScope scope = svGetScope();
    cvm::registry::messenger.signal<svScope>(loc, scope);
}