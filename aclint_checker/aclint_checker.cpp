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
#include "cvm/logger.hpp"
#include <cassert>

REGISTRY_register(aclint_checker, TOP.PLATFORM.ACLINT_CHECKER, 0);
DEFINE_bool(aclint, false, "Enable aclint checks");

extern "C" {
    uint64_t get_mtime_value();
    uint64_t get_ctime_value();
    int get_hart_enable_ids_from_plusargs(int* result, const char* plusargs_name, int NHARTS);
}

aclint_checker::aclint_checker(cvm::topology::loc_t loc, unsigned) {

    cvm::registry::messenger.connect < rv_tester_transactions::aclint_checker::cr_ac_mmrwrite < >> (loc, [this](const auto & v) {
        return this -> process(v);
    });
    cvm::registry::messenger.connect < rv_tester_transactions::aclint_checker::axi_ac_write < >> (loc, [this](const auto & v) {
        return this -> process(v);
    });
    cvm::registry::messenger.connect < rv_tester_transactions::aclint_checker::mtip_check < >> (loc, [this](const auto & v) {
        return this -> process(v);
    });
    cvm::registry::messenger.connect < rv_tester_transactions::aclint_checker::timesync_check < >> (loc, [this](const auto & v) {
        return this -> process(v);
    });
    cvm::registry::messenger.connect < rv_tester_transactions::aclint_checker::time_mtime_synch_check < >> (loc, [this](const auto & v) {
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
    cvm::registry::messenger.connect <clear_outstanding_txn>(loc, [this](const auto& signal) {
        return this -> clear_core_outstanding_transactions(signal);
    });
    cvm::registry::messenger.connect<svScope>(loc, [this](svScope s) {
        return this->set_scope(s);
    });
    reset();
}

void aclint_checker::process(const rv_tester_transactions::aclint_checker::mtip_check < > & mtip_check) {
    if (mtip_check.check_1) {
        cvm::log(cvm::ERROR, "Error: [{}] Did not expect MTIP, but MTIP[{}] generated\n", mtip_check.clock, mtip_check.hart);
        return;
    }

    if (mtip_check.check_2) {
        cvm::log(cvm::ERROR, "Error: [{}] Expected MTIP, but MTIP[{}] not generated\n", mtip_check.clock, mtip_check.hart);
        return;
    }
}

void aclint_checker::process(const rv_tester_transactions::aclint_checker::timesync_check < > & timesync_check) {
    cvm::log(cvm::ERROR, "Error: [{}] Expected TIMESYNC, but TIMESYNC[{}] not received\n", timesync_check.clock, timesync_check.hart);
    return;
}

void aclint_checker::process(const rv_tester_transactions::aclint_checker::time_mtime_synch_check < > & time_mtime_synch_check) {
    cvm::log(cvm::ERROR, "Error: [{}] Hart[{}] Time and Mtime out of sync, mtime_data {:#x} ctime_data {:#x} \n", time_mtime_synch_check.clock, time_mtime_synch_check.hart, time_mtime_synch_check.mtime_data, time_mtime_synch_check.ctime_data);
    return;
}

void aclint_checker::process(const rv_tester_transactions::aclint_checker::cr_ac_mmrwrite < > & cr_ac_mmrwrite) {
    MmrWr m;
    
    m.addr = cr_ac_mmrwrite.addr;
    m.mask = cr_ac_mmrwrite.mask;
    size_t sz = (cr_ac_mmrwrite.mask == 0xFF) ?  3 :
                (cr_ac_mmrwrite.mask == 0x0F) ?  2 :
                (cr_ac_mmrwrite.mask == 0x03) ?  1 : 0;
    uint64_t sz_mask = (sz == 3) ? ~uint64_t(0) : ((uint64_t)1 << ((1<<sz)*8)) - 1;
    m.data = cr_ac_mmrwrite.data & sz_mask;
    m.datavalid = false;

    cvm::log(cvm::HIGH, "[ACLINT CHECKER] AC MMR WRITES: location {} addr {:#x} data {:#x} mask {:#x} \n", cr_ac_mmrwrite.location, cr_ac_mmrwrite.addr, cr_ac_mmrwrite.data, cr_ac_mmrwrite.mask);

    auto mmr_addr = static_cast<aclint_addr>(cr_ac_mmrwrite.addr);
    if (mmr_addr == aclint_addr::AC_TIMESYNC || mmr_addr == aclint_addr::AC_CLUSTERFUSE) return;
    if (aclint_mmrs.find(mmr_addr) != aclint_mmrs.end()) {
        cr_ac_mmr_v_.insert(cr_ac_mmr_v_.begin(), m);
        popifpossible(txn_src::CR_AC);
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

        aclint_mmrs[mmr_addr].write(axi_ac_write.data, sz);
        
        if (user == 3) {
            // Transaction from SMC
            axi_ac_smc_mmr_v_.insert(axi_ac_smc_mmr_v_.begin(), m);
            popifpossible(txn_src::AXI_SMC_AC);
        }
        else if (srcid == 0b0001) {
            // Transaction from SCB (i.e. Core)
            axi_ac_cr_mmr_v_.insert(axi_ac_cr_mmr_v_.begin(), m);
            popifpossible(txn_src::AXI_CR_AC);
        }

        if (mmr_addr == aclint_addr::AC_CLUSTERFUSE && (aclint_mmrs[mmr_addr].lock_bit == 0) &&
            (((aclint_mmrs[mmr_addr].data >> 63) & 1) == 1)){
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
    if (mmrReqFlags[mmr_addr].writes) --mmrReqFlags[mmr_addr].writes;
    if (!(w.addr == aclint_addr::CR_CTIME || w.addr == aclint_addr::CR_WTIME)) {
        smc_ac_mmr_v_.insert(smc_ac_mmr_v_.begin(), m);
        popifpossible(txn_src::SMC_AC);
    }
    else aclint_mmrs[mmr_addr].write(w.data, w.size);
}

void aclint_checker::process(const smc_req_pkt & req_pkt) {
    if (!FLAGS_aclint) return;
    auto mmr_addr = static_cast<aclint_addr>(req_pkt.addr);
    if (aclint_mmrs.find(mmr_addr) == aclint_mmrs.end()) return;

    if (req_pkt.req_type == smc_txn_type::SMC_READ) {
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] SMC-AC READ REQ: addr {:#x}\n", req_pkt.addr);
        // If read req is true, check for any writes when true, if write happens then set the second to true
        ++mmrReqFlags[mmr_addr].reads;
        return;
    }
    else {
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] SMC-AC WRITE REQ: addr {:#x}\n", req_pkt.addr);
        ++mmrReqFlags[mmr_addr].writes;
    }
}

void aclint_checker::process(const smc_read_pkt & r) {
    auto mmr_addr = static_cast<aclint_addr>(r.addr);
    if (aclint_mmrs.find(mmr_addr) == aclint_mmrs.end()) return;
    // Ignore read check if write happened after the read req is issued from smc
    if (mmrReqFlags[mmr_addr].reads && mmrReqFlags[mmr_addr].writes) {
        --mmrReqFlags[mmr_addr].reads;
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] Ignore SMC-AC READ: addr {:#x} data {:#x}, Data overwritten\n", r.addr, r.data);
        return;
    }
    if (mmrReqFlags[mmr_addr].reads) --mmrReqFlags[mmr_addr].reads;
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
        uint64_t mtime_expected_lower = (mtime_expected - 80);
        if (in_range(mtime_actual, mtime_expected_lower, mtime_expected)){
            if (mmrReqFlags[mmr_addr].reads) mmrReqFlags[mmr_addr].reads = 0;
            cvm::log(cvm::HIGH, "[SMC-AC] ACLINT MMR match - Name = MTIME, Address = {:#x} - Actual: {:#x} Expected: {:#x}\n", aclint_mmrs[mmr_addr].address, mtime_actual, mtime_expected);
        } else {
            if (mmrReqFlags[mmr_addr].reads) return;    // Ignoring mismatch as read req was issued before data was overwritten.
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
        if (((actual & aclint_mmrs[mmr_addr].read_mask) != (expected & sz_mask & aclint_mmrs[mmr_addr].read_mask)) 
           && ((actual & aclint_mmrs[mmr_addr].read_mask) != (aclint_mmrs[mmr_addr].read_prev() & sz_mask & aclint_mmrs[mmr_addr].read_mask))) {

            cvm::log(cvm::ERROR, "Error: [SMC-AC] Mismatch:- ACLINT MMR mismatch - Address = {:#x} - Actual: {:#x} Expected: {:#x}\n", aclint_mmrs[mmr_addr].address, actual & aclint_mmrs[mmr_addr].write_mask, expected & sz_mask & aclint_mmrs[mmr_addr].write_mask);
        } else {
            cvm::log(cvm::HIGH, "[ACLINT CHECKER] ACLINT MMR match - Name = {}, Address = {:#x} - Actual: {:#x} Expected: {:#x}\n", aclint_mmrs[mmr_addr].name, aclint_mmrs[mmr_addr].address, actual & aclint_mmrs[mmr_addr].read_mask, expected & sz_mask & aclint_mmrs[mmr_addr].read_mask);
        }
    }
}

void aclint_checker::reset() {
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Reset \n");
}

void aclint_checker::popifpossible(txn_src v_type) {
    if (v_type == txn_src::AXI_CR_AC) {
        MmrWr &axi_ac_cr_v_front = axi_ac_cr_mmr_v_.front();
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: AC_AXI for Core txn addr {:#x} data {:#x} mask {:#x}\n",
                 axi_ac_cr_v_front.addr, axi_ac_cr_v_front.data, axi_ac_cr_v_front.mask);
        
        auto it = std::find(cr_ac_mmr_v_.begin(), cr_ac_mmr_v_.end(), axi_ac_cr_v_front);
        if (it != cr_ac_mmr_v_.end()) {
            cvm::log(cvm::HIGH, "[ACLINT CHECKER] Popped: CR_AC_MMR addr {:#x} data {:#x} mask {:#x}\n",
                     it->addr, it->data, it->mask);
            cr_ac_mmr_v_.erase(it);
            axi_ac_cr_mmr_v_.erase(axi_ac_cr_mmr_v_.begin());
            return;
        }
    }
    else if (v_type == txn_src::AXI_SMC_AC) {
        MmrWr &axi_ac_smc_v_front = axi_ac_smc_mmr_v_.front();
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: AC_AXI for SMC txn addr {:#x} data {:#x} mask {:#x}\n",
                 axi_ac_smc_v_front.addr, axi_ac_smc_v_front.data, axi_ac_smc_v_front.mask);
        
        auto it = std::find(smc_ac_mmr_v_.begin(), smc_ac_mmr_v_.end(), axi_ac_smc_v_front);
        if (it != smc_ac_mmr_v_.end()) {
            cvm::log(cvm::HIGH, "[ACLINT CHECKER] Popped: SMC_AC_MMR addr {:#x} data {:#x} mask {:#x}\n",
                     it->addr, it->data, it->mask);
            smc_ac_mmr_v_.erase(it);
            axi_ac_smc_mmr_v_.erase(axi_ac_smc_mmr_v_.begin());
            return;
        }
    }
    else if (v_type == txn_src::CR_AC) {
        MmrWr &cr_ac_v_front = cr_ac_mmr_v_.front();
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: CR_AC addr {:#x} data {:#x} mask {:#x}\n",
                 cr_ac_v_front.addr, cr_ac_v_front.data, cr_ac_v_front.mask);
        
        auto it = std::find(axi_ac_cr_mmr_v_.begin(), axi_ac_cr_mmr_v_.end(), cr_ac_v_front);
        if (it != axi_ac_cr_mmr_v_.end()) {
            cvm::log(cvm::HIGH, "[ACLINT CHECKER] Popped: AC_AXI for core txn addr {:#x} data {:#x} mask {:#x}\n",
                     it->addr, it->data, it->mask);
            axi_ac_cr_mmr_v_.erase(it);
            cr_ac_mmr_v_.erase(cr_ac_mmr_v_.begin());
            return;
        }
    }
    else if (v_type == txn_src::SMC_AC) {
        MmrWr &smc_ac_v_front = smc_ac_mmr_v_.front();
        cvm::log(cvm::HIGH, "[ACLINT CHECKER] Pop if possible: SMC_AC addr {:#x} data {:#x} mask {:#x}\n",
                 smc_ac_v_front.addr, smc_ac_v_front.data, smc_ac_v_front.mask);
        
        auto it = std::find(axi_ac_smc_mmr_v_.begin(), axi_ac_smc_mmr_v_.end(), smc_ac_v_front);
        if (it != axi_ac_smc_mmr_v_.end()) {
            cvm::log(cvm::HIGH, "[ACLINT CHECKER] Popped: AC_AXI for SMC txn addr {:#x} data {:#x} mask {:#x}\n",
                     it->addr, it->data, it->mask);
            axi_ac_smc_mmr_v_.erase(it);
            smc_ac_mmr_v_.erase(smc_ac_mmr_v_.begin());
            return;
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

inline bool in_range(uint64_t v, uint64_t a, uint64_t b) {
    if (a <= b) {
        // straight range: [a..b]
        return v >= a && v <= b;
    } else {
        // wrapped range: [a..2^64–1] ∪ [0..b]
        return v >= a || v <= b;
    }
}

void aclint_checker::set_scope(svScope s) {
    aclint_checker_scope_ = s;
}

void aclint_checker::check_outstanding_transactions(uint64_t clocks) {
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] Checking for outstanding MMR writes...\n");
    
    std::string error_str = "";
    if (!cr_ac_mmr_v_.empty()) {
        error_str += fmt::format("[{}] Error: {} Outstanding CORE-ACLINT MMR writes\n", clocks, cr_ac_mmr_v_.size());
    } else {
        cvm::log(cvm::MEDIUM, "[ACLINT CHECKER] No outstanding CORE-ACLINT MMR writes\n");
    }

    if (!smc_ac_mmr_v_.empty()) {
        error_str += fmt::format("[{}] Error: {} Outstanding SMC-ACLINT MMR writes\n", clocks, smc_ac_mmr_v_.size());
    } else {
        cvm::log(cvm::MEDIUM, "[ACLINT CHECKER] No outstanding SMC-ACLINT MMR writes\n");
    }

    if (!error_str.empty()) {
        cvm::log(cvm::ERROR, "{}", error_str);
    }
}

void aclint_checker::clear_core_outstanding_transactions(const clear_outstanding_txn& signal_pkt) {
    cvm::log(cvm::HIGH, "[ACLINT CHECKER] warm_reset de-asserted, Clearing for outstanding MMR writes...\n");
    if (!signal_pkt.signal) return;

    cr_ac_mmr_v_.clear();
    axi_ac_cr_mmr_v_.clear();

}

extern "C" void check_outstanding_transactions(cvm::topology::loc_t loc, uint64_t clocks) {
    cvm::registry::messenger.signal<uint64_t>(loc, clocks);
}

extern "C" void time_mtime_eot_error() {
    cvm::log(cvm::ERROR, "[ACLINT CHECKER] Error: No mtime broadcast after CLCX exit\n");
}

extern "C" void clear_core_outstanding_transactions(cvm::topology::loc_t loc) {
    cvm::registry::messenger.signal<clear_outstanding_txn>(loc, {uint64_t(1)});
}

extern "C" void aclint_checker_scope(cvm::topology::loc_t loc) {
    cvm::log(cvm::HIGH, "Getting ACLINT CHECKER scope\n");
    svScope scope = svGetScope();
    cvm::registry::messenger.signal<svScope>(loc, scope);
}

extern "C" int get_hart_enable_ids_from_plusargs(int* result, const char* plusargs_name, int NHARTS) {
    std::string hart_enable_ids = cvm_plusargs_get_string(plusargs_name);
    std::vector<uint32_t> numbers;
    std::istringstream ss(hart_enable_ids);
    std::string token;
    while (std::getline(ss, token, ',')) {
      if (token != "") {
        uint32_t t = std::stoull(token);
        numbers.push_back(t);
      }
    }

    if ((int)numbers.size() > NHARTS) {
        cvm::log(cvm::ERROR, "Error: {} hart enable ids provided, but only {} harts are supported\n", numbers.size(), NHARTS);
        assert(false);
        return 0;
    }

    for (size_t i = 0; i < numbers.size(); ++i) {
        if ((int)i < NHARTS) {
            result[i] = numbers[i];
        }
    }
    return numbers.size();
}
