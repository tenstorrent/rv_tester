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
      rv_tester_transactions::axi_sw::aw,
      rv_tester_transactions::axi_sw::ar,
      rv_tester_transactions::axi_sw::w,
      rv_tester_transactions::axi_sw::r_q_ptr>();
}

axi_sw::~axi_sw() {
    if (axi_) {
        delete axi_;
        axi_ = nullptr;
    }
}

void axi_sw::process(const rv_tester_transactions::axi_sw::aw& aw) {
    cvm::log(cvm::FULL, "[axi_sw] aw: [id={}, addr={:#x}, size={}]\n", aw.id, aw.addr, aw.size);
    a(axi::a_t{true, aw.id, aw.addr, aw.len, aw.size, axi::burst_t(aw.burst), aw.lock != 0, aw.atop});
    r_resp();
}

void axi_sw::process(const rv_tester_transactions::axi_sw::ar& ar) {
    cvm::log(cvm::FULL, "[axi_sw] ar: [id={}, addr={:#x}, size={}]\n", ar.id, ar.addr, ar.size);
    a(axi::a_t{false, ar.id, ar.addr, ar.len, ar.size, axi::burst_t(ar.burst), ar.lock != 0});
    r_resp();
}

void axi_sw::process(const rv_tester_transactions::axi_sw::w& w) {
    cvm::log(cvm::FULL, "[axi_sw] w: [strb={:#x}, last={}]\n", w.strb, w.last);

    axi::data_t vdata = cvm::bitmanip::slice<decltype(w.data), axi::data_t>(w.data);
    axi::strb_t vstrb = cvm::bitmanip::slice<decltype(w.strb), axi::strb_t>(w.strb);

    this->w(axi::w_t(
            vdata,
            vstrb,
            w.last
            )
    );
    r_resp();
}

void axi_sw::process(const rv_tester_transactions::axi_sw::r_q_ptr& r_q_ptr) {
    cvm::log(cvm::FULL, "[axi_sw] r_q_ptr: [rptr={}]\n", r_q_ptr.r_ptr);
    r_q_rptr_ = r_q_ptr.r_ptr;
    r_resp();
}

void axi_sw::r_resp() {
    while ( (r_q_wptr_ - r_q_rptr_) < r_q_max_ ) {
      auto [valid, result] = axi_->r(false);
      cvm::log(cvm::FULL, "[axi_sw] r_resp: [r_q dequeue valid={}]\n", valid);
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

extern "C" {

  void axi_sw_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();
    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

}
