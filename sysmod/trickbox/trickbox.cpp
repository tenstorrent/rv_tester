#include "cvm/plusargs.hpp"
#include "trickbox.h"

DEFINE_bool(RANDOM_ITP, false, "Drive random interrups");
DEFINE_int32(ITP_DELAY_MIN, 3, "Minimum Delay between 2 consecutive interrupts");
DEFINE_int32(ITP_DELAY_MAX, 5, "Maximum Delay between 2 consecutive interrupts");
//DECLARE_int32(ITP_DELAY_MIN, 4, "Minimum Delay between 2 consecutive interrupts");
//DECLARE_int32(ITP_DELAY_MAX, 7, "Maximum Delay between 2 consecutive interrupts");
DEFINE_int32(seed, 1, "Simulation seed passed down for randomization");
trickbox::trickbox(const std::string& tag, uint64_t addr, unsigned hartCount)
  : device(tag, addr, 0xc0000 /* size */), hartCount_(hartCount), soft_(hartCount),
    timeCompare_(6),IntHart_(6),delayedRandomIntValid_(6),IntValue_(6), timerIntPrev_(hartCount), timer_(0)
{
  rng.seed(FLAGS_seed);
  if(FLAGS_RANDOM_ITP){
      //std::cout<<"EN RNDM ITP "<<FLAGS_RANDOM_ITP<<"\n";
      //generate random number between max_delay and min-delay
      //uint32_t rand_num =  (rng() % ( FLAGS_ITP_DELAY_MAX - FLAGS_ITP_DELAY_MIN + 1)) + FLAGS_ITP_DELAY_MIN;
      uint32_t rand_num =  (rng() %  2)+1;
      //std::cout<<"RAND NUM "<<rand_num<<"\n";
      timer_ = 0;
      timer_rand_itp = timer_ +(rand_num*timer_advance);
      //std::cout<<"RAND NUM "<<rand_num<<" timer "<<timer_<<" timer_rand "<<timer_rand_itp<<"\n";

    }
}


trickbox::~trickbox()
{
  terminate_ = true;
  if (timerThread_.joinable())
    timerThread_.join();
}


void
trickbox::selfTick(useconds_t delta)
{
  auto func = [this, delta]() {
    while (true)
      {
	usleep(delta);
	if (terminate_)
	  return;
	else
	  {
	    std::lock_guard<std::mutex> lock(mutex_);
	   //  tick();
	  }
      }
  };

  timerThread_ = std::thread(func);
}


void
trickbox::read(uint64_t addr, size_t length, data_t& data, cbs_t& cbs)
{
  if (not has_addr(addr))
    return;

}


void
trickbox::write(uint64_t addr, size_t length, const data_t& data,
		 const strb_t& strb, cbs_t& cbs)
{
  //std::cout<<"Trickbox write: 0x"<<std::hex<<addr;
  if (not has_addr(addr))
    return;
  uint64_t t_data=0;
  deserializeInt(data, t_data);
  if(addr==0x9000000){
    unsigned hart = t_data & 0xfff;
    unsigned event = (t_data >> 12) & 0xff;
    unsigned eventValue = (t_data >> 20);
    trickboxInterrupt(hart,event,eventValue,cbs); 
    }else if((addr > 0x9000000)&& (addr < 0x9001000)){
     //std::cout<<"\nTrickbox DELAYED write: 0x"<<std::hex<<addr<<" data: "<<std::hex<<t_data<<"\n";
     int itp_loc  = addr & 0xf8;  
     itp_loc = (itp_loc >>3);
     unsigned hart = t_data & 0xfff; 
     int eventFlag = (t_data >> 12) & 0x1;
     int eventDelay = (t_data >> 13); 
     timeCompare_.at(itp_loc) = timer_ + (eventDelay * timer_advance);
     IntHart_.at(itp_loc) = hart;  // Hart to be interrupted.
     delayedRandomIntValid_.at(itp_loc) = 1; // Valid 
     IntValue_.at(itp_loc) = eventFlag;
     //std::cout<<"\nTrickbox DELAYED write: 0x"<<std::hex<<addr<<" itp_loc: "<<itp_loc<<" time: "<<timer_<<" eventDelay: "<<eventDelay<<" timercompare :"<<timeCompare_.at(itp_loc)<<" hart "<<hart<<" flag: "<<eventFlag<<"\n";
    }else if(addr==0x9004000){
     unsigned hart = t_data & 0xfff; 
     int eventFlag = (t_data >> 12) & 0x1; 
     //TODO 
    }
    if(FLAGS_RANDOM_ITP){
      //std::cout<<"EN RNDM ITP "<<FLAGS_RANDOM_ITP<<"\n";
      //generate random number between max_delay and min-delay
      //uint32_t rand_num =  (rng() % ( FLAGS_ITP_DELAY_MAX - FLAGS_ITP_DELAY_MIN + 1)) + FLAGS_ITP_DELAY_MIN;
      //std::cout<<"RAND NUM "<<rand_num<<"\n";
      //timer_rand_itp = timer_ +(rand_num*timer_advance);

    }
    
}
