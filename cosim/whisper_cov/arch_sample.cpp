#include <iostream>
#include <fstream>
#include <unistd.h>
#include <netdb.h>
#include <cassert>
#include <cstring>
#include <unordered_map>
#include "svdpi.h"
#include "arch_sample.h"
#include "cvm/plusargs.hpp"
#include "vpi_user.h"      

using namespace ArchCov;

extern "C" void cov_sample(uint64_t cp, uint64_t val);

// Constructor
ArchSample::ArchSample(int num_harts) : log("cov.log"), scope_(nullptr) {
   log(cvm::MEDIUM, "Constructing Cosim ArchSample ...\n");
}

// Destructor
ArchSample::~ArchSample() {
}

void ArchSample::reset(){
    assert(sampleConnect("tracer_ext") > 0);
}

void ArchSample::coverage_sample(int hart, int step, const whisper_state_t& w) {

   // print instr
   log(cvm::MEDIUM, "<{}> Whisper Step #{}  : [Hart={}, Mode={}, Tag={}, ChangeCount={}, PC={:#x}, Opcode={:#x}, Disasm={}]\n", 
    w.time, step, hart, w.priv_mode, w.tag, w.change_count, w.pc, w.opcode, w.buffer);

   svScope scope = svGetScopeFromName("top.tester.arch_sample");
   svSetScope(scope);
   
   if (sampleConnect("tracer_ext") > 0){
    // donot call recvMsg() post EndOfSim
    if (!sample_done) {
      arch_t table;
      recvMsg(table);

      for(auto entry : table.entries_) {
        Point p = static_cast<Point>(entry.first);
        if(p == Point::EndOfSim && entry.second == 1) {
          sample_done = true;
        }

      log(cvm::HIGH, "coverpoint : {}, value : {}\n", entry.first, entry.second);
      cov_sample(entry.first, entry.second);
      }
    }
   }
   else{
    cvm::log(cvm::NONE, "Error: [cosim-coverage] Whisper connect failed ...\n");
    vpi_control(vpiFinish);
   }
}
