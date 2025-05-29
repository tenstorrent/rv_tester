#include <thread>
#include <cassert>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <regex>
#include "axi.h"
#include "cvm/logger.hpp"

// Error responses
DEFINE_string(axi_resp_slverr_addr, "", "List of addresses that need slverr response, can be a single value or a range. Ex: 0x1000,0x2000-0x3000");
DEFINE_string(axi_resp_decerr_addr, "", "List of addresses that need decerr response, can be a single value or a range. Ex: 0x1000,0x2000-0x3000");
DEFINE_string(axi_resp_hang_addr, "", "List of addresses that give no response causing core hang, can be a single value or a range. Ex: 0x1000,0x2000-0x3000");
DEFINE_int32(axi_resp_slverr_threshold, 2, "Threshold upto which  slverr injection happens for a particular address");
DEFINE_int32(axi_resp_decerr_threshold, 2, "Threshold upto which decerr injection happens for a particular address");
DEFINE_string(axi_resp_slverr_pattern, "", "Pattern for alternating slverr responses in format 'n:e' where n is normal responses and e is error responses");
DEFINE_string(axi_resp_decerr_pattern, "", "Pattern for alternating decerr responses in format 'n:e' where n is normal responses and e is error responses");
DEFINE_bool(axi_err_after_test_start, false, "Keep axi errors disabled till test_start_label");

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

axi::axi(const data_width_t& data_width, const cvm::topology::loc_t loc, const std::string& tag)
  : transactor(loc, tag), tag_(tag), data_width_(data_width)
{
    cvm::log(cvm::MEDIUM, "[axi] Constructing axi for loc={} id={}\n", loc, tag);

    // RPC to allow external components to configure responses
    cvm::registry::messenger.procedure<configure_error_rpc>(loc, [this] () { return this->configure_error(); });
    cvm::registry::messenger.procedure<enable_error_rpc>(loc, [this] () { return this->enable_error(); });
    cvm::registry::messenger.procedure<disable_error_rpc>(loc, [this] () { return this->disable_error(); });
    cvm::registry::messenger.procedure<check_error_rpc>(loc, [this] (addr_t addr) { return this->check_error(addr); });

    hang_list_.parse(FLAGS_axi_resp_hang_addr);
    setup_error_lists();
    // Enable when test start label is observed
    if (FLAGS_axi_err_after_test_start) {
        disable_error();
    }
}

void axi::setup_error_lists() {
    cvm::log(cvm::MEDIUM, "[axi] configure error resp: slverr={}\n", FLAGS_axi_resp_slverr_addr);
    cvm::log(cvm::MEDIUM, "[axi] configure error resp: decerr={}\n", FLAGS_axi_resp_decerr_addr);

    slverr_list_.parse(FLAGS_axi_resp_slverr_addr);
    decerr_list_.parse(FLAGS_axi_resp_decerr_addr);

    // Set thresholds
    slverr_list_.set_threshold(READ, FLAGS_axi_resp_slverr_threshold);
    slverr_list_.set_threshold(WRITE, FLAGS_axi_resp_slverr_threshold);
    decerr_list_.set_threshold(READ, FLAGS_axi_resp_decerr_threshold);
    decerr_list_.set_threshold(WRITE, FLAGS_axi_resp_decerr_threshold);

    // Set patterns for slverr and decerr
    slverr_list_.set_pattern(READ, FLAGS_axi_resp_slverr_pattern);
    slverr_list_.set_pattern(WRITE, FLAGS_axi_resp_slverr_pattern);
    decerr_list_.set_pattern(READ, FLAGS_axi_resp_decerr_pattern);
    decerr_list_.set_pattern(WRITE, FLAGS_axi_resp_decerr_pattern);
}

void axi::configure_error() {
    // Clear existing ranges before parsing new ones to avoid accumulation
    slverr_list_ = bus_error_list<NUM_ACCESS_TYPES>();
    decerr_list_ = bus_error_list<NUM_ACCESS_TYPES>();

    setup_error_lists();
}

void axi::enable_error() {
    error_en_ = true;
    cvm::log(cvm::HIGH, "[axi] enable error resp for {}\n", tag_);
}

void axi::disable_error() {
    error_en_ = false;
    cvm::log(cvm::HIGH, "[axi] disable error resp for {}\n", tag_);
}

bool axi::check_error(addr_t addr) {
    bool has_slverr = slverr_list_.check_inject_error(addr, READ);
    bool has_decerr = decerr_list_.check_inject_error(addr, READ);
    
    cvm::log(cvm::HIGH, "[axi] check_error for addr={:#x}: slverr={}, decerr={}\n", addr, has_slverr, has_decerr);
    
    return has_slverr || has_decerr;
}

axi::~axi() {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"{}_resp_slverr_count\": \"{}\"}}\n", tag_, num_slverr_resp_);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"{}_resp_decerr_count\": \"{}\"}}\n", tag_, num_decerr_resp_);
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
    a_q_.enqueue(p);
    co_await (*this)();
    co_return;
}

cvm::messenger::task<void> axi::w(w_t&& p) {
    w_q_.enqueue(std::move(p));
    co_await (*this)();
    co_return;
}

std::pair<bool, axi::r_t> axi::r(bool block) {
    if (block)
        return std::make_pair(true, r_q_.dequeue());
    return r_q_.try_dequeue();
}

std::pair<bool, axi::b_t> axi::b() {
    return b_q_.try_dequeue();
}

cvm::messenger::task<void> axi::operator()() {
    while (1)  {
        a_t a;

        bool valid;
        std::tie(valid, a) = a_q_.try_peek();
        if (!valid)
            co_return;

        addr_t burst_len            = a.len + 1;

        if (a.w && w_q_.size() < burst_len) {
            co_return;
        }

        a_q_.dequeue();

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

                axi::data_t read_data;
                if (!a.w || a.atop.transaction != NON_ATOMIC) {
                    cvm::log(cvm::FULL, "[axi] ar: id={}, addr={:#x}, len={}, size={}. tr: len={}\n", a.id, start, a.len, a.size, len);
                    read_data = co_await transactor::read(start, len);
                    read_data.resize(data_bus_bytes, 0);
                }

                if (a.w) {
                    cvm::log(cvm::FULL, "[axi] aw: id={}, addr={:#x}, len={}, size={}. tr: len={}\n", a.id, start, a.len, a.size, len);
                    auto w = w_q_.dequeue();
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

                    // Resp
                    axi::resp_t write_resp = RESP_OKAY;

                    // Check and increment counters for error injection policies
                    bool inject_slverr = error_en_ && slverr_list_.check_inject_error(addr, WRITE);
                    bool inject_decerr = error_en_ && decerr_list_.check_inject_error(addr, WRITE);

                    // Always increment counters for addresses in error ranges (regardless of injection)
                    if (slverr_list_.find(addr)) {
                        auto count = slverr_list_.incr_count(addr, WRITE);
                        if (inject_slverr) {
                            write_resp = RESP_SLVERR;
                            cvm::log(cvm::HIGH, "[axi] slverr write resp addr={:#x} count={}\n", addr, count.value());
                            num_slverr_resp_++;
                        }
                    }
                    if (decerr_list_.find(addr)) {
                        auto count = decerr_list_.incr_count(addr, WRITE);
                        if (inject_decerr) {
                            write_resp = RESP_DECERR;
                            cvm::log(cvm::HIGH, "[axi] decerr write resp addr={:#x} count={}\n", addr, count.value());
                            num_decerr_resp_++;
                        }
                    }

                    b_q_.enqueue(b_t(a.id, write_resp));
                    cvm::log(cvm::HIGH, "[axi] b: id={}, addr={:#x}, resp={}\n", a.id, addr, +write_resp);
                }

                if (!a.w || (a.atop.transaction != NON_ATOMIC && a.atop.transaction != ATOMIC_STORE)) {
                    // use std::shift_right in C++20
                    std::rotate(
                            std::begin(read_data),
                            std::next(std::begin(read_data), data_bus_bytes - lower_byte_lane),
                            std::end(read_data)
                            );

                    // Resp
                    axi::resp_t read_resp = a.lock ? RESP_EXOKAY : RESP_OKAY;

                    // Check and increment counters for error injection policies
                    bool inject_slverr = error_en_ && slverr_list_.check_inject_error(addr, READ);
                    bool inject_decerr = error_en_ && decerr_list_.check_inject_error(addr, READ);

                    // Always increment counters for addresses in error ranges (regardless of injection)
                    if (slverr_list_.find(addr)) {
                        auto count = slverr_list_.incr_count(addr, READ);
                        if (inject_slverr) {
                            read_resp = RESP_SLVERR;
                            cvm::log(cvm::HIGH, "[axi] slverr read resp addr={:#x} count={}\n", addr, count.value());
                            num_slverr_resp_++;
                        }
                    }
                    if (decerr_list_.find(addr)) {
                        auto count = decerr_list_.incr_count(addr, READ);
                        if (inject_decerr) {
                            read_resp = RESP_DECERR;
                            cvm::log(cvm::HIGH, "[axi] decerr read resp addr={:#x} count={}\n", addr, count.value());
                            num_decerr_resp_++;
                        }
                    }

                    // Drop response (artificial hang scenario)
                    bool drop_resp = hang_list_.find(addr);
                    if(!drop_resp) {
                        r_q_.enqueue(r_t(a.id, read_resp, read_data, last));
                    }
                    std::string d;
                    if (cvm::logger::check_verbosity(cvm::FULL))
                      for (int i=read_data.size()-1; i>=0; i--)
                        d += fmt::format("{:02x}", read_data[i]);
                    cvm::log(cvm::HIGH, "[axi] r: id={}, addr={:#x}, resp={}, last={}, len={}, size={}, data={}\n", a.id, addr, +read_resp, last, len, read_data.size(), d);
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
