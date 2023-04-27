#include <iostream>
#include <fstream>
#include <unistd.h>
#include <netdb.h>
#include <cassert>
#include <cstring>
#include <unordered_map>
#include "svdpi.h"
#include "arch_sample.h"
#include "src/cov_sample/cov_sample_interface.hpp"
#include "cvm/plusargs.hpp"

using namespace ArchCov;
covSampleInterface covSampleInterface_;

extern "C" void sample_sv(const cp_pkt*);

// Constructor
ArchSample::ArchSample(int num_harts) : log("cov.log"), scope_(nullptr) {
   log(cvm::MEDIUM, "Constructing Cosim ArchSample ...\n");
}

// Destructor
ArchSample::~ArchSample() {
}

void ArchSample::reset(){
    assert(covSampleInterface_.connect("tracer_ext") > 0);
}

void ArchSample::coverage_sample(int hart, int step, const whisper_state_t& w) {

   // print instr
   log(cvm::MEDIUM, "<{}> Whisper Step #{}  : [Hart={}, Mode={}, Tag={}, ChangeCount={}, PC={:#x}, Opcode={:#x}, Disasm={}]\n",
    w.time, step, hart, w.priv_mode, w.tag, w.change_count, w.pc, w.opcode, w.disasm);

   svScope scope = svGetScopeFromName("top.tester.arch_sample");
   svSetScope(scope);

   if (covSampleInterface_.connect("tracer_ext") > 0){
     if (!sample_done){
       sample_done = covSampleInterface_.sample_packets();
     }
   }
   else
    assert(false);
}
