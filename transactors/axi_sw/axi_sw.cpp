#include "axi_sw.h"
#include "cvm/topology.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/bitmanip.hpp"
#include "cvm/logger.hpp"
#include "cvm/random.hpp"
#include "rv_tester/rv_tester_plusargs.h"

REGISTRY_register((axi_sw<rv_tester_transactions::axi_sw::w<>,
                         rv_tester_transactions::axi_sw::aw<>,
                         rv_tester_transactions::axi_sw::ar<>,
                         rv_tester_transactions::axi_sw::r_q_ptr<>>), AXI, cvm::registry::all);

REGISTRY_register((axi_sw<rv_tester_transactions::axi_sw::w<1>,
                         rv_tester_transactions::axi_sw::aw<1>,
                         rv_tester_transactions::axi_sw::ar<1>,
                         rv_tester_transactions::axi_sw::r_q_ptr<1>>), NCIO_AXI, cvm::registry::all);

DEFINE_int32(axi_sw_read_latency_max, 0, "Maximum latency of axi reads");
DEFINE_int32(axi_sw_read_latency_timeout_threshold, 1, "How many cycles under axi_sw_read_latency_max before stopping the clock on zebu. This should usually be more than zero to prevent race conditions on the zebu");
DEFINE_int32(axi_sw_read_latency_fifo_threshold, 1, "How many remaining fifo entries in the read request history fifo before stopping the clock on zebu.");
DEFINE_int32(axi_sw_read_latency_fixed, 0, "Fixed latency of axi reads");
DEFINE_bool(axi_sw_read_no_callbacks, false, "Plusarg to test synchronous read flushes are working by turning off asynchronous callbacks. Must use with +axi_sw_read_latenxy_*");
DEFINE_int32(axi_sw_read_consecutive_spurious_calls_allowed, -1, "Ignore N spurious call after a non-spurious call. Set to -1 to ignore all spurious calls. Spurious calls should not break function but slow down emulation.");
DEFINE_bool(axi_sw_reorder_r, false, "Will randomly sample read response queue, when generating read response.");

namespace {
    bool destroyed = false;
}

extern "C" {

  void axi_sw_r_reset();
  void axi_sw_r_8(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last);
  void axi_sw_r_64(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last);
}

template <typename W, typename AW, typename AR, typename RQ>
axi_sw<W,AW,AR,RQ>::axi_sw(cvm::topology::loc_t loc, unsigned id)
  : scope_(nullptr), loc_(loc), id_(id),
    id_width_(cvm::topology::attr(loc_, "ID_WIDTH").second),
    data_width_(cvm::topology::attr(loc_, "DATA_WIDTH").second),
    strb_width_(cvm::topology::attr(loc_, "STRB_WIDTH").second),
    r_q_max_(cvm::topology::attr(loc, "R_Q_MAX").second), r_q_ptr_max_(cvm::topology::attr(loc, "R_Q_PTR_MAX").second),
    r_q_rptr_(0), r_q_wptr_(r_q_max_), r_q_rptr_update_time_(0),
    read_bytes_(0), write_bytes_(0)
    {
    cvm::log(cvm::FULL, "[axi_sw] Constructing axi_sw for loc={} id={}\n", loc,id);

    ::destroyed = false;

    auto data_width = cvm::topology::attr(loc, "DATA_WIDTH").second;
    axi_ = new axi(data_width, loc, "axi" + std::to_string(id));
    cvm::registry::messenger.connect<svScope>(
        loc_,
        [this](svScope s) {
        this->set_scope(s);
        return reset_ptrs();
    });

   connect_task<W,AW,AR>();

   connect<RQ,axi_sw_defs::r_q_ptr_blocking_update_t>();
}

template <typename W, typename AW, typename AR, typename RQ>
axi_sw<W,AW,AR,RQ>::~axi_sw() {

    std::string name = cvm::topology::name(loc_);
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c){ return std::tolower(c); });
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"{}{}_read_bytes\": {}}}\n", name, id_, read_bytes_);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"{}{}_write_bytes\": {}}}\n", name, id_, write_bytes_);

    if (axi_) {
        delete axi_;
        axi_ = nullptr;
    }
    ::destroyed = true;
}

template <typename W, typename AW, typename AR, typename RQ>
cvm::messenger::task<void> axi_sw<W,AW,AR,RQ>::process(const AW& aw) {
    cvm::log(cvm::FULL, "[axi_sw] aw: [id={}, addr={:#x}, size={}]\n", aw.id, aw.addr, aw.size);
    write_bytes_ = write_bytes_ + (1ull << aw.size);
    co_await a(axi::a_t{true, aw.id, aw.addr, aw.len, aw.size, axi::burst_t(aw.burst), aw.lock != 0,axi::cache_mem_attr_t(aw.cache),aw.prot,aw.qos,aw.region, aw.atop,aw.user});
    r_resp();
    co_return;
}

template <typename W, typename AW, typename AR, typename RQ>
cvm::messenger::task<void> axi_sw<W,AW,AR,RQ>::process(const AR& ar) {
    cvm::log(cvm::FULL, "[axi_sw] ar: [id={}, addr={:#x}, size={}]\n", ar.id, ar.addr, ar.size);
    read_bytes_ = read_bytes_ + (1ull << ar.size);
    co_await a(axi::a_t{false, ar.id, ar.addr, ar.len, ar.size, axi::burst_t(ar.burst), ar.lock != 0,axi::cache_mem_attr_t(ar.cache),ar.prot,ar.qos,ar.region, 0,ar.user});
    r_resp();
    co_return;
}

template <typename W, typename AW, typename AR, typename RQ>
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

template <typename W, typename AW, typename AR, typename RQ>
void axi_sw<W,AW,AR,RQ>::process(const axi_sw_defs::r_q_ptr_blocking_update_t& r_q_ptr) {
    struct _t {
        std::atomic<bool>& d;
        _t(std::atomic<bool>& d) : d(d) {}
        ~_t() {
            d = true;
            d.notify_one();
        }
    } _(*r_q_ptr.done);

    cvm::log(cvm::FULL, "[axi_sw] r_q_ptr_blocking_update: [rptr={} clock={}]\n", r_q_ptr.r_ptr, r_q_ptr.clock);
    assert(r_q_ptr.clock > r_q_rptr_update_time_);
    r_q_rptr_update_time_ = r_q_ptr.clock;
    r_q_rptr_ = r_q_ptr.r_ptr;
    r_resp();

    *r_q_ptr.successful = false;
    std::unique_lock<std::mutex> l{r_dpi_mutex_, std::defer_lock};
    if (!l.try_lock()) return;
    int sent = 0;
    svSetScope(scope_);
    while(r_dpi()) sent++;
    if (sent) {
        r_q_rptr_blocking_update_consecutive_spurious_calls_ = 0;
    } else {
        r_q_rptr_blocking_update_consecutive_spurious_calls_ ++;
    }

    if (FLAGS_axi_sw_read_consecutive_spurious_calls_allowed >= 0 && r_q_rptr_blocking_update_consecutive_spurious_calls_ > FLAGS_axi_sw_read_consecutive_spurious_calls_allowed) {
        cvm::log(cvm::ERROR, "[axi_sw] Error: no dpis sent in blocking read data update {} after {} failed attempts\n", r_q_ptr.clock, r_q_rptr_blocking_update_consecutive_spurious_calls_);
    }
    *r_q_ptr.successful = sent != 0;
}

template < typename W,typename AW,typename AR, typename RQ>
void axi_sw<W,AW,AR,RQ>::process(const RQ& r_q_ptr) {
    cvm::log(cvm::FULL, "[axi_sw] r_q_ptr: [rptr={} clock={}]\n", r_q_ptr.r_ptr, r_q_ptr.clock);
    if (r_q_ptr.clock > r_q_rptr_update_time_) {
        r_q_rptr_update_time_ = r_q_ptr.clock;
        r_q_rptr_ = r_q_ptr.r_ptr;
        r_resp();
    }
}

template < typename W,typename AW,typename AR, typename RQ>
bool axi_sw<W,AW,AR,RQ>::r_dpi() {
    axi::r_t r;
    {
        std::lock_guard<std::mutex> l(r_q_mutex_);
        if (r_q_.empty()) return false;
        unsigned position = FLAGS_axi_sw_reorder_r? cvm::rand::lcg::generate(r_q_.size()) : 0;
        axi::id_t id = r_q_[position].id;

        // Same IDs must be in-order, but this means our probability distribution will be biased towards IDs which repeat
        auto it = std::find_if(r_q_.begin(), r_q_.end(), [id](const auto& p) {
              return p.id == id;
            });
        r = *it;
        r_q_.erase(it);
    }

    std::string d;
    for (int i=0; i<int(data_width_/8); i++)
        d += fmt::format("{:02x}", r.data[i]);
    cvm::log(cvm::FULL, "[axi_sw] axi_sw_r_{}: id={}, last={}, data={}\n", data_width_/8, r.id, r.last, d);

    if(data_width_ == 64)
        axi_sw_r_8(r.id, r.resp, r.data.data(), r.last);
    else if(data_width_ == 512)
        axi_sw_r_64(r.id, r.resp, r.data.data(), r.last);
    else
        cvm::log(cvm::ERROR, "[axi_sw] Error: unsupported data width for axi_sw");

    return true;
}

template <typename W, typename AW, typename AR, typename RQ>
void axi_sw<W,AW,AR,RQ>::r_resp() {
    while ( (r_q_wptr_ - r_q_rptr_) < r_q_max_ ) {
      auto [valid, result] = axi_->r(false);
      cvm::log(cvm::FULL, "[axi_sw] r_resp: [r_q dequeue valid={} wptr={} rptr={}]\n", valid, r_q_wptr_, r_q_rptr_);
      if (!valid)
        break;
      r_q_wptr_ = (r_q_wptr_ + 1) % r_q_ptr_max_;
      {
          std::lock_guard<std::mutex> l(r_q_mutex_);
          r_q_.emplace_back(std::move(result));
      }

      if (!FLAGS_axi_sw_read_no_callbacks) {
        cvm::registry::callbacks.push(
            scope_,
              [this]() {
                  std::lock_guard<std::mutex> l(r_dpi_mutex_);
                  r_dpi();
              }
        );
      }
    }
}

template <typename W, typename AW, typename AR, typename RQ>
void axi_sw<W,AW,AR,RQ>::reset_ptrs() {
    cvm::log(cvm::HIGH, "[axi_sw] reset_ptrs loc={}\n", loc_);
    r_q_rptr_ = 0;
    r_q_wptr_ = 0;
    r_q_rptr_update_time_ = 0;
}

template <typename W, typename AW, typename AR, typename RQ>
void axi_sw<W,AW,AR,RQ>::set_scope(svScope scope) {
    scope_ = scope;
}

extern "C" {

  std::uint8_t axi_sw_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    axi_sw_r_reset();

    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);

    return 0;
  }

  std::uint8_t axi_sw_flush(cvm::topology::loc_t loc, std::uint64_t clock, axi_sw_defs::r_q_ptr_t ptr) {
    if (::destroyed) return true;
    bool successful = false;
    std::atomic<bool> done(false);
    cvm::registry::messenger.signal_async<axi_sw_defs::r_q_ptr_blocking_update_t>(loc, axi_sw_defs::r_q_ptr_blocking_update_t{clock, ptr, &successful, &done}, cvm::messenger::highest_priority);
    done.wait(false);
    return successful;
  }

}
