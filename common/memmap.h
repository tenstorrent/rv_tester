#pragma once

#include <map>
#include <string>
#include <cstdint>
#include "nlohmann/json.hpp"
#include "cvm/messenger.hpp"

struct memmap_entry_t {
    std::string base_str;
    uint64_t base;
    uint64_t size;
    std::string   type;
    std::string   tag ;
    uint64_t end ;
    nlohmann::json attributes;
};

class memmap {
  public:
    memmap(cvm::topology::loc_t loc, unsigned);
    bool get(std::map<std::string, memmap_entry_t>& m) {
      if (parsed_ || parse())
          m = memmap_;
      return parse_status_;
    };
    bool parse();
    void add_entry(std::string base_str, std::string type, std::string tag, uint64_t base, uint64_t size, nlohmann::json attributes);
    CVM_MESSENGER_procedure_call(parseRPC, bool (void));
    CVM_MESSENGER_procedure_call(getRPC,   bool (std::map<std::string, memmap_entry_t>&));
  private:
    std::map<std::string, memmap_entry_t> memmap_;
    bool parsed_ = false; // cache parsing
    bool parse_status_ = false;
};
