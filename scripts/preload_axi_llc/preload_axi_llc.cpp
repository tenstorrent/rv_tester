#include "preload_axi_llc.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include "cvm/logger.hpp"
#include "cvm/bitmanip.hpp"
#include <bit>

preload_axi_llc::PreloadFiles preload_axi_llc::convert_csv_to_preload_files_per_way(const std::string& csv_path, int num_sets, int num_blocks, int num_ways, int addr_width, int data_width) {
  PreloadFiles pf;
  pf.dataFiles.resize(num_ways);
  pf.tagFiles.resize(num_ways);

  std::vector<std::vector<bool>> cache(num_sets, std::vector(num_ways, false));
  std::unordered_map<std::uint64_t, std::pair<int, std::streampos>> address_positions;

  auto block_bytes = data_width / 8;
  auto block_addr_offset = std::bit_width(unsigned(block_bytes - 1));
  auto block_addr_width = std::bit_width(unsigned(num_blocks - 1));
  auto set_offset = block_addr_width + block_addr_offset;
  auto set_bits = std::bit_width(unsigned(num_sets - 1));
  auto tag_offset = set_bits + set_offset;
  auto tag_addr_bits = addr_width - tag_offset;
  auto tag_bits = tag_addr_bits + 1 /* dirty */ + 1 /* valid */;

  if (block_bytes > 64) {
    cvm::log(cvm::ERROR, "Error: unsupported block width\n");
    return pf;
  }
  auto blocks_per_cacheline = 64 / block_bytes;
  auto blocks_per_tag = num_blocks;
  auto cachelines_per_tag = blocks_per_tag / blocks_per_cacheline;
  if (cachelines_per_tag < 1) {
    cvm::log(cvm::ERROR, "Error: unsupported num blocks\n");
  }

  auto cachelines_per_tag_bits = std::bit_width(unsigned(cachelines_per_tag - 1));
  auto cachelines_per_tag_offset = block_addr_offset + block_addr_width - cachelines_per_tag_bits;

  auto tag_string_width = (tag_bits + 3) / 4;

  // Open an output file stream for each way (for data and tag files).
  std::vector<std::ofstream> dataOut(num_ways);
  std::vector<std::ofstream> tagOut(num_ways);
  for (int i = 0; i < num_ways; i++) {
    pf.dataFiles[i] = "preload_data_way" + std::to_string(i) + ".hex";
    pf.tagFiles[i] = "preload_tag_way" + std::to_string(i) + ".hex";

    dataOut[i].open(pf.dataFiles[i]);
    if (!dataOut[i].is_open()) {
      cvm::log(cvm::ERROR, "Error: could not open output file %s", pf.dataFiles[i].c_str());
    }
    tagOut[i].open(pf.tagFiles[i]);
    if (!tagOut[i].is_open()) {
      cvm::log(cvm::ERROR, "Error: could not open output file %s", pf.tagFiles[i].c_str());
    }
  }

  // Open the CSV file for reading.
  std::ifstream csv(csv_path);
  if (!csv.is_open()) {
    cvm::log(cvm::ERROR, "Error: could not open CSV file: %s", csv_path.c_str());
    return pf;
  }

  std::string line;
  while (std::getline(csv, line)) {
    if (line.empty())
      continue;
    std::istringstream iss(line);
    std::string type, va, pa, data, cacheable;
    // Expecting five comma-separated fields.
    std::getline(iss, type, ',');
    std::getline(iss, va, ',');
    std::getline(iss, pa, ',');
    std::getline(iss, data, ',');
    std::getline(iss, cacheable, ',');

    std::uint64_t pa_val = 0;
    std::istringstream iss_pa(pa);
    iss_pa >> std::hex >> pa_val;

    auto base_cacheline_addr = cachelines_per_tag <= 1 ? pa_val : pa_val & ~cvm::bitmanip::mask<decltype(pa_val)>(cachelines_per_tag_bits + cachelines_per_tag_offset - 1, cachelines_per_tag_offset);
    if (auto it = address_positions.find(base_cacheline_addr); it == address_positions.end()) {

      int index = cvm::bitmanip::slice(base_cacheline_addr, set_offset + set_bits - 1, set_offset);
      auto& set = cache[index];

      int way;
      for (way = 0; way < num_ways; way++) {
        if (!set[way]) {
          set[way] = true;
          break;
        }
      }

      if (way == num_ways) {
        cvm::log(cvm::LOW, "Warning: Could not fit address 0x{:x} into cache axi_llc preloading\n", pa_val);
        continue;
      }

      tagOut[way].seekp(0, std::ios::end);
      dataOut[way].seekp(0, std::ios::end);
      auto tag_lines_written = tagOut[way].tellp() / (tag_string_width + 1);
      for (int s = tag_lines_written; s < index; s++) {
        tagOut[way] << std::setw(tag_string_width) << std::setfill('0') << std::hex << 0 << "\n";
        for (int c = 0; c < cachelines_per_tag; c++) {
          for (int b = 0; b < blocks_per_cacheline; b++) {
            dataOut[way] << std::setw(block_bytes * 2) << std::setfill('0') << std::hex << 0 << "\n";
          }
        }
      }
      tagOut[way].seekp((tag_string_width + 1) * index);
      dataOut[way].seekp(((block_bytes * 2 + 1) * cachelines_per_tag * blocks_per_cacheline) * index);

      auto tag_addr = (base_cacheline_addr >> tag_offset);
      auto tag = tag_addr | uint64_t(0) << (tag_addr_bits + 0) /*dirty*/ | uint64_t(1) << (tag_addr_bits + 1) /* valid */;
      tagOut[way] << std::setw(tag_string_width) << std::setfill('0') << std::hex << tag << "\n";

      for (int c = 0; c < cachelines_per_tag; c++) {
        auto cacheline_addr = base_cacheline_addr + 64 * c;
        for (int b = 0; b < blocks_per_cacheline; b++) {
          auto block_addr = cacheline_addr + block_bytes * b;
          address_positions[block_addr] = std::make_pair(way, dataOut[way].tellp());
          dataOut[way] << std::setw(block_bytes * 2) << std::setfill('0') << std::hex << 0 << "\n";
        }
      }
    }

    for (int b = 0; b < blocks_per_cacheline; b++) {
      auto block_addr = pa_val + block_bytes * b;
      auto it = address_positions.find(block_addr);

      if (it == address_positions.end()) {
        cvm::log(cvm::ERROR, "Error: internal error, could not find address to put data\n");
        return pf;
      }

      std::string_view block_data(data.end() - block_bytes * b * 2 - block_bytes * 2, data.end() - block_bytes * b * 2);
      auto [way, pos] = it->second;
      dataOut[way].seekp(pos);
      // write out line
      dataOut[way] << block_data;
    }
  }
  csv.close();

  // zero out rest of the tag array
  for (int way = 0; way < num_ways; way++) {
    tagOut[way].seekp(0, std::ios::end);
    auto tag_lines_written = tagOut[way].tellp() / (tag_string_width + 1);
    for (int s = tag_lines_written; s < num_sets; s++) {
      tagOut[way] << std::setw(tag_string_width) << std::setfill('0') << std::hex << 0 << "\n";
    }
  }

  // Close all the output file streams.
  for (int i = 0; i < num_ways; i++) {
    dataOut[i].close();
    tagOut[i].close();
  }
  return pf;
}

#ifdef PRELOAD_AXI_LLC_MAIN

int main(int argc, char* argv[]) {
  if (argc != 7) {
    std::cerr << "Usage: " << argv[0] << " <csv_path> <num_sets> <num_blocks> <num_ways> <addr_width> <data_width>\n";
    return 1;
  }

  std::string csv_path = argv[1];
  int num_sets = std::stoi(argv[2]);
  int num_blocks = std::stoi(argv[3]);
  int num_ways = std::stoi(argv[4]);
  int addr_width = std::stoi(argv[5]);
  int data_width = std::stoi(argv[6]);

  preload_axi_llc::PreloadFiles result = preload_axi_llc::convert_csv_to_preload_files_per_way(csv_path, num_sets, num_blocks, num_ways, addr_width, data_width);

  std::cout << "Data Files:\n";
  for (const auto& file : result.dataFiles) {
    std::cout << file << "\n";
  }

  std::cout << "Tag Files:\n";
  for (const auto& file : result.tagFiles) {
    std::cout << file << "\n";
  }

  return 0;
}

#endif
