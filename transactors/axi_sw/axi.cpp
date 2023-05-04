#include <thread>
#include <cassert>
#include <algorithm>
#include "axi.h"

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

                axi::data_t read_data(data_bus_bytes, 0);
                if (!a.w || a.atop.transaction != NON_ATOMIC) {
                    transactor::read(
                            start,
                            len,
                            read_data
                            );
                }

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
                    r_q_.enqueue(r_t(a.id, a.lock ? RESP_EXOKAY : RESP_OKAY, read_data, last));
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
