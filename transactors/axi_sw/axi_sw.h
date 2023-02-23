#pragma once

#include <cinttypes>
#include <cstdio>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <cassert>
#include <iostream>
#include "axi.h"
#include "axi_sw.h"
#include "svdpi.h"

class axi_sw {

    public:

        typedef std::uint32_t r_q_ptr_t   ;

    private:

        svScope scope_;

        bool r_poll_;

        const r_q_ptr_t     r_q_max_    ;
        const r_q_ptr_t     r_q_ptr_max_;


        r_q_ptr_t r_q_rptr_, r_q_wptr_;

        // TODO switch to c++20, change r_q_rtpr_ to std::atomic, get rid of this mutex and conditional
        mutable std::mutex r_q_rptr_m_, r_q_wptr_m_;
        std::condition_variable r_q_rptr_c_;

        axi* axi_;

    public:

        axi_sw(const svScope& scope, unsigned num, bool r_poll, const axi::data_width_t& data_width, const std::string& tag, const r_q_ptr_t& r_q_max, const r_q_ptr_t& r_q_ptr_max);

        ~axi_sw();

        void r_q_rptr(const r_q_ptr_t& r_q_rptr);

        void a(const axi::a_t&  p) { axi_->a(std::forward<const axi::a_t>(p)); }

        void w(      axi::w_t&& p) { axi_->w(std::forward<      axi::w_t>(p)); }

        void r(bool block = false);

        axi::data_width_t   data_width()   const { return axi_->data_width()  ; }
        axi::strobe_width_t strobe_width() const { return axi_->strobe_width(); }

};
