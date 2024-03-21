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
DEFINE_string(jtag_txn_file,"","File containing jtag transaction requests");

jtag_driver::jtag_driver(const std::string &tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc)
    : subdevice(tag, addr, 0x20000 /* size */, loc), soft_(hartCount),
      timeCompare_(6), IntrHart_(6), delayedRandomIntValid_(6), IntrValue_(6), timerIntPrev_(hartCount), timer_(0)
{
  rng.seed(FLAGS_seed);
  jtag_driver_base = addr;
  jtag_driver_trigger = addr + 0x10000;
  jtag_driver_status_addr = addr + 0x500;
  jtag_driver_num_cmds_addr = addr + 0x600;
  reset();
  if(FLAGS_jtag_input_file_path != "")
     parse_jtag_from_csv();
  // jtag_trigger = 1;
  auto tbox_loc = cvm::topology::get_from_type("TRICKBOX", 0); 
  cvm::registry::messenger.connect<jtag_driver::jtag_req_t>(
            tbox_loc,
            [&](jtag_driver::jtag_req_t i) { return this->update_jtag_status(i); });
}

jtag_driver::~jtag_driver()
{
}
void
jtag_driver::update_jtag_status(jtag_driver::jtag_req_t& i) {
  cvm::log(cvm::HIGH, "\n *** GOT RESP FROM JTAG TDO  {:#x}", i.jtag_op_data);
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

void jtag_driver::parse_jtag_from_csv()
{


  std::string file_name;
  if (FLAGS_random_jtag_entry)
    file_name = csvFilePaths[file_idx];
  else
    file_name = FLAGS_jtag_input_file_path;

  cvm::log(cvm::NONE, "Parse JTAG Commands from CSV:{}\n", file_name);
  // std::fstream file (FLAGS_jtag_input_file_path, std::ios::in);
  std::fstream file(file_name, std::ios::in);
  if (file.is_open())
  {
    std::string line, word;
    while (getline(file, line))
    {
      //std::cout<<"********* Process new line: "<<line <<" ***************\n";
      row.clear();

      //std::cout<<"\n JTAG INP row clr : row0:"<<row[0]<<" row1 "<<row[1]<<"\n";
      line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
      std::stringstream str(line);

      while (getline(str, word, ','))
      {
        row.push_back(word);
        //std::cout<<"pushing "<< word<< "in a row\n";
      }
      //std::cout<<"row size: "<<row.size()<<"\n";
      
 
      std::string instr;
      std::string data_s;
      std::string jtag_cmd;
      std::string length;
      
      jtag_req_t jtag_req;
      
      //std::cout<<"JTAG INP after clr: row0:"<<row[0]<<" row1 "<<row[1]<<"\n";
      instr = row[0];
      data_s = row[1];
      length = row[2];
      // remove empty spaces from string
      instr.erase(std::remove_if(instr.begin(), instr.end(), ::isspace), instr.end());
      // convert string to lowercase for uniformity
      std::transform(instr.begin(), instr.end(), instr.begin(), ::tolower);
      jtag_cmd = instr.substr(0, 2);
      if(jtag_cmd == "ir"){
         jtag_req.jtag_cmd = 0;
      }else if(jtag_cmd == "dr"){
         jtag_req.jtag_cmd = 1;
      }else if(jtag_cmd == "up"){
         jtag_req.jtag_cmd = 2;
      }else{
        cvm::log(cvm::ERROR, "Error: unknown command {} in jtag cfg file {}\n",jtag_cmd, FLAGS_jtag_input_file_path);
      }
      
      data_s.erase(std::remove_if(data_s.begin(), data_s.end(), ::isspace), data_s.end());
      // convert string to lowercase for uniformity
      std::transform(data_s.begin(), data_s.end(), data_s.begin(), ::tolower);
      std::cout<<" JTAG INP : row0:"<<row[0]<<" row1 "<<row[1]<<" row2 "<<row[2]<<"\n";
      // cvm::log(cvm::HIGH, "[jtag_driver] length {:#x}\n",length);
      std::cout<<" JTAG INP : instr:"<<instr<<" instr2char "<<jtag_cmd<<" data: "<<data_s<<" length: "<<length<<"\n";
      //check data length
      unsigned data_len = data_s.length();
      if(data_len>16){
       std::string data_s_upper = data_s.substr(0, data_len-16);
       std::string data_s_lower = data_s.substr(data_len-16, data_len);
      try{
        jtag_req.jtag_ip_data_lower = std::stoul(data_s_lower,nullptr,16);
        
      } catch (const std::invalid_argument& e) {
          std::cerr << "[JTAG DRIVER] Invalid argument for stoul csv arg 1: " << e.what() << std::endl;
      }
      try{
        jtag_req.jtag_ip_data_upper = std::stoul(data_s_upper,nullptr,16);
        
      } catch (const std::invalid_argument& e) {
          std::cerr << "[JTAG DRIVER] Invalid argument for stoul csv arg 1: " << e.what() << std::endl;
      }
      }
      else {
         try{
           jtag_req.jtag_ip_data_lower = std::stoul(data_s,nullptr,16);
           jtag_req.jtag_ip_data_upper = 0;
           
         } catch (const std::invalid_argument& e) {
             std::cerr << "[JTAG DRIVER] Invalid argument for stoul csv arg 1: " << e.what() << std::endl;
         }
      }
      try{
        jtag_req.jtag_length_data = std::stoul(length,nullptr,10);
        
      } catch (const std::invalid_argument& e) {
          std::cerr << "[JTAG DRIVER] Invalid argument for stoul csv arg 2: " << e.what() << std::endl;
      }
 
      content.push_back(row);
      jtag_cmd_q.push(jtag_req);
      // PRINT CSV DATA
      //cvm::log(cvm::MEDIUM, "Pushing jtag request: op {} addr {:#x} data {:#x}\n", jtag_req.op, jtag_req.addr, jtag_req.data);
    }
  }
  else
  {
    cvm::log(cvm::ERROR, "Error: Could not open jtag cfg file {}\n", FLAGS_jtag_input_file_path);
  }

  jtag_file_mode = 1; // Clean up later
}

void jtag_driver::drive_csv_jtag_cmds()
{

  if (!jtag_cmd_q.empty())
  {
    jtag_req_t jtag_req;
    jtag_req = jtag_cmd_q.front();
    jtag_cmd_q.pop(); // pop front eleme7t
   // cvm::log(cvm::MEDIUM, "Popping jtag request: op {} addr {:#x} data {:#x} func bits {:#x}\n", jtag_req.op, jtag_req.addr, jtag_req.data, jtag_req.func_bits);
    unsigned jtag_cmd = 0;
    unsigned long  upper_jtag_data = 0;
    unsigned long lower_jtag_data = 0;
    unsigned reg_length_data = 0;
    unsigned hart = 0;
    jtag_cmd = jtag_req.jtag_cmd;
    upper_jtag_data = jtag_req.jtag_ip_data_upper;
    lower_jtag_data = jtag_req.jtag_ip_data_lower;
    reg_length_data = jtag_req.jtag_length_data;
    std::cout<<" JTAG INP : jtag_req.jtag_length_data:"<<jtag_req.jtag_length_data<<" reg_length_data "<<reg_length_data<<"\n";
    hart = 0; // hart bits position TBD, till TBD it is always zero
    trickboxJtagWrite(hart, jtag_cmd, upper_jtag_data, lower_jtag_data,reg_length_data);
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
  if (addr == jtag_driver_status_addr)
  {
    cvm::log(cvm::MEDIUM, "[jtag_driver] jtag_driver status : {:#x}\n",status);
    data[0] = status;
  }
  if (addr == jtag_driver_num_cmds_addr)
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
  if (addr == jtag_driver_status_addr)
  {
    cvm::log(cvm::MEDIUM, "[jtag_driver] jtag_driver status : {:#x}\n",status);
    data[0] = status;
  }
  if (addr == jtag_driver_num_cmds_addr)
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
    cvm::log(cvm::MEDIUM, "[Trickbox] jtag_driver BASE ADDR WRITTEN\n");
    // unsigned upper_jtag_data = 0;
    // unsigned lower_jtag_data = 0;
    // unsigned reg_length_data = 0;
    // unsigned hart = 0;
    // upper_jtag_data = t_data >> 32;
    // lower_jtag_data = t_data & 0xffffffff;
    // hart = 0;                                               // hart bits position TBD, till TBD it is always zero
    //trickboxJtagWrite(hart, upper_jtag_data, lower_jtag_data,reg_length_data); // Commented until jtag PORT is not in master
  }

  if (addr == jtag_driver_trigger && !FLAGS_random_jtag_entry)
  {
    cvm::log(cvm::MEDIUM, "[Trickbox] jtag_driver file trigger\n");
    parse_jtag_from_csv();
    jtag_trigger = 1;
  }
}
