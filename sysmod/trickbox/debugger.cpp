#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/logger.hpp"
#include "debugger.h"
#include "sysmod/sysmod_plusargs.h"
#include "rv_tester/rv_tester_plusargs.h"
#include <fstream>

DEFINE_string(dbg_input_file_path, "", "Path to file containing debugger commands");
DEFINE_bool(random_dbg_entry, false, "Enter debug mode randomly after random intervals");
DEFINE_int32(random_dbg_start_delay, 300, "delay after which random interrupts should start");
DEFINE_int32(dbg_delay_min, 6, "Minimum Delay between 2 consecutive debug mode requests");
DEFINE_int32(dbg_delay_max, 9, "Maximum Delay between 2 consecutive debug mopde requests");
DEFINE_int32(dbg_max_snippets, 1, "Maximum number of debug snippets to be driven");
DEFINE_string(dbg_template_dir_path, "", "Path to file containing debugger commands");
DEFINE_bool(enable_cross, false, "Are cross features are enabled");
DEFINE_bool(dbg_rand_core, false, "To randomize the core-id to which the core the DM snippet is targetted to");
DEFINE_int32(dbg_rand_core_idx, 0, "Random Core idx to which the DM commands are targetted");
DEFINE_bool(dry_space_access, false, "Dry space access guarding");
DEFINE_bool(dm_model_check_bypass, false, "Bypass the DM Model checks");
 // FIXME: pwrmmgmt has been moved out
DEFINE_string(warm_reset_debug_hold, "0:1", "Debug hold");
DEFINE_string(warm_reset, "off", "Enable warm resets in the sim - off/random/trigger");
debugger::debugger(const std::string &tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc)
    : subdevice(tag, addr, 0x20000 /* size */, loc), soft_(hartCount),
      timeCompare_(6), IntrHart_(6), delayedRandomIntValid_(6), IntrValue_(6), timerIntPrev_(hartCount), timer_(0)
{
  rng.seed(FLAGS_seed);
  debugger_command_exec_trigger = addr;
  debugger_file_load_trigger = addr + 0x10000;
  dmi_driver_status_addr = addr + 0x500;
  dmi_driver_num_cmds_addr = addr + 0x600;
  dmi_driver_warm_reset_addr = addr + 0x700;
  reset();

  auto tbox_loc = cvm::topology::get_from_type("TRICKBOX", 0);
  cvm::registry::messenger.connect<debugger::dmi_status_t>(
            tbox_loc,
            [&](debugger::dmi_status_t i) { return this->update_dm_status(i); });

  std::ifstream myfile;
  cvm::log(cvm::HIGH, "[Debugger]:Constructor: read  reset state in Debugger at clocks {} divisor {}\n", clocks,divisor);
  myfile.open ("reset_state.txt");
  if(myfile.is_open()) {
    // fin.seekg(-1,ios_base::end);                // go to one spot before the EOF
    // fin.readline();
    std::string line;
    std::getline(myfile, line);
    cvm::log(cvm::MEDIUM, "[Debugger]:Reset State in Debugger is: {} at clocks {} divisor {}\n", line,clocks,divisor);
    if (line == "Ndm-Reset") {
      snippets_driven = FLAGS_dbg_max_snippets; 
      ndm_reset_occured = true;
      FLAGS_warm_reset_debug_hold = "1:1";
    }
    
  }
  myfile.close();
    cvm::log(cvm::HIGH, "[Debugger]: Reset_req: {} ndm_reset_occured: {} clocks: {}\n",dut_reset_req,ndm_reset_occured,clocks);
}

debugger::~debugger()
{
  if (FLAGS_metrics) {
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"dm_rand_snippets_mode\": \"{}\"}}\n", FLAGS_random_dbg_entry);
    if (FLAGS_random_dbg_entry) {
      cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"dm_rand_snippets_max_count\": \"{}\"}}\n", FLAGS_dbg_max_snippets);
      cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"dm_rand_snippets_delay\": \"{}_{}\"}}\n", FLAGS_dbg_delay_min, FLAGS_dbg_delay_max);
    }
    cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"dm_rand_snippets_name\": \"{}\"}}\n", dbg_snippets_name);
  }
}
void
debugger::update_dm_status(debugger::dmi_status_t& i) {
  cvm::log(cvm::HIGH, "[Debugger]:Debug module status :{:#x} cmds in queue :{:#x} warm reset :{:#x}\n", i.status,i.commands_in_queue,i.warm_reset);
  status = i.status;
  commands_in_queue = i.commands_in_queue;
  warm_reset = i.warm_reset;
}

void debugger::get_all_csv_templates()
{
    std::string directoryPath = FLAGS_dbg_template_dir_path;
    cvm::log(cvm::NONE, "[Debugger]:Debug commands directory:{}\n", directoryPath);

    if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath))
    {
        throw std::invalid_argument("Invalid directory path");
    }

    for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
    {
        if (entry.is_regular_file())
        {
            std::string filename = entry.path().filename().string();
            if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".csv")
            {
              if (FLAGS_enable_cross && (filename.size() > 14) && filename.substr(filename.size() - 14) == "scratchpad.csv") {
                cvm::log(cvm::NONE, "[Debugger]:Skipping Scratchpad file in cross:{}\n", filename); 
              }
              else if (FLAGS_time_mtime_sync_enable && filename.size() > 13 && filename.substr(filename.size() - 13) == "stopcount.csv") {
                cvm::log(cvm::NONE, "[Debugger]:Skipping Stopcount file in time_mtime_sync:{}\n", filename); 
              }
              else {
                csvFilePaths.push_back(entry.path().string());
                cvm::log(cvm::MEDIUM, "[Debugger]:Pushing file:{}\n", filename);
              }
            }
        }
    }
    std::sort(csvFilePaths.begin(), csvFilePaths.end());
}

void debugger::parse_dmi_from_csv()
{

  std::string file_name, file_csv_name;
  if (FLAGS_random_dbg_entry) {
    file_name = csvFilePaths[file_idx];
    file_csv_name = file_name.substr(file_name.find_last_of('/') + 1, file_name.size() - file_name.find_last_of('/') - 5);
    dbg_snippets_name.append(file_csv_name); 
  }
  else {
    file_name = FLAGS_dbg_input_file_path;
    file_csv_name = file_name.substr(file_name.find_last_of('/') + 1, file_name.size() - file_name.find_last_of('/') - 5);
    dbg_snippets_name.append(file_csv_name);
  }
  std::string word = "ndm";
  if (file_name.find(word) != std::string::npos) {
        cvm::log(cvm::LOW,  "The csv file name contains the word 'ndm' hence disabling random dbg entry.\n");
        FLAGS_dm_model_check_bypass = true;
        FLAGS_warm_reset = "trigger"; //Set the Warm reset mode as Trigger
        snippets_driven = FLAGS_dbg_max_snippets; //Make snippets_driven as max_snippets to not pick any more
        FLAGS_warm_reset_debug_hold = "1:1"; //Pass the warm_reset_debug_hold to 1
        cvm::log(cvm::LOW,  "The FLAGS_warm_reset_debug_hold 'ndm' hence disabling random dbg entry. is {}\n", FLAGS_warm_reset_debug_hold);
  }
  cvm::log(cvm::NONE, "[Debugger]:Parse DMI Commands from CSV:{}\n", file_name);
  FLAGS_dbg_rand_core_idx = rng() % FLAGS_num_harts; //FIXME
  cvm::log(cvm::NONE, "[Debugger]:DMI Requests core randomization state: {}, Num-harts:{:#x}, Core-ID:{:#x}\n", FLAGS_dbg_rand_core, FLAGS_num_harts, FLAGS_dbg_rand_core?FLAGS_dbg_rand_core_idx:0);
  std::fstream file(file_name, std::ios::in);
  if (file.is_open())
  {
    file_parsing_done = 0;
    std::string line, word;
    while (getline(file, line))
    {
      row.clear();

      std::stringstream str(line);

      while (getline(str, word, ','))
      {
        row.push_back(word);
      }

      dmi_req_t dmi_req;
      dmi_req.op = 0;
      dmi_req.addr = 0;
      dmi_req.data = 0;
      dmi_req.func_bits = 0;
      std::string instr;
      std::string instr_2char;
      instr = row[0];
      // remove empty spaces from string
      instr.erase(std::remove_if(instr.begin(), instr.end(), ::isspace), instr.end());
      // convert string to lowercase for uniformity
      std::transform(instr.begin(), instr.end(), instr.begin(), ::tolower);
      instr_2char = instr.substr(0, 2);
      if (instr_2char == "rd")
      {
        dmi_req.op = 1;
      }
      else if (instr_2char == "wr")
      {
        dmi_req.op = 2;
      }
      else if (instr_2char == "//")
      {
        continue; // skip line may be comment
      }
      else if (instr_2char == "cp")
      {
        // checkpoint
        dmi_req.op = 3;
      }
      else if (instr_2char == "cs")
      {
        //custom op's
        dmi_req.op = 4;
      }
      else if (instr_2char == "sd")
      {
        // checkpoint
        if (instr == "sdtrig_halt_queue_on")
        {
          sdtrig_halt_queue_on = 1;
        }
        if (instr == "sdtrig_halt_queue_off")
        {
          sdtrig_halt_queue_on = 0;
        }
        if (instr == "sdtrig_cause_queue_on")
        {
          sdtrig_cause_queue_on = 1;
        }
        if (instr == "sdtrig_cause_queue_off")
        {
          sdtrig_cause_queue_on = 0;
        }
        if (instr == "sdtrig_disable_queue_on")
        {
          sdtrig_disable_queue_on = 1;
        }
        if (instr == "sdtrig_disable_queue_off")
        {
          sdtrig_disable_queue_on = 0;
        }
        if (instr == "sdtrig_progbuf_queue_on")
        {
          sdtrig_progbuf_queue_on = 1;
        }
        if (instr == "sdtrig_progbuf_queue_off")
        {
          sdtrig_progbuf_queue_on = 0;
        }
      }
      else if (instr_2char == "st")
      {
        // step ahead/back q
        if (instr == "step_ahead_queue_on")
        {
          step_ahead_queue_on = 1;
        }
        if (instr == "step_ahead_queue_off")
        {
          step_ahead_queue_on = 0;
        }
        if (instr == "step_quit_queue_on")
        {
          step_quit_queue_on = 1;
        }
        if (instr == "step_quit_queue_off")
        {
          step_quit_queue_on = 0;
        }
        if (instr == "step_instr_cnt")
        {
          step_instr_cnt = std::stoul(row[1], nullptr, 16);
          // will continue loop with proper dmi write
          dmi_req.func_bits = 1;
          dmi_req.data = step_instr_cnt;
          content.push_back(row);
          dmi_cmd_q.push(dmi_req);
          continue;
        }
      }
      else
      {
        // invalid command seen in the csv file
        cvm::log(cvm::ERROR, "Error: [Trickbox] Invalid command in csv file {}\n", instr);
      }

      // Check commands to push to specific queues
      if (step_ahead_queue_on)
      {
        dmi_req.func_bits = 2;
      }
      if (step_quit_queue_on)
      {
        dmi_req.func_bits = 4;
      }
      if (sdtrig_halt_queue_on)
      {
        dmi_req.func_bits = 3;
      }
      if (sdtrig_cause_queue_on)
      {
        dmi_req.func_bits = 5;
      }
      if (sdtrig_disable_queue_on)
      {
        dmi_req.func_bits = 7;
      }
      if (sdtrig_progbuf_queue_on)
      {
        dmi_req.func_bits = 6;
      }

      if ((dmi_req.op != 4)&&(dmi_req.op != 3)&&(dmi_req.op != 0))
      {
        // remove underscores from addr
        row[1].erase(std::remove(row[1].begin(), row[1].end(), '_'), row[1].end());
        try
        {
          dmi_req.addr = std::stoul(row[1], nullptr, 16);
        }
        catch (const std::invalid_argument &e)
        {
          cvm::log(cvm::ERROR, "Error: [Trickbox] Invalid argument: addr for stoul csv arg 1: {}\n", e.what());
        }
      }

      if (dmi_req.op == 2)
      {
        cvm::log(cvm::NONE, "[Debugger]: Write op for addr : {:#x}\n", dmi_req.addr);
        // remove underscores from data
        row[2].erase(std::remove(row[2].begin(), row[2].end(), '_'), row[2].end());
        try
        {
          dmi_req.data = std::stoul(row[2], nullptr, 16);
          if (FLAGS_dbg_rand_core) {
            if (dmi_req.addr == 0x10){
              cvm::log(cvm::NONE, "[Debugger]: Randomizing the core ID to be : {:#x}\n", FLAGS_dbg_rand_core_idx);
              dmi_req.data = dmi_req.data + (FLAGS_dbg_rand_core_idx << 16);
            }
          }
        }
        catch (const std::invalid_argument &e)
        {
          cvm::log(cvm::ERROR, "Error: [Trickbox] Invalid argument: data for stoul csv arg 2: {}\n", e.what());
        }
      }
      
      if (dmi_req.op == 4)
      {
        if (instr == "cs_csr_rand_read")
        {
          // Select a random address from the combined list
          std::vector<int> addresses =  { 
            0xF11, 0xF12, 0xF13, 0xF14, // Machine Information Registers
            0x300, 0x301, 0x302, 0x303, 0x304, 0x305, 0x306, 0x310, 0x312 // Machine Trap Setup
            }; // FIXME: add all csr's from core to be randomized
          int rand_address = addresses[rng() % addresses.size()];

          dmi_req.op = 2;
          dmi_req.addr = 0x17;
          dmi_req.data = 0x00320000 + rand_address;
          cvm::log(cvm::NONE, "[Debugger]: Read core_csr address : {:#x}\n", rand_address);
        }
      }
      
      if (dmi_req.op != 0) {
        content.push_back(row);
        dmi_cmd_q.push(dmi_req);
        // PRINT CSV DATA
        cvm::log(cvm::MEDIUM, "[Debugger]:Pushing dmi request: op {} addr {:#x} data {:#x}\n", dmi_req.op, dmi_req.addr, dmi_req.data);
      }
    }

    // Add a dummy check-point to ensure all DMI commands part of the previous trigger is executed
    if (FLAGS_random_dbg_entry) {
      dmi_req_t dmi_req;
      dmi_req.op = 3;
      dmi_req.addr = 0;
      dmi_req.data = 0;
      dmi_req.func_bits = 0;
      dmi_cmd_q.push(dmi_req);
    }

    file_parsing_done = 1;
  }
  else
  {
    cvm::log(cvm::ERROR, "Error: [Trickbox] Could not open dmi cfg file {}\n", FLAGS_dbg_input_file_path);
  }
}

void debugger::drive_csv_dmi_cmds()
{
  if (rand_dbg_entry_cmd_trigger) {
    cvm::log(cvm::HIGH, "[Debugger]: Driving Cmd Trigger, Number of checkpoints pending is {}\n", checkpoint_triggers_pending); 
    unsigned long t_data = 0x800000000fffffff;
    unsigned upper_dmi_data = 0;
    unsigned lower_dmi_data = 0;
    unsigned hart = 0;
    upper_dmi_data = t_data >> 32;
    lower_dmi_data = t_data & 0xffffffff;
    hart = 0;                                               // hart bits position TBD, till TBD it is always zero
    trickboxDmiWrite(hart, upper_dmi_data, lower_dmi_data); // Commented until DMI PORT is not in master
    rand_dbg_entry_cmd_trigger = 0; 
  }
  
  else if (!dmi_cmd_q.empty())
  {
    dmi_req_t dmi_req;
    dmi_req = dmi_cmd_q.front();
    dmi_cmd_q.pop(); // pop front element
    if (dmi_req.op == 3) {
      checkpoint_triggers_pending += 1;
      cvm::log(cvm::HIGH, "[Debugger]: Encountered checkpoint in csv parsing, Number of checkpoints pending is {}\n", checkpoint_triggers_pending);  
    } 
    cvm::log(cvm::MEDIUM, "[Debugger]:Popping dmi request: op {} addr {:#x} data {:#x} func bits {:#x}\n", dmi_req.op, dmi_req.addr, dmi_req.data, dmi_req.func_bits);
    unsigned upper_dmi_data = 0;
    unsigned lower_dmi_data = 0;
    unsigned hart = 0;
    upper_dmi_data = (dmi_req.func_bits << 27) | (dmi_req.addr << 2) | dmi_req.op;
    lower_dmi_data = dmi_req.data;
    hart = 0; // hart bits position TBD, till TBD it is always zero
    trickboxDmiWrite(hart, upper_dmi_data, lower_dmi_data);
  }
}

cvm::messenger::task<void>
debugger::read(uint64_t addr, size_t , data_t& data)
{
   if (not has_addr(addr)){
    cvm::log(cvm::HIGH, "[DEBUGGER] Descarding read request at uc_helper since tag {} is not matching \n",tag());
   co_return;
  }
  if (addr == dmi_driver_status_addr)
  {
    cvm::log(cvm::MEDIUM, "[DEBUGGER] Debugger status : {:#x}\n",status);
    data[0] = status;
  }
  if (addr == dmi_driver_num_cmds_addr)
  {
    cvm::log(cvm::MEDIUM, "[DEBUGGER] Debugger num cmds in queue : {:#x}\n",commands_in_queue);
    data[0] = commands_in_queue;
  } 
  if (addr == dmi_driver_warm_reset_addr)
  {
    cvm::log(cvm::MEDIUM, "[DEBUGGER] Debugger warm reset : {:#x}\n",warm_reset);
    data[0] = warm_reset;
  }
  co_return;
}
void debugger::read_dev(uint64_t addr, size_t ,  data_t& data ){
   if (not has_addr(addr)){
    cvm::log(cvm::HIGH, "[DEBUGGER] Descarding read request at uc_helper since tag {} is not matching \n",tag());
   return;
  }
  if (addr == dmi_driver_status_addr)
  {
    cvm::log(cvm::MEDIUM, "[DEBUGGER] Debugger status : {:#x}\n",status);
    data[0] = status;
  }
  if (addr == dmi_driver_num_cmds_addr)
  {
    cvm::log(cvm::MEDIUM, "[DEBUGGER] Debugger num cmds in queue : {:#x}\n",commands_in_queue);
    data[0] = commands_in_queue;
  } 
  if (addr == dmi_driver_warm_reset_addr)
  {
    cvm::log(cvm::MEDIUM, "[DEBUGGER] Debugger warm reset : {:#x}\n",warm_reset);
    data[0] = warm_reset;
  }
  return;
}
void debugger::write(uint64_t addr, size_t, const data_t &data,
                     const strb_t &)
{
  if (not has_addr(addr))
    return;

  cvm::log(cvm::HIGH, "[Trickbox] Debugger write addr: {:#x}\n", addr);
  uint64_t t_data = 0;
  deserializeInt(data, t_data);
  if (addr == debugger_command_exec_trigger && !FLAGS_random_dbg_entry)
  {
    cvm::log(cvm::NONE, "[Trickbox] Debugger command Execution trigger\n");
    cmd_exc_trig_rcv = true;
    t_data_buf = t_data;
    // trickboxDmiWrite(hart, upper_dmi_data, lower_dmi_data); // Commented until DMI PORT is not in master
  }

  if (addr == debugger_file_load_trigger && !FLAGS_random_dbg_entry && !ndm_reset_occured)
  {
    file_loading_done = true;
    cvm::log(cvm::NONE, "[Trickbox] Debugger file loading trigger\n");
    parse_dmi_from_csv();
  }
}
