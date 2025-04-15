#pragma once
#include <string>
#include <cinttypes>
#include "safe_queue.h"
#include <iostream>
#include <functional>
#include "transactor.h"
#include "cvm/bitmanip.hpp"
#include "cvm/messenger.hpp"

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
        typedef enum : std::uint8_t {
            RESP_OKAY  ,
            RESP_EXOKAY,
            RESP_SLVERR,
            RESP_DECERR,
        } resp_t;
        typedef std::uint8_t  datum_t ;
        typedef std::uint8_t  strbum_t;
        typedef std::vector<datum_t > data_t;
        typedef std::vector<bool>     strb_t;
        typedef std::uint32_t data_width_t  ;
        typedef data_width_t  strobe_width_t;
        typedef std::uint8_t  last_t      ;
        typedef std::uint16_t beat_t      ;

        typedef enum {
            NON_ATOMIC,
            ATOMIC_STORE,
            ATOMIC_LOAD,
            ATOMIC_SWAP,
            ATOMIC_COMPARE,
        } atop_transaction;

        typedef enum {
            ATOP_ADD,
            ATOP_CLR,
            ATOP_EOR,
            ATOP_SET,
            ATOP_SMAX,
            ATOP_SMIN,
            ATOP_UMAX,
            ATOP_UMIN,
            ATOP_INVALID,
        } atop_operation;

        typedef enum {
           DEV_NBUF,
           DEV_BUF,
           NC_NBUF,
           NC_BUF,
           WT_NA,
           WT_RA,
           WT_WA,
           WT_RWA,
           WB_NA,
           WB_RA,
           WB_WA,
           WB_RWA,
        } cache_mem_attr_t;

        typedef std::uint8_t  prot_t;
        typedef std::uint8_t  qos_t;
        typedef std::uint8_t  region_t;
        typedef std::uint8_t  user_t;

        struct atop_t {
            atop_transaction transaction;
            atop_operation   operation;
            atop_t(std::uint8_t raw) :
                transaction(
                        atop_transaction(
                            raw >> 4 == 3 ?
                            ((raw >> 4) + (raw & 1)) :
                            raw >> 4
                        )),
                operation(atop_operation(raw & 0xf)) {}
        };

        struct a_no_id_t {
            bool              w    ;
            addr_t            addr ;
            sz_t              size ;
            len_t             len = len_t(0);
            burst_t           burst = axi::BURST_INCR;
            bool              lock = false;
            cache_mem_attr_t  cache = cache_mem_attr_t(0);
            prot_t            prot = prot_t(0);
            qos_t             qos = qos_t(0);
            region_t          region = region_t(0);
            atop_t            atop = atop_t(0);
            user_t            user = user_t(0);
            bool              rsp_err_chk = true;

            a_no_id_t(const bool& w, const addr_t& addr, const sz_t& size) : w(w), addr(addr), size(size) {}
            a_no_id_t(const addr_t& addr, const sz_t& size) : addr(addr), size(size) {}
            a_no_id_t(const addr_t& addr, const sz_t& size, const bool& rsp_err_chk) : addr(addr), size(size), rsp_err_chk(rsp_err_chk) {}
            a_no_id_t(const addr_t& addr, const sz_t& size, const user_t& user) : addr(addr), size(size), user(user) {}
            a_no_id_t(const addr_t& addr, const sz_t& size, const user_t& user, const bool& rsp_err_chk) : addr(addr), size(size), user(user), rsp_err_chk(rsp_err_chk) {}
            a_no_id_t() = default;
            a_no_id_t(a_no_id_t&&) = default;
            a_no_id_t& operator=(a_no_id_t&&) = default;
            a_no_id_t(const a_no_id_t&) = default;
            a_no_id_t& operator=(const a_no_id_t&) = default;
        };

        struct a_t {
            bool              w    ;
            id_t              id   ;
            addr_t            addr ;
            len_t             len = len_t(0);
            sz_t              size ;
            burst_t           burst = axi::BURST_INCR;
            bool              lock = false;
            cache_mem_attr_t  cache = cache_mem_attr_t(0);
            prot_t            prot = prot_t(0);
            qos_t             qos = qos_t(0);
            region_t          region = region_t(0);
            atop_t            atop = atop_t(0);
            user_t            user = user_t(0);
            bool              rsp_err_chk = true;

            a_t(const bool& w, const id_t& id, const addr_t& addr, const len_t& len, const sz_t& size, const burst_t& burst, const bool& lock,
                const cache_mem_attr_t& cache, const prot_t& prot, const qos_t& qos, const region_t& region, const atop_t& atop, const user_t& user) :
                w(w), id(id), addr(addr), len(len), size(size), burst(burst), lock(lock), cache(cache), prot(prot), qos(qos), region(region), atop(atop), user(user) {}
            a_t(const bool& w, const addr_t& addr, const sz_t& size) : w(w), addr(addr), size(size) {}
            a_t(const a_no_id_t& a) : w(a.w), addr(a.addr), len(a.len), size(a.size), burst(a.burst), lock(a.lock),
              cache(a.cache), prot(a.prot), qos(a.qos), region(a.region), atop(a.atop), user(a.user), rsp_err_chk(a.rsp_err_chk) {}
            a_t() = default;
            a_t(a_t&&) = default;
            a_t& operator=(a_t&&) = default;
            a_t(const a_t&) = default;
            a_t& operator=(const a_t&) = default;
        };

        struct w_t {
            data_t data;
            strb_t strb;
            last_t last;

            w_t(const data_t& data, const strb_t& strb, const last_t& last) : data(data), strb(strb), last(last) {}
            w_t() = default;
            w_t(w_t&&) = default;
            w_t& operator=(w_t&&) = default;
            w_t(const w_t&) = default;
            w_t& operator=(const w_t&) = default;
        };

        struct r_t {
            id_t   id  ;
            resp_t resp;
            data_t data;
            last_t last;

            r_t(const id_t& id, const resp_t& resp, const data_t& data, const last_t& last) : id(id), resp(resp), data(data), last(last) {}
            r_t() = default;
            r_t(r_t&&) = default;
            r_t& operator=(r_t&&) = default;
            r_t(const r_t&) = default;
            r_t& operator=(const r_t&) = default;
        };

        struct b_t {
            id_t   id;
            resp_t resp;
            b_t(const id_t& id, const resp_t& resp) : id(id), resp(resp) {}
            b_t() = default;
            b_t(b_t&&) = default;
            b_t& operator=(b_t&&) = default;
            b_t(const b_t&) = default;
            b_t& operator=(const b_t&) = default;
        };

    private:

        std::vector<std::pair<uint64_t, uint64_t>> hang_addr_;
        std::vector<std::pair<uint64_t, uint64_t>> slverr_addr_;
        std::vector<std::pair<uint64_t, uint64_t>> decerr_addr_;
        std::vector<int> slverr_count_;
        std::vector<int> decerr_count_;
        const data_width_t  data_width_ ;

        // to/from RTL
        SafeQueue<a_t> a_q_;
        SafeQueue<w_t> w_q_;
        SafeQueue<r_t> r_q_;
        SafeQueue<b_t> b_q_;

        cvm::messenger::task<void> operator()();
        void atop_modify_write_data(const atop_t& atop, const data_t& read_data, data_t& write_data, const len_t& len);
        std::vector<std::pair<uint64_t, uint64_t>> parse_hex_ranges(const std::string& input);

        // Metric counts
        std::string tag_;
        int num_slverr_resp_;
        int num_decerr_resp_;

    public:

        axi(const data_width_t& data_width, const cvm::topology::loc_t loc, const std::string& tag);
        axi(axi&&) = delete;
        axi& operator=(axi&&) = delete;
        axi(const axi&) = delete;
        axi& operator=(const axi&) = delete;
        ~axi();

        CVM_MESSENGER_procedure_call(configure_resp_rpc, void ());
        void configure_resp();

        data_width_t   data_width()   const { return data_width_   ; }
        strobe_width_t strobe_width() const { return data_width()/8; }

        cvm::messenger::task<void> a(const a_t&);
        cvm::messenger::task<void> w (w_t &&);
        std::pair<bool, r_t> r(bool block = false);
        std::pair<bool, b_t> b();
};
