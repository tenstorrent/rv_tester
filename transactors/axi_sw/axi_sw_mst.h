#pragma once
#include <vector>
#include <variant>
#include "axi.h"

#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"

DECLARE_bool(axi_rand_id_alloc);
template <typename B, typename R, typename ARQ, typename AWQ, typename WQ>
class axi_sw_mst {

    public:

        struct lock_t {

          public:

            lock_t() = delete;
            lock_t(const lock_t&) = delete;
            lock_t& operator= (const lock_t&) = delete;
            lock_t(bool* ptr) : lock_(ptr) {};

            ~lock_t() {
              release();
            };

            constexpr explicit operator bool() const { return lock_ && *lock_; };

            void release() {
              if (lock_)
                *lock_ = false;
            }

          private:

            bool* lock_ = nullptr;
        };

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

        inline void alloc_id(uint32_t id) {
            ids_[id] = false;
        }
        inline bool used_id(uint32_t id) {
            return !ids_[id];
        }

        inline void free_id(uint32_t id) {
            ids_[id] = true;
        }

        bool next_id(uint32_t& id,  axi::seqid_t seqid) {
          std::vector<size_t> valid_indices;
          size_t ids_start = seqid_width_ ? (seqid << (id_width_ - seqid_width_)) : 0;
          size_t ids_end = ids_start + ids_.size() / (1<<seqid_width_);
          for (size_t i = ids_start; i < ids_end; ++i) {
            if (ids_[i]) {
              valid_indices.push_back(i);
            }
          }

          if (valid_indices.empty()){
            return false;
          }

          // Randomly select one of the valid indices
          size_t random_index;
          if(FLAGS_axi_rand_id_alloc){
            uint32_t idx =  (rng() % valid_indices.size()) & 0xffffff;
            random_index = valid_indices[idx];	     
          }
          else {
            random_index = valid_indices[0];
          }

          id = random_index;
          ids_[random_index] = false; // Mark as used
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
        bool push_a_no_id(const bool& aw, const axi::a_no_id_t& a_no_id, id_t& id);
        //uint32_t find_id(const std::vector<bool>& vec);
        void push_w(const axi::w_t& w);
        void push_transactions();
        void reset_ptrs();
        void set_scope(svScope scope);

        // If an id is available, claim it and return the value.
        // Otherwise, indicate no ID's are available.
        std::optional<unsigned> claim_id() {
          unsigned id;
          if (next_id(id)) {
            return id;
          }
          else
            return std::nullopt;
        }

        lock_t try_lock() {
          bool locked = locked_;
          locked_ = true;
          return lock_t{locked? nullptr : &locked_};
        }

        std::optional<unsigned> read(transactor::read_request_t r);

        std::string name_;
        svScope scope_;
        cvm::topology::loc_t loc_;
        unsigned id_;
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
        std::vector<bool> chk_rsp_err_ids_;
        std::vector<size_t> sizes_;
        std::unordered_map<size_t, std::vector<uint8_t>> read_data_;
        std::vector<std::variant<axi::a_t, axi::w_t>> transactions_;

        uint64_t read_bytes_;
        uint64_t write_bytes_;

        cvm::rand::uniform_dist<uint32_t> rng;
        bool locked_ = false;

    public:

        axi_sw_mst(cvm::topology::loc_t loc, unsigned id);
        ~axi_sw_mst();

        CVM_MESSENGER_procedure_call(push_ar_no_id_rpc, bool (const axi::a_no_id_t& ar, axi::id_t& id));
        CVM_MESSENGER_procedure_call(push_aw_no_id_rpc, bool (const axi::a_no_id_t& aw, axi::id_t& id));
        CVM_MESSENGER_procedure_call(push_w_rpc, void (const axi::w_t& w));
        CVM_MESSENGER_procedure_call(try_lock_rpc, lock_t ());
};
