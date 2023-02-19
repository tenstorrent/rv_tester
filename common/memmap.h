#pragma once

#include <unordered_map>
#include <string>

namespace memmap {

  struct memmap_entry_t {
      uint64_t base;
      uint64_t size;
      std::string   type;
      std::string   tag ;
      uint64_t end ;
  };
  
  using memmap_t = std::unordered_map<std::string, memmap_entry_t>;
  
  extern memmap_t m;

  void get(memmap_t& memmap);
  void parse();
}
