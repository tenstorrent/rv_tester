#include "preload_axi_llc.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdint>

namespace preload_axi_llc {

PreloadFiles convert_csv_to_preload_files_per_way(const std::string& csv_path, unsigned numWays) {
    std::ifstream csv(csv_path);
    if (!csv.is_open()) {
        std::cerr << "Error: could not open CSV file: " << csv_path << std::endl;
        return PreloadFiles{};
    }
    
    std::vector<std::string> dataLines;
    std::vector<std::string> tagLines;
    std::string line;
    while (std::getline(csv, line)) {
        if (line.empty())
            continue;
        std::istringstream iss(line);
        std::string type, va, pa, data, cacheable;
        std::getline(iss, type, ',');
        std::getline(iss, va, ',');
        std::getline(iss, pa, ',');
        std::getline(iss, data, ',');
        std::getline(iss, cacheable, ',');
        dataLines.push_back(data);

        std::uint64_t pa_val = 0;
        std::istringstream iss_pa(pa);
        iss_pa >> std::hex >> pa_val;
        std::uint64_t tag = pa_val >> (DEFAULT_INDEX_BITS + DEFAULT_BLOCK_OFFSET_BITS);
        std::stringstream tag_ss;
        tag_ss << std::setw(8) << std::setfill('0') << std::hex << tag;
        tagLines.push_back(tag_ss.str());
    }
    csv.close();
    
    // Distribute the CSV rows among the ways round-robin.
    std::vector<std::vector<std::string>> perWayData(numWays);
    std::vector<std::vector<std::string>> perWayTag(numWays);
    for (size_t i = 0; i < dataLines.size(); i++) {
        unsigned way = i % numWays;
        perWayData[way].push_back(dataLines[i]);
        perWayTag[way].push_back(tagLines[i]);
    }
    
    // Write out each way's data and tag preload file.
    PreloadFiles pf;
    for (unsigned w = 0; w < numWays; w++) {
        std::string dataFilename = "preload_data_way" + std::to_string(w) + ".hex";
        std::ofstream dataFile(dataFilename);
        if (!dataFile.is_open()) {
            std::cerr << "Error: could not open output file " << dataFilename << std::endl;
            continue;
        }
        for (const auto& row : perWayData[w]) {
            dataFile << row << "\n";
        }
        dataFile.close();
        pf.dataFiles.push_back(dataFilename);
        std::cout << "Created preload data file for way " << w << ": " << dataFilename << std::endl;
        
        std::string tagFilename = "preload_tag_way" + std::to_string(w) + ".hex";
        std::ofstream tagFile(tagFilename);
        if (!tagFile.is_open()) {
            std::cerr << "Error: could not open output file " << tagFilename << std::endl;
            continue;
        }
        for (const auto& row : perWayTag[w]) {
            tagFile << row << "\n";
        }
        tagFile.close();
        pf.tagFiles.push_back(tagFilename);
        std::cout << "Created preload tag file for way " << w << ": " << tagFilename << std::endl;
    }
    return pf;
}

} // namespace preload_axi_llc
