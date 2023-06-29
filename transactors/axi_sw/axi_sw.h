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

        cvm::messenger::task<void> process_aw();
        cvm::messenger::task<void> process_ar();
        cvm::messenger::task<void> process_w();
        void process(const rv_tester_transactions::axi_sw::r_q_ptr& r_ptr);
        void r_resp();
        void set_scope(svScope scope);
        void reset_ptrs();

        svScope scope_;
        cvm::topology::loc_t loc_;

        const r_q_ptr_t     r_q_max_    ;
        const r_q_ptr_t     r_q_ptr_max_;

        r_q_ptr_t r_q_rptr_, r_q_wptr_;

        axi* axi_;

    public:

        axi_sw(cvm::topology::loc_t loc, unsigned id);

        ~axi_sw();

        cvm::messenger::task<void> a(const axi::a_t&  p) { co_await axi_->a(std::forward<const axi::a_t>(p)); co_return; }

        cvm::messenger::task<void> w(      axi::w_t&& p) { co_await axi_->w(std::forward<      axi::w_t>(p)); co_return; }

        void r();

        axi::data_width_t   data_width()   const { return axi_->data_width()  ; }
        axi::strobe_width_t strobe_width() const { return axi_->strobe_width(); }

};
