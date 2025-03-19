#ifndef PRELOAD_AXI_LLC_HPP
#define PRELOAD_AXI_LLC_HPP

#include <string>
#include <vector>

// Set the default number of cache ways to split the preload file into.
#ifndef DEFAULT_NUM_WAYS
#define DEFAULT_NUM_WAYS 4
#endif

struct PreloadFiles {
    std::vector<std::string> dataFiles;
    std::vector<std::string> tagFiles;
};

namespace preload_axi_llc {

// Converts a CSV file (each row: type,va,pa,data,cacheable)
// into two vectors of file names (one for data and one for tags),
// splitting the rows among the ways round-robin.
// The number of ways is configurable via the numWays parameter (default is DEFAULT_NUM_WAYS).
PreloadFiles convert_csv_to_preload_files_per_way(const std::string& csv_path, unsigned numWays = DEFAULT_NUM_WAYS);

} // namespace preload_axi_llc

#endif // PRELOAD_AXI_LLC_HPP
