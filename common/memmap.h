#pragma once

#include <iostream>
#include <fstream>
#include <unordered_map>
#include "nlohmann/json.hpp"
#include "cvm/plusargs.hpp"

DECLARE_string(memmap_json_path);

namespace memmap {

  using memmap_t = std::unordered_map<std::string, uint64_t>;
  using memmap_list_t = std::unordered_map<std::string, memmap_t>;

  inline void load(memmap_list_t& m) {
    std::ifstream f(FLAGS_memmap_json_path);
    if (!f.good()) {
      std::cerr << "Error: Failed to open memmap config file: " << FLAGS_memmap_json_path << "\n"; 
      assert(false);
    }

    nlohmann::json j;
    try {
      j = nlohmann::json::parse(f);
    }
    catch (nlohmann::json::parse_error& e) {
      std::cerr << "Error: Memmap json::parse exception.\n"
        << "  File: " << FLAGS_memmap_json_path << "\n"
        << "  Message: " << e.what() << "\n"
        << "  Excp ID: " << e.id << "\n"
        << "  Byte pos: " << e.byte << "\n";
      assert(false);
    }
    catch (...) {
      std::cerr << "Error: Memmap json unknown exception.\n  File: " << FLAGS_memmap_json_path << "\n";
      assert(false);
    }

    std::cout << "----Memory map----\n";
    for (auto& el : j) {
      memmap_t map;
      map["base"] = std::stoull((std::string)el.at("base"), nullptr, 16);
      map["size"] = std::stoull((std::string)el.at("size"), nullptr, 16);
      map["end"] = map["base"] + map["size"];

      m[(std::string)el.at("tag")] = map;

      std::cout << (std::string)el.at("tag") << ": " << 
        " Base: 0x" << std::hex << map["base"] <<
        " Size: 0x" << std::hex << map["size"] << "\n";
    }
    std::cout << "------------------\n";
  }
}
