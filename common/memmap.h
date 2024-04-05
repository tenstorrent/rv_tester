#pragma once

#include <map>
#include <string>
#include <cstdint>

namespace memmap {

  struct memmap_entry_t {
      std::string base_str;
      uint64_t base;
      uint64_t size;
      std::string   type;
      std::string   tag ;
      uint64_t end ;
  };

  // ordered so when we iterate to fill SV and create devices it will be deterministic
  using memmap_t = std::map<std::string, memmap_entry_t>;

  extern memmap_t m;

  void get(memmap_t& memmap);
  void parse();
}
