// -*- c++ -*-

#pragma once

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
#include <filesystem>
#include <dirent.h>
#include <cstdlib>
#include <ctime>
#include "pcg_random.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "subdevice.h"
#include "vpi_user.h"
//#include "dm_model/dm_model.hpp"

// DEFINE_string(jtag_input_file_path, "", "Path to file containing jtag_driver commands");
DECLARE_string(jtag_input_file_path);
DECLARE_int32(seed);
DECLARE_bool(random_jtag_entry);
DECLARE_int32(random_jtag_start_delay);
DECLARE_int32(jtag_delay_min);
DECLARE_int32(jtag_delay_max);
DECLARE_int32(jtag_max_snippets);
DECLARE_string(jtag_template_dir_path);
// Define a core local  (jtag_driver) at the given address
// and for the given hart count. The size will be 48k bytes.
class jtag_driver : public subdevice
{
public:
  /// Define a jtag_driver device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  jtag_driver(const std::string &tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc);

  // Destructor.
  virtual ~jtag_driver();

  // Copy n bytes from the given integer, x, to the data iterator
  // following little endian convention. If n is larger than the size
  // of x, then copy zero bytes after copying the bytes of x.
  template <typename INT>
  void serializeInt(INT x, size_t n, data_t &data)
  {
    for (unsigned i = 0; i < n; ++i, x >>= 8)
      data[i] = x & 0xff;
  }

  // Copy bytes from data iterator into the given integer following
  // lilttle endian convention.
  template <typename INT>
  void deserializeInt(const data_t &data, INT &x)
  {
    x = 0;
    for (unsigned i = 0; i < sizeof(x); ++i)
      x |= INT(data[i]) << i * 8;
  }

  /// Read length bytes from the given address to the data iterator.
  /// No-op if address is outside the range of this jtag_driver or if
  /// address is not properly aligned.
  cvm::messenger::task<void> read(uint64_t addr, size_t length, data_t &data);
  void read_dev(uint64_t addr, size_t length,  data_t& data) override;
  // Write to this jtag_driver.
  virtual void write(uint64_t addr, size_t length, const data_t &data,
                     const strb_t &strb) override;

  virtual void tick(uint64_t advance) override
  {
    num_ticks++;
    cvm::log(cvm::HIGH, "[jtag_driver]: Tick {}\n",num_ticks);
    timer_ += advance;
    timer_advance = advance;
    if( num_ticks > 30) 
    {
    checkJtagEvents();

    if(executing_nop){
      nop_count--;
      cvm::log(cvm::HIGH, "[jtag_driver]: Executing Nop ,Nop count {}\n",nop_count);
      if(nop_count==0)
        executing_nop = false;
    }else if(executing_loop){
      cvm::log(cvm::HIGH, "[jtag_driver]: Executing loop \n");
      Run_cmd_loop();
    }else{
    drive_csv_jtag_cmds();
    }
   }
  }
 bool isNthBitSet(uint64_t number,int N) {
    // Shift the number right by 62 bits and check if the least significant bit is set
    return (number >> N) & 1;
 }
 void Run_cmd_loop()
  {
    if(loop_idx == 0 && loop_execution_cnt>0 && (isNthBitSet(loop_rdata,63))){
      //Check for status bit in rdata
        executing_loop = false;
        jtag_loop_q.clear();
        return;
    }else{
        drive_cmd_loop_txn();
    }
  }

  void drive_cmd_loop_txn(){
    
    jtag_req_t jtag_req;
    unsigned jtag_cmd = 0;
    unsigned long  upper_jtag_data = 0;
    unsigned long lower_jtag_data = 0;
    unsigned reg_length_data = 0;
    unsigned hart = 0;
    
    jtag_req = jtag_loop_q[loop_idx];
    jtag_cmd = jtag_req.jtag_cmd;
    upper_jtag_data = jtag_req.jtag_ip_data_upper;
    lower_jtag_data = jtag_req.jtag_ip_data_lower;
    reg_length_data = jtag_req.jtag_length_data;
    
    cvm::log(cvm::HIGH, "[jtag_driver]: JTAG loop command {}\n",jtag_cmd);
    
    if(jtag_cmd<3){
      hart = 0; // hart bits position TBD, till TBD it is always zero
      trickboxJtagWrite(hart, jtag_cmd, upper_jtag_data, lower_jtag_data,reg_length_data);
    }else{
      cvm::log(cvm::ERROR, "[jtag_driver]: Unsupported keyword in jtag csv loop {}\n",jtag_cmd);
    }

    if(loop_idx<loop_size){
      loop_idx++;
    }else if(loop_idx == loop_size){
      loop_idx = 0;
    }
  } 
  void reset() override
  {
    cvm::log(cvm::HIGH, "[jtag_driver]: Reset jtag_driver\n");
    uint32_t rand_num = 0;
    if (FLAGS_random_jtag_entry)
    {
      cvm::log(cvm::HIGH, "[jtag_driver]: Enable random injection of debug mode :: {}\n", FLAGS_random_jtag_entry);
      get_all_csv_templates();
      if (FLAGS_jtag_delay_min)
      {
        rand_num = (rng() % (FLAGS_jtag_delay_max - FLAGS_jtag_delay_min + 1)) + FLAGS_jtag_delay_min;
      }
      timer_ = 0;
      file_idx = rng() % csvFilePaths.size();
      timer_rand_debug = timer_ + FLAGS_random_jtag_start_delay + (rand_num * timer_advance);
      cvm::log(cvm::HIGH, "Random Debug Injection of CSV file ID:{} Timer delay:{}\n", file_idx, timer_rand_debug);
    }
  }
  void parse_jtag_from_csv();
  void drive_csv_jtag_cmds();
  void get_all_csv_templates();

  struct jtag_data_t
  {
    unsigned hart;
    unsigned jtag_cmd;
    unsigned long upper_jtag_data;
    unsigned long lower_jtag_data;
    unsigned jtag_length_data;
  };


  struct jtag_req_t
  {
    unsigned jtag_cmd; 
    uint64_t jtag_ip_data_lower;
    uint64_t jtag_ip_data_upper;
    uint64_t jtag_op_data;
    unsigned jtag_length_data;
  };
  typedef struct{ 
        unsigned status;
        unsigned commands_in_queue;
  }jtag_status_t; 
  // Used to assert/deassert a trickbox interrupt (PIPI) for given hart.
  // virtual void trickboxjtagWrite(unsigned hart, unsigned upper_jtag_data, unsigned lower_jtag_data, cbs_t& cbs)
  virtual void trickboxJtagWrite(unsigned hart,unsigned jtag_cmd, unsigned long upper_jtag_data, unsigned long lower_jtag_data,unsigned reg_length_data)
  {
    cvm::log(cvm::HIGH, "TrickBox jtag Write to hart:{}, upper jtag data:{:#x}, lower jtag data:{:#x}, reg length data:{:#x}", hart, upper_jtag_data, lower_jtag_data,reg_length_data);
    // cbs.push_back(cb_t{Callback::TRICKBOX_jtag_WR, hart, upper_jtag_data, lower_jtag_data, 0});
    cvm::registry::messenger.signal(loc(), jtag_data_t{hart,jtag_cmd, upper_jtag_data, lower_jtag_data,reg_length_data});
    // cvm::messenger::send(jtag_t, jtag_pkt);
  }

  void update_jtag_status(jtag_req_t& i);

  void checkJtagEvents()
  {
    cvm::log(cvm::FULL, "Timer chk jtag evt \n");
    if (FLAGS_random_jtag_entry)
    {
      if (timer_ >= timer_rand_debug)
      {
        cvm::log(cvm::HIGH, "Timer passed random evt Value\n");
        rnd_jtag_trigger = 1;
        if (snippets_driven < (unsigned)FLAGS_jtag_max_snippets)
        {
          parse_jtag_from_csv();
          genNextJtagEvents();
          snippets_driven++;
        }
      }
    }
  }

  void genNextJtagEvents()
  {
    cvm::log(cvm::HIGH, "Generating Next timer evt value\n");
    if (FLAGS_random_jtag_entry)
    {
      int32_t rand_num = (rng() % (FLAGS_jtag_delay_max - FLAGS_jtag_delay_min + 1)) + FLAGS_jtag_delay_min;
      timer_rand_debug = timer_ + (rand_num * timer_advance);
      file_idx = rng() % csvFilePaths.size();
    }
  }

private:
  std::vector<uint32_t> soft_;              // Software interrupt: one per hart.
  std::vector<uint64_t> timeCompare_;       // One per interrupt type.
  std::vector<uint32_t> IntrHart_;          // Hart to be interrupted.
  std::vector<bool> delayedRandomIntValid_; // Valid bit for interrupt
  std::vector<bool> IntrValue_;             // Value of interrupt pin
  std::vector<bool> timerIntPrev_;          // Value of interrupt pin
  uint64_t timer_ = 0;
  uint64_t timer_advance = 200;
  uint64_t jtag_driver_base = 0x9050000;
  uint64_t jtag_driver_trigger = 0x9060000;
  uint64_t jtag_driver_status_addr = 0x9061000;
  uint64_t jtag_driver_num_cmds_addr = 0x9061000;
  uint32_t status;
  uint32_t commands_in_queue;
  
  bool      executing_nop = false;
  uint32_t  nop_count = 0; 
  
 // bool      expecting_check = false;

  bool      executing_loop = false;
  uint32_t  loop_size = 0; 
  uint32_t  loop_idx = 0; 
  uint32_t  loop_execution_cnt = 0; 
  //uint32_t  loop_check_bit_num = 0;
  //uint64_t  expected_check_value = 0x0;
  //bool      loop_check_bit_type = 0; //0->chk if bit is zero 1-> chk if bit is 1
  
  uint64_t loop_rdata;
  std::vector<std::vector<std::string>> csv_data;
  std::queue<jtag_req_t> jtag_cmd_q;
  std::vector<jtag_req_t> jtag_loop_q;
  std::queue<jtag_req_t> jtag_rsp_q;
  unsigned jtag_file_mode = 0;
  unsigned jtag_trigger = 0;
  unsigned rnd_jtag_trigger = 0;
  //unsigned step_ahead_queue_on = 0;
  //unsigned step_quit_queue_on = 0;
  //unsigned step_instr_cnt = 0;
  uint64_t timer_rand_debug = 500;
  std::vector<std::vector<std::string>> content;
  std::vector<std::string> row;
  // file(FLAGS_jtag_input_file_path);
  pcg_extras::seed_seq_from<std::random_device> seed_source;
  pcg32 rng;
  // Create a vector to store the file paths
  std::vector<std::string> filePaths;
  std::vector<std::string> csvFilePaths;
  unsigned file_idx = 0;
  unsigned snippets_driven = 0;
  unsigned num_ticks= 0;
};
