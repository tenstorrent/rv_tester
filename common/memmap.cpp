#include "memmap.h"
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <cstdint>
#include "nlohmann/json.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/messenger.hpp"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
 
DEFINE_string(memmap_json_path, "", "Path to memory map json");

REGISTRY_register(memmap, TOP.PLATFORM.MEMMAP, 0);

memmap::memmap(cvm::topology::loc_t loc, unsigned) {
 cvm::log(cvm::MEDIUM, "[memmap] initializing memmap RPC\n");
 cvm::registry::messenger.procedure<parseRPC>(loc, [this] ()                                         { return this->parse();});
 cvm::registry::messenger.procedure<getRPC>  (loc, [this] (std::map<std::string, memmap_entry_t>& m) { return this->get(m);});
}

void memmap::add_entry(std::string base_str, std::string type, std::string tag, uint64_t base, uint64_t size, nlohmann::json attributes) {
  uint64_t end = base + size - 1;
  memmap_entry_t memory(base_str, base, size, type, tag, end, attributes);
  memmap_[tag] = memory;
}

bool memmap::parse() {
  if (parsed_)
    return parse_status_;
  parsed_ = true;
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
  for (auto& el : j) {
    memmap_entry_t map;
    std::string base_str = el.at("base");
    std::string type     = el.at("type");
    std::string tag      = el.at("tag");
    uint64_t base = std::stoull((std::string)el.at("base"), nullptr, 16);
    uint64_t size = std::stoull((std::string)el.at("size"), nullptr, 16);
    nlohmann::json attributes = el.contains("attributes")
      ? el.at("attributes")
      : nlohmann::json();

    add_entry(base_str, type, tag, base, size, attributes);
  }
  //check legality + overlaps
  std::vector<std::pair<std::string, memmap_entry_t>> vec_m(memmap_.begin(), memmap_.end());
  std::sort(vec_m.begin(), vec_m.end(), [] (const std::pair<std::string, memmap_entry_t>& a, const std::pair<std::string, memmap_entry_t>& b)
                                                                                             { return a.second.base < b.second.base; });
  cvm::log(cvm::HIGH,"Sorted Memmap by base : \n");
  for (const auto& entry : vec_m) {
      cvm::log(cvm::HIGH,"Key: {}, Base Addr: {:#x}, Size {:#x}, Type: {}  \n",entry.first,entry.second.base,entry.second.size,entry.second.type);
  }

  bool overlapDetected = false;
  for (size_t i = 0; i < vec_m.size(); ++i) {
    for (size_t j = i + 1; j < vec_m.size(); ++j) {
      uint64_t end_i = vec_m[i].second.base + vec_m[i].second.size;
      uint64_t end_j = vec_m[j].second.base + vec_m[j].second.size;
      if ((vec_m[i].second.base < end_j && vec_m[j].second.base < end_i)) {
          std::string log_str;
          log_str += fmt::format("\n\n");
          log_str += fmt::format("****************************************** \n");
          log_str += fmt::format(" Error: Memmap.json check failed: Overlap detected between   {}   and    {} \n",vec_m[i].first,vec_m[j].first);
          log_str += fmt::format("****************************************** \n");
          cvm::log(cvm::NONE, fmt::to_string(log_str));
          overlapDetected = true;
      }
    }
  }

    if (!overlapDetected) {
        cvm::log(cvm::HIGH,"\nMemmap check Passed: No overlaps detected. \n");
    } else {
        cvm::log(cvm::LOW,"\n\n");
        cvm::log(cvm::LOW,"******************************************************* \n");
        cvm::log(cvm::LOW,"******************************************************* \n");
        cvm::log(cvm::ERROR,"\nError: Memmap.json check failed, Stopping simulation memmap.json has overlaps \n");
        cvm::log(cvm::LOW,"******************************************************* \n");
        cvm::log(cvm::LOW,"******************************************************* \n");
        assert(false);
    }
    parse_status_ = true;
    return parse_status_;
}
