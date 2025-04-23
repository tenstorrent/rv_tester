#include <vector>
#include <array>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <functional>
#include <string>
#include <regex>

template <size_t N>
class address_range {
public:
    // Insert a new interval [lo..hi] (counters start at 0)
    void insert(uint64_t lo, uint64_t hi) {
        range_entry entry{ lo, hi, {} };
        auto it = std::lower_bound(ranges_.begin(), ranges_.end(), lo,
            [](auto const &r, uint64_t val){ return r.lo < val; }
        );
        ranges_.insert(it, std::move(entry));
    }

    // Parse a comma‑separated list of hex ranges (e.g. "0x10-0x1F,0x30")
    // and insert each [lo..hi] into the data structure.
    void parse(const std::string &input) {
        static const std::regex hex_range_regex(
            R"((0x[0-9A-Fa-f]+)(?:-(0x[0-9A-Fa-f]+))?)"
        );
        std::smatch match;
        auto cursor = input.cbegin();

        while (std::regex_search(cursor, input.cend(), match, hex_range_regex)) {
            uint64_t lo = std::stoull(match[1].str(), nullptr, 16);
            uint64_t hi = match[2].matched
                          ? std::stoull(match[2].str(), nullptr, 16)
                          : lo;
            insert(lo, hi);
            cursor = match.suffix().first;
        }
    }

    // Return a reference to the counter[idx] for addr, if found.
    // Returns nullopt if idx >= N or addr not in any range.
    std::optional<std::reference_wrapper<size_t>>
    get_count(uint64_t addr, size_t idx) {
        if (idx >= N) return std::nullopt;
        if (auto it = find_entry(addr); it != ranges_.end())
            return it->counters[idx];
        return std::nullopt;
    }

    // Increment counter[idx] by amount, returning the new value if found.
    // Returns nullopt if idx >= N or addr not in any range.
    std::optional<size_t>
    incr_count(uint64_t addr, size_t idx, size_t amount = 1) {
        if (idx >= N) return std::nullopt;
        if (auto it = find_entry(addr); it != ranges_.end()) {
            it->counters[idx] += amount;
            return it->counters[idx];
        }
        return std::nullopt;
    }

    // Check whether addr lies in any stored range.
    bool find(uint64_t addr) const {
        return find_entry(addr) != ranges_.cend();
    }

    // Check whether counter[idx] for addr is below threshold.
    // Returns false if idx >= N or addr not in any range.
    bool is_count_below_threshold(uint64_t addr, size_t idx, size_t threshold) const {
        if (idx >= N) return false;
        if (auto it = find_entry(addr); it != ranges_.cend())
            return it->counters[idx] < threshold;
        return false;
    }

private:
    // One entry per interval, with N counters
    struct range_entry {
        uint64_t             lo;
        uint64_t             hi;
        std::array<size_t, N> counters{};  // zero‑initialized
    };

    // Sorted list of ranges
    std::vector<range_entry> ranges_;

    // Find mutable iterator to the entry covering addr, or end()
    auto find_entry(uint64_t addr) {
        auto it = std::upper_bound(ranges_.begin(), ranges_.end(), addr,
            [](uint64_t val, auto const &r){ return val < r.lo; }
        );
        if (it == ranges_.begin()) return ranges_.end();
        --it;
        return (it->lo <= addr && addr <= it->hi) ? it : ranges_.end();
    }

    // Find const_iterator to the entry covering addr, or cend()
    auto find_entry(uint64_t addr) const {
        auto it = std::upper_bound(ranges_.cbegin(), ranges_.cend(), addr,
            [](uint64_t val, auto const &r){ return val < r.lo; }
        );
        if (it == ranges_.cbegin()) return ranges_.cend();
        --it;
        return (it->lo <= addr && addr <= it->hi) ? it : ranges_.cend();
    }
};
