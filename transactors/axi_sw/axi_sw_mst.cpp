#include "axi_sw_mst.h"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "cvm/bitmanip.hpp"

//REGISTRY_register(axi_sw_mst, AXI_MST, cvm::registry::all);
//

axi_sw_mst::axi_sw_mst(cvm::topology::loc_t loc, unsigned /*id*/)
    : scope_(nullptr), loc_(loc), id_max_(cvm::bitmanip::mask<decltype(id_max_)>(cvm::topology::attr(loc_, "ID_WIDTH").second)) {

    cvm::registry::messenger.connect<svScope>(
        loc_,
        [&](svScope s) { return this->set_scope(s); });

    connect<
        rv_tester_transactions::axi_sw_mst::b,
        rv_tester_transactions::axi_sw_mst::r
    >();

}

void axi_sw_mst::process(const rv_tester_transactions::axi_sw_mst::b& b) {
    /*
    if (b.resp != axi::RESP_OKAY) {
        // could have EXOKAY if it was locked, but assume not for now
        cvm::log(cvm::ERROR, "[AXI] bad response id:{} resp:{}", b.id, b.resp);
    }
    */
}

void axi_sw_mst::process(const rv_tester_transactions::axi_sw_mst::r& /*r*/) {
    /*
    cvm::registry::messenger.signal<axi::r_t>(
        loc_,
        axi::r_t(r.id, axi::resp_t(r.resp), r.data, r.last)
    );
    */
}

/*
void axi_sw_mst::read_request(const read_request_t& r) {
    cvm::registry::callbacks.push(
        scope_,
        [r]() { axi_sw_ar(copy.id, copy.resp, copy.data.data(), copy.last); }
    );
}
*/

void axi_sw_mst::set_scope(svScope scope) {
    scope_ = scope;
}

extern "C" {

  void axi_sw_mst_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();
    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

}
