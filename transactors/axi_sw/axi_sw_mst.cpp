#include "axi_sw_mst.h"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/bitmanip.hpp"
#include "cvm/logger.hpp"
#include "rv_tester/rv_tester_plusargs.h"

#include <typeinfo>

REGISTRY_register((axi_sw_mst<rv_tester_transactions::axi_sw_mst::b<>,
                              rv_tester_transactions::axi_sw_mst::r<>,
                              rv_tester_transactions::axi_sw_mst::ar_q_ptr<>,
                              rv_tester_transactions::axi_sw_mst::aw_q_ptr<>,
                              rv_tester_transactions::axi_sw_mst::w_q_ptr<>>), AXI_MST, cvm::registry::all);

REGISTRY_register((axi_sw_mst<rv_tester_transactions::axi_sw_mst::b<1>,
                              rv_tester_transactions::axi_sw_mst::r<1>,
                              rv_tester_transactions::axi_sw_mst::ar_q_ptr<1>,
                              rv_tester_transactions::axi_sw_mst::aw_q_ptr<1>,
                              rv_tester_transactions::axi_sw_mst::w_q_ptr<1>>), SMC_AXI_MST, cvm::registry::all);

DEFINE_bool(axi_allow_err_resp, false, "Allow error responses on axi_mst transactions");
DEFINE_bool(axi_rand_id_alloc, true, "Allow random ID allocation for axi_mst transactions");
DEFINE_bool(axi_sw_mst_greedy_queue, false, "Enables greedy behavior for transaction queue. This prevents HOL blocking on C++ side.");
DEFINE_bool(axi_sw_rsp_toggle_en, false, "Allow axi_sw_rsp_toggle_en responses on axi_mst transactions");
DEFINE_int32(axi_mst_brdy_high, 4, "Maximum cycles of axi bready assertion");
DEFINE_int32(axi_mst_brdy_low, 4, "Maximum  cycles of axi bready de-assertion");
DEFINE_int32(axi_mst_rrdy_high, 4, "Maximum  cycles of axi rready assertion");
DEFINE_int32(axi_mst_rrdy_low, 4, "Maximum  cycles of axi rready de-assertion");
DEFINE_int64(axi_sw_rsp_toggle_start, 1000, "cycles of axi clock for axi_sw_rsp_toggle_start ");
DEFINE_uint32(axi_resp_timeout, 10000, "Cycles to wait after Transactor-id pool overflow condition before raising no free ids error");



extern "C" {
    void axi_sw_mst_ar_reset();
    void axi_sw_mst_aw_reset();
    void axi_sw_mst_w_reset();

    void axi_sw_mst_ar(uint32_t id, uint64_t addr, uint8_t len, uint8_t size, uint8_t burst, uint8_t lock, uint8_t cache, uint8_t prot, uint8_t qos, uint8_t region, uint8_t user);
    void axi_sw_mst_aw(uint32_t id, uint64_t addr, uint8_t len, uint8_t size, uint8_t burst, uint8_t lock, uint8_t cache, uint8_t prot, uint8_t qos, uint8_t region, uint8_t atop, uint8_t user);
    void axi_sw_mst_w_8(const uint8_t* data, const uint8_t* strb, uint8_t last);
    void axi_sw_mst_w_64(const uint8_t* data, const uint8_t* strb, uint8_t last);
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
axi_sw_mst<B, R, ARQ, AWQ, WQ>::axi_sw_mst(cvm::topology::loc_t loc, unsigned id)
    : scope_(nullptr), loc_(loc), id_(id),
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
      ids_(size_t(1) << id_width_, true),
      chk_rsp_err_ids_(size_t(1) << id_width_, true),
      allow_err_resp_ids_(size_t(1) << id_width_, false),
      read_bytes_(0), write_bytes_(0)
{
    name_ = cvm::topology::name(loc_);
    std::transform(name_.begin(), name_.end(), name_.begin(), [](unsigned char c){ return std::tolower(c); });

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

    cvm::registry::messenger.procedure<push_ar_no_id_rpc>(loc, [this] (const axi::a_no_id_t& ar, axi::id_t& id) { return this->push_a_no_id(false, ar, id); });
    cvm::registry::messenger.procedure<push_aw_no_id_rpc>(loc, [this] (const axi::a_no_id_t& aw, axi::id_t& id) { return this->push_a_no_id(true, aw, id); });
    cvm::registry::messenger.procedure<push_w_rpc>(loc, [this] (const axi::w_t& w) { return this->push_w(w); });
    cvm::registry::messenger.procedure<try_lock_rpc>(loc, [this] () { return this->try_lock(); });
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
axi_sw_mst<B, R, ARQ, AWQ, WQ>::~axi_sw_mst() {

    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"{}{}_read_bytes\": {}}}\n", name_, id_, read_bytes_);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"{}{}_write_bytes\": {}}}\n", name_, id_, write_bytes_);
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::process(const B& b) {
    if (used_id(b.id) && ((chk_rsp_err_ids_[b.id] && b.resp != axi::RESP_OKAY) || (!chk_rsp_err_ids_[b.id] && b.resp == axi::RESP_OKAY))) {
        if (!FLAGS_axi_allow_err_resp || !allow_err_resp_ids_[b.id]) {
            cvm::log(cvm::ERROR, "[{}] Error: bad b.response id: {} Expected: {} Actual: {} \n", name_, b.id, chk_rsp_err_ids_[b.id] ? uint8_t(axi::RESP_OKAY) : uint8_t(axi::RESP_DECERR), uint8_t(b.resp));
            return;
        } else {
            cvm::log(cvm::HIGH, "[{}] Allowing error b.response id: {} resp: {}\n", name_, b.id, b.resp);
        }
    }

    cvm::log(cvm::FULL, "[axi_sw_mst]  b.response id: {} resp: {}\n", b.id, b.resp);

    cvm::registry::messenger.signal<axi::b_t>(
        loc_,
        axi::b_t(b.id, axi::resp_t(b.resp))
    );

    cvm::registry::messenger.signal<transactor::write_response_t>(
        loc_,
        transactor::write_response_t{b.id});

    free_id(b.id);
    push_transactions();
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::process(const R& r) {
    if (used_id(r.id) && ((chk_rsp_err_ids_[r.id] && r.resp != axi::RESP_OKAY) || (!chk_rsp_err_ids_[r.id] && r.resp == axi::RESP_OKAY))) {
        if (!FLAGS_axi_allow_err_resp || !allow_err_resp_ids_[r.id]) {
            cvm::log(cvm::ERROR, "[{}] Error: bad r.response id: {} Expected: {} Actual: {} \n", name_, r.id, chk_rsp_err_ids_[r.id] ? uint8_t(axi::RESP_OKAY) : uint8_t(axi::RESP_DECERR), uint8_t(r.resp));
            return;
        } else {
            cvm::log(cvm::HIGH, "[{}] Allowing error r.response id: {} resp: {} last: {}\n", name_, r.id, r.resp, r.last);
        }
    }

    cvm::log(cvm::FULL, "[axi_sw_mst]  r.response id: {} resp: {} last: {}\n", r.id, r.resp, r.last);

    axi::data_t vdata = cvm::bitmanip::slice<decltype(r.data), axi::data_t>(r.data);
    std::string d;
    for (int i=int(data_width_/8)-1; i>=0; i--)
        d += fmt::format("{:02x}", vdata[i]);
    cvm::log(cvm::FULL, "[axi_sw_mst] axi_sw_r_{}: id={}, last={}, data={}\n", data_width_/8, r.id, r.last, d);

    cvm::registry::messenger.signal<axi::r_t>(
        loc_,
        axi::r_t(r.id, axi::resp_t(r.resp), vdata, r.last)
    );

    cvm::log(cvm::FULL, "[axi_sw_mst]  Copy Data to read_data_t id: {} resp: {} last: {}\n", r.id, r.resp, r.last);
    read_data_[r.id].insert(read_data_[r.id].end(), vdata.begin(), vdata.end());

    if (r.last) {
        cvm::registry::messenger.signal<transactor::read_response_t>(
            loc_,
            transactor::read_response_t{r.id, std::move(read_data_[r.id])});

        free_id(r.id);
        read_data_[r.id] = {};
    }
    cvm::log(cvm::FULL, "[axi_sw_mst]  Free id {} and call PUSH_transactions \n", r.id);
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
    //id check
    if (used_id(a.id)) {
        cvm::log(cvm::ERROR, "[{}] Error: bad request id: {}, pass unused id for transaction \n", name_, a.id);
        return;
    }
    alloc_id(a.id);
    chk_rsp_err_ids_[a.id] = a.rsp_err_chk;
    allow_err_resp_ids_[a.id] = a.allow_err_resp;

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
    a.burst =axi::BURST_INCR; a.atop = false; a.lock = false;

    if (!next_id(a.id, a.seqid)) {
        cvm::log(cvm::NONE, "[{}] No free id's remaining for axi master\n", name_);
        return false;
    }

    if ((a.addr & axi::addr_t(~0xFFF)) != ((a.addr + req_length - 1) & axi::addr_t(~0xFFF))) {
        cvm::log(cvm::ERROR, "[{}] Error: Request crosses 4k boundary, addr: {}, length: {}\n", name_, req_addr, req_length);
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
bool
axi_sw_mst<B, R, ARQ, AWQ, WQ>::push_a_no_id(const bool& aw, const axi::a_no_id_t& a_no_id, axi::id_t& id) {
    axi::a_t a {a_no_id};
    a.w = aw;

    if (!next_id(id, a.seqid)) {
        cvm::log(cvm::NONE, "[{}] No free id's remaining for axi master\n", name_);
        return false;
    }
    a.id = id;
    chk_rsp_err_ids_[a.id] = a.rsp_err_chk;
    allow_err_resp_ids_[a.id] = a.allow_err_resp;

    transactions_.emplace_back(a);
    push_transactions();
    return true;
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::push_w(const axi::w_t& w) {
    transactions_.emplace_back(w);
    push_transactions();
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::push_transactions() {

  cvm::log(cvm::FULL, "Calling Push transactions\n");

  auto push = [this](auto&& arg) -> bool {
      using T = std::decay_t<decltype(arg)>;

      if constexpr (std::is_same_v<T, axi::a_t>) {
          bool write = arg.w;

          if (!write) {
              read_bytes_ = read_bytes_ + (1ull << arg.size);
              cvm::log(cvm::FULL, "[axi_sw_mst] ar: [id={}, addr={:#x},len={} size={} burst={} lock={}]\n", arg.id, arg.addr, arg.len, arg.size, arg.burst, arg.lock);
              cvm::log(cvm::FULL, "[axi_sw_mst] ar: [ar_q_wptr:{} ar_q_rptr:{} ar_q_max_:{}]\n", ar_q_wptr_, ar_q_rptr_, ar_q_max_);
              if ((ar_q_wptr_ - ar_q_rptr_ ) < ar_q_max_) {
                  ar_q_wptr_ = (ar_q_wptr_ + 1) % ar_q_ptr_max_;
                  cvm::registry::callbacks.push(
                    scope_,
                    [=]() { axi_sw_mst_ar(arg.id, arg.addr, arg.len, arg.size, arg.burst, arg.lock, arg.cache, arg.prot, arg.qos, arg.region, arg.user); });
              }
              else {
                  cvm::log(cvm::FULL, "[axi_sw_mst] skipping ar_req\n");
                  return false;
              }
          }
          else {
              write_bytes_ = write_bytes_ + (1ull << arg.size);
              cvm::log(cvm::FULL, "[axi_sw_mst] aw: [id={}, addr={:#x}, len={}, size={}, burst={}, lock={}]\n", arg.id, arg.addr, arg.len, arg.size, arg.burst, arg.lock);
              cvm::log(cvm::FULL, "[axi_sw_mst] aw: [aw_q_wptr:{} aw_q_rptr:{} aw_q_max_:{}]\n", aw_q_wptr_, aw_q_rptr_, aw_q_max_);
              if ((aw_q_wptr_ - aw_q_rptr_ ) < aw_q_max_) {
                  aw_q_wptr_ = (aw_q_wptr_ + 1) % aw_q_ptr_max_;
                  cvm::registry::callbacks.push(
                    scope_,
                    [=]() { axi_sw_mst_aw(arg.id, arg.addr, arg.len, arg.size, arg.burst, arg.lock, arg.cache, arg.prot, arg.qos, arg.region, arg.atop.transaction, arg.user); });
              }
              else {
                  cvm::log(cvm::FULL, "[axi_sw_mst] skipping aw_req\n");
                  return false;
              }
          }
      }
      else if constexpr (std::is_same_v<T, axi::w_t>) {
        cvm::log(cvm::FULL, "[axi_sw_mst] wdata w_q_wptr:{} w_q_rptr:{} w_q_max_:{} \n", w_q_wptr_ ,w_q_rptr_ ,w_q_max_);
          if ((w_q_wptr_ - w_q_rptr_ ) < w_q_max_) {
              w_q_wptr_ = (w_q_wptr_ + 1) % w_q_ptr_max_;

              if (arg.strb.size() == 0) {
                  cvm::log(cvm::ERROR, "[{}] Error: strb size is 0\n", name_);
                  return false;
              }

              if (arg.strb.size() != arg.data.size()) {
                  cvm::log(cvm::ERROR, "[{}] Error: strb size != data size\n", name_);
                  return false;
              }

              cvm::registry::callbacks.push(
                  scope_,
                  [=, this]() {
                      std::vector<uint8_t> strb(((arg.strb.size() - 1) >> 3) + 1, 0);
                      for (size_t i = 0; i < arg.strb.size(); i++) {
                          size_t idx = i >> 3;
                          strb[idx] |= arg.strb[i] << i%8;
                      }
                      std::string d;
                      for (int i=0; i<int(data_width_/8); i++)
                        d += fmt::format("{:02x}", arg.data[i]);
                      cvm::log(cvm::FULL, "[axi_sw_mst] axi_sw_w_{}: data={}\n", data_width_/8, d);
                      if (data_width_ == 64)
                          axi_sw_mst_w_8(arg.data.data(), strb.data(), arg.last);
                      else if (data_width_ == 512)
                          axi_sw_mst_w_64(arg.data.data(), strb.data(), arg.last);
                      else
                          cvm::log(cvm::ERROR, "[{}] Error: unsupported data width {}", name_, data_width_);
                  });
          } else {
              cvm::log(cvm::FULL, "[axi_sw_mst] skipping wdata\n");
              return false;
          }
      }
      else {
          cvm::log(cvm::ERROR, "[{}] Error: unhandled axi_mst transaction type {}\n");
          return false;
      }
      return true;
  };

  auto head = transactions_.begin();
  while (head != transactions_.end()) {
      int r = std::visit(push, *head);
      cvm::log(cvm::FULL, "[axi_sw_mst] visit returned {}\n", r);
      if (!r) {
        if (!FLAGS_axi_sw_mst_greedy_queue)
          break;
        else {
          head++;
          continue;
        }
      }
      head = transactions_.erase(head);
  }

  cvm::log(cvm::FULL, "[axi_sw_mst] transactions left {}\n", transactions_.size());

}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::process(const transactor::read_request_t& req) {
    axi::a_t a;
    a.w = false;
    a.rsp_err_chk = req.rsp_err_chk;

     if (!a_wrapper(req.addr, req.length, a))
        return;
    chk_rsp_err_ids_[a.id] = a.rsp_err_chk;
    allow_err_resp_ids_[a.id] = a.allow_err_resp;
    transactions_.emplace_back(a);
    push_transactions();
}

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
void
axi_sw_mst<B, R, ARQ, AWQ, WQ>::process(const transactor::write_request_t& req) {
    axi::a_t a;
    a.w = true;
    a.rsp_err_chk = req.rsp_err_chk;

    if (!a_wrapper(req.addr, req.length, a))
        return;
    chk_rsp_err_ids_[a.id] = a.rsp_err_chk;
    allow_err_resp_ids_[a.id] = a.allow_err_resp;
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

    cvm::log(cvm::HIGH, "[axi_sw_mst] reset_ptrs loc={}\n", loc_);
    ar_q_wptr_ = 0;
    aw_q_wptr_ = 0;
    w_q_wptr_ = 0;

}

extern "C" {

  std::uint8_t axi_sw_mst_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    axi_sw_mst_ar_reset();
    axi_sw_mst_aw_reset();
    axi_sw_mst_w_reset();

    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);

    return 0;
  }

}
