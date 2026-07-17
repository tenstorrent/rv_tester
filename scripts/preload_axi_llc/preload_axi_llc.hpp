// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <vector>

namespace preload_axi_llc {

struct PreloadFiles {
  std::vector<std::string> dataFiles;
  std::vector<std::string> tagFiles;
};

PreloadFiles convert_csv_to_preload_files_per_way(const std::string& csv_path, int num_sets, int num_blocks, int num_ways, int addr_width, int data_width);

} // namespace preload_axi_llc
