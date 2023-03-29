#include <iostream>
#include <fstream>
#include <unistd.h>
#include <netdb.h>
#include <cassert>
#include <cstring>
#include <unordered_map>
#include "svdpi.h"
#include "arch_sample.h"
#include "src/cov_socket.hpp"
#include "cvm/plusargs.hpp"

using namespace ArchCov;
CovSocket covSocket;

typedef struct  {
  uint64_t cp;
  uint64_t val;
  uint64_t isLastPkt;
} cp_pkt;

extern "C" void sample_sv(const cp_pkt*);

// Constructor
ArchSample::ArchSample(int num_harts) : log("cov.log"), scope_(nullptr) {
   log(cvm::MEDIUM, "Constructing Cosim ArchSample ...\n");
}

// Destructor
ArchSample::~ArchSample() {
}

void ArchSample::reset(){
    assert(covSocket.sampleConnect("tracer_ext") > 0);
}

void ArchSample::coverage_sample(int hart, int step, const whisper_state_t& w) {

   // print instr
   log(cvm::MEDIUM, "<{}> Whisper Step #{}  : [Hart={}, Mode={}, Tag={}, ChangeCount={}, PC={:#x}, Opcode={:#x}, Disasm={}]\n",
    w.time, step, hart, w.priv_mode, w.tag, w.change_count, w.pc, w.opcode, w.buffer);

   svScope scope = svGetScopeFromName("top.tester.arch_sample");
   svSetScope(scope);

   if (covSocket.sampleConnect("tracer_ext") > 0){
    // donot call recvMsg() post EndOfSim
    if (!sample_done) {
      arch_t table;
      covSocket.recvMsg(table);

      int i=0;
      for(auto entry : table.entries_) {
        Point p = static_cast<Point>(entry.first);
        if(p == Point::EndOfSim && entry.second == 1) {
          sample_done = true;
        }
        cp_pkt pkt;
        pkt.cp  = entry.first;
        pkt.val = entry.second;

        //Mark the last packet of the table to indicate instruction boundary
        if(i == (table.entries_.size()-1)) {
                pkt.isLastPkt = 1;
        } else {
                pkt.isLastPkt = 0;
        }
        log(cvm::HIGH, "coverpoint : {}, value : {}\n", entry.first, entry.second);
        //cov_sample(entry.first, entry.second);

        i++;
        sample_sv(&pkt);
      }
    }
   }
   else
    assert(false);
}
