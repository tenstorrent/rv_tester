#ifndef PRELOAD_AXI_LLC_HPP
#define PRELOAD_AXI_LLC_HPP

#include <string>
#include <vector>


namespace preload_axi_llc {
    
    struct PreloadFiles {
        std::vector<std::string> dataFiles;
        std::vector<std::string> tagFiles;
    };

    // Converts a CSV file (each row: type,va,pa,data,cacheable)
    // into two vectors of file names (one for data and one for tags),
    // splitting the rows among the ways round-robin.
    // The number of ways is configurable via the numWays parameter (default is DEFAULT_NUM_WAYS).
    PreloadFiles convert_csv_to_preload_files_per_way(const std::string& csv_path, int index_bits, int offset_bits, int numWays);

} // namespace preload_axi_llc

#endif // PRELOAD_AXI_LLC_HPP
