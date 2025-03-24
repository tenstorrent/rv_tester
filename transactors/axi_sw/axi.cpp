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
  : transactor(loc, tag), data_width_(data_width), num_slverr_resp_(0), num_decerr_resp_(0)
{
    cvm::log(cvm::MEDIUM, "[axi] Constructing axi for loc={} id={}\n", loc, tag);

    // RPC to allow external components to configure responses
    cvm::registry::messenger.procedure<configure_resp_rpc>(loc, [this] () { return this->configure_resp(); });

    hang_addr_   = parse_hex_ranges(FLAGS_axi_resp_hang_addr);
    slverr_addr_ = parse_hex_ranges(FLAGS_axi_resp_slverr_addr);
    decerr_addr_ = parse_hex_ranges(FLAGS_axi_resp_decerr_addr);
    // Resize the counter vector to match the number of address ranges
    slverr_count_.resize(slverr_addr_.size(), 0);
    decerr_count_.resize(decerr_addr_.size(), 0);
}

void axi::configure_resp() {
    cvm::log(cvm::HIGH, "[axi] configure axi err resp: slverr={}\n", FLAGS_axi_resp_slverr_addr);
    cvm::log(cvm::HIGH, "[axi] configure axi err resp: decerr={}\n", FLAGS_axi_resp_decerr_addr);

    slverr_addr_ = parse_hex_ranges(FLAGS_axi_resp_slverr_addr);
    decerr_addr_ = parse_hex_ranges(FLAGS_axi_resp_decerr_addr);
    // Resize the counter vector to match the number of address ranges
    slverr_count_.resize(slverr_addr_.size(), 0);
    decerr_count_.resize(decerr_addr_.size(), 0);
}

axi::~axi() {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"axi_resp_slverr_count\": \"{}\"}}\n", num_slverr_resp_);
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"axi_resp_decerr_count\": \"{}\"}}\n", num_decerr_resp_);
}

// Function to parse a string containing hexadecimal numbers and ranges
std::vector<std::pair<uint64_t, uint64_t>> axi::parse_hex_ranges(const std::string& input) {
    std::vector<std::pair<uint64_t, uint64_t>> result;
    std::regex hex_range_regex(R"((0x[0-9a-fA-F]+)(?:-(0x[0-9a-fA-F]+))?)");
    std::smatch match;
    auto search_start = input.cbegin();

    // Loop through all matches found in the input string
    while (regex_search(search_start, input.cend(), match, hex_range_regex)) {
        uint64_t min = stoull(match[1].str(), nullptr, 16);
        uint64_t max = match[2].matched ? stoull(match[2].str(), nullptr, 16) : min;

        result.emplace_back(min, max);
        // Move search start to remaining part of the string
        search_start = match.suffix().first;
    }

    return result;
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

cvm::messenger::task<void> axi::operator()() {
    while (1)  {
        a_t a;

        bool valid;
        std::tie(valid, a) = a_q_.try_peek();
        if (!valid) {
            co_return;
        }

        addr_t burst_len            = a.len + 1;

        if (a.w && w_q_.size() < burst_len) {
            co_return;
        }

        a_q_.dequeue();

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
                
                std::string d;
                std::string s;
                axi::data_t read_data;
                axi::resp_t read_resp;
                if (!a.w || a.atop.transaction != NON_ATOMIC) {
                    cvm::log(cvm::FULL, "[axi] ar: id={}, addr={:#x}, len={}, size={}. tr: len={}\n", a.id, start, a.len, a.size, len);
                    read_data = co_await transactor::read(id, start, len);
                    if (cvm::logger::check_verbosity(cvm::FULL))
                      for (int i=read_data.size()-1; i>=0; i--)
                        d += fmt::format("{:02x}", read_data[i]);
                    cvm::log(cvm::FULL, "[axi] r: id={}, last={}, len={}, size={}, data={}\n", a.id, last, len, read_data.size(), d);
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
                }

                if (!a.w || (a.atop.transaction != NON_ATOMIC && a.atop.transaction != ATOMIC_STORE)) {
                    // use std::shift_right in C++20
                    std::rotate(
                            std::begin(read_data),
                            std::next(std::begin(read_data), data_bus_bytes - lower_byte_lane),
                            std::end(read_data)
                            );

                    read_resp = a.lock ? RESP_EXOKAY : RESP_OKAY;
                    // Error responses
                    int idx = 0;
                    for (const auto& [min, max] : slverr_addr_) {
                        if (addr >= min && addr <= max && slverr_count_[idx] <= FLAGS_axi_resp_slverr_threshold) {
                            read_resp = RESP_SLVERR;
                            slverr_count_[idx]++;
                            num_slverr_resp_++;
                        }
                        idx++;
                    }
                    idx = 0;
                    for (const auto& [min, max] : decerr_addr_) {
                        if (addr >= min && addr <= max && decerr_count_[idx] <= FLAGS_axi_resp_decerr_threshold) {
                            read_resp = RESP_DECERR;
                            decerr_count_[idx]++;
                            num_decerr_resp_++;
                        }
                        idx++;
                    }
                    // Drop responses (artificial hang scenario)
                    bool drop_resp = false;
                    for (const auto& [min, max] : hang_addr_) {
                        if (addr >= min && addr <= max) {
                            drop_resp = true;
                        }
                    }
                    if(!drop_resp)
                        r_q_.enqueue(r_t(a.id, read_resp, read_data, last));
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
