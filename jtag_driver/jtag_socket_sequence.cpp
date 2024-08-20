#include "jtag_socket_sequence.hpp"
//#include "sysmod/sysmod_plusargs.h"

REGISTRY_register(jtag_socket_sequence, JTAG_DRIVER, cvm::registry::all);

DEFINE_string(jtag_driver_mode, "off", "Enable jtag_socket_sequence in the sim - off/csv/socket");
DEFINE_string(jtag_input_file_path, "", "Path to file containing jtag_driver commands");
DEFINE_bool(random_jtag_entry, false, "Enter debug mode randomly after random intervals");
DEFINE_bool(jtag_remote_debugger_mode, false, "Accept JTAG transactions over scoket");
DEFINE_int32(random_jtag_start_delay, 300, "delay after which random interrupts should start");
DEFINE_int32(jtag_delay_min, 6, "Minimum Delay between 2 consecutive debug mode requests");
DEFINE_int32(jtag_max_loop_count, 50, "Number of times loop should run before flagging error");
DEFINE_int32(jtag_delay_max, 9, "Maximum Delay between 2 consecutive debug mopde requests");
DEFINE_int32(jtag_socket_port, 8088, "Port number for JTAG socket communication");
DEFINE_int32(jtag_max_snippets, 1, "Maximum number of debug snippets to be driven");
DEFINE_string(jtag_template_dir_path, "", "Path to file containing jtag_driver commands");
DEFINE_string(jtag_txn_file,"","File containing jtag transaction requests");
extern "C" {
  void jtag_driver_init();
  void jtag_driver_jtag_socket(uint8_t val);
  void drive_jtag_req(unsigned cmd,unsigned long upper_val, unsigned long lower_val, unsigned length, unsigned quit,unsigned tap_cfg_sel);

  uint8_t jtag_driver_get_en(const char* mode) {
    return (std::string(mode) != "off");
  }
}

jtag_socket_sequence::jtag_socket_sequence(cvm::topology::loc_t loc, unsigned id) : loc_(loc), id_(id), scope_(nullptr) {

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });
  cvm::registry::messenger.connect<rv_tester_transactions::jtag_driver::jtag_rdata<>>(
      loc_,
      [this](const rv_tester_transactions::jtag_driver::jtag_rdata<>& t) { return this->jtag_resp(t.rdata); }); 
  cvm::registry::messenger.connect<rv_tester_transactions::jtag_driver::m_jtag_driver_tick<>>(
      loc_,
      [this](const rv_tester_transactions::jtag_driver::m_jtag_driver_tick<>& t) { return this->jtag_tick(t.cycle); }); 

  // jtag_socket sequence threads
  std::cout<<"\n PRT creating JATG socket sequence \n";
  //socket_mode_thread();
  if (FLAGS_jtag_driver_mode == "csv") {
    csv_mode_thread();
  } else if (FLAGS_jtag_driver_mode == "socket") {
    socket_mode_thread();
  }
}

 jtag_socket_sequence::~jtag_socket_sequence() {
    //cvm::log(cvm::NONE, "INFO_PASS_METRIC:{{\"hart{}_jtag_driver_jtag_socket_count\": \"{}\"}}\n", id_, jtag_socket_count_);
}

void jtag_socket_sequence::csv_mode_thread() {
  if(FLAGS_jtag_input_file_path != "")
     parse_jtag_from_csv();
  auto *task = +[] (jtag_socket_sequence* m) -> cvm::messenger::task<void> {
    co_await m->random_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void jtag_socket_sequence::socket_mode_thread() {
  auto *task = +[] (jtag_socket_sequence* m) -> cvm::messenger::task<void> {
    co_await m->open_socket_to_listen();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> jtag_socket_sequence::random_mode() {
  while (true) {
    // Wait for next tick generated after a random interval "jtag_socket_interval"
    co_await tick();

  }
  co_return;
}

void jtag_socket_sequence::init() {
  cvm::registry::callbacks.push(
    scope_,
    []() {
      jtag_driver_init();
    });
}

void jtag_socket_sequence::jtag_socket(unsigned hart, uint8_t assert) {
  cvm::registry::callbacks.push(
    scope_,
    [assert, hart]() {
      cvm::log(cvm::HIGH, "[jtag_driver][h{}] {} jtag_socket\n", hart, assert ? "assert" : "deassert");
      jtag_driver_jtag_socket(assert);
    });
}

cvm::messenger::task<void> jtag_socket_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::jtag_driver::m_jtag_driver_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> jtag_socket_sequence::trigger() {
  co_return;
}


void
jtag_socket_sequence::update_jtag_status(jtag_socket_sequence::jtag_req_t& i) {
  cvm::log(cvm::HIGH, "\n *** GOT RESP FROM JTAG TDO  {:#x}", i.jtag_op_data);
  loop_rdata = i.jtag_op_data;
}

void jtag_socket_sequence::get_all_csv_templates()
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

void jtag_socket_sequence::parse_jtag_from_csv()
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
      // cvm::log(cvm::HIGH, "[jtag_socket_sequence] length {:#x}\n",length);
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


void jtag_socket_sequence::process_input_string(std::string line)
{


  cvm::log(cvm::HIGH, "[JTAG_DRIVER.CPP] PROCESS INPUT STRING :{}\n", line);
  std::string word;
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
        return; // skip line may be comment
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
      // cvm::log(cvm::HIGH, "[jtag_socket_sequence] length {:#x}\n",length);
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
void jtag_socket_sequence::drive_csv_jtag_cmds()
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


void jtag_socket_sequence::drive_jtag_cmds()
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



// std::string jtag_socket_sequence::getLocalIPAddress() {
//     struct ifaddrs *ifaddr, *ifa;
//     int family, s;
//     char host[NI_MAXHOST];

//     if (getifaddrs(&ifaddr) == -1) {
//         perror("getifaddrs");
//         exit(EXIT_FAILURE);
//     }

//     std::string ipAddress = "Unable to get IP address";

//     for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
//         if (ifa->ifa_addr == nullptr)
//             continue;

//         family = ifa->ifa_addr->sa_family;

//         if (family == AF_INET) {
//             s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
//                             host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
//             if (s != 0) {
//                 std::cerr << "getnameinfo() failed: " << gai_strerror(s) << std::endl;
//                 exit(EXIT_FAILURE);
//             }

//             if (std::string(ifa->ifa_name) == "lo")
//                 continue; // Skip loopback address

//             ipAddress = host;
//         }
//     }

//     freeifaddrs(ifaddr);
//     return ipAddress;
// }
std::string jtag_socket_sequence::process_string(const std::string& input) {
    size_t pos = input.find(',');
    if (pos != std::string::npos) {
        return input.substr(0, pos);
    }
    return "";
}

std::string jtag_socket_sequence::get_local_ip_address() {
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;

    // Retrieve hostname
    gethostname(hostbuffer, sizeof(hostbuffer));

    // Retrieve host information
    host_entry = gethostbyname(hostbuffer);

    // Convert an Internet network address into ASCII string
    IPbuffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));

    return std::string(IPbuffer);
}

cvm::messenger::task<void> jtag_socket_sequence::open_socket_to_listen(){
    int server_fd, new_socket;
    struct sockaddr_in address;
    bool quit_communication = false;
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = {0};
    //int PORT=8088;
    int PORT=FLAGS_jtag_socket_port;
    int tick_count = 0;
   std::cout<<"\n PRT opening socket to listen ...\n";
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket to non-blocking mode
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1"); // Bind to 127.0.0.1
    address.sin_port = htons(PORT);

    // Binding the socket to the network address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is listening on " << inet_ntoa(address.sin_addr) << ":" << PORT << std::endl;
    std::cout << "Server's local IP address: " << get_local_ip_address() << std::endl;


    while (true) {
       tick_count++;
       std::cout << "Server is listening on  tick cnt" <<std::dec<< tick_count << std::endl;
       std::cout << "Server's local IP address: " << get_local_ip_address() << std::endl;
       if(quit_communication){
        break;
       }
       co_await tick();
       if(tick_count < 10){
        co_await tick();
        continue;
       }
        // Accepting incoming connection (non-blocking)
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket >= 0) {
            std::cout << "Connection accepted from " << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port) << std::endl;

            // Process multiple requests from the same client
            while (true) {
                int valread = ::read(new_socket, buffer, 1024);
                if (valread > 0) {
                    std::cout << "Received: " << buffer << std::endl;
                    // Got buffer from client
                    //convert char buffer to string
                    std::string input_line(buffer);
                    std::cout << "Received input line: " << input_line << std::endl;
                    process_input_string(input_line);
                    drive_jtag_cmds();

                    // Send Response back
                    std::stringstream ss;
                    ss << std::hex << loop_rdata; // Convert to hexadecimal string
    
                    std::string str = ss.str();
                    std::cout << "Hexadecimal string for jtag Rdata: " << str << std::endl;
                    std::string response = "ACK," + str;
                    co_await tick();
                    send(new_socket, response.c_str(), response.length(), 0);
                    std::cout << "Response sent: " << response << std::endl;
                } else if (valread == 0) {
                    std::cout << "Client disconnected" << std::endl;
                    quit_communication = true;
                    break; // Client disconnected
                } else if (valread < 0 && errno != EWOULDBLOCK) {
                    perror("read");
                    quit_communication = true;
                    break;
                }
                memset(buffer, 0, sizeof(buffer)); // Clear buffer
            }

            close(new_socket);
        }
        usleep(10000);  // Small delay to prevent busy-waiting
    }

    close(server_fd);
} 

  // Used to assert/deassert a trickbox interrupt (PIPI) for given hart.
  // virtual void trickboxjtagWrite(unsigned hart, unsigned upper_jtag_data, unsigned lower_jtag_data, cbs_t& cbs)
  void jtag_socket_sequence::trickboxJtagWrite(unsigned hart,unsigned jtag_cmd, unsigned long upper_jtag_data, unsigned long lower_jtag_data,unsigned reg_length_data,unsigned jtag_quit, unsigned tap_cfg_sel)
  {
    cvm::log(cvm::HIGH, "TrickBox jtag Write to hart:{}, upper jtag data:{:#x}, lower jtag data:{:#x}, reg length data:{:#x}", hart, upper_jtag_data, lower_jtag_data,reg_length_data);
    // cbs.push_back(cb_t{Callback::TRICKBOX_jtag_WR, hart, upper_jtag_data, lower_jtag_data, 0});
    //cvm::registry::messenger.signal(12, jtag_data_t{hart,jtag_cmd, upper_jtag_data, lower_jtag_data,reg_length_data,jtag_quit,tap_cfg_sel});
    // cvm::messenger::send(jtag_t, jtag_pkt);
    cvm::registry::callbacks.push(
    scope_,
    [jtag_cmd,upper_jtag_data, lower_jtag_data,reg_length_data,jtag_quit,tap_cfg_sel]() {
      drive_jtag_req(jtag_cmd,upper_jtag_data, lower_jtag_data,reg_length_data,jtag_quit,tap_cfg_sel);
    });

  }

  void jtag_socket_sequence::jtag_resp(std::bitset<70> rdata){
  std::vector<uint64_t> convertedArray = bitsetToUint64Array(rdata);
  cvm::log(cvm::FULL, "[JTAG_DRIVER.CPP] In JTAG RESP converted array size = {}\n", convertedArray.size());
  
  for (uint64_t num : convertedArray) {
        cvm::log(cvm::FULL, "[JTAG_DRIVER.CPP] In JTAG RESP converted array element = {}\n", num);
  }
  loop_rdata = convertedArray[0]; 

}