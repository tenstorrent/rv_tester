#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "cvm/logger.hpp"
#include "msi_driver.h"

//DECLARE_string(msi_input_file_path);
DEFINE_string(msi_input_file_path, "", "Path to file containing msi_driver commands");
DEFINE_bool(random_msi_entry, false, "Enter debug mode randomly after random intervals");
DEFINE_int32(random_msi_start_delay, 300, "delay after which random interrupts should start");
DEFINE_int32(msi_delay_min, 6, "Minimum Delay between 2 consecutive debug mode requests");
DEFINE_int32(msi_delay_max, 9, "Maximum Delay between 2 consecutive debug mopde requests");
DEFINE_int32(msi_max_snippets, 1, "Maximum Delay between 2 consecutive debug mopde requests");
DEFINE_string(msi_template_dir, "", "Path to file containing msi_driver commands");
msi_driver::msi_driver(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc)
  : subdevice(tag, addr, 0x40000 /* size */, loc), soft_(hartCount),
    timeCompare_(6),IntrHart_(6),delayedRandomIntValid_(6),IntrValue_(6), timerIntPrev_(hartCount), timer_(0)
{
  rng.seed(FLAGS_seed);
  msi_driver_base = addr;
  msi_driver_trigger = addr + 0x10000;
  reset();
  //parse_dmi_from_csv();
  //msi_trigger = 1;
}


msi_driver::~msi_driver()
{
  terminate_ = true;

}

void msi_driver::get_all_csv_templates(){
  
  std::string directoryPath =  FLAGS_msi_template_dir;
  DIR* dir = opendir(directoryPath.c_str());
  std::cout<<"msi_driver template dir "<<directoryPath<<"\n";
  if (!dir) {
        throw std::invalid_argument("Invalid directory path");
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".csv") {
            csvFilePaths.push_back(directoryPath + "/" + filename);
	    std::cout<<"Pushing file :" <<filename<<"\n";
        }
    }

    closedir(dir);
    

}
void msi_driver::parse_dmi_from_csv()
{
  //if ((FLAGS_msi_input_file_path == "")) {
  //  msi_file_mode = 0;
  //} else {
    //cvm::log(cvm::NONE, "[Trickbox] Parsing dmi cfg file {}\n", FLAGS_msi_input_file_path);
    //std::fstream file (FLAGS_msi_input_file_path, std::ios::in);
    std::string file_name;
    if(FLAGS_random_msi_entry){
      file_name = csvFilePaths[file_idx]; 
    }else{
      file_name =  FLAGS_msi_input_file_path;
    }
    std::cout<<"PARSE_DMI_FROM_CSV: "<<file_name<<"\n"; 
    //std::fstream file (FLAGS_msi_input_file_path, std::ios::in);
    std::fstream file (file_name, std::ios::in);
    if(file.is_open())
    {
      std::string line, word;
      while(getline(file, line))
      {
        row.clear();

        std::stringstream str(line);


        while(getline(str, word, ',')){
          row.push_back(word);
        }

        dmi_req_t dmi_req;
        dmi_req.op = 0;
        dmi_req.addr = 0;
        dmi_req.data = 0;
        dmi_req.func_bits = 0;
        std::string instr;
        std::string instr_2char;
        instr =   row[0];
        //remove empty spaces from string
        instr.erase(std::remove_if(instr.begin(), instr.end(), ::isspace), instr.end());
        //convert string to lowercase for uniformity
        std::transform(instr.begin(), instr.end(), instr.begin(), ::tolower);
        instr_2char = instr.substr(0,2);
        if(instr_2char == "rd"){
          dmi_req.op = 1;
        }else if(instr_2char =="wr"){
          dmi_req.op = 2;
        }else if(instr_2char =="//"){
          continue; //skip line may be comment
        }else if(instr_2char =="cp"){
          //checkpoint
          dmi_req.op = 3;
        }else if(instr_2char =="st"){
              //step ahead/back q
              if(instr == "step_ahead_queue_on"){
                step_ahead_queue_on = 1;
              }
              if(instr == "step_ahead_queue_off"){
                step_ahead_queue_on = 0;
              }
              if(instr == "step_quit_queue_on"){
                step_quit_queue_on = 1;
              }
              if(instr == "step_quit_queue_off"){
                step_quit_queue_on = 0;
              }
              if(instr == "step_instr_cnt"){
                step_instr_cnt = std::stoul(row[1],nullptr,16);
                // will continue loop with proper dmi write
                dmi_req.func_bits = 1;
                dmi_req.data = step_instr_cnt;
                content.push_back(row);
                dmi_cmd_q.push(dmi_req);
                continue;
              }
        }else{
          //invalid
          cvm::log(cvm::ERROR, "[Trickbox] Invalid command in csv file {}\n", instr);
        }

        //
         if(step_ahead_queue_on){
            dmi_req.func_bits = 2;
         }
         if(step_quit_queue_on){
            dmi_req.func_bits = 4;
         }

        //remove underscores from addr
        row[1].erase(std::remove(row[1].begin(), row[1].end(), '_'), row[1].end());
        try{
        dmi_req.addr = std::stoul(row[1],nullptr,16);
        } catch (const std::invalid_argument& e) {
          cvm::log(cvm::ERROR, "[Trickbox] Invalid argument :addr for stoul csv arg 1: {}\n", e.what());
        }

        //remove underscores from data
        row[2].erase(std::remove(row[2].begin(), row[2].end(), '_'), row[2].end());

        try{
        dmi_req.data = std::stoul(row[2],nullptr,16);
        } catch (const std::invalid_argument& e) {
          cvm::log(cvm::ERROR, "[Trickbox] Invalid argument :data for stoul csv arg 2: {}\n", e.what());
        }
        content.push_back(row);
        dmi_cmd_q.push(dmi_req);
        //PRINT CSV DATA
        cvm::log(cvm::MEDIUM, "Pushing dmi request: op {} addr {:#x} data {:#x}\n", dmi_req.op, dmi_req.addr, dmi_req.data);

      }
    }
	  else{
      cvm::log(cvm::ERROR, "Error: Could not open dmi cfg file {}\n", FLAGS_msi_input_file_path);
    }
    msi_file_mode = 1;

}


void msi_driver::drive_csv_dmi_cmds()
{

  if(!dmi_cmd_q.empty()) {
    dmi_req_t dmi_req;
    dmi_req = dmi_cmd_q.front();
    dmi_cmd_q.pop();//pop front eleme7t
    cvm::log(cvm::MEDIUM, "Popping dmi request: op {} addr {:#x} data {:#x} func bits {:#x}\n", dmi_req.op, dmi_req.addr, dmi_req.data, dmi_req.func_bits);
    unsigned upper_dmi_data = 0;
    unsigned lower_dmi_data = 0;
    unsigned hart = 0;
    upper_dmi_data = ( dmi_req.func_bits << 27 )|(dmi_req.addr << 2) | dmi_req.op;
    lower_dmi_data = dmi_req.data;
    hart = 0; //hart bits position TBD, till TBD it is always zero
    trickboxDmiWrite(hart,upper_dmi_data,lower_dmi_data);
    }else{
      msi_trigger = 0;
      rnd_msi_trigger = 0;
    }


}


cvm::messenger::task<void>
msi_driver::read(uint64_t addr, size_t, data_t&)
{
  co_return;
}


void
msi_driver::write(uint64_t addr, size_t, const data_t& data,
		 const strb_t&)
{
  if (not has_addr(addr))
    return;

  cvm::log(cvm::HIGH, "[Trickbox] msi_driver write addr: {:#x}\n", addr);
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  if(addr==msi_driver_base)
  {
    unsigned upper_dmi_data = 0;
    unsigned lower_dmi_data = 0;
    unsigned hart = 0;
    upper_dmi_data = t_data >>32;
    lower_dmi_data = t_data & 0xffffffff;
    hart = 0; //hart bits position TBD, till TBD it is always zero
    trickboxDmiWrite(hart,upper_dmi_data,lower_dmi_data);// Commented until DMI PORT is not in master

  }

  if (addr==msi_driver_trigger) {
    cvm::log(cvm::MEDIUM, "[Trickbox] msi_driver file trigger\n");
    parse_dmi_from_csv();
    msi_trigger = 1;
  }

}

