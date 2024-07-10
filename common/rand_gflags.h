#include <gflags/gflags.h>
#include "cvm/logger.hpp"
#include <random>
#include <map>

namespace gflags {

  class rand {

    public:

      rand(const std::vector<std::string>& flags) : gen(1) {
        for (const auto& name : flags) {
          std::string value;
          bool found = gflags::GetCommandLineOption(name.c_str(), &value);
          assert(found && "Plusarg not found");
          entries[name] = parse(value);
        }
      }

      void randomize() {
        for (auto& [name, entry] : entries) {
          entry.current_value = entry.distrib(gen);
        }
      }

      uint32_t get(const std::string& name) {
        auto it = entries.find(name);
        if (it == entries.end())
          cvm::log(cvm::ERROR, "Error: Incorrect plusarg: {}\n", name);
        return it->second.current_value;
      }

    private:

      std::mt19937 gen;

      struct entry {
        uint32_t min;
        uint32_t max;
        std::uniform_int_distribution<uint32_t> distrib;
        uint32_t current_value;

        entry() : min(0), max(0), distrib(min, max), current_value(0) {}
        entry(uint32_t min_, uint32_t max_) : min(min_), max(max_), distrib(min, max), current_value(0) {}
      };

      std::map<std::string, entry> entries;

      entry parse(const std::string& flag) {
        size_t colon_pos = flag.find(':');
        try {
          if (colon_pos == std::string::npos) {
            uint32_t val = std::stoul(flag);
            return entry(val, val);
          } else {
            uint32_t min = std::stoul(flag.substr(0, colon_pos));
            uint32_t max = std::stoul(flag.substr(colon_pos + 1));
            if (min > max) {
              cvm::log(cvm::ERROR, "Error: Invalid flag {}: min must be less than or equal to max\n", flag);
            }
            return entry(min, max);
          }
        }
        catch (const std::exception& e) {
          cvm::log(cvm::ERROR, "Error: Failed to parse flag {}: {}\n", flag, e.what());
        }
        return entry();
      };
  };
}
