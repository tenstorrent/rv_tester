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
#include <chrono>
#include <mutex>

#include "rv_tester_transactions.hpp"

template < typename W,typename AW,typename AR, typename RQ>
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

        typedef std::uint32_t r_q_ptr_t   ;

    private:

        cvm::messenger::task<void> process(const AW& aw);
        cvm::messenger::task<void> process(const AR& ar);
        cvm::messenger::task<void> process(const  W& w);
        void process(const RQ& r_ptr);
        void r_resp();
        void set_scope(svScope scope);
        void reset_ptrs();

        svScope scope_;
        cvm::topology::loc_t loc_;
        size_t id_width_;
        size_t data_width_;
        size_t strb_width_;
        
        const r_q_ptr_t     r_q_max_    ;
        const r_q_ptr_t     r_q_ptr_max_;

        r_q_ptr_t r_q_rptr_, r_q_wptr_;

        axi* axi_;

        std::mutex start_times_mutex;
        using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;
        std::unordered_map<axi::id_t, std::tuple<time_point, time_point, time_point, time_point, time_point, time_point, time_point, time_point, time_point>> start_times;
        std::vector<std::tuple<time_point, time_point, time_point, time_point, time_point, time_point, time_point, time_point, time_point, time_point, time_point>> durations;

    public:

        axi_sw(cvm::topology::loc_t loc, unsigned id);

        ~axi_sw();

        cvm::messenger::task<void> a(const axi::a_t&  p) { co_await axi_->a(std::forward<const axi::a_t>(p)); co_return; }

        cvm::messenger::task<void> w(      axi::w_t&& p) { co_await axi_->w(std::forward<      axi::w_t>(p)); co_return; }

        void r();

        axi::data_width_t   data_width()   const { return axi_->data_width()  ; }
        axi::strobe_width_t strobe_width() const { return axi_->strobe_width(); }

};
