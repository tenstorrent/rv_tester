#include "axi_sw.h"
#include "cvm/topology.hpp"
#include "cvm/plusargs.hpp"

DEFINE_bool(axi_sw_r_poll, true, "poll for read data every cycle, or asynchronously push read data");

extern "C" {

  void axi_sw_r(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last);
}

axi_sw::axi_sw(const svScope& scope, unsigned num, bool r_poll, const axi::data_width_t& data_width, const std::string& tag, const r_q_ptr_t& r_q_max, const r_q_ptr_t& r_q_ptr_max)
  : scope_(scope), r_poll_(r_poll), r_q_max_(r_q_max), r_q_ptr_max_(r_q_ptr_max), r_q_rptr_(0), r_q_wptr_(0) {

    auto location = cvm::topology::get("TOP.PLATFORM", num);
    axi_ = new axi(data_width, location, tag);

    if (!r_poll_) {
        std::thread([&] () {
                while(1) {
                    r(true);
                }
                }).detach();
    }
}

axi_sw::~axi_sw() {
    if (axi_) {
        delete axi_;
        axi_ = nullptr;
    }
}

void axi_sw::r(bool block) {
    if (block) {
      auto [_, r] = axi_->r(true);
      std::unique_lock<std::mutex> lock(r_q_rptr_m_);
      while ( (r_q_wptr_ - r_q_rptr_) >= r_q_max_ ) {
        r_q_rptr_c_.wait(lock);
      }
      r_q_wptr_ = (r_q_wptr_ + 1) % r_q_ptr_max_;
      svSetScope(scope_);
      axi_sw_r(r.id, r.resp, r.data.data(), r.last);
    } else {
      std::unique_lock<std::mutex> lock(r_q_rptr_m_);
      if ( (r_q_wptr_ - r_q_rptr_) >= r_q_max_ ) {
        return;
      }
      auto [valid, r] = axi_->r(false);
      if (!valid) {
        return;
      }
      r_q_wptr_ = (r_q_wptr_ + 1) % r_q_ptr_max_;
      svSetScope(scope_);
      axi_sw_r(r.id, r.resp, r.data.data(), r.last);
    }
}

void axi_sw::r_q_rptr(const r_q_ptr_t& r_q_rptr) {
    std::lock_guard<std::mutex> lock(r_q_rptr_m_);
    r_q_rptr_ = r_q_rptr;
    r_q_rptr_c_.notify_one();
}

extern "C" {
  axi_sw* axi_sw_new(unsigned num, axi::data_width_t data_width, const char* tag,
    axi_sw::r_q_ptr_t r_q_max, axi_sw::r_q_ptr_t r_q_ptr_max) {
    svScope scope = svGetScope();
    return new axi_sw(scope, num, FLAGS_axi_sw_r_poll, data_width, tag, r_q_max, r_q_ptr_max);
  }

  void axi_sw_aw(axi_sw* a, axi::id_t id, axi::addr_t addr, axi::len_t len, axi::sz_t size, axi::burst_t burst, std::uint8_t lock, std::uint8_t atop) {
    a->a(axi::a_t{true , id, addr, len, size, burst, lock != 0, atop});
  }

  void axi_sw_ar(axi_sw* a, axi::id_t id, axi::addr_t addr, axi::len_t len, axi::sz_t size, axi::burst_t burst, std::uint8_t lock) {
    a->a(axi::a_t{false, id, addr, len, size, burst, lock != 0});
  }

  void axi_sw_w(axi_sw* a, const axi::datum_t* data, const axi::strbum_t* strb, axi::last_t last) {
    axi::strb_t vstrb(a->strobe_width(), false);
    for (std::size_t i = 0; i < vstrb.size(); i++) {
        vstrb[i] = (strb[i/8] >> (i%8)) & 1;
    }
    a->w(axi::w_t{
                axi::data_t(data, data + a->data_width()/8),
                vstrb,
                last
                }
        );
  }

  void axi_sw_r_ptr(axi_sw* a, axi_sw::r_q_ptr_t r_ptr) {
    a->r_q_rptr(r_ptr);
  }

  void axi_sw_r_poll(axi_sw* a) {
    a->r(false);
  }
}
