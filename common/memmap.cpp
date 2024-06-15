#include "memmap.h"

#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"

DEFINE_string(memmap_json_path, "", "Path to memory map json");

namespace memmap {

memmap_t m;

void get(memmap_t& memmap) {
  memmap = m;
}
bool compareByMMBase(const std::pair<std::string, memmap_entry_t>& a, const std::pair<std::string, memmap_entry_t>& b) {
    return a.second.base < b.second.base;
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
    map.base_str = el.at("base");
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
  // Check memmap for legality
  std::vector<std::pair<std::string, memmap_entry_t>> vec_m(m.begin(), m.end());
  std::sort(vec_m.begin(), vec_m.end(), compareByMMBase);
  // Print sorted memmap
  cvm::log(cvm::HIGH,"Sorted Memmap by base : \n");
  for (const auto& entry : vec_m) {
      cvm::log(cvm::HIGH,"Key: {}, Base Addr: {:#x}, Size {:#x}, Type: {}  \n",entry.first,entry.second.base,entry.second.size,entry.second.type);
  }

  // Check for overlaps
  bool overlapDetected = false;
    for (size_t i = 0; i < vec_m.size(); ++i) {
        for (size_t j = i + 1; j < vec_m.size(); ++j) {
            uint64_t end_i = vec_m[i].second.base + vec_m[i].second.size;
            uint64_t end_j = vec_m[j].second.base + vec_m[j].second.size;

            if ((vec_m[i].second.base < end_j && vec_m[j].second.base < end_i)) {
                std::string log_str;
                log_str += fmt::format("\n\n");
                log_str += fmt::format("****************************************** \n");
                log_str += fmt::format(" ERROR: Memmap.json check failed: Overlap detected between   {}   and    {} \n",vec_m[i].first,vec_m[j].first);
                log_str += fmt::format("****************************************** \n");
                log(cvm::NONE, fmt::to_string(log_str));
                overlapDetected = true;
            }
        }
    }

    if (!overlapDetected) {
        cvm::log(cvm::HIGH,"\nMemmap check Passed: No overlaps detected. \n");
    }else{
        cvm::log(cvm::LOW,"\n\n");
        cvm::log(cvm::LOW,"******************************************************* \n");
        cvm::log(cvm::LOW,"******************************************************* \n");
        cvm::log(cvm::ERROR,"\nERROR: Memmap.json check failed, Stopping simulation memmap.json has overlaps \n");
        cvm::log(cvm::LOW,"******************************************************* \n");
        cvm::log(cvm::LOW,"******************************************************* \n");
        assert(false);
    }
}

}
