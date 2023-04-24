#include "cvm/plusargs.hpp"
#include "cvm/topology.hpp"
#include "debugger.h"

//DECLARE_string(dbg_input_file_path);
DEFINE_string(dbg_input_file_path, "", "Path to file containing debugger commands");

debugger::debugger(const std::string& tag, uint64_t addr, unsigned hartCount, cvm::topology::loc_t loc)
  : device(tag, addr, 0x40000 /* size */, loc), hartCount_(hartCount), soft_(hartCount),
    timeCompare_(6),IntrHart_(6),delayedRandomIntValid_(6),IntrValue_(6), timerIntPrev_(hartCount), timer_(0)
{
  debugger_base = addr;
  debugger_trigger = addr + 0x10000;
  reset(); 
  //parse_dmi_from_csv();
  //dbg_trigger = 1; 
}


debugger::~debugger()
{
  terminate_ = true;
  
}


void debugger::parse_dmi_from_csv()
{
  if ((FLAGS_dbg_input_file_path == "")) {
    dbg_file_mode = 0;
    }else{
    std::cout<< "trying to read: "<< FLAGS_dbg_input_file_path<<"\n";
    std::fstream file (FLAGS_dbg_input_file_path, std::ios::in);
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
          std::cerr << "Invalid command in csv file "<< instr << std::endl;
        }
        
        //
         if(step_ahead_queue_on){
            dmi_req.func_bits = 2;
         }
         if(step_quit_queue_on){
            dmi_req.func_bits = 4;
         }
            
            
          }
        //
        //remove underscores from addr
        row[1].erase(std::remove(row[1].begin(), row[1].end(), '_'), row[1].end());
        try{
        dmi_req.addr = std::stoul(row[1],nullptr,16);
        } catch (const std::invalid_argument& e) {
          std::cerr << "Invalid argument for stoul csv arg 1: " << e.what() << std::endl;
        }
        
        //remove underscores from data
        row[2].erase(std::remove(row[2].begin(), row[2].end(), '_'), row[2].end());

        try{
        dmi_req.data = std::stoul(row[2],nullptr,16);
        } catch (const std::invalid_argument& e) {
          std::cerr << "Invalid argument for stoul csv arg 2 " << e.what() << std::endl;
        }
        content.push_back(row);
        dmi_cmd_q.push(dmi_req);
        //PRINT CSV DATA
        std::cout<<"Pushing op:"<<dmi_req.op<<" addr: "<<dmi_req.addr<<" data "<<dmi_req.data<<"\n";
      
      }
    
   
	}
	else{
		std::cout<<"Could not open debugger input file\n";
    vpi_control(vpiFinish);
  }
    dbg_file_mode = 1;
  }
    
  
}


void debugger::drive_csv_dmi_cmds()
{

  if(!dmi_cmd_q.empty()) {
    //std::cout << myQueue.front() << " ";
    dmi_req_t dmi_req;
    dmi_req = dmi_cmd_q.front();
    dmi_cmd_q.pop();//pop front eleme7t
    std::cout<<"Popping dmi request OP:"<<dmi_req.op<<" ADDR: "<<dmi_req.addr<<" DATA: "<<dmi_req.data<<" Func Bits: "<<dmi_req.func_bits<<"\n";
    unsigned upper_dmi_data = 0;
    unsigned lower_dmi_data = 0;
    unsigned hart = 0;
    upper_dmi_data = ( dmi_req.func_bits << 27 )|(dmi_req.addr << 2) | dmi_req.op;
    lower_dmi_data = dmi_req.data;
    hart = 0; //hart bits position TBD, till TBD it is always zero
    trickboxDmiWrite(hart,upper_dmi_data,lower_dmi_data);
    }else{
      dbg_trigger = 0;
    }
    

}

void
debugger::read(uint64_t addr, size_t length, data_t& data)
{
  if (not has_addr(addr))
    return;

}


void
debugger::write(uint64_t addr, size_t length, const data_t& data,
		 const strb_t& strb)
{
  std::cout<<"debugger write: 0x"<<std::hex<<addr;
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  if(addr==debugger_base)
  {
    unsigned upper_dmi_data = 0;
    unsigned lower_dmi_data = 0;
    unsigned hart = 0;
    upper_dmi_data = t_data >>32;
    lower_dmi_data = t_data & 0xffffffff;
    hart = 0; //hart bits position TBD, till TBD it is always zero
    trickboxDmiWrite(hart,upper_dmi_data,lower_dmi_data);// Commented until DMI PORT is not in master
 
  }
  
  if(addr==debugger_trigger)
  {
    std::cout <<"\nDEBUGGER FILE TRIGGER\n";
      parse_dmi_from_csv();
      dbg_trigger = 1;
  }

 
    
}
