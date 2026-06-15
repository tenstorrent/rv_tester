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
#include "common/pcg_random.hpp"
#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/registry.hpp"
#include "src/sysmod/trickbox/subdevice.h"
#include "vpi_user.h"
// #include "rv_tester_transactions.hpp"
//#include "dm_model/dm_model.hpp"

// DEFINE_string(dbg_input_file_path, "", "Path to file containing debugger commands");
DECLARE_string(dbg_input_file_path);
DECLARE_bool(random_dbg_entry);
DECLARE_int32(random_dbg_start_delay);
DECLARE_int32(dbg_delay_min);
DECLARE_int32(dbg_delay_max);
DECLARE_int32(dbg_max_snippets);
DECLARE_string(dbg_template_dir_path);
DECLARE_bool(enable_cross);
DECLARE_bool(dry_space_access);

// Define a core local  (debugger) at the given address
// and for the given hart count. The size will be 48k bytes.
class debugger : public subdevice
{
public:
  /// Define a debugger device at the given address for the given hart count.
  /// Range of addresses reserved is: [addr, addr + 0xbfff]
  debugger(const std::string &tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc);

  // Destructor.
  virtual ~debugger();

  void configure() override;

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
  /// No-op if address is outside the range of this debugger or if
  /// address is not properly aligned.
  cvm::messenger::task<void> read(uint64_t addr, size_t length, data_t &data);
  void read_dev(uint64_t addr, size_t length,  data_t& data) override;
  // Write to this debugger.
  virtual void write(uint64_t addr, size_t length, const data_t &data,
                     const strb_t &strb) override;

  virtual void tick(uint64_t advance) override
  {
    std::lock_guard<std::mutex> lock(mutex_);
    timer_ += advance;
    timer_advance = advance;
    cvm::log(cvm::FULL, "[Debugger]: Tick, timer:{} advance:{}\n",timer_,advance);
    if(timer_ > (5*advance)){ 
    checkDebugEvents();
    if (cmd_exc_trig_rcv){ 
      unsigned upper_dmi_data = 0;
      unsigned lower_dmi_data = 0;
      unsigned hart = 0;
      upper_dmi_data = t_data_buf >> 32;
      lower_dmi_data = t_data_buf & 0xffffffff;
      hart = 0;                                               // hart bits position TBD, till TBD it is always zero
      trickboxDmiWrite(hart, upper_dmi_data, lower_dmi_data); // Commented until DMI PORT is not in master
      cmd_exc_trig_rcv = false;
      t_data_buf = 0;
    }
    else{
      drive_csv_dmi_cmds();
    }
    }
  }

  virtual void is_dut_reset_req(bool dut_reset_req_f,uint64_t iclocks,uint64_t idivisor) override
  {
    dut_reset_req = dut_reset_req_f;
    clocks = iclocks;
    divisor = idivisor;
    if(dut_reset_req){
      ndm_reset_occured = true;
      std::ofstream myfile;
      myfile.open ("reset_state.txt", std::ios_base::app);
      cvm::log(cvm::HIGH, "[Debugger]:Debugger is_dut_reset_req Attempting to write the State in Debugger: dut_reset_req: {} clocks: {} divisor {} \n",dut_reset_req,clocks,divisor);
      cvm::log(cvm::HIGH, "[Debugger]:State written to Debugger : Ndm-Reset\n");
      myfile << "Ndm-Reset\n";
      myfile.flush();
      myfile.close();
    }
    cvm::log(cvm::HIGH, "[Debugger]: Reset_req: {} ndm_reset_occured: {} clocks: {}\n",dut_reset_req,ndm_reset_occured,clocks);

  }

  void reset() override
  {
    cvm::log(cvm::HIGH, "[Debugger]: Reset debugger\n");
    cvm::log(cvm::HIGH, "[Debugger]: Reset_req: {} ndm_reset_occured: {} clocks: {}\n",dut_reset_req,ndm_reset_occured,clocks);

    std::ofstream myfile;
    myfile.open ("reset_state.txt", std::ios_base::app);
    cvm::log(cvm::HIGH, "[Debugger]:Debugger destructor Attempting to write the State in Debugger: dut_reset_req: {} clocks: {} divisor {} \n",dut_reset_req,clocks,divisor);
    if (dut_reset_req){
      cvm::log(cvm::HIGH, "[Debugger]:State written to Debugger : Ndm-Reset\n");
      myfile << "Ndm-Reset\n";
    }
    myfile.flush();
    myfile.close();

    std::ifstream myfile1;
    myfile1.open ("reset_state.txt");
    if(myfile1.is_open()) {
      // fin.seekg(-1,ios_base::end);                // go to one spot before the EOF
      // fin.readline();
      std::string line;
      std::getline(myfile1, line);
      cvm::log(cvm::LOW, "[Debugger]:Reset State in Debugger is: {} at clocks {} divisor {}\n", line,clocks,divisor);
      if (line == "Ndm-Reset") {
        snippets_driven = FLAGS_dbg_max_snippets;
        ndm_reset_occured = true;
      }
      
    }
    myfile1.close();

    // dbg_snippets_name = "";
    // ndm_reset_occured = 0;
    uint32_t rand_num = 0;
    if (FLAGS_random_dbg_entry & !ndm_reset_occured)
    {
      cvm::log(cvm::HIGH, "[Debugger]: Enable random injection of debug mode :: {}\n", FLAGS_random_dbg_entry);
      get_all_csv_templates();
      if (FLAGS_dbg_delay_min)
      {
        rand_num = (rng() % (FLAGS_dbg_delay_max - FLAGS_dbg_delay_min + 1)) + FLAGS_dbg_delay_min;
      }
      timer_ = 0;
      file_idx = rng() % csvFilePaths.size();
      timer_rand_debug = timer_ + FLAGS_random_dbg_start_delay + (rand_num * timer_advance);
      // cmd_trigger_rand_debug = timer_ + 50*FLAGS_random_dbg_start_delay + (rand_num * timer_advance); 
      cvm::log(cvm::HIGH, "[Debugger]:Random Debug Injection of CSV file ID:{} Timer delay:{}\n", file_idx, timer_rand_debug);
      // cvm::log(cvm::HIGH, "Command Execution Trigger Timer delay:{}\n", cmd_trigger_rand_debug);
    }
  }
  void parse_dmi_from_csv();
  void drive_csv_dmi_cmds();
  void get_all_csv_templates();

  struct dmi_data_t
  {
    unsigned hart;
    unsigned upper_dmi_data;
    unsigned lower_dmi_data;
  };

  struct dmi_req_t
  {
    unsigned func_bits;
    unsigned addr;
    unsigned op;
    unsigned data;
  };
  typedef struct{ 
    unsigned status;
    unsigned commands_in_queue;
    unsigned warm_reset;
    unsigned debug_hold;
  }dmi_status_t; 
  // Used to assert/deassert a trickbox interrupt (PIPI) for given hart.
  // virtual void trickboxDmiWrite(unsigned hart, unsigned upper_dmi_data, unsigned lower_dmi_data, cbs_t& cbs)
  virtual void trickboxDmiWrite(unsigned hart, unsigned upper_dmi_data, unsigned lower_dmi_data)
  {
    cvm::log(cvm::HIGH, "TrickBox DMI Write to hart:{}, upper dmi data:{}, lower dmi data:{}", hart, upper_dmi_data, lower_dmi_data);
    // cbs.push_back(cb_t{Callback::TRICKBOX_DMI_WR, hart, upper_dmi_data, lower_dmi_data, 0});
    cvm::registry::messenger.signal(loc(), dmi_data_t{hart, upper_dmi_data, lower_dmi_data});
    // cvm::messenger::send(dmi_t, dmi_pkt);
  }

  void update_dm_status(dmi_status_t& i);

  void checkDebugEvents()
  {
    if (FLAGS_random_dbg_entry)
    {
      if ((timer_ >= timer_rand_debug) & (checkpoint_triggers_pending == 0) & (cmd_trigger_in_progress == false) & (file_parsing_in_progress == false))
      { 
        if (snippets_driven < (unsigned)FLAGS_dbg_max_snippets)
        {
          cvm::log(cvm::HIGH, "[Debugger]:Rand Debug Timer passed, parsing the CSV snippet\n");
          parse_dmi_from_csv();
          snippets_driven++;
          file_parsing_in_progress = true;
        }

        if (file_parsing_done & file_parsing_in_progress){
          cvm::log(cvm::HIGH, "[Debugger]:CSV snippet parsing done, generating CmdTrigger event\n");
          genNextCmdTriggerEvents();
          file_parsing_in_progress = false;
          cmd_trigger_in_progress = true;
        }
      }

      if ((timer_ >= cmd_trigger_rand_debug) & (checkpoint_triggers_pending>=0) & dmi_cmd_q.empty() & file_parsing_done & cmd_trigger_in_progress) 
      {
        if (checkpoint_triggers_pending == 0)
          genNextDebugEvents();
        
        else {
          rand_dbg_entry_cmd_trigger = 1;
          cvm::log(cvm::HIGH, "[Debugger]:Timer passed random evt Value to provide cmd trigger\n");
          checkpoint_triggers_pending -= 1;
          if (checkpoint_triggers_pending>0)
            genNextCmdTriggerEvents();
        }

      }
    }
  }

  void genNextCmdTriggerEvents()
  {
    int32_t rand_num = (rng() % (FLAGS_dbg_delay_max - FLAGS_dbg_delay_min + 1)) + 5*FLAGS_dbg_delay_min;
    cmd_trigger_rand_debug = timer_ + (rand_num * timer_advance);
    cvm::log(cvm::HIGH, "[Debugger]:Next Command Execution Trigger Timer delay:{}\n", cmd_trigger_rand_debug);
  }

  void genNextDebugEvents()
  {    
    if (FLAGS_random_dbg_entry)
    {
      int32_t rand_num = (rng() % (FLAGS_dbg_delay_max - FLAGS_dbg_delay_min + 1)) + FLAGS_dbg_delay_min;
      timer_rand_debug = timer_ + (rand_num * timer_advance);
      file_idx = rng() % csvFilePaths.size();
      cvm::log(cvm::HIGH, "[Debugger]:Next Random Debug Injection of CSV file ID:{} Timer delay:{}\n", file_idx, timer_rand_debug);

      cmd_trigger_in_progress = false;
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
  uint64_t debugger_command_exec_trigger = 0x9050000;
  uint64_t debugger_file_load_trigger = 0x9060000;
  uint64_t dmi_driver_status_addr = 0x9061000;
  uint64_t dmi_driver_num_cmds_addr = 0x9061000;
  uint64_t dmi_driver_warm_reset_addr = 0x9061000;
  uint32_t status;
  uint32_t commands_in_queue;
  uint32_t warm_reset;
  uint64_t checkpoint_triggers_pending = 0;
  uint64_t cmd_trigger_rand_debug = 3000;
  uint32_t rand_dbg_entry_cmd_trigger = 0;
  uint32_t file_parsing_done = 0; 
  uint64_t t_data_buf = 0;
  std::atomic<bool> terminate_ = false;
  std::mutex mutex_;

  std::vector<std::vector<std::string>> csv_data;
  std::queue<dmi_req_t> dmi_cmd_q;
  std::queue<dmi_req_t> dmi_rsp_q;
  unsigned step_ahead_queue_on = 0;
  unsigned step_quit_queue_on = 0;
  unsigned step_instr_cnt = 0;
  unsigned sdtrig_halt_queue_on = 0;
  unsigned sdtrig_cause_queue_on = 0;
  unsigned sdtrig_disable_queue_on = 0;
  unsigned sdtrig_progbuf_queue_on = 0;

  uint64_t timer_rand_debug = 500;
  std::vector<std::vector<std::string>> content;
  std::vector<std::string> row;
  pcg_extras::seed_seq_from<std::random_device> seed_source;
  pcg32 rng;
  // Create a vector to store the file paths
  std::vector<std::string> filePaths;
  std::vector<std::string> csvFilePaths;
  unsigned file_idx = 0;
  bool file_loading_done = false;
  unsigned snippets_driven = 0;
  bool file_parsing_in_progress = false;
  bool cmd_trigger_in_progress = false;
  std::string dbg_snippets_name = "";
  bool dut_reset_req = false;
  bool ndm_reset_occured = false;
  bool cmd_exc_trig_rcv = false;
  uint64_t clocks=0,divisor=0;
};
