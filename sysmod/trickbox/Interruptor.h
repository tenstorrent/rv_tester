//void Interruptor::handle_itp(uint64_t offset,uint64_t dword,cbs_t& cbs){
//  
//  if(offset == 0){
//   //immidiate interrupt
//  unsigned hart = dword & 0xfff;
//  unsigned  event = (dword >> 12) & 0xff;
//  unsigned eventValue = (dword >> 20);
//  bool flag = eventValue != 0;
//  trickbox::trickboxInterrupt(hart, flag, event, cbs);
//  }else if(offset < 1000){
//  //delayed interrupt
//  //unsigned hart = dword & 0xfff;
//  //unsigned  event = (dword >> 12) & 0xff;
//  //unsigned eventValue = (dword >> 20);
//  //unsigned flag = eventValue != 0;
//  //delayed interrupts
//    unsigned event = (addr>> 7 )& 0xf; //[10:7];//(value >> 12) & 0xff;
//    unsigned eventValue_delay = (dword>>12) & 0x7ffff;//[30:0] ;
//    unsigned flag_m = dword & 0x80000000;
//    bool flag = flag_m !=0;
//    unsigned hart = dword & 0xfff; //
//  trickbox::trickboxDelayedInterrupt(hart, flag, event,eventValue_delay, cbs);
//  }
//
//
//}

#include <vector>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <string>
#include<map>
#include <fstream>
#include <queue>
#include <list>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include "device.h"
#include "svdpi.h"
#pragma once
//#include "vpi_user.h"  
//#include "trickbox.h"

//svScope g_scope;
/// This must be implemented on the verilog side: set the machine interrupt
/// pending CSR (MIP) bit corresponding to the given interript id (must be
/// between 0 and 64) for the given hart to the given value.
//extern "C" void SetHartMip(int hart, int interruptId, int value); 
//extern "C" void SetHartDelayedMip(int hart, int interruptId, int value,int delay); 
/// Local interrupt controller. This is a memory mapped device with a
/// single advertised address. Writing to the advertised address will
/// trigger an interrupt in some hart.
class Interruptor : public device 
{
  
public:

  enum Event { SS = 1, VSS = 2, MS = 3, ST = 5, MT = 7, SE = 9, VSE = 10, ME = 11 };

  /// Constructor. Throw an exception if hartCount is is larger than 4094.
  Interruptor(unsigned hartCount = 0, uint64_t address = 0,uint64_t size = 0);

  /// Destructor
  ~Interruptor();
  /// Configure/re-configure the hart count and address of this interruptor.
  /// Return true on success and false if hart count is out of bounds./
  bool configure(unsigned hartCount, uint64_t address);

  /// This should be called after each memory write whenever a write
  /// may affect this object. The written value encodes a target hart
  /// and an event for that hart (such as external interrupt).  This
  /// method will then invoke a trigger method (below) to change the
  /// interrupt pin of the targeted hart. Return true on success
  /// return false if hart or event ids encoded in value are out of
  /// bouneds.
  bool handleWrite(uint64_t addr, uint64_t value,cbs_t& cbs);
  bool handleWriteHelper(uint64_t addr, uint64_t value);

  /// This is called by handleWrite whenever there is a write
  /// targeting this object.  An override of this method is expected
  /// to change the bit corresponding to eventId in the MIP CSR of the
  /// given hart.
  void triggerInterrupt(int hartId, int eventId, int flag);
  void triggerDelayedInterrupt(int hartId, int eventId, int flag, int delay);
  virtual void cbstriggerInterrupt(unsigned hart, bool flag,unsigned event, cbs_t& cbs)
  {
    cbs.push_back(cb_t{Callback::TRICKBOX_EVT, hart, flag,event,0});
  }

  // Used to assert/deassert a timer interrupt for given hart.
  virtual void cbstriggerDelayedInterrupt(unsigned hart, bool flag,unsigned event, unsigned delay, cbs_t& cbs)
  {
    cbs.push_back(cb_t{Callback::TRICKBOX_EVT, hart, flag,event,delay});
  }
  // Reads outside of device range are ignored. Reads with length
  // different than 8 are ignored.
  virtual void read(uint64_t addr, size_t length, data_t& data,
                    cbs_t& cbs) override;

  // Writes outside of device range are ignored. Writes with length
  // different than 8 are ignored.
  virtual void write(uint64_t addr, size_t length, const data_t& data,
		     const strb_t& strb, cbs_t& cbs) override; 
  //trickbox tbox{"tbox",0x800};
private:
  static std::map<std::string, svScope> g_sv_scope;
  unsigned hartCount_ = 0;
  uint64_t addr_ = 0;
  std::vector<std::string> eventNames_;
};

