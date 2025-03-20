#include "preload_axi_llc.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include "cvm/logger.hpp"

PreloadFiles preload_axi_llc::convert_csv_to_preload_files_per_way(const std::string& csv_path, int index_bits, int offset_bits, int numWays) {
    PreloadFiles pf;
    pf.dataFiles.resize(numWays);
    pf.tagFiles.resize(numWays);

    // Open an output file stream for each way (for data and tag files).
    std::vector<std::ofstream> dataOut(numWays);
    std::vector<std::ofstream> tagOut(numWays);
    for (int i = 0; i < numWays; i++) {
        pf.dataFiles[i] = "preload_data_way" + std::to_string(i) + ".hex";
        pf.tagFiles[i]  = "preload_tag_way"  + std::to_string(i) + ".hex";

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
    size_t lineCount = 0;
    while (std::getline(csv, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string type, va, pa, data, cacheable;
        // Expecting five comma-separated fields.
        std::getline(iss, type, ',');
        std::getline(iss, va, ',');
        std::getline(iss, pa, ',');
        std::getline(iss, data, ',');
        std::getline(iss, cacheable, ',');

        // Determine which way this row belongs to (round-robin).
        unsigned way = lineCount % numWays;
        dataOut[way] << data << "\n";

        // Compute the tag from the physical address.
        std::uint64_t pa_val = 0;
        std::istringstream iss_pa(pa);
        iss_pa >> std::hex >> pa_val;
        std::uint64_t tag = pa_val >> (index_bits + offset_bits);
        std::stringstream tag_ss;
        tag_ss << std::setw(8) << std::setfill('0') << std::hex << tag;
        tagOut[way] << tag_ss.str() << "\n";

        lineCount++;
    }
    csv.close();

    // Close all the output file streams.
    for (int i = 0; i < numWays; i++) {
        dataOut[i].close();
        tagOut[i].close();
    }
    std::cout << "CSV conversion complete. " << lineCount
              << " lines distributed among " << numWays << " ways." << std::endl;
    return pf;
}
