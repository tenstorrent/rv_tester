#include "axi_sw.h"
#include "cvm/topology.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/bitmanip.hpp"
#include "cvm/logger.hpp"

REGISTRY_register(axi_sw, AXI, cvm::registry::all);

extern "C" {

  void axi_sw_r(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last);
}

axi_sw::axi_sw(cvm::topology::loc_t loc, unsigned id)
  : scope_(nullptr), loc_(loc), r_q_rptr_(0), r_q_wptr_(0),
    r_q_max_(cvm::topology::attr(loc, "R_Q_MAX").second), r_q_ptr_max_(cvm::topology::attr(loc, "R_Q_PTR_MAX").second) {

    auto data_width = cvm::topology::attr(loc, "DATA_WIDTH").second;
    axi_ = new axi(data_width, loc, "axi" + std::to_string(id));

    cvm::registry::messenger.connect<svScope>(
        loc_,
        [&](svScope s) { return this->set_scope(s); });

    connect<
      rv_tester_transactions::aw,
      rv_tester_transactions::ar,
      rv_tester_transactions::w,
      rv_tester_transactions::r_q_ptr>();
}

axi_sw::~axi_sw() {
    if (axi_) {
        delete axi_;
        axi_ = nullptr;
    }
}

void axi_sw::process(const rv_tester_transactions::aw& aw) {
    cvm::log(cvm::FULL, "<{}> [axi_sw] aw: [id={}, addr={:#x}, size={}]\n", aw.cycle, aw.id, aw.addr, aw.size);
    a(axi::a_t{true, aw.id, aw.addr, aw.len, aw.size, axi::burst_t(aw.burst), aw.lock != 0, aw.atop});
    r_resp(aw.cycle);
}

void axi_sw::process(const rv_tester_transactions::ar& ar) {
    cvm::log(cvm::FULL, "<{}> [axi_sw] ar: [id={}, addr={:#x}, size={}]\n", ar.cycle, ar.id, ar.addr, ar.size);
    a(axi::a_t{false, ar.id, ar.addr, ar.len, ar.size, axi::burst_t(ar.burst), ar.lock != 0});
    r_resp(ar.cycle);
}

template <typename T>
static uint32_t slice_wrap(const T& val, size_t msb, size_t lsb) {
    if constexpr (cvm::bitmanip::is_bitset_v<T>)
      return cvm::bitmanip::slice(val, msb, lsb).to_ulong();
    else
      return cvm::bitmanip::slice(val, msb, lsb);
}

void axi_sw::process(const rv_tester_transactions::w& w) {
    cvm::log(cvm::FULL, "<{}> [axi_sw] w: [strb={:#x}, last={}]\n", w.cycle, w.strb, w.last);
    axi::data_t vdata(data_width()/8, 0);
    axi::strb_t vstrb(strobe_width(), false);

    for (std::size_t i = 0; i < vdata.size(); i++)
      vdata[i] = slice_wrap(w.data, (i+1)*(8*sizeof(axi::data_t::value_type)) - 1, i*(8*sizeof(axi::data_t::value_type)));

    for (std::size_t i = 0; i < vstrb.size(); i++)
      vstrb[i] = slice_wrap(w.strb, i, i);

    this->w(axi::w_t(
            vdata,
            vstrb,
            w.last
            )
    );
}

void axi_sw::process(const rv_tester_transactions::r_q_ptr& r_q_ptr) {
    cvm::log(cvm::FULL, "<{}> [axi_sw] r_q_ptr: [rptr={}]\n", r_q_ptr.cycle, r_q_ptr.r_ptr);
    r_q_rptr(r_q_ptr.r_ptr);
    r_resp(r_q_ptr.cycle);
}

void axi_sw::r_resp(uint64_t cycle) {
    std::unique_lock<std::mutex> lock(r_q_rptr_m_);
    while ( (r_q_wptr_ - r_q_rptr_) < r_q_max_ ) {
      auto [valid, result] = axi_->r(false);
      cvm::log(cvm::FULL, "<{}> [axi_sw] r_resp: [r_q dequeue valid={}]\n", cycle, valid);
      if (!valid)
        break;
      r_q_wptr_ = (r_q_wptr_ + 1) % r_q_ptr_max_;

      // clang doesn't like structured bindings in a capture list
      auto copy = result;
      cvm::registry::callbacks.push(
          scope_,
          [copy]() { axi_sw_r(copy.id, copy.resp, copy.data.data(), copy.last); }
      );
    }
}

void axi_sw::set_scope(svScope scope) {
    scope_ = scope;
}

void axi_sw::r_q_rptr(const r_q_ptr_t& r_q_rptr) {
    std::lock_guard<std::mutex> lock(r_q_rptr_m_);
    r_q_rptr_ = r_q_rptr;
    r_q_rptr_c_.notify_one();
}

extern "C" {

  void axi_sw_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();
    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }
}
