#pragma once
#include <deque>
#include <variant>
#include "axi.h"

#include "rv_tester_transactions.hpp"

template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
class axi_sw_mst {

    private:

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

        inline bool used_id(uint32_t id) {
            return !ids_[id];
        }

        inline void free_id(uint32_t id) {
            ids_[id] = true;
        }

        bool next_id(uint32_t& id) {
            auto it = std::find(ids_.begin(), ids_.end(), true);

            if (it == ids_.end())
              return false;

            id = it - ids_.begin();
            *it = false;
            return true;
        }

        void process(const B& b);
        void process(const R& r);
        void process(const ARQ& ar_q_ptr);
        void process(const AWQ& aw_q_ptr);
        void process(const WQ& w_q_ptr);
        void process(const axi::a_t& a);
        void process(const axi::w_t& w);
        void process(const transactor::read_request_t& req);
        void process(const transactor::write_request_t& req);

        bool a_wrapper(uint64_t req_addr, size_t req_length, axi::a_t& a);
        //bool a_wrapper(uint64_t req_addr, size_t req_length, axi::burst_t req_burst,axi::cache_mem_attr_t req_cache,axi::prot_t req_prot,axi::qos_t req_qos,axi::user_t req_user , axi::a_t& a);
        void push_transactions();

        void reset_ptrs();

        void set_scope(svScope scope);
        svScope scope_;
        cvm::topology::loc_t loc_;
        size_t id_width_;
        size_t data_width_;
        size_t strb_width_;

        const size_t ar_q_max_, ar_q_ptr_max_;
        const size_t aw_q_max_, aw_q_ptr_max_;
        const size_t w_q_max_, w_q_ptr_max_;

        uint32_t ar_q_rptr_, ar_q_wptr_;
        uint32_t aw_q_rptr_, aw_q_wptr_;
        uint32_t w_q_rptr_, w_q_wptr_;

        std::vector<bool> ids_;
        std::vector<size_t> sizes_;
        std::unordered_map<size_t, std::vector<uint8_t>> read_data_;
        std::deque<std::variant<axi::a_t, axi::w_t>> transactions_;

    public:

        axi_sw_mst(cvm::topology::loc_t loc, unsigned id);
};
