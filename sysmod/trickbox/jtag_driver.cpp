#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/logger.hpp"
#include "jtag_driver.h"
#include "sysmod/sysmod_plusargs.h"

DEFINE_string(jtag_input_file_path, "", "Path to file containing jtag_driver commands");
DEFINE_bool(random_jtag_entry, false, "Enter debug mode randomly after random intervals");
DEFINE_int32(random_jtag_start_delay, 300, "delay after which random interrupts should start");
DEFINE_int32(jtag_delay_min, 6, "Minimum Delay between 2 consecutive debug mode requests");
DEFINE_int32(jtag_max_loop_count, 50, "Number of times loop should run before flagging error");
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
  loop_rdata = i.jtag_op_data;
}

void jtag_driver::get_all_csv_templates()
{
    std::string directoryPath = FLAGS_jtag_template_dir_path;
    cvm::log(cvm::NONE, "Debug commands directory:{}\n", directoryPath);

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
                csvFilePaths.push_back(entry.path().string());
                cvm::log(cvm::NONE, "Pushing file:{}\n", filename);
            }
        }
    }
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
      row.clear();

      line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
      std::stringstream str(line);

      while (getline(str, word, ','))
      {
        row.push_back(word);
      }
      
      std::string instr;
      std::string data_s;
      std::string jtag_cmd;
      std::string length;
      
      jtag_req_t jtag_req;
      instr = row[0];

      if (instr == "//"){
        continue; // skip line may be comment
      }
      data_s = row[1];
      length = row[2]; //TODO
      
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
      }else if(jtag_cmd == "np"){ //nop,10
         jtag_req.jtag_cmd = 3;
      }else if(jtag_cmd == "ck"){ //chk,456
         jtag_req.jtag_cmd = 4;
      }else if(jtag_cmd == "ls"){ //loop start,loop_limit
         jtag_req.jtag_cmd = 5;
      }else if(jtag_cmd == "le"){ //loop end, checkbitNum, CheckbitValue
         jtag_req.jtag_cmd = 6;
      }else if(jtag_cmd == "qt"){ //end test
         jtag_req.jtag_cmd = 7;
      }else if(jtag_cmd == "rv"){ //reverse
         jtag_req.jtag_cmd = 8;
      }else if(jtag_cmd == "sl"){ //shift left
         jtag_req.jtag_cmd = 9;
      }else if(jtag_cmd == "sr"){ //shift right
         jtag_req.jtag_cmd = 10;
      }else if(jtag_cmd == "ts"){
         jtag_req.jtag_cmd = 11;
        
      }
      else{
        cvm::log(cvm::ERROR, "Error: unknown command {} in jtag cfg file {}\n",jtag_cmd, FLAGS_jtag_input_file_path);
      }
      
      if(jtag_req.jtag_cmd<3 || jtag_req.jtag_cmd == 4){ 
         length = row[2];  //length NA for nop 
      }
      
      data_s.erase(std::remove_if(data_s.begin(), data_s.end(), ::isspace), data_s.end());
      // convert string to lowercase for uniformity
      std::transform(data_s.begin(), data_s.end(), data_s.begin(), ::tolower);
      // cvm::log(cvm::HIGH, "[jtag_driver] length {:#x}\n",length);
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
      if((jtag_req.jtag_cmd<3) || (jtag_req.jtag_cmd ==6) || (jtag_req.jtag_cmd ==4)){
        try{
          jtag_req.jtag_length_data = std::stoul(length,nullptr,10);
          
        } catch (const std::invalid_argument& e) {
            std::cerr << "[JTAG DRIVER] Invalid argument for stoul csv arg 2: " << e.what() << std::endl;
        }
      }else{
         jtag_req.jtag_length_data = 0;
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
  if(!executing_nop || !executing_loop){
  if (!jtag_cmd_q.empty())
  {
    
    jtag_req_t jtag_req;
    jtag_req = jtag_cmd_q.front();

   // cvm::log(cvm::MEDIUM, "Popping jtag request: op {} addr {:#x} data {:#x} func bits {:#x}\n", jtag_req.op, jtag_req.addr, jtag_req.data, jtag_req.func_bits);
    unsigned       jtag_cmd = 0;
    unsigned long  upper_jtag_data = 0;
    unsigned long  lower_jtag_data = 0;
    unsigned       reg_length_data = 0;
    unsigned       hart = 0;

    jtag_cmd        = jtag_req.jtag_cmd;
    upper_jtag_data = jtag_req.jtag_ip_data_upper;
    lower_jtag_data = jtag_req.jtag_ip_data_lower;
    reg_length_data = jtag_req.jtag_length_data;
    
    cvm::log(cvm::HIGH, "[JTAGDRIVER] Driving jtag cmd {}\n", jtag_cmd);

    if(jtag_cmd<3){
      hart = 0; // hart bits position TBD, till TBD it is always zero
      jtag_cmd_q.pop(); // pop front eleme7t
      trickboxJtagWrite(hart, jtag_cmd, upper_jtag_data, lower_jtag_data,reg_length_data,0,tap_cfg_sel);
    }

    if(jtag_cmd == 3){ //nop
      executing_nop = true;
      nop_count = lower_jtag_data;
      cvm::log(cvm::HIGH, "[JTAGDRIVER] Pushing jtag nops for {} ticks\n", nop_count);
      jtag_cmd_q.pop(); // pop front eleme7t
    }
    if(jtag_cmd == 7 && !FLAGS_random_jtag_entry){  //JTAG quit, signal to end simulation once csv ends
 
      cvm::log(cvm::HIGH, "[JTAGDRIVER] ******************* \n");
      cvm::log(cvm::HIGH, "[JTAGDRIVER] Sending Quit signal \n");
      cvm::log(cvm::HIGH, "[JTAGDRIVER] ******************* \n");
      trickboxJtagWrite(hart, jtag_cmd, 0, 0,0,1,tap_cfg_sel);
    
    }else if(jtag_cmd == 7){
      csv_completed = 1;
      jtag_cmd_q.pop();
    }

    if(jtag_cmd == 4){  //ck expecting check on rdata
      //check last saved rdata == lower_jtag_data ??
      uint64_t mask = (1ULL << reg_length_data) - 1;
      auto result = reg_length_data == 64 ? loop_rdata : loop_rdata & mask;

      cvm::log(cvm::HIGH, "[JTAGDRIVER] reg_length_data {} loop_rdata {:#x} lower_jtag_data {:#x} mask {:#x} expression {:#x}\n",reg_length_data,loop_rdata,lower_jtag_data,mask,(1 << reg_length_data));
      
      if(result == lower_jtag_data){
       //PASS
       cvm::log(cvm::HIGH, "[JTAGDRIVER] jtag check opcode Passed! expected {:#x} got {:#x} \n", lower_jtag_data,result);
      }else{
       //FAIL
       cvm::log(cvm::ERROR, "\nERROR: [JTAGDRIVER] jtag check opcode failed! expected {:#x} got {:#x} \n", lower_jtag_data,result);
      }
      jtag_cmd_q.pop(); // pop front eleme7t
    }

    if(jtag_cmd == 8){  //Reverse

      uint64_t temp_rev = 0;
 
      // traversing bits of 'n' from the right
      while (loop_rdata > 0) {
          // bitwise left shift
          // 'rev' by 1
          temp_rev <<= 1;
  
          // if current bit is '1'
          if ((loop_rdata & 1) == 1)
              temp_rev ^= 1;
  
          // bitwise right shift
          // 'n' by 1
          loop_rdata >>= 1;
      }
      loop_rdata  = temp_rev ;

      jtag_cmd_q.pop(); // pop front element
    }

    if(jtag_cmd == 9){  //shift left
      uint64_t shifted = loop_rdata;

      shifted <<= lower_jtag_data;

      loop_rdata = shifted ;    
      jtag_cmd_q.pop(); // pop front element
    }

    if(jtag_cmd == 10){  //shift right
      uint64_t shifted = loop_rdata;
      shifted >>= lower_jtag_data;

      loop_rdata = shifted ;    
      jtag_cmd_q.pop(); // pop front element
    }
    
    if(jtag_req.jtag_cmd == 11 && lower_jtag_data <= 8){
        
        try{
          tap_cfg_sel = lower_jtag_data; 
          cvm::log(cvm::HIGH, "[JTAGDRIVER] tap_sel {} \n", tap_cfg_sel);
          
        } catch (const std::invalid_argument& e) {
            std::cerr << "[JTAG DRIVER] Invalid argument for stoul csv arg 2: " << e.what() << std::endl;
        }
        jtag_cmd_q.pop();
        // continue;
    }

    if(jtag_cmd == 5){ //ls loop start
      
      unsigned jtag_cmd_temp = 0;
      
      jtag_loop_q.clear();
      jtag_cmd_q.pop(); // pop front eleme7t which is loop start
      
      if(lower_jtag_data == 0){//loop start
            max_num_loops = FLAGS_jtag_max_loop_count;
         }else{
            max_num_loops = lower_jtag_data; 
      }

      do{
        jtag_req_t jtag_req_temp;
        
        jtag_req             = jtag_cmd_q.front();
        jtag_cmd_temp        = jtag_req.jtag_cmd;
        jtag_req_temp        = jtag_req;
        if(jtag_cmd_temp!=6){
          jtag_loop_q.push_back(jtag_req_temp);
        }
        if(jtag_cmd_temp == 6){
          loop_check_bit_num   = jtag_req.jtag_ip_data_lower;
          loop_check_bit_type  = jtag_req.jtag_length_data;
        }
        jtag_cmd_q.pop(); // pop elemnt

      }while(jtag_cmd_temp != 6);
      
      executing_loop = true;
      loop_size = jtag_loop_q.size();
      loop_idx = 0;
    }
    
    if(jtag_cmd == 6){ //le loop end, checkbitNum, CheckbitValue
       cvm::log(cvm::ERROR, "ERROR: [JTAGDRIVER] jtag loop end detected without loop start \n");
    }


  }
  else
  {
    jtag_trigger = 0;
    rnd_jtag_trigger = 0;
  }
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
  }

  if (addr == jtag_driver_trigger && !FLAGS_random_jtag_entry)
  {
    cvm::log(cvm::MEDIUM, "[Trickbox] jtag_driver file trigger\n");
    parse_jtag_from_csv();
    jtag_trigger = 1;
  }
}