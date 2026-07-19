// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <vector>
#include <array>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <functional>
#include <string>
#include <sstream>
#include <cstdio>

/**
 * @brief Template class for managing bus error injection lists with configurable error policies
 *
 * This class supports two modes of error injection:
 *
 * 1. THRESHOLD MODE (default):
 *    - Errors are injected for the first N accesses to each address
 *    - After N accesses, normal responses are returned
 *    - Controlled by set_threshold() - default threshold is 1
 *    - Use case: Inject errors for initial accesses, then allow normal operation
 *
 * 2. PATTERN MODE:
 *    - Errors follow a repeating pattern: N normal responses, then E error responses
 *    - Pattern repeats indefinitely: NNNN...EEEE...NNNN...EEEE...
 *    - Controlled by set_pattern(idx, "N:E") where N=normal count, E=error count
 *    - Use case: Periodic error injection for stress testing
 *
 * @tparam N Number of access types (e.g., READ=0, WRITE=1 for N=2)
 */
template <size_t N>
class bus_error_list {
public:
  // Constructor: initialize with default thresholds
  bus_error_list() {
    thresholds_.fill(1); // Default to threshold of 1 so first access can inject error
  }

  // Insert a new interval [lo..hi] (counters start at 0)
  void insert(uint64_t lo, uint64_t hi) {
    list_entry entry{lo, hi, {}, {}};
    auto it = std::lower_bound(address_list_.begin(), address_list_.end(), lo,
                               [](auto const& entry, uint64_t val) { return entry.lo < val; });
    address_list_.insert(it, std::move(entry));
  }

  // Parse a comma‑separated list of hex ranges (e.g. "0x10-0x1F,0x30")
  // and insert each [lo..hi] into the data structure.
  void parse(const std::string& input) {
    if (input.empty())
      return;

    std::stringstream ss(input);
    std::string token;

    // Split by comma
    while (std::getline(ss, token, ',')) {
      // Trim whitespace
      token.erase(0, token.find_first_not_of(" \t"));
      token.erase(token.find_last_not_of(" \t") + 1);

      if (token.empty())
        continue;

      // Look for range separator '-'
      size_t dash_pos = token.find('-');

      if (dash_pos == std::string::npos) {
        // Single address - align to cache line boundaries (64 bytes)
        uint64_t addr = parse_hex_string(token);
        if (addr != INVALID_ADDRESS) {
          uint64_t aligned_addr = addr & ~0x3f;      // Align to cache line
          insert(aligned_addr, aligned_addr + 0x3f); // Insert entire cache line
        }
      } else {
        // Address range
        std::string lo_str = token.substr(0, dash_pos);
        std::string hi_str = token.substr(dash_pos + 1);

        // Trim whitespace around dash
        lo_str.erase(lo_str.find_last_not_of(" \t") + 1);
        hi_str.erase(0, hi_str.find_first_not_of(" \t"));

        uint64_t lo = parse_hex_string(lo_str);
        uint64_t hi = parse_hex_string(hi_str);

        if (lo != INVALID_ADDRESS && hi != INVALID_ADDRESS) {
          insert(lo, hi);
        }
      }
    }
  }

  /**
     * @brief Set pattern for alternating normal/error responses
     * @param idx Access type index (e.g., READ=0, WRITE=1)
     * @param pattern Format "N:E" where N=normal responses, E=error responses
     *
     * Example: set_pattern(READ, "3:2") means:
     * - 3 normal responses, then 2 error responses, then repeat
     * - Access sequence: OK, OK, OK, ERR, ERR, OK, OK, OK, ERR, ERR, ...
     *
     * Empty pattern or invalid format disables pattern mode (reverts to threshold mode)
     */
  void set_pattern(size_t idx, const std::string& pattern) {
    if (idx >= N)
      return;

    if (pattern.empty()) {
      pattern_normal_count_[idx] = 0;
      pattern_error_count_[idx] = 0;
      return;
    }

    // Find delimiter position
    size_t delimiter_pos = pattern.find(':');
    if (delimiter_pos != std::string::npos) {
      try {
        pattern_normal_count_[idx] = std::stoi(pattern.substr(0, delimiter_pos));
        pattern_error_count_[idx] = std::stoi(pattern.substr(delimiter_pos + 1));
      } catch (const std::exception&) {
        // Invalid pattern format, reset to no pattern
        pattern_normal_count_[idx] = 0;
        pattern_error_count_[idx] = 0;
      }
    } else {
      // Invalid pattern format, reset to no pattern
      pattern_normal_count_[idx] = 0;
      pattern_error_count_[idx] = 0;
    }
  }

  /**
     * @brief Set threshold for error injection (threshold mode)
     * @param idx Access type index
     * @param threshold Number of initial accesses that should inject errors
     *
     * In threshold mode: first 'threshold' accesses inject errors, rest are normal
     * Example: threshold=2 means first 2 accesses get errors, 3rd+ get normal responses
     */
  void set_threshold(size_t idx, size_t threshold) {
    if (idx < N) {
      thresholds_[idx] = threshold;
    }
  }

  // Get threshold for a specific index
  size_t get_threshold(size_t idx) const {
    return (idx < N) ? thresholds_[idx] : 0;
  }

  // Return a reference to the counter[idx] for addr, if found.
  // Returns nullopt if idx >= N or addr not in any range.
  std::optional<std::reference_wrapper<size_t>>
  get_count(uint64_t addr, size_t idx) {
    if (idx >= N)
      return std::nullopt;
    if (auto it = find_entry(addr); it != address_list_.end())
      return it->counters[idx];
    return std::nullopt;
  }

  /**
     * @brief Increment access tracking and return new count
     * @param addr Address being accessed
     * @param idx Access type index
     * @param amount Amount to increment (default 1)
     * @return New count value, or nullopt if address not found
     *
     * Behavior depends on mode:
     * - Threshold mode: increments access counters
     * - Pattern mode: increments pattern position counters
     */
  std::optional<size_t>
  incr_count(uint64_t addr, size_t idx, size_t amount = 1) {
    if (idx >= N)
      return std::nullopt;
    if (auto it = find_entry(addr); it != address_list_.end()) {
      // If pattern is defined, increment pattern position; otherwise increment counter
      if (pattern_normal_count_[idx] > 0 && pattern_error_count_[idx] > 0) {
        it->pattern_positions[idx] += amount;
        return it->pattern_positions[idx];
      } else {
        it->counters[idx] += amount;
        return it->counters[idx];
      }
    }
    return std::nullopt;
  }

  // Check whether addr lies in any stored range.
  bool find(uint64_t addr) const {
    return find_entry(addr) != address_list_.cend();
  }

  // Check whether counter[idx] for addr is below threshold.
  // Returns false if idx >= N or addr not in any range.
  bool is_count_below_threshold(uint64_t addr, size_t idx) const {
    if (idx >= N)
      return false;
    if (auto it = find_entry(addr); it != address_list_.cend())
      return it->counters[idx] < thresholds_[idx];
    return false;
  }

  /**
     * @brief Check whether an error should be injected for this access
     * @param addr Address being accessed
     * @param idx Access type index
     * @return true if error should be injected, false for normal response
     *
     * This function does NOT advance any state - call incr_count() separately to advance tracking
     *
     * Logic:
     * - If address not in list: return false (normal response)
     * - If pattern mode enabled: use pattern-based logic
     * - Otherwise: use threshold-based logic
     */
  bool check_inject_error(uint64_t addr, size_t idx) const {
    if (!find(addr))
      return false;

    // If pattern is defined, use pattern-based logic
    if (pattern_normal_count_[idx] > 0 && pattern_error_count_[idx] > 0) {
      auto it = find_entry(addr);

      // Calculate cycle position in the pattern (without advancing)
      size_t total_cycle = pattern_normal_count_[idx] + pattern_error_count_[idx];
      size_t cycle_position = it->pattern_positions[idx] % total_cycle;

      // Return true if in error phase of pattern (after normal responses)
      return cycle_position >= pattern_normal_count_[idx];
    }

    // Otherwise use threshold-based logic
    return is_count_below_threshold(addr, idx);
  }

  // Get pattern counts
  std::pair<size_t, size_t> get_pattern(size_t idx) const {
    if (idx >= N)
      return {0, 0};
    return {pattern_normal_count_[idx], pattern_error_count_[idx]};
  }

private:
  static constexpr uint64_t INVALID_ADDRESS = UINT64_MAX;

  // One entry per address interval, with N counters and N pattern positions
  struct list_entry {
    uint64_t lo;                               // Start address of interval
    uint64_t hi;                               // End address of interval
    std::array<size_t, N> counters{};          // Access counters (threshold mode)
    std::array<size_t, N> pattern_positions{}; // Pattern position counters (pattern mode)
  };

  // Sorted list of address intervals
  std::vector<list_entry> address_list_;

  // Pattern settings per access type
  std::array<size_t, N> pattern_error_count_{};  // Number of error responses in pattern
  std::array<size_t, N> pattern_normal_count_{}; // Number of normal responses in pattern

  // Threshold values per access type
  std::array<size_t, N> thresholds_{}; // Error injection threshold

  /**
     * @brief Parse a hex string to uint64_t
     * @param hex_str String containing hex number (with or without 0x prefix)
     * @return Parsed value or INVALID_ADDRESS on error
     */
  uint64_t parse_hex_string(const std::string& hex_str) const {
    if (hex_str.empty())
      return INVALID_ADDRESS;

    try {
      // std::stoull handles both "0x" prefix and plain hex
      return std::stoull(hex_str, nullptr, 16);
    } catch (const std::exception&) {
      return INVALID_ADDRESS;
    }
  }

  // Find mutable iterator to the entry covering addr, or end()
  auto find_entry(uint64_t addr) {
    auto it = std::upper_bound(address_list_.begin(), address_list_.end(), addr,
                               [](uint64_t val, auto const& entry) { return val < entry.lo; });
    if (it == address_list_.begin())
      return address_list_.end();
    --it;
    return (it->lo <= addr && addr <= it->hi) ? it : address_list_.end();
  }

  // Find const_iterator to the entry covering addr, or cend()
  auto find_entry(uint64_t addr) const {
    auto it = std::upper_bound(address_list_.cbegin(), address_list_.cend(), addr,
                               [](uint64_t val, auto const& entry) { return val < entry.lo; });
    if (it == address_list_.cbegin())
      return address_list_.cend();
    --it;
    return (it->lo <= addr && addr <= it->hi) ? it : address_list_.cend();
  }
};
