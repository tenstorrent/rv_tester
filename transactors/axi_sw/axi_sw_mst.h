#pragma once
#include "axi.h"

#include "rv_tester_transactions.hpp"

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

        void process(const rv_tester_transactions::axi_sw_mst::b& b);
        void process(const rv_tester_transactions::axi_sw_mst::r& r);

        void set_scope(svScope scope);
        svScope scope_;
        cvm::topology::loc_t loc_;
        size_t id_max_;

    public:

        axi_sw_mst(cvm::topology::loc_t loc, unsigned id);


        /*
        struct read_request_t {
            uint64_t addr;
            size_t length;
            std::function<void(read_response_t)>& cb;
        };
        struct read_response_t {
            std::vector<uint8_t> data;
        };
        void read_request(const read_request_t&);
        */
};
