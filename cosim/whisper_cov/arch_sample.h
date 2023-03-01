#pragma once

#include <string>
#include "bridge_if.h"
#include "cvm/logger.hpp"
#include "src/cosim_sample/cosim_sample.h"
#include "svdpi.h"

class ArchSample {
public:
   ArchSample(int num_harts);
   ~ArchSample();

  void reset();
  void coverage_sample(int hart, int step, const whisper_state_t& w);
  void set_scope(svScope s) { scope_ = s; }
  
private:
  cvm::file_logger log;

  svScope scope_;
  int num_harts_ = 0;
  bool sample_done = false;
};
