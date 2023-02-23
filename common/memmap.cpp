#include "memmap.h"

#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
#include "cvm/plusargs.hpp"

DEFINE_string(memmap_json_path, "", "Path to memory map json");

namespace memmap {

memmap_t m;

void get(memmap_t& memmap) {
  memmap = m;
}

void parse() {
    
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
    memmap_entry_t map;
    map.base = std::stoull((std::string)el.at("base"), nullptr, 16);
    map.size = std::stoull((std::string)el.at("size"), nullptr, 16);
    map.end  = map.base + map.size;
    map.tag  = el.at("tag");
    map.type = el.at("type");

    m[map.tag] = map;

    std::cout << map.tag << ": " << 
      " Base: 0x" << std::hex << map.base <<
      " Size: 0x" << std::hex << map.size << "\n";
  }
  std::cout << "------------------\n";

}

}
