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
#include "cvm/registry.hpp"

using namespace ArchCov;
covSampleInterface covSampleInterface_;
svScope scope_;

extern "C" void sample_sv(const cp_pkt*);

extern "C" void trace(arch_t table) {
  cvm::registry::callbacks.push(
      scope_,
      [=]() { covSampleInterface_.sample_packets(const_cast<ArchCov::arch_t&>(table)); });
}

extern "C" void archcov_set_scope() {
  scope_ = svGetScope();
}
