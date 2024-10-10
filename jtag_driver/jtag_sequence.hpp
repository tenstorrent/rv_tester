#pragma once

#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <map>
#include <random>
#include <cmath>
#include <fstream>
#include <string>
#include <queue>
#include <vector>
#include <filesystem>
#include <dirent.h>
#include <cstdlib>
#include <ctime>

#include <string>
#include <cstring>
#include <algorithm>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include "cvm/logger.hpp"
#include "cvm/random.hpp"
#include "svdpi.h"

#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"

#include "vpi_user.h"
#include "cvm/registry.hpp"
#include "cvm/logger.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/random.hpp"
#include "rv_tester_transactions.hpp"
#include "jtag_driver.hpp"
#include "transactor.h"
#include "svdpi.h"
#include "sysmod/sysmod_plusargs.h"

 DECLARE_bool(random_jtag_entry);
 DECLARE_int32(random_jtag_start_delay);
 DECLARE_int32(jtag_delay_min);
 DECLARE_int32(jtag_delay_max);
 DECLARE_int32(jtag_max_snippets);
class jtag_sequence {

  public:

    jtag_sequence(cvm::topology::loc_t loc, unsigned id);
    virtual ~jtag_sequence() ;

    void set_scope(svScope s) { scope_ = s; }

  virtual void jtag_tick(uint64_t advance) 
  {
    num_ticks++;
    cvm::log(cvm::DEBUG, "[jtag_sequence]: JTAG Tick {}\n",num_ticks);
    timer_ += advance;
    timer_advance = advance;
    if( num_ticks > 30) 
    {
    checkJtagEvents();

    if(executing_nop){
      nop_count--;
      cvm::log(cvm::HIGH, "[jtag_sequence]: Executing Nop ,Nop count {}\n",nop_count);
      if(nop_count==0)
        executing_nop = false;
    }else if(executing_loop){
      cvm::log(cvm::HIGH, "[jtag_sequence]: Executing loop \n");
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
 
 bool isNthBitClear(uint64_t number,int N) {
    // Shift the number right by 62 bits and check if the least significant bit is not set
    return !((number >> N) & 1);
 }

bool exitLoop() {
    if(loop_check_bit_type>0){
      cvm::log(cvm::HIGH, "[jtag_sequence]: loop_rdata {:#x},loop_check_bit_num {},loop_execution_cnt {}\n",loop_rdata,loop_check_bit_num,loop_execution_cnt);
      return isNthBitSet(loop_rdata,loop_check_bit_num);
    }else if(loop_check_bit_type == 0){
      cvm::log(cvm::HIGH, "[jtag_sequence]: loop_rdata {:#x},loop_check_bit_num {},loop_execution_cnt {}\n",loop_rdata,loop_check_bit_num,loop_execution_cnt);
      return isNthBitClear(loop_rdata,loop_check_bit_num);
    }else{
     cvm::log(cvm::HIGH, "[jtag_driver]: Wrong Exit loop condition detected \n");
     return false;
    }
    
 }
 void Run_cmd_loop()
  {
    if(loop_idx == 0 && loop_execution_cnt>0 && exitLoop()){
      //Check for status bit in rdata
        loop_execution_cnt = 0;
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
    
    cvm::log(cvm::HIGH, "[jtag_sequence]: JTAG loop command {}\n",jtag_cmd);
    
    if(jtag_cmd<3){
      hart = 0; // hart bits position TBD, till TBD it is always zero
      trickboxJtagWrite(hart, jtag_cmd, upper_jtag_data, lower_jtag_data,reg_length_data,0,tap_cfg_sel);
      if(loop_idx<loop_size){
        loop_idx++;
      } 
      if(loop_idx == loop_size){
        loop_idx = 0;
        loop_execution_cnt++;
        if(loop_execution_cnt > max_num_loops){
          cvm::log(cvm::ERROR, "[jtag_sequence]: Maximum number of polling attempts reached {}\n",loop_execution_cnt);
        }
      }
    }else{
      cvm::log(cvm::ERROR, "[jtag_sequence]: Unsupported keyword in jtag csv loop {}\n",jtag_cmd);
    }

    
  } 
  void reset() 
  {
    cvm::log(cvm::HIGH, "[jtag_sequence]: Reset jtag_sequence\n");
    uint32_t rand_num = 0;
    if (FLAGS_random_jtag_entry)
    {
      cvm::log(cvm::HIGH, "[jtag_sequence]: Enable random injection of debug mode :: {}\n", FLAGS_random_jtag_entry);
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
  void setNonBlocking(int socket);
  std::string process_string(const std::string& input);
  cvm::messenger::task<void> open_socket_to_listen();

  std::string get_local_ip_address();

  // std::string getLocalIPAddress();
  
  struct jtag_data_t
  {
    unsigned hart;
    unsigned jtag_cmd;
    unsigned long upper_jtag_data;
    unsigned long lower_jtag_data;
    unsigned jtag_length_data;
    unsigned jtag_quit;
    unsigned tap_cfg_sel;
  };


  struct jtag_req_t
  {
    unsigned jtag_cmd; 
    uint64_t jtag_ip_data_lower;
    uint64_t jtag_ip_data_upper;
    uint64_t jtag_op_data;
    uint64_t jtag_cm_value;
    unsigned jtag_length_data;
    unsigned long ip_data_array[21];
  };
  typedef struct{ 
        unsigned status;
        unsigned commands_in_queue;
  }jtag_status_t; 


  void update_jtag_status(jtag_req_t& i);
  cvm::messenger::task<void> jtag_tick();

  void checkJtagEvents()
  {
    cvm::log(cvm::DEBUG, "Timer chk jtag evt \n");
    if (FLAGS_random_jtag_entry)
    {
      if (timer_ >= timer_rand_debug && csv_completed )
      {
        cvm::log(cvm::DEBUG, "Timer passed random evt Value\n");
        rnd_jtag_trigger = 1;
        csv_completed = 0;
        if (snippets_driven < (unsigned)FLAGS_jtag_max_snippets)
        {
          parse_jtag_from_csv();
          genNextJtagEvents();
          snippets_driven++;
        }else{
          cvm::log(cvm::HIGH, "[JTAGDRIVER] ******************* \n");
          cvm::log(cvm::HIGH, "[JTAGDRIVER] Sending Quit signal \n");
          cvm::log(cvm::HIGH, "[JTAGDRIVER] ******************* \n");
          trickboxJtagWrite(0, 7, 0, 0,0,1,tap_cfg_sel);
          //arg1 hart = 0, arg2 jtag_cmd = 7(qt)
        }
      }
    }
  }

  void genNextJtagEvents()
  {
    cvm::log(cvm::HIGH, "[jtag_sequence]Generating Next timer evt value\n");
    if (FLAGS_random_jtag_entry)
    {
      int32_t rand_num = (rng() % (FLAGS_jtag_delay_max - FLAGS_jtag_delay_min + 1)) + FLAGS_jtag_delay_min;
      timer_rand_debug = timer_ + (rand_num * timer_advance);
      file_idx = rng() % csvFilePaths.size();
    }
  }
template <std::size_t N>
std::bitset<N> reverseLowerBits(const std::bitset<N>& bs, std::size_t split_length) {
    // Ensure split_length does not exceed the bitset size N
    int max_sib_length = 10;
    split_length = std::min((split_length+max_sib_length), N);
    
     cvm::log(cvm::FULL, "[jtag_sequence] split_length {}\n",split_length);
    // Create a new bitset to store the reversed bits
    std::bitset<N> reversed;
    
    // Reverse the lower 'split_length' bits
    for (std::size_t i = 0; i < split_length; ++i) {
        reversed[i] = bs[split_length - 1 - i];
     cvm::log(cvm::FULL, "[jtag_sequence] reversed[{}] = {}\n",i,reversed[i]);
    }

    return reversed;
}
  std::vector<uint64_t> bitsetToUint64Array(const std::bitset<1344>& bitset) {
    const size_t bitsetSize = 1344;//64;//70;
    const size_t ulongSize = sizeof(uint64_t) * 8;
    const size_t arraySize = (bitsetSize + ulongSize - 1) / ulongSize;

    //std::bitset<70> bitset_shifted = bitset>>2;
    std::bitset<1344> bitset_shifted = bitset;

    //jtag rx -> jtag.op_Data , we are shifting only by 2 since from jtag_xtor for each tap point we shift accordingly but all of them are shifted by 2
    //std::cout<<"[JTAG RESP] original = " <<bitset<<" shifted = "<<bitset_shifted<<"\n";
    std::vector<uint64_t> ulongArray(arraySize);

    for (size_t i = 0; i < bitsetSize; i += ulongSize) {
        size_t ulongIndex = i / ulongSize;
        uint64_t value = 0;

        for (size_t j = 0; j < ulongSize && (i + j) < bitsetSize; ++j) {
            value |= (bitset_shifted[i + j] ? 1UL : 0UL) << j;
        }

        ulongArray[ulongIndex] = value;
    }

    return ulongArray;
}

  private:

    void csv_mode_thread();
    void socket_mode_thread();

    cvm::messenger::task<void> random_mode();
   
    cvm::messenger::task<void> tick();
    cvm::messenger::task<void> trigger();
    cvm::messenger::task<void> resp();
    virtual void trickboxJtagWrite(unsigned hart,unsigned jtag_cmd, unsigned long upper_jtag_data, unsigned long lower_jtag_data,unsigned reg_length_data,unsigned jtag_quit, unsigned tap_cfg_sel);
    virtual void trickboxJtagWriteSocket(unsigned hart,unsigned jtag_cmd,  unsigned long* lower_jtag_data,unsigned reg_length_data,unsigned jtag_quit, unsigned tap_cfg_sel);
    //void jtag_resp(std::bitset<70> rdata);
    void jtag_resp(std::bitset<1344> rdata);
    void init();
    void jtag_socket(unsigned hart, uint8_t assert);
    void drive_jtag_cmds();
    void process_input_string(std::string line);
    std::string formatHexWithPadding(uint64_t hexNumber, int n);
    std::string bitset_to_hex(const std::bitset<1344>& bits, int n);
    std::bitset<1344> hexStringToBitset( std::string& hex);

  private:

    cvm::topology::loc_t loc_;
    unsigned id_;
    svScope scope_;

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
  uint8_t  csv_completed = 1;
  bool execute_qt = false;
  uint32_t status;
  uint32_t commands_in_queue;
  cvm::rand::uniform_dist<int64_t> rng;
  bool      executing_nop = false;
  uint32_t  nop_count = 0; 
  
  //bool      expecting_check = false;

  bool      executing_loop = false;
  uint32_t  loop_size = 0; 
  uint32_t  loop_idx = 0; 
  uint32_t  loop_execution_cnt = 0; 
  uint32_t  max_num_loops = 0; 
  uint32_t  loop_check_bit_num = 0;
  //uint64_t  expected_check_value = 0x0;
  bool      loop_check_bit_type = 0; //0->chk if bit is zero 1-> chk if bit is 1
  
  uint64_t loop_rdata;
  std::vector<uint64_t> rdata_Array;
  std::bitset<1344> jtag_rdata;
  std::bitset<1344> jtag_reversed_rdata;
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
  int padding_length = 64;
  std::vector<std::vector<std::string>> content;
  std::vector<std::string> row;
  // file(FLAGS_jtag_input_file_path);
  // Create a vector to store the file paths
  std::vector<std::string> filePaths;
  std::vector<std::string> csvFilePaths;
  unsigned file_idx = 0;
  unsigned snippets_driven = 0;
  unsigned num_ticks= 0;
  unsigned tap_cfg_sel= 0;
};
