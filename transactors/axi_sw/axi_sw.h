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

struct axi_sw_defs {

    typedef std::uint32_t r_q_ptr_t   ;

    struct r_q_ptr_blocking_update_t {
        std::uint64_t clock;
        r_q_ptr_t r_ptr;
        bool* successful;
        std::atomic<bool>* done;
    };

};

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

        using r_q_ptr_t = axi_sw_defs::r_q_ptr_t;

    private:

        cvm::messenger::task<void> process(const AW& aw);
        cvm::messenger::task<void> process(const AR& ar);
        cvm::messenger::task<void> process(const  W& w);
        void process(const RQ& r_ptr);
        void process(const axi_sw_defs::r_q_ptr_blocking_update_t& r_q_ptr);
        void r_resp();
        void set_scope(svScope scope);
        void reset_ptrs();

        svScope scope_;
        cvm::topology::loc_t loc_;
        unsigned id_;
        std::string name_;
        size_t id_width_;
        size_t data_width_;
        size_t strb_width_;

        const r_q_ptr_t     r_q_max_    ;
        const r_q_ptr_t     r_q_ptr_max_;

        r_q_ptr_t r_q_rptr_, r_q_wptr_;
        std::uint64_t r_q_rptr_update_time_;
        int r_q_rptr_blocking_update_consecutive_spurious_calls_ = 0;

        std::vector<axi::r_t> r_q_;
        std::mutex r_q_mutex_;
        std::mutex r_dpi_mutex_;
        bool r_dpi();

        axi* axi_;

        uint64_t read_bytes_;
        uint64_t write_bytes_;

    public:

        axi_sw(cvm::topology::loc_t loc, unsigned id);

        ~axi_sw();

        cvm::messenger::task<void> a(const axi::a_t&  p) { co_await axi_->a(std::forward<const axi::a_t>(p)); co_return; }

        cvm::messenger::task<void> w(      axi::w_t&& p) { co_await axi_->w(std::forward<      axi::w_t>(p)); co_return; }

        void r();

        axi::data_width_t   data_width()   const { return axi_->data_width()  ; }
        axi::strobe_width_t strobe_width() const { return axi_->strobe_width(); }

        // Helper function to convert a string to lowercase.
        static std::string to_lower(const std::string& s) {
          std::string result = s;
          std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
          return result;
        }
};
