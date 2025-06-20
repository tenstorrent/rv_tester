#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <type_traits>

namespace parser {

// Template range type for any integral type
template<typename T>
using range = std::pair<T, T>;

// Template vector type for any type
template<typename T>
using vector = std::vector<T>;

// Template pair_map type: key -> vector of ranges
template<typename KeyType, typename ValueType>
using pair_map = std::unordered_map<KeyType, std::vector<range<ValueType>>>;

// Template map type: key -> value
template<typename KeyType, typename ValueType>
using map = std::unordered_map<KeyType, ValueType>;

// A simple type_tag used to select the proper overload
template <typename T>
struct type_tag {};

// Helper for static_assert false in dependent contexts
template<typename T>
struct dependent_false : std::false_type {};

namespace detail {
    // Helper function to trim whitespace
    inline std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    // Helper function to check if string starts with hex prefix
    inline bool is_hex_string(const std::string& str) {
        return str.size() >= 3 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X');
    }

    // Template helper function to parse hex string to any integral type
    template<typename T>
    T parse_hex(const std::string& hex_str) {
        static_assert(std::is_integral_v<T>, "T must be an integral type");
        if (!is_hex_string(hex_str)) {
            throw std::invalid_argument("Invalid hex format: " + hex_str);
        }
        return static_cast<T>(std::stoull(hex_str, nullptr, 0));
    }

    // Template helper function to parse string to any integral type
    template<typename T>
    T parse_integral(const std::string& str) {
        static_assert(std::is_integral_v<T>, "T must be an integral type");
        if (is_hex_string(str)) {
            return parse_hex<T>(str);
        } else {
            if constexpr (std::is_signed_v<T>) {
                return static_cast<T>(std::stoll(str));
            } else {
                return static_cast<T>(std::stoull(str));
            }
        }
    }

    // Helper function to split string by delimiter
    inline std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        if (str.empty()) return tokens;

        size_t start = 0;
        size_t end = str.find(delimiter);

        while (end != std::string::npos) {
            tokens.push_back(str.substr(start, end - start));
            start = end + 1;
            end = str.find(delimiter, start);
        }
        tokens.push_back(str.substr(start));
        return tokens;
    }
} // namespace detail

// Generic function to parse a comma-separated list using a converter callable
template <typename T, typename Converter>
std::vector<T> parse_list(const std::string &input, Converter converter) {
    std::vector<T> result;
    if (input.empty()) return result;

    std::istringstream stream(input);
    std::string token;
    while (std::getline(stream, token, ',')) {
        // Trim whitespace
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");
        if (start != std::string::npos) {
            token = token.substr(start, end - start + 1);
            result.push_back(converter(token));
        }
    }
    return result;
}

// Template implementation for parse_pair_map
template<typename KeyType, typename ValueType>
pair_map<KeyType, ValueType> parse_pair_map(const std::string &input) {
    static_assert(std::is_integral_v<KeyType>, "KeyType must be an integral type");
    static_assert(std::is_integral_v<ValueType>, "ValueType must be an integral type");

    pair_map<KeyType, ValueType> mapping;

    if (input.empty()) return mapping;

    // Split by comma to get individual entries
    auto entries = detail::split(input, ',');

    for (const auto& entry_str : entries) {
        std::string entry = detail::trim(entry_str);
        if (entry.empty()) continue;

        // Find the colon separator
        size_t colon_pos = entry.find(':');
        if (colon_pos == std::string::npos) {
            throw std::invalid_argument("Invalid pair format, missing colon: " + entry);
        }

        // Extract key part (before colon)
        std::string key_str = detail::trim(entry.substr(0, colon_pos));
        if (key_str.empty()) {
            throw std::invalid_argument("Empty key in pair: " + entry);
        }

        // Parse key
        KeyType key = detail::parse_integral<KeyType>(key_str);

        // Extract value part (after colon)
        std::string value_part = detail::trim(entry.substr(colon_pos + 1));
        if (value_part.empty()) {
            throw std::invalid_argument("Empty value in pair: " + entry);
        }

        // Check if it's a range (contains dash)
        size_t dash_pos = value_part.find('-');
        if (dash_pos != std::string::npos) {
            // Range format: start-end
            std::string start_str = detail::trim(value_part.substr(0, dash_pos));
            std::string end_str = detail::trim(value_part.substr(dash_pos + 1));

            if (start_str.empty() || end_str.empty()) {
                throw std::invalid_argument("Invalid range format: " + value_part);
            }

            ValueType start_val = detail::parse_integral<ValueType>(start_str);
            ValueType end_val = detail::parse_integral<ValueType>(end_str);

            mapping[key].push_back({start_val, end_val});
        } else {
            // Single value
            ValueType value = detail::parse_integral<ValueType>(value_part);
            mapping[key].push_back({value, value});
        }
    }

    return mapping;
}

// Template implementation for parse_map
template<typename KeyType, typename ValueType>
map<KeyType, ValueType> parse_map(const std::string &input) {
    static_assert(std::is_integral_v<KeyType>, "KeyType must be an integral type");
    static_assert(std::is_integral_v<ValueType>, "ValueType must be an integral type");

    map<KeyType, ValueType> mapping;

    if (input.empty()) return mapping;

    // Split by comma to get individual entries
    auto entries = detail::split(input, ',');

    for (const auto& entry_str : entries) {
        std::string entry = detail::trim(entry_str);
        if (entry.empty()) continue;

        // Find the colon separator
        size_t colon_pos = entry.find(':');
        if (colon_pos == std::string::npos) {
            throw std::invalid_argument("Invalid map format, missing colon: " + entry);
        }

        // Extract old value part (before colon)
        std::string old_str = detail::trim(entry.substr(0, colon_pos));
        if (old_str.empty()) {
            throw std::invalid_argument("Empty old value in map: " + entry);
        }

        // Extract new value part (after colon)
        std::string new_str = detail::trim(entry.substr(colon_pos + 1));
        if (new_str.empty()) {
            throw std::invalid_argument("Empty new value in map: " + entry);
        }

        // Parse both values
        KeyType old_value = detail::parse_integral<KeyType>(old_str);
        ValueType new_value = detail::parse_integral<ValueType>(new_str);

        mapping[old_value] = new_value;
    }

    return mapping;
}

template <typename T>
// Template specializations for different vector types
vector<T> parse_input(const std::string &input, type_tag<std::vector<T>>) {
    if constexpr (std::is_same_v<T, int>) {
        auto converter = [](const std::string &token) -> int {
            return std::stoi(token);
        };
        return parse_list<int>(input, converter);
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        auto converter = [](const std::string &token) -> uint64_t {
            return detail::parse_integral<uint64_t>(token);
        };
        return parse_list<uint64_t>(input, converter);
    } else if constexpr (std::is_same_v<T, std::string>) {
        auto converter = [](const std::string &token) -> std::string {
            return token;
        };
        return parse_list<std::string>(input, converter);
    } else {
        static_assert(dependent_false<T>::value, "Unsupported type for parse_input");
    }
}

// Template specializations for different map types
template<typename KeyType, typename ValueType>
pair_map<KeyType, ValueType> parse_input(const std::string &input, type_tag<pair_map<KeyType, ValueType>>) {
    return parse_pair_map<KeyType, ValueType>(input);
}

template<typename KeyType, typename ValueType>
map<KeyType, ValueType> parse_input(const std::string &input, type_tag<map<KeyType, ValueType>>) {
    return parse_map<KeyType, ValueType>(input);
}

// Generic find functions
template <typename Container, typename T>
bool find(const Container &container, const T &value) {
    if (container.empty()) return false;
    return std::find(container.begin(), container.end(), value) != container.end();
}

// Template find function for pair_map: looks for a given key and numeric value
template<typename KeyType, typename ValueType>
bool find(const pair_map<KeyType, ValueType> &m, const KeyType &key, const ValueType &value) {
    if (m.empty()) return false;
    auto it = m.find(key);
    if (it == m.end()) return false;
    for (const auto &range : it->second) {
        if (value >= range.first && value <= range.second) {
            return true;
        }
    }
    return false;
}

// Template find function for map: looks for a given key
template<typename KeyType, typename ValueType>
bool find(const map<KeyType, ValueType> &m, const KeyType &key) {
    if (m.empty()) return false;
    return m.find(key) != m.end();
}

} // namespace parser
