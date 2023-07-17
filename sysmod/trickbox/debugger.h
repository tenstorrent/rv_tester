// -*- c++ -*-

#pragma once

#include <mutex>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <random>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <queue>
#include <vector>
#include "pcg_random.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "subdevice.h"
#include "vpi_user.h"

//DEFINE_string(dbg_input_file_path, "", "Path to file containing debugger commands");
DECLARE_string(dbg_input_file_path);
// Define a core local  (debugger) at the given address
// and for the given hart count. The size will be 48k bytes.
class debugger : public subdevice
{
public:

  /// Define a debugger device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  debugger(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc);

  // Destructor.
  virtual ~debugger();

  // Copy n bytes from the given integer, x, to the data iterator
  // following little endian convention. If n is larger than the size
  // of x, then copy zero bytes after copying the bytes of x.
  template <typename INT>
  void serializeInt(INT x, size_t n, data_t& data)
  {
    for (unsigned i = 0; i < n; ++i, x >>= 8)
      data[i] = x & 0xff;
  }

  // Copy bytes from data iterator into the given integer following
  // lilttle endian convention.
  template <typename INT>
  void deserializeInt(const data_t& data, INT& x)
  {
    x = 0;
    for (unsigned i = 0; i < sizeof(x); ++i)
      x |= INT(data[i]) << i*8;
  }

  /// Read length bytes from the given address to the data iterator.
  /// No-op if address is outside the range of this debugger or if
  /// address is not properly aligned.
  cvm::messenger::task<void> read(uint64_t addr, size_t length, data_t& data);

  // Write to this debugger.
  virtual void write(uint64_t addr, size_t length, const data_t& data,
                      const strb_t& strb) override;

  virtual void tick(uint64_t advance) override
  {
    //std::cout<<"[debugger]: tick\n";
    std::lock_guard<std::mutex> lock(mutex_);
    timer_ += advance;
    timer_advance = advance;
    //if(dbg_trigger){
      drive_csv_dmi_cmds();
    //}
  }

  void reset() override {
      std::cout<<"[TRICKBOX]: Reset debugger\n";
  }
  void parse_dmi_from_csv();
  void drive_csv_dmi_cmds();

  struct dmi_data_t {
    unsigned hart;
    unsigned upper_dmi_data;
    unsigned lower_dmi_data;
  };

  struct dmi_req_t {
    unsigned  func_bits;
    unsigned  addr;
    unsigned  op;
    unsigned  data;
  };
  // Used to assert/deassert a trickbox interrupt (PIPI) for given hart.
  //virtual void trickboxDmiWrite(unsigned hart, unsigned upper_dmi_data, unsigned lower_dmi_data, cbs_t& cbs)
  virtual void trickboxDmiWrite(unsigned hart, unsigned upper_dmi_data, unsigned lower_dmi_data)
  {
    std::cout<<"TrickBox DMI Write to hart "<<hart<<" upper dmi data "<<upper_dmi_data<<" lower dmi data "<<lower_dmi_data<<" \n";
    //cbs.push_back(cb_t{Callback::TRICKBOX_DMI_WR, hart, upper_dmi_data, lower_dmi_data, 0});
    cvm::registry::messenger.signal(loc(), dmi_data_t{hart, upper_dmi_data, lower_dmi_data});
    //cvm::messenger::send(dmi_t, dmi_pkt);
  }


private:
  std::vector<uint32_t> soft_;  // Software interrupt: one per hart.
  std::vector<uint64_t> timeCompare_;  // One per interrupt type.
  std::vector<uint32_t> IntrHart_;  // Hart to be interrupted.
  std::vector<bool> delayedRandomIntValid_; // Valid bit for interrupt
  std::vector<bool> IntrValue_; // Value of interrupt pin
  std::vector<bool> timerIntPrev_; // Value of interrupt pin
  uint64_t timer_ = 0;
  uint64_t timer_advance = 200;
  uint64_t debugger_base = 0x9050000;
  uint64_t debugger_trigger = 0x9060000;

  std::atomic<bool> terminate_ = false;
  std::mutex mutex_;

  std::vector<std::vector<std::string>> csv_data;
  std::queue<dmi_req_t> dmi_cmd_q;
  std::queue<dmi_req_t> dmi_rsp_q;
  unsigned dbg_file_mode = 0;
  unsigned dbg_trigger = 0;
  unsigned step_ahead_queue_on =0;
  unsigned step_quit_queue_on =0;
  unsigned step_instr_cnt = 0;

  std::vector<std::vector<std::string>> content;
	std::vector<std::string> row;
  //file(FLAGS_dbg_input_file_path);

};

