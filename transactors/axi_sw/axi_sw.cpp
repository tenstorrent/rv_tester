#include "axi_sw.h"
#include "cvm/topology.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/registry.hpp"
#include "cvm/bitmanip.hpp"
#include "cvm/logger.hpp"
#include <numeric>
#include <algorithm>

REGISTRY_register((axi_sw<rv_tester_transactions::axi_sw::w<>,
                         rv_tester_transactions::axi_sw::aw<>,
                         rv_tester_transactions::axi_sw::ar<>,
                         rv_tester_transactions::axi_sw::r_q_ptr<>>), AXI, cvm::registry::all);

REGISTRY_register((axi_sw<rv_tester_transactions::axi_sw::w<1>,
                         rv_tester_transactions::axi_sw::aw<1>,
                         rv_tester_transactions::axi_sw::ar<1>,
                         rv_tester_transactions::axi_sw::r_q_ptr<1>>), NCIO_AXI, cvm::registry::all);

extern "C" {

  void axi_sw_r_reset();
  void axi_sw_r_8(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last);
  void axi_sw_r_64(axi::id_t id, axi::resp_t resp, const axi::datum_t* data, axi::last_t last);
}



template < typename W,typename AW,typename AR, typename RQ>
axi_sw<W,AW,AR,RQ>::axi_sw(cvm::topology::loc_t loc, unsigned id)
  : scope_(nullptr), loc_(loc),
    id_width_(cvm::topology::attr(loc_, "ID_WIDTH").second),
    data_width_(cvm::topology::attr(loc_, "DATA_WIDTH").second),
    strb_width_(cvm::topology::attr(loc_, "STRB_WIDTH").second),
    r_q_max_(cvm::topology::attr(loc, "R_Q_MAX").second), r_q_ptr_max_(cvm::topology::attr(loc, "R_Q_PTR_MAX").second),
    r_q_rptr_(0), r_q_wptr_(r_q_max_) 

    {
    cvm::log(cvm::FULL, "[axi_sw] Constructing axi_sw for loc=%d id=%d\n", loc,id);
    auto data_width = cvm::topology::attr(loc, "DATA_WIDTH").second;
    axi_ = new axi(data_width, loc, "axi" + std::to_string(id));
    cvm::registry::messenger.connect<svScope>(
        loc_,
        [this](svScope s) {
        this->set_scope(s);
        return reset_ptrs();
    });

   connect_task<W,AW,AR>();

    connect<RQ>();
}

template<typename tuple_t>
constexpr auto get_array_from_tuple(tuple_t&& tuple)
{
        constexpr auto get_array = [](auto&& ... x){ return std::array{std::forward<decltype(x)>(x) ... };  };
            return std::apply(get_array, std::forward<tuple_t>(tuple));
            
}

template < typename W,typename AW,typename AR, typename RQ>
axi_sw<W,AW,AR,RQ>::~axi_sw() {
    if (durations.size()) {
        //auto average = double(std::accumulate(durations.begin(), durations.end(), std::chrono::duration<std::chrono::nanoseconds>{0}).count())/durations.size();
        std::array<std::uint64_t, 11> sum{{0,}};
        std::array<std::uint64_t, 11> max{{0,}};
        enum times {
            e_birth,
            e_signal_enqueued,
            e_prev_func_start,
            e_prev_func_finish,
            e_sleep,
            e_wakeup,
            e_signal_swap,
            e_dispatch,
            e_receive,
            e_enqueued,
            e_total,
        };
        constexpr std::array<std::pair<int, const char*>, sum.size()> info {{
            {-1, "BIRTH"},
            {e_birth, "SIGNAL_ENQUEUED"},
            {-1, "PREV_FUNC_START"},
            {-1, "PREV_FUNC_FINISH"},
            {-1, "SLEEP"},
            {-1, "WAKEUP"},
            {e_signal_enqueued, "SIGNAL_SWAP"},
            {e_signal_swap, "DISPATCH"},
            {e_dispatch, "RECEIVE"},
            {e_receive, "ENQUEUD"},
            {e_enqueued, "TOTAL"},
        }};
        size_t max_index = 0;
        size_t index = 0;
        for (const auto& tup : durations) {
            const auto& [birth, signal_enqueued, prev_func_start_time, prev_func_finish_time, sleep_time, wakeup_time, signal_swap, dispatch, receive, enqueued, end] = tup;
            const auto arr = get_array_from_tuple(tup);
            if (signal_enqueued < birth          ) cvm::log(cvm::NONE, "Error: signal_enqueued before birth\n");
            if (signal_swap     < signal_enqueued) cvm::log(cvm::NONE, "Error: signal_swap before signal_enqueued\n");
            if (dispatch        < signal_swap    ) cvm::log(cvm::NONE, "Error: dispatch before signal_swap\n");
            if (receive         < dispatch       ) cvm::log(cvm::NONE, "Error: receive before dispatch\n");
            if (enqueued        < receive        ) cvm::log(cvm::NONE, "Error: enqueued before receive\n");
            if (end             < enqueued       ) cvm::log(cvm::NONE, "Error: end before enqueued\n");

            static_assert(arr.size() == sum.size(), "bad sum size");
            static_assert(sum.size() == max.size(), "bad max size");
            for(size_t i = 1; i < arr.size(); i++) {
                if (info[i].first >= 0 && arr[i] < arr[info[i].first]) {
                    cvm::log(cvm::NONE, "Error: bad ordering {} before {}", info[i].second, info[info[i].first].second);
                }
                if (arr[i] > arr[0]) {
                    std::uint64_t d = std::chrono::duration_cast<std::chrono::nanoseconds>(arr[i] - arr[0]).count();
                    sum[i] += d;
                    max[i] = std::max(max[i], d);
                    if (i == (arr.size() - 1) && max[i] == d) {
                        max_index = index;
                    }
                }
            }
            index++;
        }

        for(size_t i = 1; i < info.size(); i++) {
            cvm::log(cvm::NONE, "{: >20} DURATIONS average {: >20}ns max {: >20}ns count {}\n", info[i].second, double(sum[i])/durations.size(), max[i], durations.size());
        }
        for(size_t i = 0; i < info.size(); i++) {
            const auto arr = get_array_from_tuple(durations[max_index]);
            cvm::log(cvm::NONE, "{: >20} DURATION OF LONGEST {: >20}ns\n", info[i].second, std::chrono::duration_cast<std::chrono::nanoseconds>(arr[i] - arr[0]).count());
        }
        cvm::log(cvm::NONE, "Longest transaction started at {}ms\n", std::chrono::duration_cast<std::chrono::milliseconds>(std::get<0>(durations[max_index]).time_since_epoch()).count());
    }
    if (axi_) {
        delete axi_;
        axi_ = nullptr;
    }
}

template < typename W,typename AW,typename AR, typename RQ>
cvm::messenger::task<void> axi_sw<W,AW,AR,RQ>::process(const AW& aw) {
    cvm::log(cvm::FULL, "[axi_sw] aw: [id={}, addr={:#x}, size={}]\n", aw.id, aw.addr, aw.size);
    co_await a(axi::a_t{true, aw.id, aw.addr, aw.len, aw.size, axi::burst_t(aw.burst), aw.lock != 0, aw.atop});
    r_resp();
    co_return;
}

template < typename W,typename AW,typename AR, typename RQ>
cvm::messenger::task<void> axi_sw<W,AW,AR,RQ>::process(const AR& ar) {
    cvm::log(cvm::FULL, "[axi_sw] ar: [id={}, addr={:#x}, size={}]\n", ar.id, ar.addr, ar.size);
    {
        std::lock_guard<std::mutex> l(start_times_mutex);
        if (start_times.find(ar.id) != start_times.end()) {
            cvm::log(cvm::NONE, "ERROR duplicate read id\n");
        }
        assert(ar.birth < ar.signal_enqueued_time);
        assert(ar.signal_enqueued_time < ar.signal_swap_time);
        assert(ar.signal_swap_time < ar.dispatch_time);
        start_times[ar.id] = std::make_tuple(ar.birth, ar.signal_enqueued_time, ar.prev_func_start_time, ar.prev_func_finish_time, ar.sleep_time, ar.wakeup_time, ar.signal_swap_time, ar.dispatch_time, std::chrono::high_resolution_clock::now());
    }
    co_await a(axi::a_t{false, ar.id, ar.addr, ar.len, ar.size, axi::burst_t(ar.burst), ar.lock != 0});
    r_resp();
    co_return;
}

template < typename W,typename AW,typename AR, typename RQ>
cvm::messenger::task<void> axi_sw<W,AW,AR,RQ>::process(const W& w) {
    cvm::log(cvm::FULL, "[axi_sw] w: [strb={:#x}, last={}]\n", w.strb, w.last);
    axi::data_t vdata = cvm::bitmanip::slice<decltype(w.data), axi::data_t>(w.data);
    axi::strb_t vstrb = cvm::bitmanip::slice<decltype(w.strb), axi::strb_t>(w.strb);

    co_await this->w(axi::w_t(
                      vdata,
                      vstrb,
                      w.last
                      )
    );
    r_resp();
    co_return;
}

template < typename W,typename AW,typename AR, typename RQ>
void axi_sw<W,AW,AR,RQ>::process(const RQ& r_q_ptr) {
    cvm::log(cvm::FULL, "[axi_sw] r_q_ptr: [rptr={}]\n", r_q_ptr.r_ptr);
    r_q_rptr_ = r_q_ptr.r_ptr;
    r_resp();
}

template < typename W,typename AW,typename AR, typename RQ>
void axi_sw<W,AW,AR,RQ>::r_resp() {
    while ( (r_q_wptr_ - r_q_rptr_) < r_q_max_ ) {
      auto [valid, result] = axi_->r(false);
      cvm::log(cvm::FULL, "[axi_sw] r_resp: [r_q dequeue valid={}]\n", valid);
      if (!valid)
        break;
      r_q_wptr_ = (r_q_wptr_ + 1) % r_q_ptr_max_;

      // clang doesn't like structured bindings in a capture list
      auto copy = result;
      cvm::registry::callbacks.push(
          scope_,
            [=,this]() {
            auto end = std::chrono::high_resolution_clock::now();
            {
                std::lock_guard<std::mutex> l(start_times_mutex);
                if (auto it = this->start_times.find(copy.id); it != this->start_times.end()) {
                    //auto starts = it->second;
                    //auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                    assert(std::get<0>(it->second) < end && "end before birth");
                    assert(std::get<0>(it->second) < std::get<1>(it->second) && "dispatch before birth");
                    durations.emplace_back(std::tuple_cat(it->second, std::tie(copy.enqueued, end)));
                    this->start_times.erase(it);
                } else {
                    cvm::log(cvm::NONE, "ERROR: start time not found for id\n");
                }
            }
            if(data_width_ == 64)
            axi_sw_r_8(copy.id, copy.resp, copy.data.data(), copy.last); 
            else if(data_width_ ==512)
            axi_sw_r_64(copy.id, copy.resp, copy.data.data(), copy.last); 
            else
            cvm::log(cvm::ERROR, "unsupported data width for axi_sw");

            }
      );
    }
}

template < typename W,typename AW,typename AR, typename RQ>
void axi_sw<W,AW,AR,RQ>::reset_ptrs() {
    r_q_rptr_ = 0;
    r_q_wptr_ = 0;
}

template < typename W,typename AW,typename AR, typename RQ>
void axi_sw<W,AW,AR,RQ>::set_scope(svScope scope) {
    scope_ = scope;
}

extern "C" {

  void axi_sw_set_scope(cvm::topology::loc_t loc) {
    svScope scope = svGetScope();

    axi_sw_r_reset();

    cvm::registry::messenger.signal<svScope>(
        loc,
        scope);
  }

}
