#pragma once
#include <string>
#include <cinttypes>
#include "safe_queue.h"
#include <iostream>
#include <functional>
#include "transactor.h"

class axi : public transactor {

    public:

        typedef std::uint32_t id_t        ;
        typedef std::uint64_t addr_t      ;
        typedef std::uint8_t  len_t       ;
        typedef std::uint8_t  sz_t        ;
        typedef enum : std::uint8_t {
            BURST_FIXED,
            BURST_INCR ,
            BURST_WRAP ,
        } burst_t;
        typedef std::uint8_t  datum_t ;
        typedef std::uint8_t  strbum_t;
        typedef std::vector<datum_t > data_t;
        typedef std::vector<bool>     strb_t;
        typedef std::uint32_t data_width_t  ;
        typedef data_width_t  strobe_width_t;
        typedef std::uint8_t  last_t      ;
        typedef std::uint16_t beat_t      ;

        struct a_t {
            bool    w    ;
            id_t    id   ;
            addr_t  addr ;
            len_t   len  ;
            sz_t    size ;
            burst_t burst;
        };

        struct w_t {
            data_t data;
            strb_t strb;
            last_t last;

            w_t(w_t&&) = default;
            w_t& operator=(w_t&&) = default;
            w_t(const w_t&) = delete;
            w_t& operator=(const w_t&) = delete;
        };

        struct r_t {
            id_t   id  ;
            data_t data;
            last_t last;

            r_t() = default;
            r_t(r_t&&) = default;
            r_t& operator=(r_t&&) = default;
            r_t(const r_t&) = delete;
            r_t& operator=(const r_t&) = delete;
        };

    private:

        const data_width_t  data_width_ ;

        // to/from RTL
        SafeQueue<a_t> a_q_;
        SafeQueue<w_t> w_q_;
        SafeQueue<r_t> r_q_;

        void operator()();
        void run();

    public:

        axi(endpoint* e, const data_width_t& data_width, const std::string& tag)
          : transactor(e, tag), data_width_(data_width)
        { run(); }

        axi(axi&&) = delete;
        axi& operator=(axi&&) = delete;
        axi(const axi&) = delete;
        axi& operator=(const axi&) = delete;

        data_width_t   data_width()   const { return data_width_   ; }
        strobe_width_t strobe_width() const { return data_width()/8; }

        void a(const a_t&);
        void w (w_t &&);
        std::pair<bool, r_t> r(bool block = false);
};
