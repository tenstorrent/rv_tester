#include <tuple>
#include <string_view>

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
                         rv_tester_transactions::axi_sw::r_q_ptr<>,
                         rv_tester_transactions::axi_sw::b_q_ptr<>>), AXI, cvm::registry::all);

REGISTRY_register((axi_sw<rv_tester_transactions::axi_sw::w<1>,
                         rv_tester_transactions::axi_sw::aw<1>,
                         rv_tester_transactions::axi_sw::ar<1>,
                         rv_tester_transactions::axi_sw::r_q_ptr<1>,
                         rv_tester_transactions::axi_sw::b_q_ptr<1>>), NCIO_AXI, cvm::registry::all);

DEFINE_int32(axi_sw_read_latency_max, 0, "Maximum latency of axi reads");
DEFINE_int32(axi_sw_read_latency_timeout_threshold, 1, "How many cycles under axi_sw_read_latency_max before stopping the clock on zebu. This should usually be more than zero to prevent race conditions on the zebu");
DEFINE_int32(axi_sw_read_latency_fifo_threshold, 1, "How many remaining fifo entries in the read request history fifo before stopping the clock on zebu.");
DEFINE_int32(axi_sw_read_latency_fixed, 0, "Fixed latency of axi reads");
DEFINE_bool(axi_sw_read_no_callbacks, false, "Plusarg to test synchronous read flushes are working by turning off asynchronous callbacks. Must use with +axi_sw_read_latenxy_*");
DEFINE_int32(axi_sw_read_consecutive_spurious_calls_allowed, -1, "Ignore N spurious call after a non-spurious call. Set to -1 to ignore all spurious calls. Spurious calls should not break function but slow down emulation.");

DEFINE_uint32(axi_sw_reorder_window, 0, "If reorder window > 1, will randomly attempt to reorder ar/aw requests within the window. Otherwise, transactions are handled in-order.");
DEFINE_uint32(axi_sw_reorder_timeout, 100, "If reorder window > 1, will attempt to flush transaction every N cycles while window is not drained (prevent stalls).");

DEFINE_bool(axi_sw_fast_write_response, false, "If fast write response, SV will immediately return write response without going through DPI.");


DEFINE_uint32(axi_sw_lfsr_seed_aw_rdy, 0, "LFSR seed for aw ready toggling.");
DEFINE_uint32(axi_sw_lfsr_mask_aw_rdy, 0, "LFSR mask for aw ready toggling. When the LFSR value AND-ed with the mask is 0, we toggle ready high.");
DEFINE_uint32(axi_sw_lfsr_seed_ar_rdy, 0, "LFSR seed for ar ready toggling.");
DEFINE_uint32(axi_sw_lfsr_mask_ar_rdy, 0, "LFSR mask for ar ready toggling. When the LFSR value AND-ed with the mask is 0, we toggle ready high.");
DEFINE_uint32(axi_sw_lfsr_seed_w_rdy, 0, "LFSR seed for w ready toggling.");
DEFINE_uint32(axi_sw_lfsr_mask_w_rdy, 0, "LFSR mask for w ready toggling. When the LFSR value AND-ed with the mask is 0, we toggle ready high.");

static std::tuple<bool, uint64_t, uint64_t> get_uint64_pair(std::string_view value) {
    using std::operator""sv;
    auto range = std::views::split(value, ":"sv);
    if (std::distance(range.begin(), range.end()) != 2) {
      cvm::log(cvm::ERROR, "Expecting <min>:<max> format for add response latency range: %s\n", value);
      return {false, 0, 0};
    }

    auto it = range.begin();
    auto min = std::stoull(std::string((*it).begin(), (*it).end()), nullptr, 0);
    it = std::next(it, 1);
    auto max = std::stoull(std::string((*it).begin(), (*it).end()), nullptr, 0);

    if ((min >= max) || (max >= 65536)) {
      cvm::log(cvm::ERROR, "Invalid add response latency range: %s, must be <= 65535 and min < max\n", value);
      return {false, min, max};
    }
    return {true, min, max};
}

static bool validate_add_response_latency_range([[maybe_unused]] const char* flagname, const std::string& value) {
    auto [ok, _, _2] = get_uint64_pair(value);
    return ok;
}

DEFINE_string(axi_sw_add_response_latency_range, "0:1", "Range of random response latency added [min, max).");
DEFINE_validator(axi_sw_add_response_latency_range, &validate_add_response_latency_range);

namespace {
    bool destroyed = false;
}

// Helper function to convert a string to lowercase.
static std::string to_lower(const std::string& s) {
  std::string result = s;
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
  return result;
}

extern "C" {

  void axi_sw_r_reset();
  void axi_sw_b_reset();
  void axi_sw_b(axi::id_t id, axi::resp_t resp, uint16_t latency);
  void axi_sw_r_8(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last, uint16_t latency);
  void axi_sw_r_64(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last, uint16_t latency);
}

template <typename W, typename AW, typename AR, typename RQ, typename BQ>
axi_sw<W,AW,AR,RQ,BQ>::axi_sw(cvm::topology::loc_t loc, unsigned id)
  : scope_(nullptr), loc_(loc), id_(id), name_(to_lower(cvm::topology::name(loc_)) + std::to_string(id)),
    id_width_(cvm::topology::attr(loc_, "ID_WIDTH").second),
    data_width_(cvm::topology::attr(loc_, "DATA_WIDTH").second),
    strb_width_(cvm::topology::attr(loc_, "STRB_WIDTH").second),
    r_dpi_fifo_(cvm::topology::attr(loc, "R_Q_MAX").second, cvm::topology::attr(loc, "R_Q_PTR_MAX").second, 0, 0, 0),
    b_dpi_fifo_(cvm::topology::attr(loc, "B_Q_MAX").second, cvm::topology::attr(loc, "B_Q_PTR_MAX").second, 0, 0, 0),
    read_bytes_(0), write_bytes_(0) {

    cvm::log(cvm::FULL, "[axi_sw] Constructing axi_sw for loc={} id={}\n", loc,id);

    ::destroyed = false;

    auto data_width = cvm::topology::attr(loc, "DATA_WIDTH").second;
    axi_ = new axi(data_width, loc, name_);
    cvm::registry::messenger.connect<svScope>(
        loc_,
        [this](svScope s) {
        this->set_scope(s);
        return reset_ptrs();
    });


    std::tie(std::ignore, add_latency_min_, add_latency_max_) = get_uint64_pair(FLAGS_axi_sw_add_response_latency_range);

    bool random_latency = add_latency_min_ != 0 or add_latency_max_ != 1 or FLAGS_axi_sw_reorder_window != 0;
    if (FLAGS_axi_sw_read_latency_fixed and random_latency)
      cvm::log(cvm::ERROR, "Error: can't specify both fixed latency and randomized latency options (add response latency, reorder window)");

    if (FLAGS_axi_sw_read_latency_max and random_latency)
      cvm::log(cvm::ERROR, "Error: can't specify both max latency and randomized latency options (add response latency, reorder window)");

    if (FLAGS_axi_sw_fast_write_response and random_latency)
      cvm::log(cvm::ERROR, "Error: can't specify both fast write response and randomized latency options (add response latency, reorder window)");

    connect_task<W,AW,AR,axi_sw_defs::reorder_q_flush_t>();

    connect<RQ,BQ,axi_sw_defs::r_q_ptr_blocking_update_t>();
}

template <typename W, typename AW, typename AR, typename RQ, typename BQ>
axi_sw<W,AW,AR,RQ,BQ>::~axi_sw() {

    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"{}_read_bytes\": {}}}\n", name_, read_bytes_);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"{}_write_bytes\": {}}}\n", name_, write_bytes_);

    if (axi_) {
        delete axi_;
        axi_ = nullptr;
    }
    ::destroyed = true;
}

// What I would really like to do instead, is pick a delay ahead-of-time and re-order
// by randomizing this delay. This also means we wouldn't need to buffer like this. We
// would also have much better guarantees.
template <typename W, typename AW, typename AR, typename RQ, typename BQ>
cvm::messenger::task<bool> axi_sw<W,AW,AR,RQ,BQ>::pop_reorder_q(bool oldest) {

  if (!a_window_.size())
    co_return false;

  unsigned position = oldest? 0 : cvm::rand::lcg::generate(a_window_.size());
  axi::id_t id = a_window_[position].id;
  bool w = a_window_[position].w;

  // check for same ID in queue, pick lowest number
  auto it = std::find_if(a_window_.begin(), a_window_.end(), [id, w](const auto& p) {
        return p.id == id and p.w == w;
      });

  position = std::distance(a_window_.begin(), it);
  axi::a_t aaa = *it;

  if (aaa.w) {
    // stupid way to check write data is available
    unsigned w_in_front = std::count_if(a_window_.begin(), a_window_.begin() + position, [] (const auto& p) {
          return p.w;
        });

    if (w_in_front >= w_window_.size())
      co_return false;

    a_window_.erase(it);
    co_await a(std::move(aaa));

    // we're guaranteed to not co_await in a() if aw, so this is safe to do
    auto it2 = w_window_.begin() + w_in_front;
    axi::w_t ww = *it2;
    w_window_.erase(it2);
    co_await this->w(std::move(ww));
  }
  else {
    a_window_.erase(it);
    co_await a(std::move(aaa));
  }

  co_return true;
}

template <typename W, typename AW, typename AR, typename RQ, typename BQ>
cvm::messenger::task<void> axi_sw<W,AW,AR,RQ,BQ>::process(const AW& aw) {
    cvm::log(cvm::FULL, "[axi_sw] aw: [id={}, addr={:#x}, size={}]\n", aw.id, aw.addr, aw.size);
    write_bytes_ = write_bytes_ + (1ull << aw.size);

    axi::a_t aa = axi::a_t{true, aw.id, aw.addr, aw.len, aw.size, axi::burst_t(aw.burst), aw.lock != 0,axi::cache_mem_attr_t(aw.cache),aw.prot,aw.qos,aw.region, aw.atop,aw.user};

    if (FLAGS_axi_sw_reorder_window <= 1)
      co_await a(std::move(aa));
    else {
      a_window_.push_back(std::move(aa));
      if (a_window_.size() >= FLAGS_axi_sw_reorder_window)
        co_await pop_reorder_q(false);
    }
    all_resp();
    co_return;
}

template <typename W, typename AW, typename AR, typename RQ, typename BQ>
cvm::messenger::task<void> axi_sw<W,AW,AR,RQ,BQ>::process(const AR& ar) {
    cvm::log(cvm::FULL, "[axi_sw] ar: [id={}, addr={:#x}, size={}]\n", ar.id, ar.addr, ar.size);
    read_bytes_ = read_bytes_ + (1ull << ar.size);

    axi::a_t aa = axi::a_t{false, ar.id, ar.addr, ar.len, ar.size, axi::burst_t(ar.burst), ar.lock != 0,axi::cache_mem_attr_t(ar.cache),ar.prot,ar.qos,ar.region, 0,ar.user};
    if (FLAGS_axi_sw_reorder_window <= 1)
      co_await a(std::move(aa));
    else {
      a_window_.push_back(std::move(aa));
      if (a_window_.size() >= FLAGS_axi_sw_reorder_window)
        co_await pop_reorder_q(false);
    }
    all_resp();
    co_return;
}

template <typename W, typename AW, typename AR, typename RQ, typename BQ>
cvm::messenger::task<void> axi_sw<W,AW,AR,RQ,BQ>::process(const W& w) {
    cvm::log(cvm::FULL, "[axi_sw] w: [strb={:#x}, last={}]\n", w.strb, w.last);
    axi::data_t vdata = cvm::bitmanip::slice<decltype(w.data), axi::data_t>(w.data);
    axi::strb_t vstrb = cvm::bitmanip::slice<decltype(w.strb), axi::strb_t>(w.strb);

    axi::w_t ww = axi::w_t(vdata, vstrb, w.last);

    if (FLAGS_axi_sw_reorder_window <= 1)
      co_await this->w(std::move(ww));
    else {
      w_window_.push_back(ww);
      if (a_window_.size() >= FLAGS_axi_sw_reorder_window)
        co_await pop_reorder_q(false);
    }
    all_resp();
    co_return;
}

template <typename W, typename AW, typename AR, typename RQ, typename BQ>
void axi_sw<W,AW,AR,RQ,BQ>::process(const axi_sw_defs::r_q_ptr_blocking_update_t& r_q_ptr) {
    struct _t {
        std::atomic<bool>& d;
        _t(std::atomic<bool>& d) : d(d) {}
        ~_t() {
            d = true;
            d.notify_one();
        }
    } _(*r_q_ptr.done);

    cvm::log(cvm::FULL, "[axi_sw] r_q_ptr_blocking_update: [rptr={} clock={}]\n", r_q_ptr.r_ptr, r_q_ptr.clock);
    assert(r_q_ptr.clock > r_dpi_fifo_.rptr_update_time_);
    r_dpi_fifo_.rptr_update_time_ = r_q_ptr.clock;
    r_dpi_fifo_.rptr_ = r_q_ptr.r_ptr;
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

template < typename W,typename AW,typename AR, typename RQ, typename BQ>
void axi_sw<W,AW,AR,RQ,BQ>::process(const RQ& r_q_ptr) {
    cvm::log(cvm::FULL, "[axi_sw] r_q_ptr: [rptr={} clock={}]\n", r_q_ptr.r_ptr, r_q_ptr.clock);
    if (r_q_ptr.clock > r_dpi_fifo_.rptr_update_time_) {
        r_dpi_fifo_.rptr_update_time_ = r_q_ptr.clock;
        r_dpi_fifo_.rptr_ = r_q_ptr.r_ptr;
        r_resp();
    }
}

template < typename W,typename AW,typename AR, typename RQ, typename BQ>
void axi_sw<W,AW,AR,RQ,BQ>::process(const BQ& b_q_ptr) {
    cvm::log(cvm::FULL, "[axi_sw] b_q_ptr: [bptr={} clock={}]\n", b_q_ptr.r_ptr, b_q_ptr.clock);
    if (b_q_ptr.clock > b_dpi_fifo_.rptr_update_time_) {
        b_dpi_fifo_.rptr_update_time_ = b_q_ptr.clock;
        b_dpi_fifo_.rptr_ = b_q_ptr.r_ptr;
        b_resp();
    }
}

template < typename W,typename AW,typename AR, typename RQ, typename BQ>
cvm::messenger::task<void> axi_sw<W,AW,AR,RQ,BQ>::process(const axi_sw_defs::reorder_q_flush_t&) {
    if (a_window_.size() != 0) {
      co_await pop_reorder_q(true);
      all_resp();
    }
}

template < typename W,typename AW,typename AR, typename RQ, typename BQ>
bool axi_sw<W,AW,AR,RQ,BQ>::r_dpi() {
    axi::r_t r;
    {
        std::lock_guard<std::mutex> l(r_q_mutex_);
        if (r_q_.empty()) return false;
        r = r_q_.front();
        r_q_.pop();
    }

    std::string d;
    for (int i=0; i<int(data_width_/8); i++)
        d += fmt::format("{:02x}", r.data[i]);
    cvm::log(cvm::FULL, "[axi_sw] axi_sw_r_{}: id={}, last={}, data={}\n", data_width_/8, r.id, r.last, d);

    uint16_t latency =  cvm::rand::lcg::generate(add_latency_max_ - add_latency_min_) + add_latency_min_;

    if(data_width_ == 64)
        axi_sw_r_8(r.id, r.resp, r.data.data(), r.last, latency);
    else if(data_width_ == 512)
        axi_sw_r_64(r.id, r.resp, r.data.data(), r.last, latency);
    else
        cvm::log(cvm::ERROR, "[axi_sw] Error: unsupported data width for axi_sw");

    return true;
}

template <typename W, typename AW, typename AR, typename RQ, typename BQ>
void axi_sw<W,AW,AR,RQ,BQ>::r_resp() {
    while ( (r_dpi_fifo_.wptr_ - r_dpi_fifo_.rptr_) < r_dpi_fifo_.max_ ) {
      auto [valid, result] = axi_->r(false);
      if (!valid)
        break;
      cvm::log(cvm::FULL, "[axi_sw] r_resp: [r_q dequeue valid={} wptr={} rptr={}]\n", valid, r_dpi_fifo_.wptr_, r_dpi_fifo_.rptr_);
      r_dpi_fifo_.wptr_ = (r_dpi_fifo_.wptr_ + 1) % r_dpi_fifo_.ptr_max_;
      {
          std::lock_guard<std::mutex> l(r_q_mutex_);
          r_q_.emplace(std::move(result));
      }

      if (!FLAGS_axi_sw_read_no_callbacks) {
        if (!scope_) {
          cvm::log(cvm::ERROR, "Error: scope_ not set before pushing r_dpi callback\n");
        } else {
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
}

template < typename W,typename AW,typename AR, typename RQ, typename BQ>
bool axi_sw<W,AW,AR,RQ,BQ>::b_dpi() {
    axi::b_t b;
    {
        std::lock_guard<std::mutex> l(b_q_mutex_);
        if (b_q_.empty()) return false;
        b = b_q_.front();
        b_q_.pop();
    }

    cvm::log(cvm::FULL, "[axi_sw] axi_sw_b: id={}\n", b.id);

    uint8_t latency = cvm::rand::lcg::generate(add_latency_max_ - add_latency_min_) + add_latency_min_;
    axi_sw_b(b.id, b.resp, latency);
    return true;
}


template <typename W, typename AW, typename AR, typename RQ, typename BQ>
void axi_sw<W,AW,AR,RQ,BQ>::b_resp() {
    if (FLAGS_axi_sw_fast_write_response)
      return;

    while ( (b_dpi_fifo_.wptr_ - b_dpi_fifo_.rptr_) < b_dpi_fifo_.max_ ) {
      // We don't need this, this is more for future-proofing if writes ever becomes a coroutine
      auto [valid, result] = axi_->b();
      if (!valid)
        break;
      cvm::log(cvm::FULL, "[axi_sw] b_resp: [b_q dequeue wptr={} rptr={}]\n", b_dpi_fifo_.wptr_, b_dpi_fifo_.rptr_);
      b_dpi_fifo_.wptr_ = (b_dpi_fifo_.wptr_ + 1) % b_dpi_fifo_.ptr_max_;
      {
          std::lock_guard<std::mutex> l(b_q_mutex_);
          b_q_.emplace(std::move(result));
      }

      cvm::registry::callbacks.push(
          scope_,
            [this]() {
                std::lock_guard<std::mutex> l(b_dpi_mutex_);
                b_dpi();
            }
      );
    }
}

template <typename W, typename AW, typename AR, typename RQ, typename BQ>
void axi_sw<W,AW,AR,RQ,BQ>::reset_ptrs() {
    cvm::log(cvm::HIGH, "[axi_sw] reset_ptrs loc={}\n", loc_);
    r_dpi_fifo_.rptr_ = 0;
    r_dpi_fifo_.wptr_ = 0;
    r_dpi_fifo_.rptr_update_time_ = 0;

    b_dpi_fifo_.rptr_ = 0;
    b_dpi_fifo_.wptr_ = 0;
    b_dpi_fifo_.rptr_update_time_ = 0;
}

template <typename W, typename AW, typename AR, typename RQ, typename BQ>
void axi_sw<W,AW,AR,RQ,BQ>::set_scope(svScope scope) {
    scope_ = scope;
}

extern "C" {

  std::uint8_t axi_sw_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    axi_sw_r_reset();
    axi_sw_b_reset();

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

  void axi_sw_reorder_flush(cvm::topology::loc_t loc) {
    cvm::registry::messenger.signal<axi_sw_defs::reorder_q_flush_t>(loc, axi_sw_defs::reorder_q_flush_t{});
    return;
  }

}
