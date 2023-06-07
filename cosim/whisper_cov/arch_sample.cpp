#include <iostream>
#include <fstream>
#include <unistd.h>
#include <netdb.h>
#include <cassert>
#include <cstring>
#include <unordered_map>
#include "svdpi.h"
#include "src/cov_sample/cov_sample_interface.hpp"
#include "cvm/plusargs.hpp"

DECLARE_bool(cov);

using namespace ArchCov;
covSampleInterface covSampleInterface_;

extern "C" void sample_sv(const cp_pkt*);

extern "C" void trace(arch_t& table) {
  covSampleInterface_.sample_packets(table);
}
