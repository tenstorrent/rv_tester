#include <thread>
#include <cassert>
#include <algorithm>
#include "axi.h"

void axi::a(const a_t& p) {
    a_q_.enqueue(p);
}

void axi::w(w_t&& p) {
    w_q_.enqueue(std::move(p));
}

std::pair<bool, axi::r_t> axi::r(bool block) {
    if (block)
        return std::make_pair(true, r_q_.dequeue());
    return r_q_.try_dequeue();
}

void axi::operator()() {
    while (1)  {
        auto a = a_q_.dequeue();

        addr_t num_bytes            = 1 << a.size;
        addr_t burst_len            = a.len + 1;
        addr_t aligned_addr         = a.addr / num_bytes * num_bytes;
        data_width_t data_bus_bytes = data_width()/8;

        addr_t addr = a.addr;
        bool aligned = addr == aligned_addr;
        addr_t dtsize = num_bytes * burst_len;

        addr_t lower_wrap_boundary, upper_wrap_boundary;
        if (a.burst == BURST_WRAP) {
            lower_wrap_boundary = addr/dtsize * dtsize;
            upper_wrap_boundary = lower_wrap_boundary + dtsize;
        }

        for (addr_t n = 1; n <= burst_len; n++) {

            addr_t lower_byte_lane = addr - addr/data_bus_bytes * data_bus_bytes;

            addr_t upper_byte_lane;
            if (aligned) {
                upper_byte_lane = lower_byte_lane + num_bytes - 1;
            } else {
                upper_byte_lane = aligned_addr + num_bytes - 1 - addr/data_bus_bytes * data_bus_bytes;
            }

            bool last = n == burst_len;

            if (1) {
                addr_t start  = (addr / strobe_width()) * strobe_width() + lower_byte_lane;
                addr_t len    = upper_byte_lane - lower_byte_lane + 1;

                if (a.w) {
                    auto w = w_q_.dequeue();
                    assert(!!w.last == last);

                    // use std::shift_left in C++20
                    std::rotate(
                            std::begin(w.data),
                            std::next(std::begin(w.data), lower_byte_lane),
                            std::end(w.data)
                            );

                    std::rotate(
                            std::begin(w.strb),
                            std::next(std::begin(w.strb), lower_byte_lane),
                            std::end(w.strb)
                            );

                    transactor::write(
                            start,
                            len,
                            w.data,
                            w.strb
                    );
                } else {
                    axi::data_t data(data_bus_bytes, 0);
                    transactor::read(
                            start,
                            len,
                            data
                            );

                    // use std::shift_right in C++20
                    std::rotate(
                            std::begin(data),
                            std::next(std::begin(data), data_bus_bytes - lower_byte_lane),
                            std::end(data)
                            );
                    r_q_.enqueue(r_t{a.id, data, last});
                }
            }

            if (a.burst != BURST_FIXED) {
                if (aligned) {
                    addr = addr + num_bytes;
                    if (a.burst == BURST_WRAP) {
                        if (addr >= upper_wrap_boundary) {
                            addr = lower_wrap_boundary;
                        }
                    }
                } else {
                    addr = aligned_addr + num_bytes;
                    aligned = true;
                }
            }
        }
    }
}

void axi::run() {
    std::thread([&] () { (*this)(); } ).detach();
}
