#include "axi_sw_mst.h"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/bitmanip.hpp"
#include "cvm/logger.hpp"

REGISTRY_register((axi_sw_mst<rv_tester_transactions::axi_sw_mst::b<>,
                              rv_tester_transactions::axi_sw_mst::r<>,
                              rv_tester_transactions::axi_sw_mst::ar_q_ptr<>,
                              rv_tester_transactions::axi_sw_mst::aw_q_ptr<>,
                              rv_tester_transactions::axi_sw_mst::w_q_ptr<>>), AXI_MST, cvm::registry::all);

REGISTRY_register((axi_sw_mst<rv_tester_transactions::axi_sw_mst::b<1>,
                              rv_tester_transactions::axi_sw_mst::r<1>,
                              rv_tester_transactions::axi_sw_mst::ar_q_ptr<1>,
                              rv_tester_transactions::axi_sw_mst::aw_q_ptr<1>,
                              rv_tester_transactions::axi_sw_mst::w_q_ptr<1>>), AXI_MST2, cvm::registry::all);

extern "C" {
    void axi_sw_mst_ar_reset();
    void axi_sw_mst_aw_reset();
    void axi_sw_mst_w_reset();

    void axi_sw_mst_ar(uint32_t id, uint64_t addr, uint8_t len, uint8_t size, uint8_t burst, uint8_t lock);
    void axi_sw_mst_aw(uint32_t id, uint64_t addr, uint8_t len, uint8_t size, uint8_t burst, uint8_t atop, uint8_t lock);
    void axi_sw_mst_w_8(const uint8_t* data, const uint8_t* strb, uint8_t last);
    void axi_sw_mst_w_64(const uint8_t* data, const uint8_t* strb, uint8_t last);
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
axi_sw_mst<B, R, ARQ, AWQ, WQ>::axi_sw_mst(cvm::topology::loc_t loc, unsigned /*id*/)
    : scope_(nullptr), loc_(loc),
      id_width_(cvm::topology::attr(loc_, "ID_WIDTH").second),
      data_width_(cvm::topology::attr(loc_, "DATA_WIDTH").second),
      strb_width_(cvm::topology::attr(loc_, "STRB_WIDTH").second),
      ar_q_max_(cvm::topology::attr(loc_, "AR_Q_MAX").second),
      ar_q_ptr_max_(cvm::topology::attr(loc_, "AR_Q_PTR_MAX").second),
      aw_q_max_(cvm::topology::attr(loc_, "AW_Q_MAX").second),
      aw_q_ptr_max_(cvm::topology::attr(loc_, "AW_Q_PTR_MAX").second),
      w_q_max_(cvm::topology::attr(loc_, "W_Q_MAX").second),
      w_q_ptr_max_(cvm::topology::attr(loc_, "W_Q_PTR_MAX").second),
      ar_q_rptr_(0), ar_q_wptr_(ar_q_max_),
      aw_q_rptr_(0), aw_q_wptr_(aw_q_max_),
      w_q_rptr_(0), w_q_wptr_(w_q_max_),
      ids_(size_t(1) << id_width_, true)
{

    // available burst sizes
    uint32_t max_size = data_width_ >> 3;
    for (uint32_t i = 1; i <= max_size; i*=2)
      sizes_.push_back(i);

    cvm::registry::messenger.connect<svScope>(
        loc_,
        [&](svScope s) {
            this->set_scope(s);
            return this->reset_ptrs();
        });

    connect<B, R, ARQ, AWQ, WQ>();

    connect<
        axi::a_t,
        axi::w_t
    >();

    connect<
        transactor::read_request_t,
        transactor::write_request_t
    >();
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::process(const B& b) {
    if (b.resp != axi::RESP_OKAY or not used_id(b.id)) {
        // could have EXOKAY if it was locked, but assume not for now
        cvm::log(cvm::ERROR, "[AXI] bad b.response id:{} resp: {}\n", b.id, b.resp);
        return;
    }

    cvm::registry::messenger.signal<axi::b_t>(
        loc_,
        axi::b_t(b.id, axi::resp_t(b.resp))
    );

    free_id(b.id);
    push_transactions();
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::process(const R& r) {
    if (r.resp != axi::RESP_OKAY or not used_id(r.id)) {
        cvm::log(cvm::ERROR, "[AXI] bad r.response id: {} resp: {} last: {}\n", r.id, r.resp, r.last);
        return;
    }

    axi::data_t vdata = cvm::bitmanip::slice<decltype(r.data), axi::data_t>(r.data);

    cvm::registry::messenger.signal<axi::r_t>(
        loc_,
        axi::r_t(r.id, axi::resp_t(r.resp), vdata, r.last)
    );

    read_data_[r.id].insert(read_data_[r.id].end(), vdata.begin(), vdata.end());

    if (r.last) {
        cvm::registry::messenger.signal<transactor::read_response_t>(
            loc_,
            transactor::read_response_t{r.id, std::move(read_data_[r.id])});

        free_id(r.id);
        read_data_[r.id] = {};
    }
    push_transactions();
}


template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::process(const ARQ& ar_q_ptr) {
    ar_q_rptr_ = ar_q_ptr.ar_ptr;
    push_transactions();
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::process(const AWQ& aw_q_ptr) {
    aw_q_rptr_ = aw_q_ptr.aw_ptr;
    push_transactions();
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::process(const WQ& w_q_ptr) {
    w_q_rptr_ = w_q_ptr.w_ptr;
    push_transactions();
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::process(const axi::a_t& a) {
    transactions_.emplace_back(a);
    push_transactions();
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::process(const axi::w_t& w) {
    transactions_.emplace_back(w);
    push_transactions();
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
bool
axi_sw_mst<B, R, ARQ, AWQ, WQ>::a_wrapper(uint64_t req_addr, size_t req_length, axi::a_t& a) {

    a.addr = req_addr;
    a.burst = axi::BURST_INCR; a.atop = false; a.lock = false;

    if (!next_id(a.id)) {
        cvm::log(cvm::ERROR, "No free id's remaining for axi master\n");
        return false;
    }

    if ((a.addr & axi::addr_t(~0xFFF)) != ((a.addr + req_length) & axi::addr_t(~0xFFF))) {
        cvm::log(cvm::ERROR, "Request crosses 4k boundary, addr: {}, length: {}\n", req_addr, req_length);
        return false;
    }

    // this can be optimized but for now, assume if doesn't fit in single burst.len == 1, then
    // use byte stream
    auto it = std::find(sizes_.begin(), sizes_.end(), req_length);
    a.len = (it != sizes_.end()) ? 0 : req_length - 1;
    a.size = (it != sizes_.end()) ? __builtin_ctz(req_length) : 0;

    return true;
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::push_transactions() {
  while (!transactions_.empty()) {
      auto& req = transactions_.front();
      std::visit([this](auto&& arg) {
          using T = std::decay_t<decltype(arg)>;

          if constexpr (std::is_same_v<T, axi::a_t>) {
              bool write = arg.w;

              if (!write) {
                  if ((ar_q_wptr_ - ar_q_rptr_ ) < ar_q_max_) {
                      ar_q_wptr_ = (ar_q_wptr_ + 1) % ar_q_ptr_max_;
                      cvm::registry::callbacks.push(
                        scope_,
                        [=]() { axi_sw_mst_ar(arg.id, arg.addr, arg.len, arg.size, arg.burst, arg.lock); });
                  }
                  else
                      return;
              }
              else {
                  if ((aw_q_wptr_ - aw_q_rptr_ ) < aw_q_max_) {
                      aw_q_wptr_ = (aw_q_wptr_ + 1) % aw_q_ptr_max_;
                      cvm::registry::callbacks.push(
                        scope_,
                        [=]() { axi_sw_mst_aw(arg.id, arg.addr, arg.len, arg.size, arg.burst, arg.atop.transaction, arg.lock); });
                  }
                  else
                      return;
              }
          }
          else if constexpr (std::is_same_v<T, axi::w_t>) {
              if ((w_q_wptr_ - w_q_rptr_ ) < w_q_max_) {
                  w_q_wptr_ = (w_q_wptr_ + 1) % w_q_ptr_max_;

                  if (arg.strb.size() == 0) {
                      cvm::log(cvm::ERROR, "strb size is 0\n");
                      return;
                  }

                  if (arg.strb.size() != arg.data.size()) {
                      cvm::log(cvm::ERROR, "strb size != data size\n");
                      return;
                  }

                  cvm::registry::callbacks.push(
                      scope_,
                      [=]() {
                          std::vector<uint8_t> strb(((arg.strb.size() - 1) >> 3) + 1, 0);
                          for (size_t i = 0; i < arg.strb.size(); i++) {
                              size_t idx = i >> 3;
                              strb[idx] |= arg.strb[i] << i%8;
                          }

                          if (data_width_ == 64)
                              axi_sw_mst_w_8(arg.data.data(), strb.data(), arg.last);
                          else if (data_width_ == 512)
                              axi_sw_mst_w_64(arg.data.data(), strb.data(), arg.last);
                          else
                              cvm::log(cvm::ERROR, "unsupported data width for axi_sw_mst");
                      });
              }
          }
          else {
              cvm::log(cvm::ERROR, "unhandled axi_mst transaction type\n");
              return;
          }

      }, req);
      transactions_.pop_front();
  }
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::process(const transactor::read_request_t& req) {
    axi::a_t a{ .w = false };

    if (!a_wrapper(req.addr, req.length, a))
        return;

    transactions_.emplace_back(a);
    push_transactions();
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::process(const transactor::write_request_t& req) {
    axi::a_t a{ .w = true };

    if (!a_wrapper(req.addr, req.length, a))
        return;

    transactions_.emplace_back(a);

    size_t pow2size = size_t(1) << a.size;
    for (int32_t i = a.len; i >= 0; i--) {
        axi::data_t data(data_width_ >> 3);
        axi::strb_t strb(strb_width_);

        data.assign(req.data.begin() + pow2size*a.len, req.data.begin() + pow2size*a.len + pow2size);
        strb.assign(req.strb.begin() + pow2size*a.len, req.strb.begin() + pow2size*a.len + pow2size);
        transactions_.emplace_back(axi::w_t{std::move(data), std::move(strb), i == 0});
    }
    push_transactions();
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::set_scope(svScope scope) {
    scope_ = scope;
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::reset_ptrs() {

    ar_q_wptr_ = 0;
    aw_q_wptr_ = 0;
    w_q_wptr_ = 0;

}

extern "C" {

  void axi_sw_mst_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    axi_sw_mst_ar_reset();
    axi_sw_mst_aw_reset();
    axi_sw_mst_w_reset();

    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

}
