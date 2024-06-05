#include "axi_sw.h"
#include "cvm/topology.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/bitmanip.hpp"
#include "cvm/logger.hpp"

REGISTRY_register((axi_sw<rv_tester_transactions::axi_sw::w<>,
                         rv_tester_transactions::axi_sw::aw<>,
                         rv_tester_transactions::axi_sw::ar<>,
                         rv_tester_transactions::axi_sw::r_q_ptr<>>), AXI, cvm::registry::all);

REGISTRY_register((axi_sw<rv_tester_transactions::axi_sw::w<1>,
                         rv_tester_transactions::axi_sw::aw<1>,
                         rv_tester_transactions::axi_sw::ar<1>,
                         rv_tester_transactions::axi_sw::r_q_ptr<1>>), NCIO_AXI, cvm::registry::all);



extern "C" {

  void axi_sw_r_reset();
  void axi_sw_r_8(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last);
  void axi_sw_r_64(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last);
}



template < typename W,typename AW,typename AR, typename RQ>
axi_sw<W,AW,AR,RQ>::axi_sw(cvm::topology::loc_t loc, unsigned id)
  : scope_(nullptr), loc_(loc),
    id_width_(cvm::topology::attr(loc_, "ID_WIDTH").second),
    data_width_(cvm::topology::attr(loc_, "DATA_WIDTH").second),
    strb_width_(cvm::topology::attr(loc_, "STRB_WIDTH").second),
    r_q_max_(cvm::topology::attr(loc, "R_Q_MAX").second), r_q_ptr_max_(cvm::topology::attr(loc, "R_Q_PTR_MAX").second),
    r_q_rptr_(0), r_q_wptr_(r_q_max_) 

    {
    cvm::log(cvm::FULL, "[axi_sw] Constructing axi_sw for loc={} id={}\n", loc,id);
    auto data_width = cvm::topology::attr(loc, "DATA_WIDTH").second;
    axi_ = new axi(data_width, loc, "axi" + std::to_string(id));
    cvm::registry::messenger.connect<svScope>(
        loc_,
        [this](svScope s) {
        this->set_scope(s);
        return reset_ptrs();
    });

   connect_task<W,AW,AR>();

    connect<RQ>();
}

template < typename W,typename AW,typename AR, typename RQ>
axi_sw<W,AW,AR,RQ>::~axi_sw() {
    if (axi_) {
        delete axi_;
        axi_ = nullptr;
    }
}

template < typename W,typename AW,typename AR, typename RQ>
cvm::messenger::task<void> axi_sw<W,AW,AR,RQ>::process(const AW& aw) {
    cvm::log(cvm::FULL, "[axi_sw] aw: [id={}, addr={:#x}, size={}]\n", aw.id, aw.addr, aw.size);
    co_await a(axi::a_t{true, aw.id, aw.addr, aw.len, aw.size, axi::burst_t(aw.burst), aw.lock != 0, aw.atop});
    r_resp();
    co_return;
}

template < typename W,typename AW,typename AR, typename RQ>
cvm::messenger::task<void> axi_sw<W,AW,AR,RQ>::process(const AR& ar) {
    cvm::log(cvm::FULL, "[axi_sw] ar: [id={}, addr={:#x}, size={}]\n", ar.id, ar.addr, ar.size);
    co_await a(axi::a_t{false, ar.id, ar.addr, ar.len, ar.size, axi::burst_t(ar.burst), ar.lock != 0});
    r_resp();
    co_return;
}

template < typename W,typename AW,typename AR, typename RQ>
cvm::messenger::task<void> axi_sw<W,AW,AR,RQ>::process(const W& w) {
    cvm::log(cvm::FULL, "[axi_sw] w: [strb={:#x}, last={}]\n", w.strb, w.last);
    axi::data_t vdata = cvm::bitmanip::slice<decltype(w.data), axi::data_t>(w.data);
    axi::strb_t vstrb = cvm::bitmanip::slice<decltype(w.strb), axi::strb_t>(w.strb);

    co_await this->w(axi::w_t(
                      vdata,
                      vstrb,
                      w.last
                      )
    );
    r_resp();
    co_return;
}

template < typename W,typename AW,typename AR, typename RQ>
void axi_sw<W,AW,AR,RQ>::process(const RQ& r_q_ptr) {
    cvm::log(cvm::FULL, "[axi_sw] r_q_ptr: [rptr={}]\n", r_q_ptr.r_ptr);
    r_q_rptr_ = r_q_ptr.r_ptr;
    r_resp();
}

template < typename W,typename AW,typename AR, typename RQ>
void axi_sw<W,AW,AR,RQ>::r_resp() {
    while ( (r_q_wptr_ - r_q_rptr_) < r_q_max_ ) {
      auto [valid, result] = axi_->r(false);
      cvm::log(cvm::FULL, "[axi_sw] r_resp: [r_q dequeue valid={} wptr={} rptr={}]\n", valid, r_q_wptr_, r_q_rptr_);
      if (!valid)
        break;
      r_q_wptr_ = (r_q_wptr_ + 1) % r_q_ptr_max_;

      // clang doesn't like structured bindings in a capture list
      auto copy = result;
      cvm::registry::callbacks.push(
          scope_,
            [=]() {
            std::string d;
            for (int i=0; i<int(data_width_/8); i++)
                d += fmt::format("{:02x}", copy.data[i]);
            cvm::log(cvm::FULL, "[axi_sw] axi_sw_r_{}: id={}, last={}, data={}\n", data_width_/8, copy.id, copy.last, d);

            if(data_width_ == 64)
            axi_sw_r_8(copy.id, copy.resp, copy.data.data(), copy.last);
            else if(data_width_ ==512)
            axi_sw_r_64(copy.id, copy.resp, copy.data.data(), copy.last);
            else
            cvm::log(cvm::ERROR, "unsupported data width for axi_sw");

            }
      );
    }
}

template < typename W,typename AW,typename AR, typename RQ>
void axi_sw<W,AW,AR,RQ>::reset_ptrs() {
    r_q_rptr_ = 0;
    r_q_wptr_ = 0;
}

template < typename W,typename AW,typename AR, typename RQ>
void axi_sw<W,AW,AR,RQ>::set_scope(svScope scope) {
    scope_ = scope;
}

extern "C" {

  void axi_sw_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    axi_sw_r_reset();

    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

}
