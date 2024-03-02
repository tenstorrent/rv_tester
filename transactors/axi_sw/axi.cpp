#include <thread>
#include <cassert>
#include <algorithm>
#include "axi.h"
#include "cvm/logger.hpp"

axi::~axi() {
    for (const auto& [name, queue_times] : {
            std::make_tuple("A", std::cref(a_queue_times)),
            std::make_tuple("R", std::cref(r_queue_times)),
    }) {
        std::uint64_t sum = 0, max = 0;
        bool max_is_write = false;
        for (const auto& [write, reached_head, dequeued] : queue_times) {
            std::uint64_t time_at_head = std::chrono::duration_cast<std::chrono::nanoseconds>(dequeued - reached_head).count();
            sum += time_at_head;
            max = std::max(max, time_at_head);
            if (max == time_at_head)
                max_is_write = write;
        }
        cvm::log(cvm::NONE, "AXI {} QUEUE queue_times average {: >20}ns max {: >20}ns count {} is_write? {}\n", name, double(sum)/queue_times.size(), max, queue_times.size(), max_is_write);
    }

    {
        std::array<std::uint64_t, 3> sum{{0,}};
        std::array<std::uint64_t, 3> max{{0,}};
        for (const auto& [is_write, enqueued, get_data, got_data, finished] : a_times) {
            if (get_data < enqueued) cvm::log(cvm::NONE, "Error: get_data before enqueued\n");
            if (got_data < get_data) cvm::log(cvm::NONE, "Error: got_data before get_data\n");
            if (finished < got_data) cvm::log(cvm::NONE, "Error: finished   before got_data\n");
            std::uint64_t a = std::chrono::duration_cast<std::chrono::nanoseconds>(get_data - enqueued).count();
            std::uint64_t b = std::chrono::duration_cast<std::chrono::nanoseconds>(got_data - get_data).count();
            std::uint64_t c = std::chrono::duration_cast<std::chrono::nanoseconds>(finished - got_data).count();
            sum[0] += a;
            sum[1] += b;
            sum[2] += c;
            max[0] = std::max(max[0], a);
            max[1] = std::max(max[1], b);
            max[2] = std::max(max[2], b);
        }
        auto average_get_data  = double(sum[0])/a_times.size();
        auto average_got_data  = double(sum[1])/a_times.size();
        auto average_finished  = double(sum[2])/a_times.size();
        cvm::log(cvm::NONE, "AXI TOTAL    DURATIONS average {: >20}ns max {: >20}ns count {}\n", average_finished, max[2], a_times.size());
        cvm::log(cvm::NONE, "AXI GOT DATA DURATIONS average {: >20}ns max {: >20}ns count {}\n", average_got_data, max[1], a_times.size());
        cvm::log(cvm::NONE, "AXI GET DATA DURATIONS average {: >20}ns max {: >20}ns count {}\n", average_get_data, max[0], a_times.size());
    }
}

template <typename T> void atop_arithmetic(const axi::data_t& read_data, axi::data_t& write_data, const axi::atop_operation operation, const axi::len_t& len) {

    T read = 0, write = 0;
    for (axi::len_t i = 0; i < len; i++) {
        read  |= T(read_data [i]) << (8*i);
        write |= T(write_data[i]) << (8*i);
    }

    T result;

    if (operation == axi::ATOP_SMAX || operation == axi::ATOP_UMAX)
        result = std::max(read, write);
    else if(operation == axi::ATOP_SMIN || operation == axi::ATOP_UMIN)
        result = std::min(read, write);
    else if(operation == axi::ATOP_ADD)
        result = read + write;
    else {
        assert(false && "unknown operation");
        result = write;
    }

    for (axi::len_t i = 0; i < len; i++) {
        write_data[i] = (result >> (8*i)) & 0xff;
    }
}

void axi::atop_modify_write_data(const atop_t& atop, const data_t& read_data, data_t& write_data, const len_t& len) {
    if (atop.transaction == NON_ATOMIC) return;
    if (atop.transaction == ATOMIC_SWAP) return;
    if (atop.transaction == ATOMIC_COMPARE) {
        assert(false && "atomic compare not supported");
        return;
    }

    atop_operation op = atop.operation;
    const std::array<atop_operation, 3> bitwise_ops = {
        ATOP_CLR,
        ATOP_EOR,
        ATOP_SET,
    };
    bool bitwise = std::find(bitwise_ops.begin(), bitwise_ops.end(), op) != bitwise_ops.end();

    if (!bitwise) {
        bool sign = op == ATOP_SMIN || op == ATOP_SMAX;

        switch(len) {
            case 1:
                if (sign) atop_arithmetic<std::int8_t>(read_data, write_data, op, len);
                else      atop_arithmetic<std::uint8_t>(read_data, write_data, op, len);
                break;
            case 2:
                if (sign) atop_arithmetic<std::int16_t>(read_data, write_data, op, len);
                else      atop_arithmetic<std::uint16_t>(read_data, write_data, op, len);
                break;
            case 4:
                if (sign) atop_arithmetic<std::int32_t>(read_data, write_data, op, len);
                else      atop_arithmetic<std::uint32_t>(read_data, write_data, op, len);
                break;
            case 8:
                if (sign) atop_arithmetic<std::int64_t>(read_data, write_data, op, len);
                else      atop_arithmetic<std::uint64_t>(read_data, write_data, op, len);
                break;
            default:
                assert(false && "unknown len");
        }

    } else {

        for (len_t i = 0; i < len; i++) {
            switch(op) {
                case ATOP_CLR:
                    write_data[i] = read_data[i] & ~write_data[i];
                    break;
                case ATOP_EOR:
                    write_data[i] = read_data[i] ^  write_data[i];
                    break;
                case ATOP_SET:
                    write_data[i] = read_data[i] |  write_data[i];
                    break;
                default:
                    assert(false && "unknown op");
            }
        }
    }
}

cvm::messenger::task<void> axi::a(const a_t& p) {
    a_t_with_time a = {.a = p, .enqueued = std::chrono::high_resolution_clock::now()};
    a_q_.enqueue(a);
    co_await (*this)();
    co_return;
}

cvm::messenger::task<void> axi::w(w_t&& p) {
    w_q_.enqueue(std::move(p));
    co_await (*this)();
    co_return;
}

std::pair<bool, axi::r_t> axi::r(bool block) {
    std::pair<bool, axi::r_t> r;
    time_point time_reached_head;
    if (block)
        r = std::make_pair(true, r_q_.dequeue(&time_reached_head));
    else
        r = r_q_.try_dequeue(&time_reached_head);

    if(std::get<0>(r)) {
        auto time_dequeued = std::chrono::high_resolution_clock::now();
        r_queue_times.emplace_back(false, time_reached_head, time_dequeued);
    }

    return r;
}

cvm::messenger::task<void> axi::operator()() {
    while (1)  {
        a_t_with_time at;

        bool valid;
        std::tie(valid, at) = a_q_.try_peek();
        if (!valid) {
            co_return;
        }
        a_t a = at.a;

        addr_t burst_len            = a.len + 1;

        if (a.w && w_q_.size() < burst_len) {
            co_return;
        }

        time_point time_reached_head;
        a_q_.dequeue(&time_reached_head);
        auto time_dequeued = std::chrono::high_resolution_clock::now();
        a_queue_times.emplace_back(a.w != 0, time_reached_head, time_dequeued);

        id_t id                     = a.id;
        addr_t num_bytes            = 1 << a.size;
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

                at.get_data = std::chrono::high_resolution_clock::now();

                axi::data_t read_data;
                if (!a.w || a.atop.transaction != NON_ATOMIC) {
                    read_data = co_await transactor::read(id, start, len);
                    read_data.resize(data_bus_bytes, 0);
                    at.got_data = std::chrono::high_resolution_clock::now();
                }

                if (a.w) {
                    if (w_q_.empty()) {
                        cvm::log(cvm::ERROR, "ERROR: write data queue empty when it should not be\n");
                    }
                    auto w = w_q_.dequeue();
                    at.got_data = std::chrono::high_resolution_clock::now();
                    if (!!w.last != last) {
                        cvm::log(cvm::ERROR, "ERROR: [axi] w.last not set in for write to addr {:#x}\n", start);
                    }

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


                    atop_modify_write_data(a.atop, read_data, w.data, len);

                    transactor::write(
                            start,
                            len,
                            w.data,
                            w.strb
                    );
                }

                if (!a.w || (a.atop.transaction != NON_ATOMIC && a.atop.transaction != ATOMIC_STORE)) {
                    // use std::shift_right in C++20
                    std::rotate(
                            std::begin(read_data),
                            std::next(std::begin(read_data), data_bus_bytes - lower_byte_lane),
                            std::end(read_data)
                            );
                    r_q_.enqueue(r_t(a.id, a.lock ? RESP_EXOKAY : RESP_OKAY, read_data, last, std::chrono::high_resolution_clock::now()));
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

        at.finished = std::chrono::high_resolution_clock::now();
        a_times.emplace_back(std::make_tuple(a.w, at.enqueued, at.get_data, at.got_data, at.finished));
    }
}
