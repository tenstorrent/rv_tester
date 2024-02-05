#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/logger.hpp"
#include "jtag_driver.h"

DEFINE_string(jtag_input_file_path, "", "Path to file containing jtag_driver commands");
DEFINE_bool(random_jtag_entry, false, "Enter debug mode randomly after random intervals");
DEFINE_int32(random_jtag_start_delay, 300, "delay after which random interrupts should start");
DEFINE_int32(jtag_delay_min, 6, "Minimum Delay between 2 consecutive debug mode requests");
DEFINE_int32(jtag_delay_max, 9, "Maximum Delay between 2 consecutive debug mopde requests");
DEFINE_int32(jtag_max_snippets, 1, "Maximum number of debug snippets to be driven");
DEFINE_string(jtag_template_dir_path, "", "Path to file containing jtag_driver commands");

jtag_driver::jtag_driver(const std::string &tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc)
    : subdevice(tag, addr, 0x20000 /* size */, loc), soft_(hartCount),
      timeCompare_(6), IntrHart_(6), delayedRandomIntValid_(6), IntrValue_(6), timerIntPrev_(hartCount), timer_(0)
{
  rng.seed(FLAGS_seed);
  jtag_driver_base = addr;
  jtag_driver_trigger = addr + 0x10000;
  dmi_driver_status_addr = addr + 0x500;
  dmi_driver_num_cmds_addr = addr + 0x600;
  reset();
  // parse_dmi_from_csv();
  // jtag_trigger = 1;
  auto tbox_loc = cvm::topology::get_from_type("TRICKBOX", 0); 
  cvm::registry::messenger.connect<jtag_driver::dmi_status_t>(
            tbox_loc,
            [&](jtag_driver::dmi_status_t i) { return this->update_dm_status(i); });
}

jtag_driver::~jtag_driver()
{
  terminate_ = true;
}
void
jtag_driver::update_dm_status(jtag_driver::dmi_status_t& i) {
  cvm::log(cvm::HIGH, "Debug module status :{:#x} cmds in queue :{:#x}\n", i.status,i.commands_in_queue);
  status = i.status;
  commands_in_queue = i.commands_in_queue;
}
void jtag_driver::get_all_csv_templates()
{
  std::string directoryPath = FLAGS_jtag_template_dir_path;
  DIR *dir = opendir(directoryPath.c_str());
  cvm::log(cvm::NONE, "Debug commands directory:{}\n", directoryPath);
  if (!dir)
  {
    throw std::invalid_argument("Invalid directory path");
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr)
  {
    std::string filename = entry->d_name;
    if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".csv")
    {
      csvFilePaths.push_back(directoryPath + "/" + filename);
      cvm::log(cvm::NONE, "Pushing file:{}\n", filename);
    }
  }
  closedir(dir);
}

void jtag_driver::parse_dmi_from_csv()
{
  // if ((FLAGS_jtag_input_file_path == "")) {
  //   jtag_file_mode = 0;
  // } else {
  // cvm::log(cvm::NONE, "[Trickbox] Parsing dmi cfg file {}\n", FLAGS_jtag_input_file_path);
  // std::fstream file (FLAGS_jtag_input_file_path, std::ios::in);

  std::string file_name;
  if (FLAGS_random_jtag_entry)
    file_name = csvFilePaths[file_idx];
  else
    file_name = FLAGS_jtag_input_file_path;

  cvm::log(cvm::NONE, "Parse DMI Commands from CSV:{}\n", file_name);
  // std::fstream file (FLAGS_jtag_input_file_path, std::ios::in);
  std::fstream file(file_name, std::ios::in);
  if (file.is_open())
  {
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
        cvm::log(cvm::ERROR, "[Trickbox] Invalid command in csv file {}\n", instr);
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

      if (dmi_req.op != 3)
      {
        // remove underscores from addr
        row[1].erase(std::remove(row[1].begin(), row[1].end(), '_'), row[1].end());
        try
        {
          dmi_req.addr = std::stoul(row[1], nullptr, 16);
        }
        catch (const std::invalid_argument &e)
        {
          cvm::log(cvm::ERROR, "[Trickbox] Invalid argument: addr for stoul csv arg 1: {}\n", e.what());
        }
      }

      if (dmi_req.op == 2)
      {
        // remove underscores from data
        row[2].erase(std::remove(row[2].begin(), row[2].end(), '_'), row[2].end());
        try
        {
          dmi_req.data = std::stoul(row[2], nullptr, 16);
        }
        catch (const std::invalid_argument &e)
        {
          cvm::log(cvm::ERROR, "[Trickbox] Invalid argument: data for stoul csv arg 2: {}\n", e.what());
        }
      }

      content.push_back(row);
      dmi_cmd_q.push(dmi_req);
      // PRINT CSV DATA
      cvm::log(cvm::MEDIUM, "Pushing dmi request: op {} addr {:#x} data {:#x}\n", dmi_req.op, dmi_req.addr, dmi_req.data);
    }
  }
  else
  {
    cvm::log(cvm::ERROR, "Error: Could not open dmi cfg file {}\n", FLAGS_jtag_input_file_path);
  }

  jtag_file_mode = 1; // Clean up later
}

void jtag_driver::drive_csv_dmi_cmds()
{

  if (!dmi_cmd_q.empty())
  {
    dmi_req_t dmi_req;
    dmi_req = dmi_cmd_q.front();
    dmi_cmd_q.pop(); // pop front eleme7t
    cvm::log(cvm::MEDIUM, "Popping dmi request: op {} addr {:#x} data {:#x} func bits {:#x}\n", dmi_req.op, dmi_req.addr, dmi_req.data, dmi_req.func_bits);
    unsigned upper_dmi_data = 0;
    unsigned lower_dmi_data = 0;
    unsigned hart = 0;
    upper_dmi_data = (dmi_req.func_bits << 27) | (dmi_req.addr << 2) | dmi_req.op;
    lower_dmi_data = dmi_req.data;
    hart = 0; // hart bits position TBD, till TBD it is always zero
    trickboxDmiWrite(hart, upper_dmi_data, lower_dmi_data);
  }
  else
  {
    jtag_trigger = 0;
    rnd_jtag_trigger = 0;
  }
}

cvm::messenger::task<void>
jtag_driver::read(uint64_t addr, size_t , data_t& data)
{
   if (not has_addr(addr)){
    cvm::log(cvm::HIGH, "[jtag_driver] Descarding read request at uc_helper since tag {} is not matching \n",tag());
   co_return;
  }
  if (addr == dmi_driver_status_addr)
  {
    cvm::log(cvm::MEDIUM, "[jtag_driver] jtag_driver status : {:#x}\n",status);
    data[0] = status;
  }
  if (addr == dmi_driver_num_cmds_addr)
  {
    cvm::log(cvm::MEDIUM, "[jtag_driver] jtag_driver num cmds in queue : {:#x}\n",commands_in_queue);
    data[0] = commands_in_queue;
  } 
  co_return;
}
void jtag_driver::read_dev(uint64_t addr, size_t ,  data_t& data ){
   if (not has_addr(addr)){
    cvm::log(cvm::HIGH, "[jtag_driver] Descarding read request at uc_helper since tag {} is not matching \n",tag());
   return;
  }
  if (addr == dmi_driver_status_addr)
  {
    cvm::log(cvm::MEDIUM, "[jtag_driver] jtag_driver status : {:#x}\n",status);
    data[0] = status;
  }
  if (addr == dmi_driver_num_cmds_addr)
  {
    cvm::log(cvm::MEDIUM, "[jtag_driver] jtag_driver num cmds in queue : {:#x}\n",commands_in_queue);
    data[0] = commands_in_queue;
  } 
  return;
}
void jtag_driver::write(uint64_t addr, size_t, const data_t &data,
                     const strb_t &)
{
  if (not has_addr(addr))
    return;

  cvm::log(cvm::HIGH, "[Trickbox] jtag_driver write addr: {:#x}\n", addr);
  uint64_t t_data = 0;
  deserializeInt(data, t_data);
  if (addr == jtag_driver_base)
  {
    unsigned upper_dmi_data = 0;
    unsigned lower_dmi_data = 0;
    unsigned hart = 0;
    upper_dmi_data = t_data >> 32;
    lower_dmi_data = t_data & 0xffffffff;
    hart = 0;                                               // hart bits position TBD, till TBD it is always zero
    trickboxDmiWrite(hart, upper_dmi_data, lower_dmi_data); // Commented until DMI PORT is not in master
  }

  if (addr == jtag_driver_trigger && !FLAGS_random_jtag_entry)
  {
    cvm::log(cvm::MEDIUM, "[Trickbox] jtag_driver file trigger\n");
    parse_dmi_from_csv();
    jtag_trigger = 1;
  }
}
