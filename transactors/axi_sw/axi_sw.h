#pragma once

#include <cinttypes>
#include <cstdio>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <cassert>
#include <iostream>
#include <type_traits>
#include "axi.h"
#include "axi_sw.h"
#include "svdpi.h"

#include "rv_tester_transactions.hpp"

DECLARE_bool(cb_async);

struct axi_sw_reset_t {};

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

        cvm::messenger::task<bool> pop_reorder_q(bool oldest);
        cvm::messenger::task<void> process(const AW& aw);
        cvm::messenger::task<void> process(const AR& ar);
        cvm::messenger::task<void> process(const  W& w);
        void process(const RQ& r_ptr);
        void process(const BQ& b_ptr);
        void process(const axi_sw_defs::r_q_ptr_blocking_update_t& r_q_ptr);
        cvm::messenger::task<void> process(const axi_sw_defs::reorder_q_flush_t&);
        void r_resp();
        void b_resp();
        void all_resp() {
          r_resp();
          b_resp();
        };
        void reset_ptrs();

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
        bool r_dpi();

        ptr_t<b_q_ptr_t> b_dpi_fifo_;
        std::queue<axi::b_t> b_q_;
        std::mutex b_q_mutex_;
        std::mutex b_dpi_mutex_;
        bool b_dpi();

        std::vector<axi::a_t> a_window_;
        std::vector<axi::w_t> w_window_;

        axi* axi_;

        uint64_t read_bytes_;
        uint64_t write_bytes_;

        uint16_t add_latency_min_ = 0;
        uint16_t add_latency_max_ = 1;

        uint64_t pending_reads_ = 0;

    public:

        axi_sw(cvm::topology::loc_t loc, unsigned id);

        ~axi_sw();

        cvm::messenger::task<void> a(const axi::a_t&  p) { co_await axi_->a(std::forward<const axi::a_t>(p)); co_return; }

        cvm::messenger::task<void> w(      axi::w_t&& p) { co_await axi_->w(std::forward<      axi::w_t>(p)); co_return; }

        void r();

        axi::data_width_t   data_width()   const { return axi_->data_width()  ; }
        axi::strobe_width_t strobe_width() const { return axi_->strobe_width(); }
};
