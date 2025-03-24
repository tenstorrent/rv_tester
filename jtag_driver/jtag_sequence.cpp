#include "jtag_sequence.hpp"
//#include "sysmod/sysmod_plusargs.h"

REGISTRY_register(jtag_sequence, JTAG_DRIVER, cvm::registry::all);

DEFINE_string(jtag_driver_mode, "off", "Enable jtag_sequence in the sim - off/csv/socGket");
DEFINE_string(jtag_input_file_path, "", "Path to file containing jtag_driver commands");
DEFINE_bool(random_jtag_entry, false, "Enter debug mode randomly after random intervals");
DEFINE_bool(reverse_jtag_rdata, false, "Reverse data recived on JTAG tdo");
DEFINE_bool(continue_on_jtag_err, false, "Continue executing JTAG snippet even after error");
DEFINE_bool(en_jtag_driver_logs, false, "Enable dumping logs for jtag driver");
//DEFINE_bool(quit_on_jtag_err, false, "Quit executing JTAG snippet after error");
DEFINE_bool(jtag_remote_debugger_mode, false, "Accept JTAG transactions over scoket");
DEFINE_int32(random_jtag_start_delay, 300, "delay after which random interrupts should start");
DEFINE_int32(jtag_delay_min, 6, "Minimum Delay between 2 consecutive debug mode requests");
DEFINE_int32(jtag_max_loop_count, 50, "Number of times loop should run before flagging error");
DEFINE_int32(jtag_delay_max, 9, "Maximum Delay between 2 consecutive debug mopde requests");
DEFINE_int32(jtag_socket_port, 8088, "Port number for JTAG socket communication");
DEFINE_string(jtag_socket_ip, "", "Server's local IP address");
DEFINE_int32(jtag_max_snippets, 1, "Maximum number of debug snippets to be driven");
DEFINE_string(jtag_template_dir_path, "", "Path to file containing jtag_driver commands");
DEFINE_string(jtag_txn_file,"","File containing jtag transaction requests");
extern "C" {
  void jtag_driver_init();
  void jtag_driver_jtag_socket(uint8_t val);
  void drive_jtag_req(unsigned cmd,unsigned long upper_val, unsigned long lower_val, unsigned length, unsigned quit,unsigned tap_cfg_sel);
  //void drive_jtag_req_socket(unsigned cmd, const unsigned long* lower_val, unsigned length, unsigned quit,unsigned tap_cfg_sel);
  void drive_jtag_req_socket(unsigned cmd, const unsigned long lower_val[], unsigned length, unsigned quit,unsigned tap_cfg_sel);

  uint8_t jtag_driver_get_en(const char* mode) {
    return (std::string(mode) != "off");
  }
}

jtag_sequence::jtag_sequence(cvm::topology::loc_t loc, unsigned id) : loc_(loc), id_(id), scope_(nullptr) {

  // Scope
  cvm::registry::messenger.connect<svScope>(loc_, [this](svScope s) { return this->set_scope(s); });
  cvm::registry::messenger.connect<rv_tester_transactions::jtag_driver::jtag_rdata<>>(
      loc_,
      [this](const rv_tester_transactions::jtag_driver::jtag_rdata<>& t) { return this->jtag_resp(t.rdata); }); 
  cvm::registry::messenger.connect<rv_tester_transactions::jtag_driver::m_jtag_driver_tick<>>(
      loc_,
      [this](const rv_tester_transactions::jtag_driver::m_jtag_driver_tick<>& t) { return this->jtag_tick(t.cycle); }); 
  cvm::registry::messenger.connect<rv_tester_transactions::jtag_driver::jtag_pkt_ack<>>(
      loc_,
      [this](const rv_tester_transactions::jtag_driver::jtag_pkt_ack<>& t) { return this->jtag_ack(t.complete); }); 
  // jtag_socket sequence threads
  if (FLAGS_jtag_driver_mode == "csv") {
    csv_mode_thread();
  } else if (FLAGS_jtag_driver_mode == "socket") {
    socket_mode_thread();
  }
}

 jtag_sequence::~jtag_sequence() {
}

void jtag_sequence::csv_mode_thread() {
  if(FLAGS_jtag_input_file_path != "")
     parse_jtag_from_csv();
  auto *task = +[] (jtag_sequence* m) -> cvm::messenger::task<void> {
    co_await m->random_mode();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

void jtag_sequence::socket_mode_thread() {
  auto *task = +[] (jtag_sequence* m) -> cvm::messenger::task<void> {
    co_await m->open_socket_to_listen();
    co_return;
  };
  cvm::registry::messenger.fork(task, this);
};

cvm::messenger::task<void> jtag_sequence::random_mode() {
  while (true) {
    co_await tick();
  }
  co_return;
}

void jtag_sequence::init() {
  cvm::registry::callbacks.push(
    scope_,
    []() {
      jtag_driver_init();
    });
}

void jtag_sequence::jtag_socket(unsigned hart, uint8_t assert) {
  cvm::registry::callbacks.push(
    scope_,
    [assert, hart]() {
      if(FLAGS_en_jtag_driver_logs)
      cvm::log(cvm::HIGH, "[jtag_sequence][h{}] {} jtag_socket\n", hart, assert ? "assert" : "deassert");
      jtag_driver_jtag_socket(assert);
    });
}

cvm::messenger::task<void> jtag_sequence::tick() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::jtag_driver::m_jtag_driver_tick<>>(loc_);
  co_return;
}

cvm::messenger::task<void> jtag_sequence::resp() {
  co_await cvm::registry::messenger.wait<rv_tester_transactions::jtag_driver::jtag_rdata<>>(loc_);
  co_await tick();
  co_return;
}

cvm::messenger::task<void> jtag_sequence::trigger() {
  co_return;
}


void
jtag_sequence::update_jtag_status(jtag_sequence::jtag_req_t& i) {
  if(FLAGS_en_jtag_driver_logs)
  cvm::log(cvm::HIGH, "\n[jtag_sequence] GOT RESP FROM JTAG TDO  {:#x}", i.jtag_op_data);
  loop_rdata = i.jtag_op_data;
}

void jtag_sequence::get_all_csv_templates()
{
    std::string directoryPath = FLAGS_jtag_template_dir_path;
    cvm::log(cvm::MEDIUM, "[jtag_sequence]JTAG template directory:{}\n", directoryPath);

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
                cvm::log(cvm::NONE, "[jtag_sequence]Pushing file:{}\n", filename);
            }
        }
    }
    std::sort(csvFilePaths.begin(), csvFilePaths.end());
}

void jtag_sequence::parse_jtag_from_csv()
{


  std::string file_path;
  if (FLAGS_random_jtag_entry) {
    file_path = csvFilePaths[file_idx];
  }
  else
    file_path = FLAGS_jtag_input_file_path;
  
  // Extract filename without path
  std::filesystem::path p(file_path);
  std::string filename = p.filename().string();

  cvm::log(cvm::NONE, "[jtag_sequence]Parse JTAG Commands from CSV:{}\n", filename);
  // std::fstream file (FLAGS_jtag_input_file_path, std::ios::in);
  std::fstream file(file_path, std::ios::in);

  if (file.is_open())
  {
    std::string line, word;
    unsigned int lineNumber = 0;
    while (getline(file, line))
    {
      row.clear();
      lineNumber++;

      size_t commentPos = line.find("//");
      if (commentPos != std::string::npos) {
          // If "//" is found, erase everything from that position onwards
          line.erase(commentPos);
      }
      line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
      if (line.empty()) {
        continue;//Skip is line is empty
      }
    
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

      std::stringstream ss,ss_row;
      ss << filename << ":" << lineNumber;
      jtag_req.snippet = ss.str();
      ss_row << row[0] << ", " << row[1] << ", " << row[2];
      jtag_req.csv_row = ss_row.str();

      instr = row[0];
      data_s = row[1];
      length = row[2]; //TODO
      if(FLAGS_en_jtag_driver_logs)
            cvm::log(cvm::FULL, "[Trickbox] Jtag_sequence : data_length_1: {}\n",data_s);
      
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
      else if(jtag_cmd == "cm"){
         jtag_req.jtag_cmd = 12;        
      }
      else{
        if(FLAGS_en_jtag_driver_logs)
        cvm::log(cvm::ERROR, "[jtag_sequence]Error: unknown command {} in jtag cfg file {}\n",jtag_cmd, FLAGS_jtag_input_file_path);
      }
      
      if(jtag_req.jtag_cmd<3 || jtag_req.jtag_cmd == 4 ||jtag_req.jtag_cmd == 12){ 
        if(FLAGS_en_jtag_driver_logs)
            cvm::log(cvm::FULL, "[Trickbox] Jtag_sequence :jtag_req.jtag_cmd {}\n",jtag_req.jtag_cmd);
         length = row[2];  //length NA for nop 
      }
      if(FLAGS_en_jtag_driver_logs)
            cvm::log(cvm::FULL, "[Trickbox] Jtag_sequence : data_length_2: {}\n",data_s);
      
      data_s.erase(std::remove_if(data_s.begin(), data_s.end(), ::isspace), data_s.end());
      if(FLAGS_en_jtag_driver_logs)
            cvm::log(cvm::FULL, "[Trickbox] Jtag_sequence : data_length_3: {}\n",data_s);
      // convert string to lowercase for uniformity
      std::transform(data_s.begin(), data_s.end(), data_s.begin(), ::tolower);
      if(FLAGS_en_jtag_driver_logs)
            cvm::log(cvm::FULL, "[Trickbox] Jtag_sequence : data_length_4: {}\n",data_s);
      // cvm::log(cvm::HIGH, "[jtag_sequence] length {:#x}\n",length);
      //check data length
      if (data_s.substr(0, 2) == "0x" || data_s.substr(0, 2) == "0x") {
        data_s = data_s.substr(2);
        }
      unsigned data_len = data_s.length();
      if(FLAGS_en_jtag_driver_logs)
            cvm::log(cvm::FULL, "[Trickbox] Jtag_sequence : data_length: {}\n",data_s);
      if(data_len>16){
        std::string data_s_upper = data_s.substr(0, data_len-16);
        std::string data_s_lower = data_s.substr(data_len-16, data_len);
      
        try{
          jtag_req.jtag_ip_data_lower = std::stoul(data_s_lower,nullptr,16);
          if(FLAGS_en_jtag_driver_logs)
            cvm::log(cvm::FULL, "[Trickbox] Jtag_sequence : jtag_req.jtag_ip_data_lower: {}\n",jtag_req.jtag_ip_data_lower);
          
        } catch (const std::invalid_argument& e) {
              cvm::log(cvm::ERROR, "[jtag_sequence] Invalid argument: data for stoul csv arg 1: {}\n", e.what());
        }
      
        try{
          jtag_req.jtag_ip_data_upper = std::stoul(data_s_upper,nullptr,16);
          if(FLAGS_en_jtag_driver_logs)
            cvm::log(cvm::FULL, "[Trickbox] Jtag_sequence : jtag_req.jtag_ip_data_upper: {}\n",jtag_req.jtag_ip_data_upper);
          
        } catch (const std::invalid_argument& e) {
            cvm::log(cvm::ERROR, "[Trickbox] Invalid argument: data for stoul csv arg 1: {}\n", e.what());
        }
      }
      else {
         try{
           jtag_req.jtag_ip_data_lower = std::stoul(data_s,nullptr,16);
           jtag_req.jtag_ip_data_upper = 0;
           
         } catch (const std::invalid_argument& e) {
             cvm::log(cvm::ERROR, "[Trickbox] Invalid argument: data for stoul csv arg 1: {}\n", e.what());
         }

      }
      if((jtag_req.jtag_cmd<3) || (jtag_req.jtag_cmd ==6) || (jtag_req.jtag_cmd ==4)){
        try{
          jtag_req.jtag_length_data = std::stoul(length,nullptr,10);
          
        } catch (const std::invalid_argument& e) {
            cvm::log(cvm::ERROR, "[Trickbox] Invalid argument: data for stoul csv arg 2: {}\n", e.what());
        }
      }else if(jtag_req.jtag_cmd ==12){
            jtag_req.jtag_length_data = 64;
            jtag_req.jtag_cm_value =  std::stoul(length,nullptr,16);
            if(FLAGS_en_jtag_driver_logs){
            cvm::log(cvm::FULL, "[Trickbox] Jtag_sequence : jtag_req.jtag_length_data: {}\n",jtag_req.jtag_length_data);
            cvm::log(cvm::FULL, "[Trickbox] Jtag_sequence : jtag_req.jtag_cm_value  {}\n", jtag_req.jtag_cm_value);
            }
      }
      else{
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
    cvm::log(cvm::ERROR, "[jtag_sequence]Error: Could not open jtag cfg file {}\n", FLAGS_jtag_input_file_path);
  }

  jtag_file_mode = 1; // Clean up later
}


void jtag_sequence::process_input_string(std::string line)
{

  if(FLAGS_en_jtag_driver_logs)
    cvm::log(cvm::FULL, "[jtag_sequence] PROCESS INPUT STRING :{}\n", line);
  std::string word;
      row.clear();
  if(FLAGS_en_jtag_driver_logs)
    cvm::log(cvm::FULL, "[jtag_sequence] REMOVE WHITESPACES :{}\n", line);
      line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
      std::transform(line.begin(), line.end(), line.begin(), ::tolower);
  if(FLAGS_en_jtag_driver_logs)
    cvm::log(cvm::FULL, "[jtag_sequence] REMOVE WHITESPACES DONE :{}\n", line);
      std::stringstream str(line);
      if(line == "qt"){
        execute_qt = true;
        return;
      }
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
        cvm::log(cvm::ERROR, "[jtag_sequence]Error: unknown command {} in jtag cfg file {}\n",jtag_cmd, FLAGS_jtag_input_file_path);
      }
      
      if(jtag_req.jtag_cmd<3 || jtag_req.jtag_cmd == 4){ 
         length = row[2];  //length NA for nop 
      }
      
      data_s.erase(std::remove_if(data_s.begin(), data_s.end(), ::isspace), data_s.end());
      // convert string to lowercase for uniformity
      std::transform(data_s.begin(), data_s.end(), data_s.begin(), ::tolower);
      // cvm::log(cvm::HIGH, "[jtag_sequence] length {:#x}\n",length);
      //check data length
      unsigned data_len = data_s.length();
      if (data_s.substr(0, 2) == "0x" || data_s.substr(0, 2) == "0X") {
        data_s = data_s.substr(2);
       }
      if(FLAGS_en_jtag_driver_logs){
      cvm::log(cvm::HIGH, "[jtag_sequence][JTAG DRIVER SOCKET]: input data length: {} \n",data_len);
      cvm::log(cvm::HIGH, "[[jtag_sequence]JTAG DRIVER SOCKET]: input data string: {} \n",data_s);
      }
      std::bitset<1344> input_shift_data = hexStringToBitset(data_s);
      std::vector<uint64_t> input_shift_data_vec = bitsetToUint64Array(input_shift_data);
      for (size_t i = 0; i < input_shift_data_vec.size(); ++i) {
        if(FLAGS_en_jtag_driver_logs)
        cvm::log(cvm::HIGH, "[jtag_sequence][JTAG DRIVER SOCKET]: input_shift_data_vec[{}] = {:#x} \n",i,input_shift_data_vec[i]);
        jtag_req.ip_data_array[i] = static_cast<unsigned long>(input_shift_data_vec[i]); // Type cast if necessary
        if(FLAGS_en_jtag_driver_logs)
        cvm::log(cvm::HIGH, "[jtag_sequence][JTAG DRIVER SOCKET]: jtag_req.ip_data_array[{}] = {:#x} \n",i,jtag_req.ip_data_array[i]);
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

std::string jtag_sequence::tapToString(unsigned tap) {
    switch(tap) {
        case 2:   return "AXI_TAP";
        case 6:   return "TRACE_TAP";
        case 3:   return "ACLINT_TAP";
        case 4:   return "PMNW_TAP";
        case 5:   return "SMC_TAP";
        case 1:   return "DTM_TAP";
        case 7:   return "CORE_TAP";
        default:  return "Unknown_tap";  
    }
}


void jtag_sequence::drive_csv_jtag_cmds()
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
    unsigned long  jtag_cm_value = 0;
    unsigned       reg_length_data = 0;
    unsigned       hart = 0;

    jtag_cmd        = jtag_req.jtag_cmd;
    upper_jtag_data = jtag_req.jtag_ip_data_upper;
    lower_jtag_data = jtag_req.jtag_ip_data_lower;
    reg_length_data = jtag_req.jtag_length_data;
    padding_length  = reg_length_data; 
    jtag_cm_value    = jtag_req.jtag_cm_value;
    if(FLAGS_en_jtag_driver_logs){
    cvm::log(cvm::HIGH, "[jtag_sequence] Driving jtag cmd {}\n", jtag_cmd);
    cvm::log(cvm::HIGH, "[jtag_sequence] Driving jtag upper_jtag_data {}\n", upper_jtag_data);
    cvm::log(cvm::HIGH, "[jtag_sequence] Driving jtag lower_jtag_data {}\n", lower_jtag_data);
    cvm::log(cvm::HIGH, "[jtag_sequence] Driving jtag reg_length_data {}\n", reg_length_data);
    cvm::log(cvm::HIGH, "[jtag_sequence] Driving jtag padding_length  {}\n", padding_length);
    cvm::log(cvm::HIGH, "[jtag_sequence] Driving jtag jtag_cm_value   {}\n", jtag_cm_value);
    cvm::log(cvm::HIGH, "[jtag_sequence] Length of jtag_cmd queue     {}\n", jtag_cmd_q.size());
    cvm::log(cvm::HIGH, "[jtag_sequence] Snippet     {}\n", jtag_req.snippet);
    }
    if(jtag_cmd<3){
      //if(!stall_jtag_xtor){
      //   stall_jtag_xtor = true;
      //}else{
      //cvm::log(cvm::LOW, "[jtag_sequence] Stall Observed Not: Driving jtag cmd {}\n", jtag_cmd);
      //cvm::log(cvm::LOW, "[jtag_sequence] Stall Observed! Length of jtag_cmd queue     {}\n", jtag_cmd_q.size());
      //return;
      //}
      hart = 0; // hart bits position TBD, till TBD it is always zero
      jtag_cmd_q.pop(); // pop front element
      cvm::log(cvm::MEDIUM, "[jtag_sequence] Driving cmd [{}] from {}\n", jtag_req.csv_row, jtag_req.snippet);
      trickboxJtagWrite(hart, jtag_cmd, upper_jtag_data, lower_jtag_data,reg_length_data,0,tap_cfg_sel);
    }

    if(jtag_cmd == 3){ //nop
      executing_nop = true;
      nop_count = lower_jtag_data;
      if(FLAGS_en_jtag_driver_logs)
      cvm::log(cvm::MEDIUM, "[jtag_sequence] Pushing jtag nops for {} ticks\n", nop_count);
      jtag_cmd_q.pop(); // pop front eleme7t
    }
    if(jtag_cmd == 7 && !FLAGS_random_jtag_entry){  //JTAG quit, signal to end simulation once csv ends
 
     cvm::log(cvm::HIGH, "[jtag_sequence] ******************* \n");
     cvm::log(cvm::HIGH, "[jtag_sequence] Sending Quit signal \n");
     cvm::log(cvm::HIGH, "[jtag_sequence] ******************* \n");
     trickboxJtagWrite(hart, jtag_cmd, 0, 0,0,1,tap_cfg_sel);
    
    }else if(jtag_cmd == 7){
      csv_completed = 1;
      cvm::log(cvm::HIGH, "[jtag_sequence] Random CSV injected\n");
      jtag_cmd_q.pop();
    }

    if(jtag_cmd == 4 || jtag_cmd == 12){  //ck expecting check on rdata
      //check last saved rdata == lower_jtag_data ??
     std::vector<uint64_t> convertedArray = {};// bitsetToUint64Array(jtag_reversed_rdata);
     convertedArray = reverseJtagAndStripSIB(jtag_rdata, reg_length_data);
      
    if(jtag_cmd == 4){
      if(convertedArray[0] == lower_jtag_data){
       //PASS
       if(FLAGS_en_jtag_driver_logs)
       cvm::log(cvm::MEDIUM, "[jtag_sequence] jtag check opcode Passed! expected 0x{:x} got 0x{:x} tap_select is {} \n", lower_jtag_data,convertedArray[0],tapToString(tap_cfg_sel));
      }else{
       //FAIL
       cvm::log(cvm::ERROR, "\nERROR: [jtag_sequence] jtag check opcode failed! expected 0x{:x} got 0x{:x} tap_select is {} \n", lower_jtag_data,convertedArray[0],tapToString(tap_cfg_sel));
      }
    }else if(jtag_cmd == 12){
      if((convertedArray[0] & lower_jtag_data) == jtag_cm_value){
       //PASS
       if(FLAGS_en_jtag_driver_logs)
       cvm::log(cvm::MEDIUM, "[jtag_sequence] jtag check mask opcode Passed! expected 0x{:x} got 0x{:x} tap_select is {} \n", jtag_cm_value,(convertedArray[0] & lower_jtag_data),tapToString(tap_cfg_sel));
      }else{
       //FAIL
       cvm::log(cvm::NONE, "\n[jtag_sequence] jtag check mask opcode: result {:#x} mask 0x{:x} expected 0x{:x} tap_select is {} \n", convertedArray[0],lower_jtag_data,jtag_cm_value,tapToString(tap_cfg_sel));
       cvm::log(cvm::ERROR, "\nERROR: [jtag_sequence] jtag check mask opcode failed! expected 0x{:x} got 0x{:x} tap_select is {} \n", jtag_cm_value,(convertedArray[0] & lower_jtag_data),tapToString(tap_cfg_sel));
      }
    }
      jtag_cmd_q.pop(); // pop front eleme7t
    }

    if(jtag_cmd == 8){  //Reverse

      uint64_t temp_rev = reverseBits(loop_rdata, reg_length_data );
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
          if(FLAGS_en_jtag_driver_logs)
          cvm::log(cvm::HIGH, "[jtag_sequence] tap_sel {} \n", tap_cfg_sel);
          
        } catch (const std::invalid_argument& e) {
            cvm::log(cvm::ERROR, "[jtag_sequence] Invalid argument: data for stoul csv arg 2: {}\n", e.what());
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
       cvm::log(cvm::ERROR, "ERROR: [jtag_sequence] jtag loop end detected without loop start \n");
    }


  }
  else
  {
    jtag_trigger = 0;
    rnd_jtag_trigger = 0;
  }
  }
}


void jtag_sequence::drive_jtag_cmds()
{
  if(!executing_nop || !executing_loop){
  if (!jtag_cmd_q.empty())
  {
    
    jtag_req_t jtag_req;
    jtag_req = jtag_cmd_q.front();

   // cvm::log(cvm::MEDIUM, "Popping jtag request: op {} addr {:#x} data {:#x} func bits {:#x}\n", jtag_req.op, jtag_req.addr, jtag_req.data, jtag_req.func_bits);
    unsigned       jtag_cmd = 0;
    //unsigned long  upper_jtag_data = 0;
    unsigned long  lower_jtag_data = 0;
    unsigned       reg_length_data = 0;
    unsigned       hart = 0;
    unsigned long  input_shift_data_array[21];

    jtag_cmd        = jtag_req.jtag_cmd;
    //upper_jtag_data = jtag_req.jtag_ip_data_upper;
    lower_jtag_data = jtag_req.jtag_ip_data_lower;
    reg_length_data = jtag_req.jtag_length_data;
    padding_length  =  reg_length_data;
    //input_shift_data_array = jtag_req.ip_data_array;
    std::copy(std::begin(jtag_req.ip_data_array), std::end(jtag_req.ip_data_array), std::begin(input_shift_data_array));
    if(FLAGS_en_jtag_driver_logs)
    cvm::log(cvm::HIGH, "[jtag_sequence] Socket Driving jtag cmd {}\n", jtag_cmd);
      // Verify copy
    for (size_t i = 0; i < 21; ++i) {
      if(FLAGS_en_jtag_driver_logs)
        cvm::log(cvm::FULL, "[jtag_sequence] input_shift_data_array[{}] ={:#x} \n",i,input_shift_data_array[i] );
    }

    if(jtag_cmd<3){
      hart = 0; // hart bits position TBD, till TBD it is always zero
      jtag_cmd_q.pop(); // pop front eleme7t
      //trickboxJtagWrite(hart, jtag_cmd, upper_jtag_data, lower_jtag_data,reg_length_data,0,tap_cfg_sel);
      trickboxJtagWriteSocket(hart, jtag_cmd, input_shift_data_array,reg_length_data,0,tap_cfg_sel);
    }

    if(jtag_cmd == 3){ //nop
      executing_nop = true;
      nop_count = lower_jtag_data;
      if(FLAGS_en_jtag_driver_logs)
      cvm::log(cvm::HIGH, "[jtag_sequence] Pushing jtag nops for {} ticks\n", nop_count);
      jtag_cmd_q.pop(); // pop front eleme7t
    }
    if(jtag_cmd == 7 && !FLAGS_random_jtag_entry){  //JTAG quit, signal to end simulation once csv ends
 
     //HACK  cvm::log(cvm::HIGH, "[jtag_sequence] ******************* \n");
     //HACK  cvm::log(cvm::HIGH, "[jtag_sequence] Sending Quit signal \n");
     //HACK  cvm::log(cvm::HIGH, "[jtag_sequence] ******************* \n");
     //HACK  trickboxJtagWrite(hart, jtag_cmd, 0, 0,0,1,tap_cfg_sel);
    
    }else if(jtag_cmd == 7){
    //HACK  csv_completed = 1;
    //HACK  jtag_cmd_q.pop();
    }

    if(jtag_cmd == 4){  //ck expecting check on rdata
      //check last saved rdata == lower_jtag_data ??
      
      uint64_t mask = (1ULL << reg_length_data) - 1;
      auto result = reg_length_data = 64 ? loop_rdata : loop_rdata & mask;
      if(result == lower_jtag_data){
       //PASS
       cvm::log(cvm::MEDIUM, "[jtag_sequence] jtag check opcode Passed! expected {} got {} tap_sel is {}\n", lower_jtag_data,result,tapToString(tap_cfg_sel));
      }else{
       //FAIL
       cvm::log(cvm::NONE, "[jtag_sequence] reg_length_data {} loop_rdata {} lower_jtag_data {} mask {} expression {}\n",reg_length_data,loop_rdata,lower_jtag_data,mask,(1 << reg_length_data));
       cvm::log(cvm::ERROR, "\nERROR: [jtag_sequence] jtag check opcode failed! expected {} got {} tap_sel{} \n", lower_jtag_data,result,tapToString(tap_cfg_sel));
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
          if(FLAGS_en_jtag_driver_logs)
          cvm::log(cvm::HIGH, "[jtag_sequence] tap_sel {} \n", tap_cfg_sel);
          
        } catch (const std::invalid_argument& e) {
            cvm::log(cvm::ERROR, "[jtag_sequence] Invalid argument: data for stoul csv arg 2: {}\n", e.what());
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
       cvm::log(cvm::ERROR, "ERROR: [jtag_sequence] jtag loop end detected without loop start \n");
    }


  }
  else
  {
    jtag_trigger = 0;
    rnd_jtag_trigger = 0;
  }
  }
}



std::string jtag_sequence::process_string(const std::string& input) {
    size_t pos = input.find(',');
    if (pos != std::string::npos) {
        return input.substr(0, pos);
    }
    return "";
}

std::string jtag_sequence::get_local_ip_address() {
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

cvm::messenger::task<void> jtag_sequence::open_socket_to_listen(){
    int server_fd, new_socket;
    struct sockaddr_in address;
    bool quit_communication = false;
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = {0};
    //int PORT=8088;
    int PORT=FLAGS_jtag_socket_port;
    int tick_count = 0;
  
    cvm::log(cvm::LOW,"\n [jtag_sequence] opening socket to listen ...\n");
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket to non-blocking mode
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    address.sin_family = AF_INET;
    std::string jtag_socket_ip_addr = FLAGS_jtag_socket_ip;
    if (jtag_socket_ip_addr == "") {
        jtag_socket_ip_addr = get_local_ip_address();
    }
    address.sin_addr.s_addr = inet_addr(jtag_socket_ip_addr.c_str());
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

    cvm::log(cvm::LOW, "[jtag_sequence]Server is listening on {} PORT {} \n",inet_ntoa(address.sin_addr), PORT );
    cvm::log(cvm::LOW, "[jtag_sequence]Server's local IP address: {} \n", jtag_socket_ip_addr);


    while (true) {
       tick_count++;
       cvm::log(cvm::HIGH,"[jtag_sequence]Server is listening on  tick cnt {} \n", tick_count );
       cvm::log(cvm::LOW,"[jtag_sequence]Server's local IP address: {} \n", jtag_socket_ip_addr);
       if(quit_communication){
        break;
       }
       co_await tick();
       if(tick_count < 5){
        co_await tick();
        continue;
       }
        // Accepting incoming connection (non-blocking)
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket >= 0) {
            cvm::log(cvm::LOW, "[jtag_sequence]Connection accepted from {}:{} \n",inet_ntoa(address.sin_addr),ntohs(address.sin_port) );

            // Process multiple requests from the same client
            while (true) {
                int valread = ::read(new_socket, buffer, 1024);
                if (valread > 0) {
                    if(FLAGS_en_jtag_driver_logs)
                      cvm::log(cvm::HIGH,"[jtag_sequence]Received: {} \n", buffer );
                    // Got buffer from client
                    //convert char buffer to string
                    std::string input_line(buffer);
                    if(FLAGS_en_jtag_driver_logs)
                      cvm::log(cvm::HIGH, "[jtag_sequence]Received input line: {} \n", input_line );
                    process_input_string(input_line);
                    drive_jtag_cmds();
                    
                    co_await resp();
                    // Send Response back
                    
                    std::string concat_response ="";
                    for (uint64_t num : rdata_Array) {
                        std::stringstream ss;
                         ss << std::hex << num; // Convert to hexadecimal string
                         concat_response = ss.str() + concat_response;
                     }
                    //TODO FIXME add padding for zeros

                    //std::string padded_str = formatHexWithPadding(loop_rdata,padding_length);
                    std::string padded_str = bitset_to_hex(jtag_rdata, padding_length);
                    //std::string str = ss.str();
                    //std::cout << "\n[jtag_sequence] Hexadecimal string for jtag Rdata: " << str << std::endl;
                    if(FLAGS_en_jtag_driver_logs)
                    cvm::log(cvm::HIGH, "\n[jtag_sequence] Hexadecimal padded string for jtag Rdata: {} \n ",padded_str );
                    std::string response;
                    if(execute_qt){
                      response = "ACK,0x00000000000000000";
                      execute_qt = false;
                    }else{
                     //OLD response = "ACK," + str;
                     response = "ACK,0x" + padded_str;
                    }
                    send(new_socket, response.c_str(), response.length(), 0);
                    if(FLAGS_en_jtag_driver_logs)
                    cvm::log(cvm::HIGH,"[jtag_sequence] Response sent: {} \n", response );
                    co_await tick();
                } else if (valread == 0) {
                    if(FLAGS_en_jtag_driver_logs)
                    cvm::log(cvm::HIGH, "[jtag_sequence]Client disconnected \n" );
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
  void jtag_sequence::trickboxJtagWrite(unsigned hart,unsigned jtag_cmd, unsigned long upper_jtag_data, unsigned long lower_jtag_data,unsigned reg_length_data,unsigned jtag_quit, unsigned tap_cfg_sel)
  {
    stall_jtag_xtor = true;
    if(FLAGS_en_jtag_driver_logs)
    cvm::log(cvm::HIGH, "[jtag_sequence]TrickBox jtag Write to hart:{}, upper jtag data:{:#x}, lower jtag data:{:#x}, reg length data:{:#x}\n", hart, upper_jtag_data, lower_jtag_data,reg_length_data);
    // cbs.push_back(cb_t{Callback::TRICKBOX_jtag_WR, hart, upper_jtag_data, lower_jtag_data, 0});
    //cvm::registry::messenger.signal(12, jtag_data_t{hart,jtag_cmd, upper_jtag_data, lower_jtag_data,reg_length_data,jtag_quit,tap_cfg_sel});
    // cvm::messenger::send(jtag_t, jtag_pkt);
    cvm::registry::callbacks.push(
    scope_,
    [jtag_cmd,upper_jtag_data, lower_jtag_data,reg_length_data,jtag_quit,tap_cfg_sel]() {
      drive_jtag_req(jtag_cmd,upper_jtag_data, lower_jtag_data,reg_length_data,jtag_quit,tap_cfg_sel);
    });

  }

   // Used to assert/deassert a trickbox interrupt (PIPI) for given hart.
  // virtual void trickboxjtagWrite(unsigned hart, unsigned upper_jtag_data, unsigned lower_jtag_data, cbs_t& cbs)
  void jtag_sequence::trickboxJtagWriteSocket(unsigned hart,unsigned jtag_cmd, unsigned long* lower_jtag_data,unsigned reg_length_data,unsigned jtag_quit, unsigned tap_cfg_sel)
  {
    if(FLAGS_en_jtag_driver_logs)
    cvm::log(cvm::HIGH, "[jtag_sequence]TrickBox socket jtag Write to hart:{},  reg length data:{:#x}", hart, reg_length_data);
    // cbs.push_back(cb_t{Callback::TRICKBOX_jtag_WR, hart, upper_jtag_data, lower_jtag_data, 0});
    //cvm::registry::messenger.signal(12, jtag_data_t{hart,jtag_cmd, upper_jtag_data, lower_jtag_data,reg_length_data,jtag_quit,tap_cfg_sel});
    // cvm::messenger::send(jtag_t, jtag_pkt);
    unsigned long jtag_ip_array[21];
     for (size_t i = 0; i < 21; ++i) {
            jtag_ip_array[i] = lower_jtag_data[i]; 
            if(FLAGS_en_jtag_driver_logs)
            cvm::log(cvm::HIGH," trickboxJtagWriteSocket JTAGDRIVER Socket Data[{}]:{:#x} \n",i, lower_jtag_data[i]);
    }
    cvm::registry::callbacks.push(
    scope_,
    [jtag_cmd, jtag_ip_array,reg_length_data,jtag_quit,tap_cfg_sel]() {
      drive_jtag_req_socket(jtag_cmd,jtag_ip_array,reg_length_data,jtag_quit,tap_cfg_sel);
      //drive_jtag_req_socket(jtag_cmd,lower_jtag_data,reg_length_data,jtag_quit,tap_cfg_sel);
    });

  }

std::bitset<1344> jtag_sequence::hexStringToBitset(std::string& hexString) {
 

    std::bitset<1344> bits;
    unsigned long long segment;
    int bitIndex = 0;

    // Process the hex string in reverse, 16 characters at a time
    for (int i = hexString.length(); i > 0; i -= 16) {
        // Ensure proper substring extraction when i < 16
        int length = (i >= 16) ? 16 : i;
        std::string subHex = hexString.substr(i - length, length);

        std::stringstream ss(subHex);
        ss >> std::hex >> segment;

        std::bitset<64> tempBits(segment);

        // Set the bits in reverse order
        for (int j = 0; j < int(tempBits.size()); ++j) {
            bits[bitIndex++] = tempBits[j];
        }
    }

    return bits;
}


std::string jtag_sequence::bitset_to_hex(const std::bitset<1344>& bits, int n) {
    std::stringstream hex_string;

    // Group bits into chunks of 4 and convert to hex
    for (int i = n - 1; i >= 0; i -= 4) {
        int hex_digit = 0;
        for (int j = 0; j < 4 && (i - j) >= 0; ++j) {
            hex_digit |= (bits[i - j] << j);
        }
        hex_string << std::hex << hex_digit;
    }

    // The hex digits will be in reverse order, reverse them back
    std::string hex_output = hex_string.str();
    std::reverse(hex_output.begin(), hex_output.end());

    // Return the resulting hex string
    return hex_output;
}
std::string jtag_sequence::formatHexWithPadding(uint64_t hexNumber, int n) {
    // Calculate the number of hexadecimal digits needed for n bits
    int hexDigits = n / 4;  // 4 bits per hex digit

    // Create a stringstream to format the output
    std::stringstream ss;
    ss << "0x"
       << std::setw(hexDigits)       // Set width to required hex digits
       << std::setfill('0')          // Fill with '0' if necessary
       << std::hex << hexNumber;     // Convert the number to hex

    return ss.str();
}
  //void jtag_sequence::jtag_resp(std::bitset<100> rdata){
  void jtag_sequence::jtag_resp(std::bitset<1344> rdata){
  
  std::vector<uint64_t> convertedArray = bitsetToUint64Array(rdata);
  jtag_rdata = rdata;
  if(FLAGS_en_jtag_driver_logs)
  cvm::log(cvm::HIGH, "[jtag_sequence] In JTAG RESP converted array size = {}\n", convertedArray.size());
  for (uint64_t num : convertedArray) {
        if(FLAGS_en_jtag_driver_logs)
           cvm::log(cvm::FULL, "[jtag_sequence] In JTAG RESP converted array element = {:#x}\n", num);
        rdata_Array.push_back(num);
        if(FLAGS_en_jtag_driver_logs)
           cvm::log(cvm::FULL, "[jtag_sequence] In JTAG RESP pushed array element = {:#x}\n", num);
  }
  loop_rdata = convertedArray[0]; 

}
