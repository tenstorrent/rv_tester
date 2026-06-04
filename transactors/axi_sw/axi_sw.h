#pragma once

#include <cinttypes>
#include <cstdio>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <cassert>
#include <iostream>
#include <type_traits>
#include <tuple>
#include <string_view>
#include <queue>
#include <ranges>
#include <iterator>
#include "axi.h"
#include "svdpi.h"

#include "cvm/topology.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/bitmanip.hpp"
#include "cvm/logger.hpp"
#include "cvm/random.hpp"
#include "rv_tester/rv_tester_plusargs.h"
#include "rv_tester_transactions.hpp"

DECLARE_bool(cb_async);

#include "axi_sw_plusargs.h"

namespace _axi_sw {

  inline std::tuple<bool, uint64_t, uint64_t> get_uint64_pair(std::string_view value) {
      using std::operator""sv;
      auto range = std::views::split(value, ":"sv);
      if (std::distance(range.begin(), range.end()) != 2) {
        cvm::log(cvm::ERROR, "Error: Expecting <min>:<max> format for add response latency range: %s\n", value);
        return {false, 0, 0};
      }

      auto it = range.begin();
      auto min = std::stoull(std::string((*it).begin(), (*it).end()), nullptr, 0);
      it = std::next(it, 1);
      auto max = std::stoull(std::string((*it).begin(), (*it).end()), nullptr, 0);

      if ((min >= max) || (max >= 65536)) {
        cvm::log(cvm::ERROR, "Error: Invalid add response latency range: %s, must be <= 65535 and min < max\n", value);
        return {false, min, max};
      }
      return {true, min, max};
  }

  inline std::string string_to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
  }

  inline bool axi_sw_destroyed = false;

  extern "C" {
    void axi_sw_b(axi::id_t id, axi::resp_t resp, uint16_t latency);
    void axi_sw_r_8(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last, uint16_t latency);
    void axi_sw_r_32(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last, uint16_t latency);
    void axi_sw_r_64(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last, uint16_t latency);
  }
}

struct axi_sw_defs {

    typedef std::uint32_t r_q_ptr_t   ;
    typedef std::uint32_t b_q_ptr_t   ;

    struct r_q_ptr_blocking_update_t {
        std::uint64_t clock;
        r_q_ptr_t r_ptr;
        bool* successful;
        std::atomic<bool>* done;
    };

    struct reorder_q_flush_t {};

};

template < typename W,typename AW,typename AR, typename RQ, typename BQ>
class axi_sw {

    template<typename T, typename... Args>
      void connect() {
      cvm::registry::messenger.connect<T>(
          loc_,
          [this] (const T& v) {
              return this->process(v);
          }
      );
      if constexpr (sizeof...(Args))
        connect<Args...>();
    }

    template<typename T, typename... Args>
      void connect_task() {
      cvm::registry::messenger.connect<T>(
          loc_,
          [this] (T v) {
              auto* task = +[] (axi_sw* axi, T t) -> cvm::messenger::task<void> { co_return co_await axi->process(t); };
              cvm::registry::messenger.fork(task, this, std::move(v));
          }
      );
      if constexpr (sizeof...(Args))
        connect_task<Args...>();
    }

    public:

        using r_q_ptr_t = axi_sw_defs::r_q_ptr_t;
        using b_q_ptr_t = axi_sw_defs::b_q_ptr_t;

        template <typename Q>
        struct ptr_t {
          const Q max_;
          const Q ptr_max_;
          Q rptr_;
          Q wptr_;
          uint64_t rptr_update_time_;
        };

    private:

        // What I would really like to do instead, is pick a delay ahead-of-time and re-order
        // by randomizing this delay. This also means we wouldn't need to buffer like this. We
        // would also have much better guarantees.
        cvm::messenger::task<bool> pop_reorder_q(bool oldest) {

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

          cvm::log(cvm::FULL, "[axi_sw] Picking position {} of {} a's\n", position, a_window_.size());

          if (aaa.w) {
            // stupid way to check write data is available
            unsigned prior_writes = 0;
            // TODO: change this to std:;reduce
            for (auto it = a_window_.begin(); it != (a_window_.begin() + position); ++it)
              if (it->w)
                prior_writes += (it->len + 1);

            size_t burst_len = aaa.len + 1;
            if ((prior_writes + burst_len) > w_window_.size()) {
              cvm::log(cvm::FULL, "[axi_sw] Insufficient write packets for burst_len={} in w window={}\n", burst_len, w_window_.size());
              co_return false;
            }

            a_window_.erase(it);
            co_await a(std::move(aaa));

            // we're guaranteed to not co_await in a() if aw, so this is safe to do
            auto it2 = w_window_.begin() + prior_writes;
            for (unsigned i = 0; i < burst_len; ++i) {
              axi::w_t ww = *it2;
              it2 = w_window_.erase(it2);
              co_await this->w(std::move(ww));
            }
          }
          else {
            a_window_.erase(it);
            co_await a(std::move(aaa));
          }

          co_return true;
        }

        cvm::messenger::task<void> process(const AW& aw) {
            cvm::log(cvm::FULL, "[axi_sw] aw: [id={}, addr={:#x}, size={}]\n", aw.id, aw.addr, aw.size);
            write_bytes_ = write_bytes_ + (1ull << aw.size);

            axi::a_t aa = axi::a_t{true, aw.id, aw.addr, aw.len, aw.size, axi::burst_t(aw.burst), aw.lock != 0,
                                   axi::cache_mem_attr_t(aw.cache), aw.prot, aw.qos, aw.region, aw.atop, aw.user};

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

        cvm::messenger::task<void> process(const AR& ar) {
            cvm::log(cvm::FULL, "[axi_sw] ar: [id={}, addr={:#x}, size={}]\n", ar.id, ar.addr, ar.size);
            read_bytes_ = read_bytes_ + (1ull << ar.size);

            axi::a_t aa = axi::a_t{false, ar.id, ar.addr, ar.len, ar.size, axi::burst_t(ar.burst), ar.lock != 0,
                                   axi::cache_mem_attr_t(ar.cache), ar.prot, ar.qos, ar.region, 0, ar.user};

            ++pending_reads_;
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

        cvm::messenger::task<void> process(const W& w) {
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

        void process(const RQ& r_q_ptr) {
            cvm::log(cvm::FULL, "[axi_sw] r_q_ptr: [rptr={} clock={}]\n", r_q_ptr.r_ptr, r_q_ptr.clock);
            if (r_q_ptr.clock > r_dpi_fifo_.rptr_update_time_) {
                r_dpi_fifo_.rptr_update_time_ = r_q_ptr.clock;
                r_dpi_fifo_.rptr_ = r_q_ptr.r_ptr;
                r_resp();
            }
        }

        void process(const BQ& b_q_ptr) {
            cvm::log(cvm::FULL, "[axi_sw] b_q_ptr: [bptr={} clock={}]\n", b_q_ptr.r_ptr, b_q_ptr.clock);
            if (b_q_ptr.clock > b_dpi_fifo_.rptr_update_time_) {
                b_dpi_fifo_.rptr_update_time_ = b_q_ptr.clock;
                b_dpi_fifo_.rptr_ = b_q_ptr.r_ptr;
                b_resp();
            }
        }

        void process(const axi_sw_defs::r_q_ptr_blocking_update_t& r_q_ptr) {
            struct _t {
                std::atomic<bool>& d;
                _t(std::atomic<bool>& d) : d(d) {}
                ~_t() {
                    d = true;
                    d.notify_one();
                }
            } _(*r_q_ptr.done);

            // It's possible that we flush on the same cycle due to a normal rptr update.
            // We allow this to pass on sim since we would flush it on the same cycle, but on zebu
            // we would almost certainly timeout, so we return an error.
            if (pending_reads_ == 0 and not FLAGS_cb_async) {
              *r_q_ptr.successful = true;
              return;
            }

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

        cvm::messenger::task<void> process(const axi_sw_defs::reorder_q_flush_t&) {
            if (a_window_.size() != 0) {
              co_await pop_reorder_q(true);
              all_resp();
            }
        }

        void r_resp() {
            while ( (r_dpi_fifo_.wptr_ - r_dpi_fifo_.rptr_) < r_dpi_fifo_.max_ ) {
              auto [valid, result] = axi_->r(false);
              if (!valid)
                break;
              --pending_reads_;
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

        void b_resp() {
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

        void all_resp() {
          r_resp();
          b_resp();
        };

        void reset_ptrs() {
            cvm::log(cvm::HIGH, "[axi_sw] reset_ptrs loc={}\n", loc_);
            r_dpi_fifo_.rptr_ = 0;
            r_dpi_fifo_.wptr_ = 0;
            r_dpi_fifo_.rptr_update_time_ = 0;

            b_dpi_fifo_.rptr_ = 0;
            b_dpi_fifo_.wptr_ = 0;
            b_dpi_fifo_.rptr_update_time_ = 0;

            if (axi_) {
                axi_->reset();
            }
        }

        void set_scope(svScope scope) {
            scope_ = scope;
        }

        svScope scope_;
        cvm::topology::loc_t loc_;
        unsigned id_;
        std::string name_;
        size_t id_width_;
        size_t data_width_;
        size_t strb_width_;

        ptr_t<r_q_ptr_t> r_dpi_fifo_;
        int r_q_rptr_blocking_update_consecutive_spurious_calls_ = 0;
        std::queue<axi::r_t> r_q_;
        std::mutex r_q_mutex_;
        std::mutex r_dpi_mutex_;

        bool r_dpi() {
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

            uint16_t latency =  cvm::rand::lcg::generate<uint64_t>(add_latency_max_ - add_latency_min_) + add_latency_min_;

            switch (data_width_) {
              case  64: _axi_sw::axi_sw_r_8(r.id, r.resp, r.data.data(), r.last, latency); break;
              case 256: _axi_sw::axi_sw_r_32(r.id, r.resp, r.data.data(), r.last, latency); break;
              case 512: _axi_sw::axi_sw_r_64(r.id, r.resp, r.data.data(), r.last, latency); break;
              default:  cvm::log(cvm::ERROR, "[axi_sw] Error: unsupported data width for axi_sw");
            }

            return true;
        }

        ptr_t<b_q_ptr_t> b_dpi_fifo_;
        std::queue<axi::b_t> b_q_;
        std::mutex b_q_mutex_;
        std::mutex b_dpi_mutex_;

        bool b_dpi() {
            axi::b_t b;
            {
                std::lock_guard<std::mutex> l(b_q_mutex_);
                if (b_q_.empty()) return false;
                b = b_q_.front();
                b_q_.pop();
            }

            cvm::log(cvm::FULL, "[axi_sw] axi_sw_b: id={}\n", b.id);

            uint8_t latency = cvm::rand::lcg::generate<uint64_t>(add_latency_max_ - add_latency_min_) + add_latency_min_;
            _axi_sw::axi_sw_b(b.id, b.resp, latency);
            return true;
        }

        std::vector<axi::a_t> a_window_;
        std::vector<axi::w_t> w_window_;

        axi* axi_;

        uint64_t read_bytes_;
        uint64_t write_bytes_;

        uint16_t add_latency_min_ = 0;
        uint16_t add_latency_max_ = 1;

        uint64_t pending_reads_ = 0;

    public:

        axi_sw(cvm::topology::loc_t loc, unsigned id)
          : scope_(nullptr), loc_(loc), id_(id), name_(_axi_sw::string_to_lower(cvm::topology::name(loc_)) + std::to_string(id)),
            id_width_(cvm::topology::attr(loc_, "ID_WIDTH").second),
            data_width_(cvm::topology::attr(loc_, "DATA_WIDTH").second),
            strb_width_(cvm::topology::attr(loc_, "STRB_WIDTH").second),
            r_dpi_fifo_(cvm::topology::attr(loc, "R_Q_MAX").second, cvm::topology::attr(loc, "R_Q_PTR_MAX").second, 0, 0, 0),
            b_dpi_fifo_(cvm::topology::attr(loc, "B_Q_MAX").second, cvm::topology::attr(loc, "B_Q_PTR_MAX").second, 0, 0, 0),
            read_bytes_(0), write_bytes_(0) {

            _axi_sw::axi_sw_destroyed = false;

            auto data_width = cvm::topology::attr(loc, "DATA_WIDTH").second;
            axi_ = new axi(data_width, loc, name_);
            cvm::registry::messenger.connect<svScope>(
                loc_,
                [this](svScope s) {
                    this->set_scope(s);
                    return this->reset_ptrs();
                });


            std::tie(std::ignore, add_latency_min_, add_latency_max_) = _axi_sw::get_uint64_pair(FLAGS_axi_sw_add_response_latency_range);

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

        ~axi_sw() {

            cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"{}_read_bytes\": {}}}\n", name_, read_bytes_);
            cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"{}_write_bytes\": {}}}\n", name_, write_bytes_);

            if (axi_) {
                delete axi_;
                axi_ = nullptr;
            }
            _axi_sw::axi_sw_destroyed = true;
        }

        cvm::messenger::task<void> a(const axi::a_t&  p) { co_await axi_->a(std::forward<const axi::a_t>(p)); co_return; }

        cvm::messenger::task<void> w(      axi::w_t&& p) { co_await axi_->w(std::forward<      axi::w_t>(p)); co_return; }

        void r();

        axi::data_width_t   data_width()   const { return axi_->data_width()  ; }
        axi::strobe_width_t strobe_width() const { return axi_->strobe_width(); }
};
