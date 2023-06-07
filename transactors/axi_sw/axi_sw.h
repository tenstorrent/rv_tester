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

class axi_sw {

    template<typename T, typename... Args> void connect() {
      cvm::registry::messenger.connect<T>(
          loc_,
          [this] (const T& v) {
              return this->process(v);
          }
      );
      if constexpr (sizeof...(Args))
        connect<Args...>();
    }

    public:

        typedef std::uint32_t r_q_ptr_t   ;

    private:

        void process(const rv_tester_transactions::axi_sw::aw& aw);
        void process(const rv_tester_transactions::axi_sw::ar& ar);
        void process(const rv_tester_transactions::axi_sw::w& w);
        void process(const rv_tester_transactions::axi_sw::r_q_ptr& r_ptr);
        void r_resp();
        void set_scope(svScope scope);

        svScope scope_;
        cvm::topology::loc_t loc_;

        r_q_ptr_t r_q_rptr_, r_q_wptr_;

        const r_q_ptr_t     r_q_max_    ;
        const r_q_ptr_t     r_q_ptr_max_;

        // TODO switch to c++20, change r_q_rtpr_ to std::atomic, get rid of this mutex and conditional
        mutable std::mutex r_q_rptr_m_, r_q_wptr_m_;
        std::condition_variable r_q_rptr_c_;

        axi* axi_;

    public:

        axi_sw(cvm::topology::loc_t loc, unsigned id);

        ~axi_sw();

        void r_q_rptr(const r_q_ptr_t& r_q_rptr);

        void a(const axi::a_t&  p) { axi_->a(std::forward<const axi::a_t>(p)); }

        void w(      axi::w_t&& p) { axi_->w(std::forward<      axi::w_t>(p)); }

        void r();

        axi::data_width_t   data_width()   const { return axi_->data_width()  ; }
        axi::strobe_width_t strobe_width() const { return axi_->strobe_width(); }

};
